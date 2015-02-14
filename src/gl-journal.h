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
} GlJournalError;

#define GL_JOURNAL_ERROR gl_journal_error_quark ()

GQuark gl_journal_error_quark (void);

typedef struct
{
    /*< private >*/
    guint ref_count;

    /*< public >*/
    guint64 timestamp;
    gchar *cursor;
    gchar *message;
    gchar *comm;
    gchar *kernel_device;
    gchar *audit_session;
    gchar *catalog;
    guint priority;
} GlJournalResult;

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

#define GL_TYPE_JOURNAL_RESULT (gl_journal_result_get_type ())
#define GL_TYPE_JOURNAL (gl_journal_get_type ())
#define GL_JOURNAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_JOURNAL, GlJournal))

GType gl_journal_result_get_type (void);
GType gl_journal_get_type (void);
void gl_journal_query_async (GlJournal *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GList * gl_journal_query_finish (GlJournal *self, GAsyncResult *res, GError **error);
GList * gl_journal_query (GlJournal *self);
void gl_journal_set_matches (GlJournal *journal, const gchar * const *matches);
GlJournalResult * gl_journal_result_ref (GlJournalResult *result);
void gl_journal_result_unref (GlJournalResult *result);
void gl_journal_results_free (GList *results);
GlJournal * gl_journal_new (void);

G_END_DECLS

#endif /* GL_JOURNAL_H_ */
