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

#ifndef GL_CATEGORY_LIST_H_
#define GL_CATEGORY_LIST_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GtkWidget parent_instance;
} GlCategoryList;

typedef struct
{
    /*< private >*/
    GtkWidgetClass parent_class;
} GlCategoryListClass;

/*
 * GlCategoryListFilter:
 * @GL_CATEGORY_LIST_FILTER_IMPORTANT:
 * @GL_CATEGORY_LIST_FILTER_ALERTS:
 * @GL_CATEGORY_LIST_FILTER_STARRED:
 * @GL_CATEGORY_LIST_FILTER_ALL:
 * @GL_CATEGORY_LIST_FILTER_APPLICATIONS:
 * @GL_CATEGORY_LIST_FILTER_SYSTEM:
 * @GL_CATEGORY_LIST_FILTER_SECURITY:
 * @GL_CATEGORY_LIST_FILTER_HARDWARE:
 * @GL_CATEGORY_LIST_FILTER_UPDATES:
 * @GL_CATEGORY_LIST_FILTER_USAGE:
 *
 * The category, selected in #GlCategoryList, to filter the events by.
 */
typedef enum
{
    GL_CATEGORY_LIST_FILTER_IMPORTANT,
    GL_CATEGORY_LIST_FILTER_ALERTS,
    GL_CATEGORY_LIST_FILTER_STARRED,
    GL_CATEGORY_LIST_FILTER_ALL,
    GL_CATEGORY_LIST_FILTER_APPLICATIONS,
    GL_CATEGORY_LIST_FILTER_SYSTEM,
    GL_CATEGORY_LIST_FILTER_SECURITY,
    GL_CATEGORY_LIST_FILTER_HARDWARE,
    GL_CATEGORY_LIST_FILTER_UPDATES,
    GL_CATEGORY_LIST_FILTER_USAGE
} GlCategoryListFilter;

#define GL_TYPE_CATEGORY_LIST (gl_category_list_get_type ())
#define GL_CATEGORY_LIST(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_CATEGORY_LIST, GlCategoryList))

GType gl_category_list_get_type (void);
GtkWidget * gl_category_list_new (void);
GlCategoryListFilter gl_category_list_get_category (GlCategoryList *list);

G_END_DECLS

#endif /* GL_CATEGORY_LIST_H_ */
