/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013  Red Hat, Inc.
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
#include <stdlib.h>

typedef struct
{
    sd_journal *journal;
    gint fd;
    guint source_id;
} GlJournalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlJournal, gl_journal, G_TYPE_OBJECT)

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

static GlJournalResult *
_gl_journal_query_result (GlJournal *self)
{
    GlJournalPrivate *priv;
    GlJournalResult *result;
    gint ret;
    sd_journal *journal;
    const gchar *message;
    const gchar *comm;
    const gchar *kernel_device;
    const gchar *audit_session;
    const gchar *priority;
    gsize length;

    priv = gl_journal_get_instance_private (self);
    journal = priv->journal;

    result = g_slice_new (GlJournalResult);

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

    ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                               &length);

    if (ret < 0)
    {
        g_debug ("Unable to get commandline from systemd journal: %s",
                 g_strerror (-ret));
        comm = "_COMM=";
    }

    result->comm = strchr (comm, '=') + 1;

    ret = sd_journal_get_data (journal, "_KERNEL_DEVICE",
                               (const void **)&kernel_device, &length);

    if (ret < 0)
    {
        g_debug ("Unable to get kernel device from systemd journal: %s",
                 g_strerror (-ret));
        kernel_device = "_KERNEL_DEVICE=";
    }

    result->kernel_device = strchr (kernel_device, '=') + 1;

    ret = sd_journal_get_data (journal, "_AUDIT_SESSION",
                               (const void **)&audit_session, &length);

    if (ret < 0)
    {
        g_debug ("Unable to get audit session from systemd journal: %s",
                 g_strerror (-ret));
        audit_session = "_AUDIT_SESSION=";
    }

    result->audit_session = strchr (audit_session, '=') + 1;

    ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                               &length);

    if (ret < 0)
    {
        g_warning ("Error getting message from systemd journal: %s",
                   g_strerror (-ret));
        free (result->cursor);
        free (result->catalog);
        goto out;
    }

    result->message = strchr (message, '=') + 1;

    ret = sd_journal_get_data (journal, "PRIORITY",
                               (const void **)&priority, &length);

    if (ret == -ENOENT)
    {
        g_warning ("No priority was set for this message");
        free (result->cursor);
        free (result->catalog);
        goto out;
    }
    else if (ret < 0)
    {
        g_warning ("Error getting priority from systemd journal: %s",
                   g_strerror (-ret));
        free (result->cursor);
        free (result->catalog);
        goto out;
    }

    result->priority = atoi (strchr (priority, '=') + 1);

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

    sd_journal_flush_matches (journal);

    return g_list_reverse (results);
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
_gl_journal_result_free (GlJournalResult *result,
                         G_GNUC_UNUSED gpointer user_data)
{
    free (result->cursor);
    free (result->catalog);
    g_slice_free (GlJournalResult, result);
}

void
gl_journal_result_free (G_GNUC_UNUSED GlJournal *self,
                        GlJournalResult *result)
{
    _gl_journal_result_free (result, NULL);
}

void
gl_journal_results_free (G_GNUC_UNUSED GlJournal *self,
                         GList *results)
{
    /* As self is unused, ignore it. */
    g_list_foreach (results, (GFunc)_gl_journal_result_free, NULL);
    g_list_free (results);
}

sd_journal *
gl_journal_get_journal (GlJournal *self)
{
    GlJournalPrivate *priv;

    g_return_val_if_fail (GL_JOURNAL (self), NULL);

    priv = gl_journal_get_instance_private (self);

    return priv->journal;
}

GlJournal *
gl_journal_new (void)
{
    return g_object_new (GL_TYPE_JOURNAL, NULL);
}
