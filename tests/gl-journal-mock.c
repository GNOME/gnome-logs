/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014  Red Hat, Inc.
 *  Copyright (C) 2015  Lars Uebernickel <lars@uebernic.de>
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

#include "gl-journal-mock.h"

#include <glib-unix.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>

struct MockGlJournalEntry
{
  GObject parent_instance;

  guint64 timestamp;
  gchar *cursor;
  gchar *message;
  gchar *comm;
  gchar *kernel_device;
  gchar *audit_session;
  gchar *catalog;
  guint priority;
};

G_DEFINE_TYPE (MockGlJournalEntry, gl_journal_entry, G_TYPE_OBJECT);

typedef struct
{
    gint fd;
    guint source_id;
    gchar **mandatory_fields;
} MockGlJournalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (MockGlJournal, gl_journal, G_TYPE_OBJECT)

GQuark
mock_gl_journal_error_quark (void)
{
    return g_quark_from_static_string ("gl-journal-error-quark");
}

static void
mock_gl_journal_finalize (GObject *object)
{
    MockGlJournal *journal = GL_JOURNAL (object);
    MockGlJournalPrivate *priv = gl_journal_get_instance_private (journal);

    g_source_remove (priv->source_id);
    g_clear_pointer (&priv->mandatory_fields, g_strfreev);
}

static void
mock_gl_journal_class_init (GlJournalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_journal_finalize;
}

static void
mock_gl_journal_init (GlJournal *self)
{
}

static gchar *
gl_journal_mock_get_data (MockGlJournal *self,
                          const gchar *field,
                          GError **error)
{
    gconstpointer data;
    gsize length;
    gsize prefix_len;

    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    g_return_val_if_fail (field != NULL, NULL);

    /* Field data proper starts after the first '='. */
    prefix_len = strchr (data, '=') - (const gchar *)data + 1;

    /* Trim the prefix off the beginning of the field. */
    return g_strndup ((const gchar *)data + prefix_len, length - prefix_len);
}

static MockGlJournalEntry *
gl_journal_mock_query_entry (GlJournal *self)
{
    MockGlJournalPrivate *priv;
    MockGlJournalEntry *entry;
    gint ret;
    GError *error = NULL;
    gchar *priority;

    priv = gl_journal_get_instance_private (self);

    entry = g_object_new (GL_TYPE_JOURNAL_ENTRY, NULL);

    entry->message = gl_journal_get_data (self, "MESSAGE", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        free (entry->cursor);
        free (entry->catalog);
        goto out;
    }

    priority = gl_journal_get_data (self, "PRIORITY", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        free (entry->cursor);
        free (entry->catalog);
        g_free (entry->message);
        goto out;
    }

    entry->priority = priority ? atoi (priority) : LOG_INFO;
    g_free (priority);

   entry->comm = gl_journal_get_data (self, "_COMM", &error);

    if (error != NULL)
    {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    entry->kernel_device = gl_journal_get_data (self, "_KERNEL_DEVICE", NULL);

    if (error != NULL)
    {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    entry->audit_session = gl_journal_get_data (self, "_AUDIT_SESSION", NULL);

    return entry;

out:
    g_object_unref (entry);

    return NULL;
}

/**
 * gl_journal_set_matches:
 * @journal: a #GlJournal
 * @matches: new matches to set
 *
 * Sets @matches on @journal. Will reset the cursor position to the
 * beginning.
 */
void
gl_journal_mock_set_matches (GlJournal           *journal,
                        const gchar * const *matches)
{
    MockGlJournalPrivate *priv = gl_journal_get_instance_private (journal);
    GPtrArray *mandatory_fields;
    int r;
    gint i;
    gboolean has_boot_id = FALSE;

    g_return_if_fail (matches != NULL);

    if (priv->mandatory_fields)
      g_clear_pointer (&priv->mandatory_fields, g_strfreev);

    mandatory_fields = g_ptr_array_new ();
    for (i = 0; matches[i]; i++)
    {
        /* matches without a value should only check for existence.
         * systemd doesn't support that, so let's remember them to
         * filter out later.
         */
        if (strchr (matches[i], '=') == NULL)
        {
            g_ptr_array_add (mandatory_fields, g_strdup (matches[i]));
            continue;
        }

        if (g_str_has_prefix (matches[i], "_BOOT_ID="))
          has_boot_id = TRUE;
    }

    /* add sentinel */
    g_ptr_array_add (mandatory_fields, NULL);

    priv->mandatory_fields = (gchar **) g_ptr_array_free (mandatory_fields, FALSE);
}

MockGlJournalEntry *
_gl_journal_mock_previous (MockGlJournal *journal)
{
    return _gl_journal_query_entry (journal);
}

GlJournal *
gl_journal_mock_new (void)
{
    return g_object_new (GL_TYPE_JOURNAL, NULL);
}

static void
gl_journal_mock_entry_init (MockGlJournalEntry *entry)
{
}

static void
gl_journal_mock_entry_finalize (GObject *object)
{
  MockGlJournalEntry *entry = GL_JOURNAL_ENTRY (object);

  free (entry->cursor);
  free (entry->catalog);
  g_free (entry->message);
  g_free (entry->comm);
  g_free (entry->kernel_device);
  g_free (entry->audit_session);

  G_OBJECT_CLASS (gl_journal_entry_parent_class)->finalize (object);
}

static void
gl_journal_mock_entry_class_init (MockGlJournalEntryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gl_journal_entry_finalize;
}

guint64
gl_journal_mock_entry_get_timestamp (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), 0);

  return entry->timestamp;
}

const gchar *
gl_journal_mock_entry_get_message (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), NULL);

  return entry->message;
}

const gchar *
gl_journal_mock_entry_get_command_line (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), NULL);

  return entry->comm;
}

const gchar *
gl_journal_mock_entry_get_kernel_device (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), NULL);

  return entry->kernel_device;
}

const gchar *
gl_journal_mock_entry_get_audit_session (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), NULL);

  return entry->audit_session;
}

const gchar *
gl_journal_mock_entry_get_catalog (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), NULL);

  return entry->catalog;
}

guint
gl_journal_mock_entry_get_priority (MockGlJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_JOURNAL_ENTRY (entry), 0);

  return entry->priority;
}
