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

typedef struct
{
    GtkWidget *important;
    GtkWidget *alerts;
    GtkWidget *starred;
    GtkWidget *all;
    GtkWidget *applications;
    GtkWidget *system;
    GtkWidget *security;
    GtkWidget *hardware;
    GtkWidget *updates;
    GtkWidget *usage;
} GlCategoryListPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlCategoryList, gl_category_list, GTK_TYPE_LIST_BOX)

static void
on_gl_category_list_row_activated (GlCategoryList *listbox,
                                   GtkListBoxRow *row,
                                   gpointer user_data)
{
    GlCategoryListPrivate *priv;

    priv = gl_category_list_get_instance_private (listbox);

    if (row == GTK_LIST_BOX_ROW (priv->important))
    {
        g_message ("Important clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->alerts))
    {
        g_message ("Alerts clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->starred))
    {
        g_message ("Starred clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->all))
    {
        g_message ("All clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->applications))
    {
        g_message ("Applications clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->system))
    {
        g_message ("System clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->security))
    {
        g_message ("Security clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->hardware))
    {
        g_message ("Hardware clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->updates))
    {
        g_message ("Updates clicked");
    }
    else if (row == GTK_LIST_BOX_ROW (priv->usage))
    {
        g_message ("Usage clicked");
    }
}

static void
gl_category_list_class_init (GlCategoryListClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-categorylist.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  important);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  alerts);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  starred);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  all);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  applications);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  system);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  security);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  hardware);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  updates);
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  usage);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_category_list_row_activated);
}

static void
gl_category_list_init (GlCategoryList *list)
{
    GlCategoryListPrivate *priv;

    gtk_widget_init_template (GTK_WIDGET (list));
    priv = gl_category_list_get_instance_private (list);

    gtk_list_box_select_row (GTK_LIST_BOX (list),
                             GTK_LIST_BOX_ROW (priv->all));
}

GtkWidget *
gl_category_list_new (void)
{
    return g_object_new (GL_TYPE_CATEGORY_LIST, NULL);
}
