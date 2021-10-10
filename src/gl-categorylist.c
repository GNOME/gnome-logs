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
#include "gl-eventviewlist.h"

enum
{
    PROP_0,
    PROP_CATEGORY,
    N_PROPERTIES
};

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
    GtkWidget *list_box;
    GlCategoryListFilter category;
} GlCategoryListPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlCategoryList, gl_category_list, GTK_TYPE_WIDGET)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static gboolean
gl_category_list_focus (GtkWidget *list, GtkDirectionType direction)
{
    GlCategoryListPrivate *priv = gl_category_list_get_instance_private (GL_CATEGORY_LIST (list));

    switch (direction)
    {
        case GTK_DIR_TAB_BACKWARD:
        case GTK_DIR_TAB_FORWARD:
            if (gtk_widget_get_focus_child (GTK_WIDGET (priv->list_box)))
            {
                /* Force tab events which jump to another child to jump out of
                 * the category list. */
                return FALSE;
            }
            else
            {
                /* Allow tab events to focus into the widget. */
                GTK_WIDGET_CLASS (gl_category_list_parent_class)->focus (priv->list_box,
                                                                         direction);
                return TRUE;
            }
            break;
        /* Allow the widget to handle all other focus events. */
        default:
            GTK_WIDGET_CLASS (gl_category_list_parent_class)->focus (priv->list_box,
                                                                     direction);
            return TRUE;
            break;
    }
}

static void
on_gl_category_list_row_selected (GtkListBox     *listbox,
                                  GtkListBoxRow  *row,
                                  GlCategoryList *list)
{
    GlCategoryListPrivate *priv;
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_category_list_get_instance_private (list);
    eclass = g_type_class_ref (GL_TYPE_CATEGORY_LIST_FILTER);

    if (row == GTK_LIST_BOX_ROW (priv->important))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_IMPORTANT);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->alerts))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_ALERTS);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->starred))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_STARRED);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->all))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_ALL);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->applications))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_APPLICATIONS);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->system))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_SYSTEM);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->security))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_SECURITY);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->hardware))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_HARDWARE);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->updates))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_UPDATES);
    }
    else if (row == GTK_LIST_BOX_ROW (priv->usage))
    {
        evalue = g_enum_get_value (eclass, GL_CATEGORY_LIST_FILTER_USAGE);
    }
    else
    {
        /* This is only for the occasion when GlCategoryList is destroyed,
         * in other words when there are no children for GlCategoryList */
        return;
    }

    priv->category = evalue->value;

    g_object_notify_by_pspec (G_OBJECT (list),
                              obj_properties[PROP_CATEGORY]);

    g_type_class_unref (eclass);
}

GlCategoryListFilter
gl_category_list_get_category (GlCategoryList *list)
{
    GlCategoryListPrivate *priv;

    priv = gl_category_list_get_instance_private (list);

    return priv->category;
}

static void
gl_category_list_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    GlCategoryList *list = GL_CATEGORY_LIST (object);
    GlCategoryListPrivate *priv = gl_category_list_get_instance_private (list);

    switch (prop_id)
    {
        case PROP_CATEGORY:
            g_value_set_enum (value, priv->category);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_category_list_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    GlCategoryList *list = GL_CATEGORY_LIST (object);
    GlCategoryListPrivate *priv = gl_category_list_get_instance_private (list);

    switch (prop_id)
    {
        case PROP_CATEGORY:
            priv->category = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_category_list_dispose (GObject *object)
{
    GlCategoryList *list = GL_CATEGORY_LIST (object);
    GlCategoryListPrivate *priv = gl_category_list_get_instance_private (list);

    gtk_widget_unparent (priv->list_box);

    G_OBJECT_CLASS (gl_category_list_parent_class)->dispose (object);
}

static void
gl_category_list_class_init (GlCategoryListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->get_property = gl_category_list_get_property;
    gobject_class->set_property = gl_category_list_set_property;
    gobject_class->dispose = gl_category_list_dispose;
    widget_class->focus = gl_category_list_focus;

    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

    obj_properties[PROP_CATEGORY] = g_param_spec_enum ("category", "Category",
                                                       "Filter events by",
                                                       GL_TYPE_CATEGORY_LIST_FILTER,
                                                       GL_CATEGORY_LIST_FILTER_ALL,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);

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
    gtk_widget_class_bind_template_child_private (widget_class, GlCategoryList,
                                                  list_box);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_category_list_row_selected);
}

static void
gl_category_list_header_func (GtkListBoxRow *row,
                              GtkListBoxRow *before,
                              GtkListBoxRow *applications)
{
    if (before != NULL && (gtk_list_box_row_get_header (row) == NULL)
        && row == applications)
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

    priv = gl_category_list_get_instance_private (list);
    gtk_list_box_set_header_func (GTK_LIST_BOX (priv->list_box),
                                  (GtkListBoxUpdateHeaderFunc)gl_category_list_header_func,
                                  priv->applications, NULL);

    g_signal_connect (priv->list_box, "row-selected",
                      G_CALLBACK (on_gl_category_list_row_selected), list);
}

GtkWidget *
gl_category_list_new (void)
{
    return g_object_new (GL_TYPE_CATEGORY_LIST, NULL);
}
