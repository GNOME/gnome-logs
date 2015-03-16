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

#include "gl-categorylist.h"
#include "gl-eventtoolbar.h"
#include "gl-eventview.h"
#include "gl-eventviewlist.h"
#include "gl-enums.h"
#include "gl-util.h"

typedef struct
{
    GtkWidget *event_toolbar;
    GtkWidget *event;
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
on_action_toggle (GSimpleAction *action,
                  GVariant *variant,
                  gpointer user_data)
{
    GVariant *variant_state;
    gboolean state;

    variant_state = g_action_get_state (G_ACTION (action));
    state = g_variant_get_boolean (variant_state);

    g_action_change_state (G_ACTION (action), g_variant_new_boolean (!state));
}

static void
on_close (GSimpleAction *action,
          GVariant *variant,
          gpointer user_data)
{
    GtkWindow *window;

    window = GTK_WINDOW (user_data);

    gtk_window_close (window);
}

static void
on_toolbar_mode (GSimpleAction *action,
                 GVariant *variant,
                 gpointer user_data)
{
    GlWindowPrivate *priv;
    const gchar *mode;
    GEnumClass *eclass;
    GEnumValue *evalue;
    GAction *search;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    mode = g_variant_get_string (variant, NULL);
    eclass = g_type_class_ref (GL_TYPE_EVENT_TOOLBAR_MODE);
    evalue = g_enum_get_value_by_nick (eclass, mode);
    search = g_action_map_lookup_action (G_ACTION_MAP (user_data), "search");

    if (evalue->value == GL_EVENT_TOOLBAR_MODE_LIST)
    {
        /* Switch the event view back to list mode if the back button is
         * clicked. */
        GlEventView *view;

        view = GL_EVENT_VIEW (priv->event);

        gl_event_view_set_mode (view, GL_EVENT_VIEW_MODE_LIST);

        g_simple_action_set_enabled (G_SIMPLE_ACTION (search), TRUE);
    }
    else
    {
        g_simple_action_set_enabled (G_SIMPLE_ACTION (search), FALSE);
        g_action_change_state (search, g_variant_new_boolean (FALSE));
    }

    g_simple_action_set_state (action, variant);

    g_type_class_unref (eclass);
}

static void
on_view_mode (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    GlWindowPrivate *priv;
    const gchar *mode;
    GlEventToolbar *toolbar;
    GlEventView *event;
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    mode = g_variant_get_string (variant, NULL);
    event = GL_EVENT_VIEW (priv->event);
    toolbar = GL_EVENT_TOOLBAR (priv->event_toolbar);
    eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_MODE);
    evalue = g_enum_get_value_by_nick (eclass, mode);

    switch (evalue->value)
    {
        case GL_EVENT_VIEW_MODE_LIST:
            gl_event_toolbar_set_mode (toolbar, GL_EVENT_TOOLBAR_MODE_LIST);
            break;
        case GL_EVENT_VIEW_MODE_DETAIL:
            gl_event_toolbar_set_mode (toolbar, GL_EVENT_TOOLBAR_MODE_DETAIL);
            gl_event_view_set_mode (event, GL_EVENT_VIEW_MODE_DETAIL);
            break;
    }

    g_simple_action_set_state (action, variant);

    g_type_class_unref (eclass);
}

static void
on_search (GSimpleAction *action,
           GVariant *variant,
           gpointer user_data)
{
    gboolean state;
    GlWindowPrivate *priv;
    GlEventView *view;

    state = g_variant_get_boolean (variant);
    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    view = GL_EVENT_VIEW (priv->event);

    gl_event_view_set_search_mode (view, state);

    g_simple_action_set_state (action, variant);
}

static gboolean
on_gl_window_key_press_event (GlWindow *window,
                              GdkEvent *event,
                              gpointer user_data)
{
    GlWindowPrivate *priv;
    GlEventView *view;
    GAction *action;
    GVariant *variant;
    const gchar *mode;
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_window_get_instance_private (window);
    action = g_action_map_lookup_action (G_ACTION_MAP (window), "search");
    view = GL_EVENT_VIEW (priv->event);

    if (gl_event_view_handle_search_event (view, action, event) == GDK_EVENT_STOP)
    {
        return GDK_EVENT_STOP;
    }

    action = g_action_map_lookup_action (G_ACTION_MAP (window), "view-mode");
    variant = g_action_get_state (action);
    mode = g_variant_get_string (variant, NULL);
    eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_MODE);
    evalue = g_enum_get_value_by_nick (eclass, mode);

    g_variant_unref (variant);

    switch (evalue->value)
    {
        case GL_EVENT_VIEW_MODE_LIST:
            break; /* Ignored, as there is no back button. */
        case GL_EVENT_VIEW_MODE_DETAIL:
            if (gl_event_toolbar_handle_back_button_event (GL_EVENT_TOOLBAR (priv->event_toolbar),
                                                           (GdkEventKey*)event)
                == GDK_EVENT_STOP)
            {
                GlEventView *view;

                view = GL_EVENT_VIEW (priv->event);
                gl_event_view_set_mode (view, GL_EVENT_VIEW_MODE_LIST);
                g_type_class_unref (eclass);

                return GDK_EVENT_STOP;
            }
            break;
    }

    g_type_class_unref (eclass);

    return GDK_EVENT_PROPAGATE;
}

void
gl_window_set_sort_order (GlWindow *window,
                          GlSortOrder sort_order)
{
    GlWindowPrivate *priv;
    GlEventView *event;

    g_return_if_fail (GL_WINDOW (window));

    priv = gl_window_get_instance_private (window);
    event = GL_EVENT_VIEW (priv->event);

    gl_event_view_set_sort_order (event, sort_order);
}

static GActionEntry actions[] = {
    { "view-mode", on_action_radio, "s", "'list'", on_view_mode },
    { "toolbar-mode", on_action_radio, "s", "'list'", on_toolbar_mode },
    { "search", on_action_toggle, NULL, "false", on_search },
    { "close", on_close }
};

static void
gl_window_class_init (GlWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-window.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  event_toolbar);
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  event);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_window_key_press_event);
}

static void
gl_window_init (GlWindow *window)
{
    GtkCssProvider *provider;
    GdkScreen *screen;

    gtk_widget_init_template (GTK_WIDGET (window));

    g_action_map_add_action_entries (G_ACTION_MAP (window), actions,
                                     G_N_ELEMENTS (actions), window);

    provider = gtk_css_provider_new ();
    g_signal_connect (provider, "parsing-error",
                      G_CALLBACK (gl_util_on_css_provider_parsing_error),
                      NULL);
    gtk_css_provider_load_from_resource (provider,
                                         "/org/gnome/Logs/gl-style.css");

    screen = gdk_screen_get_default ();
    gtk_style_context_add_provider_for_screen (screen,
                                               GTK_STYLE_PROVIDER (provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref (provider);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
