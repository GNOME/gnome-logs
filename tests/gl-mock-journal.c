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

#include "gl-mock-journal.h"
#include "gl-util.h"

#include <glib-unix.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>


struct _GlMockJournalEntry
{
  GObject parent_instance;

  guint64 timestamp;
  gchar *cursor;
  gchar *message;
  gchar *comm;
  gchar *kernel_device;
  gchar *audit_session;
  gchar *transport;
  gchar *catalog;
  guint priority;
  gint uid;

};

G_DEFINE_TYPE (GlMockJournalEntry, gl_mock_journal_entry, G_TYPE_OBJECT)

/* "_BOOT_ID=" contains 9 characters, and 33 more characters is needed to
 * store the string formated from a 128-bit ID. The ID will be formatted as
 * 32 lowercase hexadecimal digits and be terminated by a NUL byte. So an
 * array of with a size 42 is need. */

typedef struct
{
    gint fd;
    guint source_id;
    gchar **mandatory_fields;
    GArray *boot_ids;
} GlMockJournalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlMockJournal, gl_mock_journal, G_TYPE_OBJECT)

GQuark
gl_mock_journal_error_quark (void)
{
    return g_quark_from_static_string ("gl-mock-journal-error-quark");
}

gchar *
gl_mock_journal_get_current_boot_time (GlMockJournal *journal,
                                  const gchar *boot_match)
{
    gchar *time = "12:00 AM";
    return g_strdup("July 29 10:55 PM - Aug 1 12:08 AM");
}

GArray *
gl_mock_journal_get_boot_ids (GlMockJournal *journal)
{
    GlMockJournalPrivate *priv;

    priv = gl_mock_journal_get_instance_private (journal);
    priv->boot_ids = g_array_new (FALSE, FALSE, sizeof (GArray));
    guint64 boot_id = 12;
    priv->boot_ids = g_array_append_val (priv->boot_ids, boot_id);
    return priv->boot_ids;
}

static void
gl_mock_journal_finalize (GObject *object)
{
    guint i;
    GlMockJournal *journal = GL_MOCK_JOURNAL (object);
    GlMockJournalPrivate *priv = gl_mock_journal_get_instance_private (journal);

    g_source_remove (priv->source_id);
    g_clear_pointer (&priv->mandatory_fields, g_strfreev);
    for (i = 0; i < priv->boot_ids->len; i++)
    {
        GlJournalBootID *boot_id;

        boot_id = &g_array_index (priv->boot_ids, GlJournalBootID, i);

        g_free (boot_id->boot_match);
    }
    g_array_free (priv->boot_ids, TRUE);

}

static void
gl_mock_journal_class_init (GlMockJournalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_mock_journal_finalize;

}

static void
gl_mock_journal_init (GlMockJournal *self)
{
}

static gchar *
gl_mock_journal_get_data (GlMockJournal *self,
                          const gchar *field,
                          GError **error)
{
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    g_return_val_if_fail (field != NULL, NULL);

    if (strcmp(field,"MESSAGE")==0)
            return g_strdup("This is a test");

    if (strcmp(field,"PRIORITY")==0)
            return g_strdup("3");

    if(strcmp(field,"_COMM")==0)
            return g_strdup("dnf");
 
    if(strcmp(field,"KERNEL_DEVICE")==0)
	    return g_strdup("c189:3");

    if(strcmp(field,"AUDIT_SESSION")==0)
	    return g_strdup("1");

    if(strcmp(field,"TRANSPORT")==0)
	    return g_strdup("Transport");

    if(strcmp(field,"UID")==0)
	    return g_strdup("0001");
   return NULL;
}

static GlMockJournalEntry *
gl_mock_journal_query_entry (GlMockJournal *self)
{
    GlMockJournalEntry *entry;
    GError *error = NULL;

    entry = g_object_new (GL_TYPE_MOCK_JOURNAL_ENTRY, NULL);
    
    entry->timestamp = (guint64)g_strdup("12");
    entry->cursor = g_strdup("start");
    entry->catalog = g_strdup("test");
    entry->message = gl_mock_journal_get_data (self, "MESSAGE", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        free (entry->cursor);
        free (entry->catalog);
        goto out;
    }

    /* FIXME: priority is an int, not a char*. */
    entry->priority = (guint64)gl_mock_journal_get_data (self, "PRIORITY", NULL);

    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
        g_free (entry->message);
        goto out;
    }

   entry->comm = gl_mock_journal_get_data (self, "_COMM", &error);

   if(error!=NULL)
   {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    entry->kernel_device = gl_mock_journal_get_data (self, "_KERNEL_DEVICE", NULL);

    if (error != NULL)
    {
        g_debug ("%s", error->message);
        g_clear_error (&error);
    }

    entry->audit_session = gl_mock_journal_get_data (self, "_AUDIT_SESSION", NULL);

    return entry;

out:
    g_object_unref (entry);

    return NULL;
}

/**
 * gl_mock_journal_set_matches:
 * @journal: a #GlMockJournal
 * @matches: new matches to set
 *
 * Sets @matches on @journal. Will reset the cursor position to the
 * beginning.
 */
void
gl_mock_journal_set_matches (GlMockJournal           *journal,
                             const gchar * const *matches)
{
    GlMockJournalPrivate *priv = gl_mock_journal_get_instance_private (journal);
    GPtrArray *mandatory_fields;
    gint i;
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
    }

    /* add sentinel */
    g_ptr_array_add (mandatory_fields, NULL);

    priv->mandatory_fields = (gchar **) g_ptr_array_free (mandatory_fields, FALSE);
}

GlMockJournalEntry *
gl_mock_journal_previous (GlMockJournal *journal)
{
    return gl_mock_journal_query_entry (journal);
}

GlMockJournal *
gl_mock_journal_new (void)
{
    return g_object_new (GL_TYPE_MOCK_JOURNAL, NULL);
}

static void
gl_mock_journal_entry_init (GlMockJournalEntry *entry)
{
}

static void
gl_mock_journal_entry_finalize (GObject *object)
{
  GlMockJournalEntry *entry = GL_MOCK_JOURNAL_ENTRY (object);

  free (entry->cursor);
  free (entry->catalog);
  g_free (entry->message);
  g_free (entry->comm);
  g_free (entry->kernel_device);
  g_free (entry->audit_session);

  G_OBJECT_CLASS (gl_mock_journal_entry_parent_class)->finalize (object);
}

static void
gl_mock_journal_entry_class_init (GlMockJournalEntryClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gl_mock_journal_entry_finalize;
}

guint64
gl_mock_journal_entry_get_timestamp (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), 0);

  return entry->timestamp;
}

const gchar *
gl_mock_journal_entry_get_message (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);

  return entry->message;
}

const gchar *
gl_mock_journal_entry_get_command_line (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);
 
   return entry->comm;
}

const gchar *
gl_mock_journal_entry_get_kernel_device (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);

  return entry->kernel_device;
}

const gchar *
gl_mock_journal_entry_get_audit_session (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);

  return entry->audit_session;
}

const gchar *
gl_mock_journal_entry_get_transport (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);

  return entry->transport;
}

const gchar *
gl_mock_journal_entry_get_catalog (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), NULL);

  return entry->catalog;
}

guint
gl_mock_journal_entry_get_priority (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), 0);

  return entry->priority;
}

gint
gl_mock_journal_entry_get_uid (GlMockJournalEntry *entry)
{
  g_return_val_if_fail (GL_IS_MOCK_JOURNAL_ENTRY (entry), -1);

  return entry->uid;
}
             
