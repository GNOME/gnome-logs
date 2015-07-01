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

#ifndef GL_MOCK_JOURNAL_H_
#define GL_MOCK_JOURNAL_H_

#include <gio/gio.h>
#define GL_TYPE_JOURNAL_ENTRY GL_TYPE_MOCK_JOURNAL_ENTRY
#define GlMockJournalEntry GlJournalEntry
#define GlMockJournalEntryClass GlJournalEntryClass
#define GlMockJournalClass GlJournalClass
#define GlMockJournal GlJournal
#define GlMockJournalPrivate GlJournalPrivate
#define GlMockJournalBootID GlJournalBootID
#define gl_mock_journal_entry_get_type(GlMockJournalEntry) gl_journal_entry_get_type(GlMockJournalEntry)
#define gl_mock_journal_entry_get_command_line(GlMockJournalEntry) gl_journal_entry_get_command_line(GlMockJournalEntry)
#define gl_mock_journal_entry_get_timestamp(GlMockJournalEntry) gl_journal_entry_get_timestamp(GlMockJournalEntry)
#define gl_mock_journal_entry_get_message(GlMockJournalEntry) gl_journal_entry_get_message(GlMockJournalEntry)
#define gl_mock_journal_entry_get_audit_session(GlMockJournalEntry) gl_journal_entry_get_audit_session(GlMockJournalEntry)
#define gl_mock_journal_entry_get_kernel_device(GlMockJournalEntry) gl_journal_entry_get_kernel_device(GlMockJournalEntry)
#define gl_mock_journal_entry_get_priority(GlMockJournalEntry) gl_journal_entry_get_priority(GlMockJournalEntry)
#define gl_mock_journal_entry_get_catalog(GlMockJournalEntry) gl_journal_entry_get_catalog(GlMockJournalEntry)
#define gl_mock_journal_entry_get_transport(GlMockJournalEntry) gl_journal_entry_get_transport(GlMockJournalEntry)
#define gl_mock_journal_entry_get_uid(GlMockJournalEntry) gl_journal_entry_get_uid(GlMockJournalEntry) 
#define gl_mock_journal_previous(GlMockJournal) gl_journal_previous(GlMockJournal)
#define gl_mock_journal_new(void) gl_journal_new(void)
#define gl_mock_journal_set_matches gl_journal_set_matches
#define gl_mock_journal_get_boot_ids(GlMockJournal) gl_journal_get_boot_ids(GlMockJournal)
G_BEGIN_DECLS

/*
 * GlMockJournalError:
 * @GL_MOCK_JOURNAL_ERROR_NO_FIELD: the requested field was not found in the current
 * journal entry
 * @GL_MOCK_JOURNAL_ERROR_INVALID_POINTER: the pointer to the current journal entry
 * is not valid
 * @GL_MOCK_JOURNAL_ERROR_FAILED: unknown failure
 */
typedef enum
{
    GL_MOCK_JOURNAL_ERROR_NO_FIELD,
    GL_MOCK_JOURNAL_ERROR_INVALID_POINTER,
    GL_MOCK_JOURNAL_ERROR_FAILED
} GlMockJournalError;

#define GL_MOCK_JOURNAL_ERROR gl_mock_journal_error_quark ()

GQuark gl_mock_journal_error_quark (void);

#define GL_TYPE_MOCK_JOURNAL_ENTRY gl_mock_journal_entry_get_type()
G_DECLARE_FINAL_TYPE (GlMockJournalEntry, gl_mock_journal_entry, GL, MOCK_JOURNAL_ENTRY, GObject)

typedef struct
{
    /*< private >*/
    GObject parent_instance;
} GlMockJournal;

typedef struct
{
    /*< private >*/
    GObjectClass parent_class;
} GlMockJournalClass;

typedef struct
{
    gchar *boot_match;
    guint64 realtime;
} GlMockJournalBootID;

#define GL_TYPE_MOCK_JOURNAL (gl_mock_journal_get_type ())
#define GL_MOCK_JOURNAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_MOCK_JOURNAL, GlMockJournal))

GType gl_mock_journal_result_get_type (void);
GType gl_mock_journal_get_type (void);
void gl_mock_journal_set_matches (GlMockJournal *journal, const gchar * const *matches);
GArray * gl_mock_journal_get_boot_ids (GlMockJournal *journal);
GlMockJournalEntry * gl_mock_journal_previous (GlMockJournal *journal);
GlMockJournal * gl_mock_journal_new (void);

guint64                 gl_mock_journal_entry_get_timestamp                  (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_message                    (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_command_line               (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_kernel_device              (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_audit_session              (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_transport                  (GlMockJournalEntry *entry);
const gchar *           gl_mock_journal_entry_get_catalog                    (GlMockJournalEntry *entry);
guint                   gl_mock_journal_entry_get_priority                   (GlMockJournalEntry *entry);
gint                    gl_mock_journal_entry_get_uid                        (GlMockJournalEntry *entry);

G_END_DECLS

#endif /* GL_MOCK_JOURNAL_H_ */
