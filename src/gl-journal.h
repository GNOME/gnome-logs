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

#ifndef GL_JOURNAL_H_
#define GL_JOURNAL_H_

#include <gio/gio.h>
#include <systemd/sd-journal.h>

G_BEGIN_DECLS

/*
 * GlJournalError:
 * @GL_JOURNAL_ERROR_NO_FIELD: the requested field was not found in the current
 * journal entry
 * @GL_JOURNAL_ERROR_INVALID_POINTER: the pointer to the current journal entry
 * is not valid
 * @GL_JOURNAL_ERROR_FAILED: unknown failure
 */
typedef enum
{
    GL_JOURNAL_ERROR_NO_FIELD,
    GL_JOURNAL_ERROR_INVALID_POINTER,
    GL_JOURNAL_ERROR_FAILED
} GlJournalError;

#define GL_JOURNAL_ERROR gl_journal_error_quark ()

GQuark gl_journal_error_quark (void);

#define GL_TYPE_JOURNAL_ENTRY gl_journal_entry_get_type()
G_DECLARE_FINAL_TYPE (GlJournalEntry, gl_journal_entry, GL, JOURNAL_ENTRY, GObject)

typedef struct
{
    /*< private >*/
    GObject parent_instance;
} GlJournal;

typedef struct
{
    /*< private >*/
    GObjectClass parent_class;
} GlJournalClass;

typedef struct
{
    gchar *boot_match;
    guint64 realtime_first;
    guint64 realtime_last;
} GlJournalBootID;

#define GL_TYPE_JOURNAL (gl_journal_get_type ())
#define GL_JOURNAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_JOURNAL, GlJournal))

GType gl_journal_result_get_type (void);
GType gl_journal_get_type (void);
void gl_journal_set_matches (GlJournal *journal, GPtrArray *matches);
void gl_journal_set_start_position (GlJournal *journal, guint64 until_timestamp);
GArray * gl_journal_get_boot_ids (GlJournal *journal);
GlJournalEntry * gl_journal_previous (GlJournal *journal);
GlJournal * gl_journal_new (void);
gchar * gl_journal_get_current_boot_time (GlJournal *journal,
                                          const gchar *boot_match);

guint64                 gl_journal_entry_get_timestamp                  (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_message                    (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_command_line               (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_kernel_device              (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_audit_session              (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_transport                  (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_catalog                    (GlJournalEntry *entry);
guint                   gl_journal_entry_get_priority                   (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_uid                        (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_pid                        (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_gid                        (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_systemd_unit               (GlJournalEntry *entry);
const gchar *           gl_journal_entry_get_executable_path            (GlJournalEntry *entry);

G_END_DECLS

#endif /* GL_JOURNAL_H_ */
