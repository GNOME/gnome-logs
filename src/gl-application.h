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

#ifndef GL_APPLICATION_H_
#define GL_APPLICATION_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct
{
    /*< private >*/
    GtkApplication parent_instance;
} GlApplication;

typedef struct
{
    /*< private >*/
    GtkApplicationClass parent_class;
} GlApplicationClass;

#define GL_TYPE_APPLICATION (gl_application_get_type ())
#define GL_APPLICATION(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GL_TYPE_APPLICATION, GlApplication))

GType gl_application_get_type (void);
GtkApplication * gl_application_new (void);

G_END_DECLS

#endif /* GL_APPLICATION_H_ */
