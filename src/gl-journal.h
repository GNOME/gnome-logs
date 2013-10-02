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

#ifndef GL_JOURNAL_H_
#define GL_JOURNAL_H_

#include <glib-object.h>
#include <systemd/sd-journal.h>

G_BEGIN_DECLS

typedef struct
{
    gsize n_results;
    gchar **matches;
} GlJournalQuery;

typedef struct
{
    guint64 timestamp;
    gchar *cursor;
    const gchar *message;
    const gchar *comm;
    const gchar *kernel_device;
    const gchar *audit_session;
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

#define GL_TYPE_JOURNAL (gl_journal_get_type ())
#define GL_JOURNAL(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_JOURNAL, GlJournal))

GType gl_journal_get_type (void);
GList * gl_journal_query (GlJournal *self, const GlJournalQuery *query);
sd_journal * gl_journal_get_journal (GlJournal *self);
GlJournal * gl_journal_new (void);

G_END_DECLS

#endif /* GL_JOURNAL_H_ */
