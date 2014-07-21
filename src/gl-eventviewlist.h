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

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GtkListBox parent_instance;
} GlEventViewList;

typedef struct
{
    /*< private >*/
    GtkListBoxClass parent_class;
} GlEventViewListClass;

/*
 * GlEventViewListFilter:
 * @GL_EVENT_VIEW_LIST_FILTER_IMPORTANT:
 * @GL_EVENT_VIEW_LIST_FILTER_ALERTS:
 * @GL_EVENT_VIEW_LIST_FILTER_STARRED:
 * @GL_EVENT_VIEW_LIST_FILTER_ALL:
 * @GL_EVENT_VIEW_LIST_FILTER_APPLICATIONS:
 * @GL_EVENT_VIEW_LIST_FILTER_SYSTEM:
 * @GL_EVENT_VIEW_LIST_FILTER_SECURITY:
 * @GL_EVENT_VIEW_LIST_FILTER_HARDWARE:
 * @GL_EVENT_VIEW_LIST_FILTER_UPDATES:
 * @GL_EVENT_VIEW_LIST_FILTER_USAGE:
 *
 * The category, selected in #GlCategoryList, to filter the events by.
 */
typedef enum
{
    GL_EVENT_VIEW_LIST_FILTER_IMPORTANT,
    GL_EVENT_VIEW_LIST_FILTER_ALERTS,
    GL_EVENT_VIEW_LIST_FILTER_STARRED,
    GL_EVENT_VIEW_LIST_FILTER_ALL,
    GL_EVENT_VIEW_LIST_FILTER_APPLICATIONS,
    GL_EVENT_VIEW_LIST_FILTER_SYSTEM,
    GL_EVENT_VIEW_LIST_FILTER_SECURITY,
    GL_EVENT_VIEW_LIST_FILTER_HARDWARE,
    GL_EVENT_VIEW_LIST_FILTER_UPDATES,
    GL_EVENT_VIEW_LIST_FILTER_USAGE
} GlEventViewListFilter;

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

#define GL_TYPE_EVENT_VIEW_LIST (gl_event_view_list_get_type ())
#define GL_EVENT_VIEW_LIST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_EVENT_VIEW_LIST, GlEventViewList))

GType gl_event_view_list_get_type (void);
GtkWidget * gl_event_view_list_new (void);
void gl_event_view_list_search (GlEventViewList *view, const gchar *needle);
void gl_event_view_list_set_filter (GlEventViewList *view, GlEventViewListFilter filter);
void gl_event_view_list_set_mode (GlEventViewList *view, GlEventViewMode mode);

G_END_DECLS

#endif /* GL_EVENT_VIEW_LIST_H_ */
