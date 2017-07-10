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
#include "gl-journal-model.h"
#include "gl-util.h"

/*
 * GlEventViewRowCategory:
 * @GL_EVENT_VIEW_ROW_CATEGORY_NONE:
 * @GL_EVENT_VIEW_ROW_CATEGORY_IMPORTANT:
 *
 * The category, a property of GlEventViewRow, to filter events from
 * "important" category.
 */
typedef enum
{
    GL_EVENT_VIEW_ROW_CATEGORY_NONE,
    GL_EVENT_VIEW_ROW_CATEGORY_IMPORTANT
} GlEventViewRowCategory;

#define GL_TYPE_EVENT_VIEW_ROW (gl_event_view_row_get_type ())
G_DECLARE_FINAL_TYPE (GlEventViewRow, gl_event_view_row, GL, EVENT_VIEW_ROW, GtkListBoxRow)

GtkWidget * gl_event_view_row_new (GlRowEntry *entry, GlUtilClockFormat clock_format, GlEventViewRowCategory category);
GlRowEntry * gl_event_view_row_get_entry (GlEventViewRow *row);
const gchar * gl_event_view_row_get_command_line (GlEventViewRow *row);
const gchar * gl_event_view_row_get_message (GlEventViewRow *row);
guint64 gl_event_view_row_get_timestamp (GlEventViewRow *row);
GtkWidget * gl_event_view_row_get_category_label (GlEventViewRow *row);
GtkWidget * gl_event_view_row_get_time_label (GlEventViewRow *row);

G_END_DECLS

#endif /* GL_EVENT_VIEW_ROW_H_ */
