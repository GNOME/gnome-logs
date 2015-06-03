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
} MockGlJournalError;

#define GL_JOURNAL_ERROR gl_journal_error_quark ()

GQuark mock_gl_journal_error_quark (void);

#define GL_TYPE_JOURNAL_ENTRY gl_journal_entry_get_type()
G_DECLARE_FINAL_TYPE (MockGlJournalEntry, gl_journal_entry, GL, JOURNAL_ENTRY, GObject)

typedef struct
{
    /*< private >*/
    GObject parent_instance;
} MockGlJournal;

typedef struct
{
    /*< private >*/
    GObjectClass parent_class;
} MockGlJournalClass;

#define GL_TYPE_JOURNAL (gl_journal_get_type ())
#define GL_JOURNAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_JOURNAL, GlJournal))

GType gl_journal_mock_result_get_type (void);
GType gl_journal_mock_get_type (void);
void gl_journal_mock_set_matches (MockGlJournal *journal, const gchar * const *matches);
MockGlJournalEntry * gl_journal_mock_previous (MockGlJournal *journal);
GlJournal * gl_journal_mock_new (void);

guint64                 gl_journal_mock_entry_get_timestamp                  (MockGlJournalEntry *entry);
const gchar *           gl_journal_mock_entry_get_message                    (MockGlJournalEntry *entry);
const gchar *           gl_journal_mock_entry_get_command_line               (MockGlJournalEntry *entry);
const gchar *           gl_journal_mock_entry_get_kernel_device              (MockGlJournalEntry *entry);
const gchar *           gl_journal_mock_entry_get_audit_session              (MockGlJournalEntry *entry);
const gchar *           gl_journal_mock_entry_get_catalog                    (MockGlJournalEntry *entry);
guint                   gl_journal_mock_entry_get_priority                   (MockGlJournalEntry *entry);

G_END_DECLS

#endif /* GL_JOURNAL_H_ */
