/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014, 2015  Red Hat, Inc.
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

#ifndef GL_EVENT_VIEW_ROW_H_
#define GL_EVENT_VIEW_ROW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "gl-journal.h"
#include "gl-util.h"

/*
 * GlEventViewRowStyle:
 * @GL_EVENT_VIEW_ROW_STYLE_CMDLINE: show the command-line in bold, if it
 * exists, as well as the log message
 * @GL_EVENT_VIEW_ROW_STYLE_SIMPLE: show only the event message and timestamp
 *
 * The style for the row.
 */
typedef enum
{
    GL_EVENT_VIEW_ROW_STYLE_CMDLINE,
    GL_EVENT_VIEW_ROW_STYLE_SIMPLE
} GlEventViewRowStyle;

#define GL_TYPE_EVENT_VIEW_ROW (gl_event_view_row_get_type ())
G_DECLARE_FINAL_TYPE (GlEventViewRow, gl_event_view_row, GL, EVENT_VIEW_ROW, GtkListBoxRow)

GtkWidget * gl_event_view_row_new (GlJournalEntry *entry, GlEventViewRowStyle style, GlUtilClockFormat clock_format);
GlJournalEntry * gl_event_view_row_get_entry (GlEventViewRow *row);
GtkWidget * gl_event_view_row_get_message_label (GlEventViewRow *row);
GtkWidget * gl_event_view_row_get_time_label (GlEventViewRow *row);

G_END_DECLS

#endif /* GL_EVENT_VIEW_ROW_H_ */
