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

#include "gl-window.h"

#include "libgd/gd.h"

G_DEFINE_TYPE (GlWindow, gl_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gl_window_class_init (GlWindowClass *klass)
{
}

static void
gl_window_init (GlWindow *window)
{
    GtkWidget *grid;
    GtkWidget *toolbar;
    GtkToolItem *item;
    GtkWidget *button;
    GtkWidget *image;
    GtkWidget *scrolled;
    GtkWidget *listbox;

    grid = gtk_grid_new ();

    toolbar = gd_main_toolbar_new ();
    gtk_widget_set_vexpand (toolbar, FALSE);
    gtk_grid_attach (GTK_GRID (grid), toolbar, 0, 0, 1, 1);

    toolbar = gtk_toolbar_new ();
    gtk_widget_set_vexpand (toolbar, FALSE);
    gtk_grid_attach (GTK_GRID (grid), toolbar, 1, 0, 1, 1);

    button = gtk_menu_button_new ();
    image = gtk_image_new_from_icon_name ("emblem-system-symbolic",
                                          GTK_ICON_SIZE_MENU);
    gtk_button_set_image (GTK_BUTTON (button), image);
    item = gtk_tool_item_new ();
    gtk_container_add (GTK_CONTAINER (item), button);
    gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (item));

    /* Category/group view. */
    listbox = gtk_list_box_new ();
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_grid_attach (GTK_GRID (grid), scrolled, 0, 1, 1, 1);

    /* Event view. */
    listbox = gtk_list_box_new ();
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_hexpand (scrolled, TRUE);
    gtk_widget_set_vexpand (scrolled, TRUE);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_grid_attach (GTK_GRID (grid), scrolled, 1, 1, 1, 1);

    gtk_container_add (GTK_CONTAINER (window), grid);

    gtk_widget_show_all (grid);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
