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

#include "gl-application.h"

#include <glib/gi18n.h>

#include "gl-window.h"

G_DEFINE_TYPE (GlApplication, gl_application, GTK_TYPE_APPLICATION)

static void
gl_application_activate (GApplication *app)
{
    GtkWidget *widget;

    widget = gl_window_new (GTK_APPLICATION (app));
    gtk_widget_show (widget);
}

static void
gl_application_init (GlApplication *application)
{
}

static void
gl_application_class_init (GlApplicationClass *klass)
{
    G_APPLICATION_CLASS (klass)->activate = gl_application_activate;
}

GtkApplication *
gl_application_new (void)
{
    return g_object_new (GL_TYPE_APPLICATION, "application-id",
                         "org.gnome.Logs", NULL);
}
