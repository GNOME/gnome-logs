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

#ifndef GL_EVENT_VIEW_LIST_H_
#define GL_EVENT_VIEW_LIST_H_

#include <gtk/gtk.h>

#include "gl-application.h"
#include "gl-journal.h"
#include "gl-journal-model.h"

G_BEGIN_DECLS

#define GL_TYPE_EVENT_VIEW_LIST (gl_event_view_list_get_type ())
G_DECLARE_FINAL_TYPE (GlEventViewList, gl_event_view_list, GL, EVENT_VIEW_LIST, GtkListBox)

GtkWidget * gl_event_view_list_new (void);
GlRowEntry *gl_event_view_list_get_detail_entry (GlEventViewList *view);
gboolean gl_event_view_list_handle_search_event (GlEventViewList *view,
                                                 GAction *action,
                                                 GdkEvent *event);
void gl_event_view_list_set_search_mode (GlEventViewList *view, gboolean state);
void gl_event_view_list_set_sort_order (GlEventViewList *view, GlSortOrder  sort_order);
void gl_event_view_list_view_boot (GlEventViewList *view, const gchar *match);
GArray * gl_event_view_list_get_boot_ids (GlEventViewList *view);
gchar * gl_event_view_list_get_output_logs (GlEventViewList *view);
gchar * gl_event_view_list_get_boot_time (GlEventViewList *view,
                                          const gchar *boot_match);

G_END_DECLS

#endif /* GL_EVENT_VIEW_LIST_H_ */
