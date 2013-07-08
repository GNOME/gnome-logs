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

#include "gl-categorylist.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (GlCategoryList, gl_category_list, GTK_TYPE_LIST_BOX)

static void
gl_category_list_class_init (GlCategoryListClass *klass)
{
}

static void
gl_category_list_init (GlCategoryList *list)
{
    GtkWidget *listbox;
    GtkWidget *label;
    GtkWidget *row;

    /* Category/group view. */
    listbox = GTK_WIDGET (list);
    label = gtk_label_new (_("Important"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Alerts"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Starred"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("All"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    row = gtk_list_box_row_new ();
    gtk_container_add (GTK_CONTAINER (row), label);
    gtk_container_add (GTK_CONTAINER (listbox), row);
    gtk_list_box_select_row (GTK_LIST_BOX (listbox), GTK_LIST_BOX_ROW (row));
    label = gtk_label_new (_("Applications"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("System"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Security"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Hardware"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Updates"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
    label = gtk_label_new (_("Usage"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (listbox), label);
}

GtkWidget *
gl_category_list_new (void)
{
    return g_object_new (GL_TYPE_CATEGORY_LIST, NULL);
}
