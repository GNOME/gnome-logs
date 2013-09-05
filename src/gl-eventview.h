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

#ifndef GL_EVENT_VIEW_H_
#define GL_EVENT_VIEW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GtkListBox parent_instance;
} GlEventView;

typedef struct
{
    /*< private >*/
    GtkListBoxClass parent_class;
} GlEventViewClass;

/*
 * GlEventViewFilter:
 * @GL_EVENT_VIEW_FILTER_IMPORTANT:
 * @GL_EVENT_VIEW_FILTER_ALERTS:
 * @GL_EVENT_VIEW_FILTER_STARRED:
 * @GL_EVENT_VIEW_FILTER_ALL:
 * @GL_EVENT_VIEW_FILTER_APPLICATIONS:
 * @GL_EVENT_VIEW_FILTER_SYSTEM:
 * @GL_EVENT_VIEW_FILTER_SECURITY:
 * @GL_EVENT_VIEW_FILTER_HARDWARE:
 * @GL_EVENT_VIEW_FILTER_UPDATES:
 * @GL_EVENT_VIEW_FILTER_USAGE:
 *
 * The category, selected in #GlCategoryList, to filter the events by.
 */
typedef enum
{
    GL_EVENT_VIEW_FILTER_IMPORTANT,
    GL_EVENT_VIEW_FILTER_ALERTS,
    GL_EVENT_VIEW_FILTER_STARRED,
    GL_EVENT_VIEW_FILTER_ALL,
    GL_EVENT_VIEW_FILTER_APPLICATIONS,
    GL_EVENT_VIEW_FILTER_SYSTEM,
    GL_EVENT_VIEW_FILTER_SECURITY,
    GL_EVENT_VIEW_FILTER_HARDWARE,
    GL_EVENT_VIEW_FILTER_UPDATES,
    GL_EVENT_VIEW_FILTER_USAGE
} GlEventViewFilter;

#define GL_TYPE_EVENT_VIEW (gl_event_view_get_type ())
#define GL_EVENT_VIEW(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_EVENT_VIEW, GlEventView))

GType gl_event_view_get_type (void);
GtkWidget * gl_event_view_new (void);
void gl_event_view_set_filter (GlEventView *view, GlEventViewFilter filter);

G_END_DECLS

#endif /* GL_EVENT_VIEW_H_ */
