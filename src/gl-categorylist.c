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

#include "gl-enums.h"
#include "gl-eventview.h"

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
    GtkWidget *toplevel;
    GActionMap *appwindow;
    GAction *category;
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_category_list_get_instance_private (listbox);
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (listbox));

    if (gtk_widget_is_toplevel (toplevel))
    {
        appwindow = G_ACTION_MAP (toplevel);
        category = g_action_map_lookup_action (appwindow, "category");
    }
    else
    {
        g_return_if_reached ();
    }

    eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_FILTER);

    if (row == GTK_LIST_BOX_ROW (priv->important))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_IMPORTANT);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->alerts))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_ALERTS);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->starred))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_STARRED);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->all))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_ALL);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->applications))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_APPLICATIONS);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->system))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_SYSTEM);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->security))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_SECURITY);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->hardware))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_HARDWARE);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->updates))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_UPDATES);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->usage))
    {
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_FILTER_USAGE);
    }
    else
    {
        g_assert_not_reached ();
    }

    g_action_activate (category, g_variant_new_string (evalue->value_nick));

    g_type_class_unref (eclass);
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
gl_category_list_header_func (GtkListBoxRow *row,
                              GtkListBoxRow *before,
                              gpointer user_data)
{
    if (before != NULL && (gtk_list_box_row_get_header (row) == NULL))
    {
        gtk_list_box_row_set_header (row,
                                     gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
    }
}

static void
gl_category_list_init (GlCategoryList *list)
{
    GlCategoryListPrivate *priv;

    gtk_widget_init_template (GTK_WIDGET (list));
    gtk_list_box_set_header_func (GTK_LIST_BOX (list),
                                  gl_category_list_header_func, NULL, NULL);
    priv = gl_category_list_get_instance_private (list);

    gtk_list_box_select_row (GTK_LIST_BOX (list),
                             GTK_LIST_BOX_ROW (priv->all));
}

GtkWidget *
gl_category_list_new (void)
{
    return g_object_new (GL_TYPE_CATEGORY_LIST, NULL);
}
