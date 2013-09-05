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

#include <glib/gi18n.h>
#include "libgd/gd.h"

#include "gl-categorylist.h"
#include "gl-eventview.h"

typedef struct
{
    GtkWidget *right_toolbar;
    GtkWidget *events;
} GlWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlWindow, gl_window, GTK_TYPE_APPLICATION_WINDOW)

static void
on_action_radio (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    g_action_change_state (G_ACTION (action), variant);
}

static void
on_category (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    GlWindowPrivate *priv;
    const gchar *category;
    GlEventView *events;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    category = g_variant_get_string (variant, NULL);
    events = GL_EVENT_VIEW (priv->events);

    /* TODO: Fetch strings from an enum generated with glib-mkenums. */
    if (g_strcmp0 (category, "important") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_IMPORTANT);
    }
    else if (g_strcmp0 (category, "alerts") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_ALERTS);
    }
    else if (g_strcmp0 (category, "starred") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_STARRED);
    }
    else if (g_strcmp0 (category, "all") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_ALL);
    }
    else if (g_strcmp0 (category, "applications") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_APPLICATIONS);
    }
    else if (g_strcmp0 (category, "system") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_SYSTEM);
    }
    else if (g_strcmp0 (category, "security") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_SECURITY);
    }
    else if (g_strcmp0 (category, "hardware") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_HARDWARE);
    }
    else if (g_strcmp0 (category, "updates") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_UPDATES);
    }
    else if (g_strcmp0 (category, "usage") == 0)
    {
        gl_event_view_set_filter (events, GL_EVENT_VIEW_FILTER_USAGE);
    }

    g_simple_action_set_state (action, variant);
}

static void
on_mode (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    /* TODO: Switch toolbar mode. */
    g_simple_action_set_state (action, variant);
}

static GActionEntry actions[] = {
    { "category", on_action_radio, "s", "'all'", on_category },
    { "mode", on_action_radio, "s", "'list'", on_mode }
};

static void
gl_window_class_init (GlWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-window.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  right_toolbar);
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  events);
}

static void
gl_window_init (GlWindow *window)
{
    gtk_widget_init_template (GTK_WIDGET (window));

    g_action_map_add_action_entries (G_ACTION_MAP (window), actions,
                                     G_N_ELEMENTS (actions), window);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
