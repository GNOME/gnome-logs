/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014  Red Hat, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gl-journal.h"

#include <glib-unix.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>

typedef struct
{
    sd_journal *journal;
    gint fd;
    guint source_id;
} GlJournalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlJournal, gl_journal, G_TYPE_OBJECT)

G_DEFINE_BOXED_TYPE (GlJournalResult, gl_journal_result, gl_journal_result_ref,
                     gl_journal_result_unref)

GQuark
gl_journal_error_quark (void)
{
    return g_quark_from_static_string ("gl-journal-error-quark");
}

static gboolean
on_journal_changed (gint fd,
                    GIOCondition condition,
                    GlJournal *self)
{
    gint ret;
    GlJournalPrivate *priv = gl_journal_get_instance_private (self);

    ret = sd_journal_process (priv->journal);

    switch (ret)
    {
        case SD_JOURNAL_NOP:
            g_debug ("Journal change was a no-op");
            break;
        case SD_JOURNAL_APPEND:
            g_debug ("New journal entries added");
            break;
        case SD_JOURNAL_INVALIDATE:
            g_debug ("Journal files added or removed");
            break;
        default:
            g_warning ("Error processing events from systemd journal: %s",
                       g_strerror (-ret));
            break;
    }

    return G_SOURCE_CONTINUE;
}

static void
gl_journal_finalize (GObject *object)
{
    GlJournal *journal = GL_JOURNAL (object);
    GlJournalPrivate *priv = gl_journal_get_instance_private (journal);

    g_source_remove (priv->source_id);
    g_clear_pointer (&priv->journal, sd_journal_close);
}

static void
gl_journal_class_init (GlJournalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_journal_finalize;
}

