/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2014, 2015  Red Hat, Inc.
 *  Copyright (C) 2014  Jonathan Kang
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

#ifndef GL_EVENT_VIEW_H_
#define GL_EVENT_VIEW_H_

#include <gtk/gtk.h>

#include "gl-application.h"

G_BEGIN_DECLS

/*
 * GlEventViewMode:
 * @GL_EVENT_VIEW_MODE_LIST:
 * @GL_EVENT_VIEW_MODE_DETAIL:
 *
 * The mode, mirroring the GlEventToolbar mode, used to show events.
 */
typedef enum
{
    GL_EVENT_VIEW_MODE_LIST,
    GL_EVENT_VIEW_MODE_DETAIL
} GlEventViewMode;

#define GL_TYPE_EVENT_VIEW (gl_event_view_get_type ())
G_DECLARE_FINAL_TYPE (GlEventView, gl_event_view, GL, EVENT_VIEW, GtkStack)

GtkWidget * gl_event_view_new (void);
void gl_event_view_search (GlEventView *view, const gchar *needle);
void gl_event_view_set_mode (GlEventView *view, GlEventViewMode mode);
void gl_event_view_show_detail (GlEventView *view);
gboolean gl_event_view_handle_search_event (GlEventView *view, GAction *action, GdkEvent *event);
void gl_event_view_set_search_mode (GlEventView *view, gboolean state);
void gl_event_view_set_sort_order (GlEventView *view, GlSortOrder sort_order);
GArray * gl_event_view_get_boot_ids (GlEventView *view);
void gl_event_view_view_boot (GlEventView *view, const gchar *match);
gchar * gl_event_view_get_output_logs (GlEventView *view);
gchar * gl_event_view_get_current_boot_time (GlEventView *view,
                                             const gchar *boot_match);

G_END_DECLS

#endif /* GL_EVENT_VIEW_H_ */
