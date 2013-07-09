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

#include "config.h"
#include "gl-application.h"

#include <glib/gi18n.h>

#include "gl-window.h"

G_DEFINE_TYPE (GlApplication, gl_application, GTK_TYPE_APPLICATION)

static void
on_about (GSimpleAction *action,
          GVariant *parameter,
          gpointer user_data)
{
    GtkApplication *application;
    GtkWindow *parent;
    static const gchar* authors[] = {
        "David King <davidk@gnome.org>",
        NULL
    };

    application = GTK_APPLICATION (user_data);
    parent = gtk_application_get_active_window (GTK_APPLICATION (application));
    gtk_show_about_dialog (parent,
                           "authors", authors,
                           "comments", _("View and search logs"),
                           "copyright", "Copyright © 2013 Red Hat, Inc.",
                           "license-type", GTK_LICENSE_GPL_3_0,
                           "program-name", PACKAGE_NAME,
                           "version", PACKAGE_VERSION,
                           "website", PACKAGE_URL, NULL);
}

static void
on_quit (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    GApplication *application;

    application = G_APPLICATION (user_data);
    g_application_quit (application);
}

static GActionEntry actions[] = {
    { "about", on_about },
    { "quit", on_quit }
};

static void
gl_application_startup (GApplication *application)
{
    GtkBuilder *builder;
    GError *error = NULL;
    GMenuModel *appmenu;

    g_action_map_add_action_entries (G_ACTION_MAP (application), actions,
                                     G_N_ELEMENTS (actions), application);

    /* Calls gtk_init() with no arguments. */
    G_APPLICATION_CLASS (gl_application_parent_class)->startup (application);

    builder = gtk_builder_new ();
    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
    gtk_builder_add_from_resource (builder, "/org/gnome/Logs/appmenu.ui",
                                   &error);

    if (error != NULL)
    {
        g_error ("Unable to get app menu from resource: %s", error->message);
    }

    appmenu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
    gtk_application_set_app_menu (GTK_APPLICATION (application), appmenu);

    g_object_unref (builder);

}

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
    GApplicationClass *app_class;

    app_class = G_APPLICATION_CLASS (klass);
    app_class->activate = gl_application_activate;
    app_class->startup = gl_application_startup;
}

GtkApplication *
gl_application_new (void)
{
    return g_object_new (GL_TYPE_APPLICATION, "application-id",
                         "org.gnome.Logs", NULL);
}