static void
gl_journal_init (GlJournal *self)
{
    GlJournalPrivate *priv;
    sd_journal *journal;
    gint ret;

    priv = gl_journal_get_instance_private (self);

    ret = sd_journal_open (&journal, 0);
    priv->journal = journal;

    if (ret < 0)
    {
        g_critical ("Error opening systemd journal: %s", g_strerror (-ret));
    }

    ret = sd_journal_get_fd (journal);

    if (ret < 0)
    {
        g_warning ("Error getting polling fd from systemd journal: %s",
                   g_strerror (-ret));
    }

    priv->fd = ret;
    ret = sd_journal_get_events (journal);

    if (ret < 0)
    {
        g_warning ("Error getting poll events from systemd journal: %s",
                   g_strerror (-ret));
    }

    priv->source_id = g_unix_fd_add (priv->fd, ret,
                                     (GUnixFDSourceFunc) on_journal_changed,
                                     self);
    ret = sd_journal_reliable_fd (journal);

    if (ret < 0)
    {
        g_warning ("Error checking reliability of systemd journal poll fd: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("Latency expected while polling for systemd journal activity");
    }
    else
    {
        g_debug ("Immediate wakeups expected for systemd journal activity");
    }

}

static gchar *
gl_journal_get_data (GlJournal *self,
                     const gchar *field,
                     GError **error)
{
    GlJournalPrivate *priv;
    gint ret;
    gconstpointer data;
    gsize length;
    gsize prefix_len;

    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    g_return_val_if_fail (field != NULL, NULL);

    priv = gl_journal_get_instance_private (self);
    ret = sd_journal_get_data (priv->journal, field, &data, &length);

    if (ret < 0)
    {
        gint code;

        switch (-ret)
        {
            case ENOENT:
                code = GL_JOURNAL_ERROR_NO_FIELD;
                break;
            case EADDRNOTAVAIL:
                code = GL_JOURNAL_ERROR_INVALID_POINTER;
                break;
            default:
                code = GL_JOURNAL_ERROR_FAILED;
                break;
        }

        g_set_error (error, GL_JOURNAL_ERROR, code,
                     "Unable to get field ‘%s’ from systemd journal: %s",
                     field, g_strerror (-ret));

        return NULL;
    }

    /* Field data proper starts after the first '='. */
    prefix_len = strchr (data, '=') - (const gchar *)data + 1;

    /* Trim the prefix off the beginning of the field. */
    return g_strndup ((const gchar *)data + prefix_len, length - prefix_len);
}

static GlJournalResult *
_gl_journal_query_result (GlJournal *self)
{
    GlJournalPrivate *priv;
    GlJournalResult *result;
    gint ret;
    sd_journal *journal;
    GError *error = NULL;
    gchar *priority;

    priv = gl_journal_get_instance_private (self);
    journal = priv->journal;

    result = g_slice_new (GlJournalResult);

    result->ref_count = 1;

    ret = sd_journal_get_realtime_usec (journal, &result->timestamp);

    if (ret < 0)
    {
        g_warning ("Error getting timestamp from systemd journal: %s",
                   g_strerror (-ret));
        goto out;
    }

    ret = sd_journal_get_cursor (journal, &result->cursor);

    if (ret < 0)
    {
        g_warning ("Error getting cursor for current journal entry: %s",
                   g_strerror (-ret));
        goto out;
    }

    ret = sd_journal_test_cursor (journal, result->cursor);

    if (ret < 0)
    {
        g_warning ("Error testing cursor string: %s", g_strerror (-ret));
        free (result->cursor);
        result->cursor = NULL;
        goto out;
    }
    else if (ret == 0)
    {
        g_warning ("Cursor string does not match journal entry");
        /* Not a problem at this point, but would be when seeking to the cursor
         * later on. */
    }

    ret = sd_journal_get_catalog (journal, &result->catalog);

    if (ret == -ENOENT)
    {
        g_debug ("No message for this log entry was found in the catalog");
        result->catalog = NULL;
    }
    else if (ret < 0)
    {
        g_warning ("Error while getting message from catalog: %s",
                   g_strerror (-ret));
        free (result->cursor);
        goto out;
    }

    result->message = gl_journal_get_data (self, "MESSAGE", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        free (result->cursor);
        free (result->catalog);
        goto out;
    }

    priority = gl_journal_get_data (self, "PRIORITY", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        free (result->cursor);
        free (result->catalog);
        g_free (result->message);
        goto out;
    }

    result->priority = priority ? atoi (priority) : LOG_INFO;
    g_free (priority);

    result->comm = gl_journal_get_data (self, "_COMM", &error);

    if (error != NULL)
    {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    result->kernel_device = gl_journal_get_data (self, "_KERNEL_DEVICE", NULL);

    if (error != NULL)
    {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    result->audit_session = gl_journal_get_data (self, "_AUDIT_SESSION", NULL);

    return result;

out:
    g_slice_free (GlJournalResult, result);

    return NULL;
}

GList *
gl_journal_query (GlJournal *self, const GlJournalQuery *query)
{
    GlJournalPrivate *priv;
    sd_journal *journal;
    gsize i;
    gint ret;
    GList *results = NULL;

    g_return_val_if_fail (GL_JOURNAL (self), NULL);
    g_return_val_if_fail (query != NULL, NULL);

    priv = gl_journal_get_instance_private (self);
    journal = priv->journal;

    if (query->matches)
    {
        const gchar *match;

        for (i = 0, match = query->matches[i]; match;
             match = query->matches[++i])
        {
            ret = sd_journal_add_match (journal, match, 0);

            if (ret < 0)
            {
                g_warning ("Error adding match '%s': %s", match,
                           g_strerror (-ret));
            }
        }
    }

    if (query->n_results == -1)
    {
        /* Take events from this boot only. */
        sd_id128_t boot_id;
        gchar boot_string[33];
        gchar *match_string;

        ret = sd_id128_get_boot (&boot_id);

        if (ret < 0)
        {
            g_warning ("Error getting boot ID of running kernel: %s",
                       g_strerror (-ret));
        }

        sd_id128_to_string (boot_id, boot_string);

        match_string = g_strconcat ("_BOOT_ID=", boot_string, NULL);

        ret = sd_journal_add_match (journal, match_string, 0);

        if (ret < 0)
        {
            g_warning ("Error adding match '%s': %s", match_string,
                       g_strerror (-ret));
        }

        g_free (match_string);

        ret = sd_journal_seek_head (journal);

        if (ret < 0)
        {
            g_warning ("Error seeking to start of systemd journal: %s",
                       g_strerror (-ret));
        }

        do
        {
            GlJournalResult *result;

            ret = sd_journal_next (journal);

            if (ret < 0)
            {
                g_warning ("Error setting cursor to next position in systemd journal: %s",
                           g_strerror (-ret));
                break;
            }
            else if (ret == 0)
            {
                g_debug ("End of systemd journal reached");
                break;
            }

            result = _gl_journal_query_result (self);

            results = g_list_prepend (results, result);
        } while (TRUE);
    }
    else
    {
        /* Take the given number of events from the end of the journal
         * backwards. */
        ret = sd_journal_seek_tail (journal);

        if (ret < 0)
        {
            g_warning ("Error seeking to end of systemd journal: %s",
                       g_strerror (-ret));
        }

        for (i = 0; i < query->n_results; i++)
        {
            GlJournalResult *result;

            ret = sd_journal_previous (journal);

            if (ret < 0)
            {
                g_warning ("Error setting cursor to end of systemd journal: %s",
                           g_strerror (-ret));
                break;
            }
            else if (ret == 0)
            {
                g_debug ("End of systemd journal reached");
                break;
            }

            result = _gl_journal_query_result (self);

            results = g_list_prepend (results, result);
            continue;
        }

        results = g_list_reverse (results);
    }

    sd_journal_flush_matches (journal);

    return results;
}

GlJournalResult *
gl_journal_query_cursor (GlJournal *self,
                         const gchar *cursor)
{
    GlJournalPrivate *priv;
    sd_journal *journal;
    gint ret;
    GlJournalResult *result = NULL;

    g_return_val_if_fail (GL_JOURNAL (self), NULL);
    g_return_val_if_fail (cursor != NULL, NULL);

    priv = gl_journal_get_instance_private (self);
    journal = priv->journal;

    ret = sd_journal_seek_cursor (journal, cursor);

    if (ret < 0)
    {
        g_warning ("Error seeking to cursor position: %s", g_strerror (-ret));
        goto out;
    }

    ret = sd_journal_next (journal);

    if (ret < 0)
    {
        g_warning ("Error positioning cursor in systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_test_cursor (journal, cursor);

    if (ret < 0)
    {
        g_warning ("Error testing cursor string: %s", g_strerror (-ret));
        goto out;
    }
    else if (ret == 0)
    {
        g_warning ("Cursor string does not match journal entry");
        goto out;
    }

    result = _gl_journal_query_result (self);

out:
    return result;
}

static void
gl_journal_result_free (GlJournalResult *result,
                        G_GNUC_UNUSED gpointer user_data)
{
    gl_journal_result_unref (result);
}

void
gl_journal_results_free (GList *results)
{
    g_list_foreach (results, (GFunc)gl_journal_result_free, NULL);
    g_list_free (results);
}

GlJournalResult *
gl_journal_result_ref (GlJournalResult *result)
{
    g_atomic_int_inc (&result->ref_count);
    return result;
}

void
gl_journal_result_unref (GlJournalResult *result)
{
    if (g_atomic_int_dec_and_test (&result->ref_count))
    {
        free (result->cursor);
        free (result->catalog);
        g_free (result->message);
        g_free (result->comm);
        g_free (result->kernel_device);
        g_free (result->audit_session);
        g_slice_free (GlJournalResult, result);
    }
}

GlJournal *
gl_journal_new (void)
{
    return g_object_new (GL_TYPE_JOURNAL, NULL);
}
