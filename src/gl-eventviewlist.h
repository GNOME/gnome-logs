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

#ifndef GL_EVENT_VIEW_LIST_H_
#define GL_EVENT_VIEW_LIST_H_

#include <gtk/gtk.h>
#include "gl-journal.h"

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GtkBox parent_instance;
} GlEventViewList;

typedef struct
{
    /*< private >*/
    GtkBoxClass parent_class;
} GlEventViewListClass;

#define GL_TYPE_EVENT_VIEW_LIST (gl_event_view_list_get_type ())
#define GL_EVENT_VIEW_LIST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_EVENT_VIEW_LIST, GlEventViewList))

GType gl_event_view_list_get_type (void);
GtkWidget * gl_event_view_list_new (void);
void gl_event_view_list_search (GlEventViewList *view, const gchar *needle);
GlJournalResult *gl_event_view_list_get_detail_result (GlEventViewList *view);
gboolean gl_event_view_list_handle_search_event (GlEventViewList *view,
                                                 GAction *action,
                                                 GdkEvent *event);
void gl_event_view_list_set_search_mode (GlEventViewList *view, gboolean state);

G_END_DECLS

#endif /* GL_EVENT_VIEW_LIST_H_ */
