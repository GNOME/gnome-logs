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
#include "gl-enums.h"
#include "gl-util.h"

typedef struct
{
    GtkWidget *event_toolbar;
    GtkWidget *event_search;
    GtkWidget *search_entry;
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
on_category (GSimpleAction *action,
             GVariant *variant,
             gpointer user_data)
{
    GlWindowPrivate *priv;
    const gchar *category;
    GlEventView *events;
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    category = g_variant_get_string (variant, NULL);
    events = GL_EVENT_VIEW (priv->events);
    eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_FILTER);
    evalue = g_enum_get_value_by_nick (eclass, category);

    gl_event_view_set_filter (events, evalue->value);

    g_simple_action_set_state (action, variant);

    g_type_class_unref (eclass);
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

        view = GL_EVENT_VIEW (priv->events);

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
    GEnumClass *eclass;
    GEnumValue *evalue;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    mode = g_variant_get_string (variant, NULL);
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

    state = g_variant_get_boolean (variant);
    priv = gl_window_get_instance_private (GL_WINDOW (user_data));

    gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (priv->event_search), state);

    if (state)
    {
        gtk_widget_grab_focus (priv->search_entry);
        gtk_editable_set_position (GTK_EDITABLE (priv->search_entry), -1);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->search_entry), "");
    }

    g_simple_action_set_state (action, variant);
}

static gboolean
on_gl_window_key_press_event (GlWindow *window,
                              GdkEvent *event,
                              gpointer user_data)
{
    GlWindowPrivate *priv;
    GAction *search;

    priv = gl_window_get_instance_private (window);
    search = g_action_map_lookup_action (G_ACTION_MAP (window), "search");

    if (g_action_get_enabled (search))
    {
        if (gtk_search_bar_handle_event (GTK_SEARCH_BAR (priv->event_search),
                                         event) == GDK_EVENT_STOP)
        {
            g_action_change_state (search, g_variant_new_boolean (TRUE));

            return GDK_EVENT_STOP;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

static void
on_gl_window_search_entry_changed (GtkSearchEntry *entry,
                                   gpointer user_data)
{
    GlWindowPrivate *priv;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));

    gl_event_view_search (GL_EVENT_VIEW (priv->events),
                          gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));
}

static void
on_gl_window_search_bar_notify_search_mode_enabled (GtkSearchBar *search_bar,
                                                    GParamSpec *pspec,
                                                    gpointer user_data)
{
    GAction *search;

    search = g_action_map_lookup_action (G_ACTION_MAP (user_data), "search");
    g_action_change_state (search,
                           g_variant_new_boolean (gtk_search_bar_get_search_mode (search_bar)));
}

static GActionEntry actions[] = {
    { "category", on_action_radio, "s", "'important'", on_category },
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
                                                  event_search);
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  search_entry);
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  events);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_window_key_press_event);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_window_search_entry_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_window_search_bar_notify_search_mode_enabled);
}

static void
gl_window_init (GlWindow *window)
{
    GFile *file;
    GtkCssProvider *provider;
    GError *err = NULL;
    GdkScreen *screen;

    gtk_widget_init_template (GTK_WIDGET (window));

    g_action_map_add_action_entries (G_ACTION_MAP (window), actions,
                                     G_N_ELEMENTS (actions), window);

    file = g_file_new_for_uri ("resource:///org/gnome/Logs/gl-style.css");
    provider = gtk_css_provider_new ();
    g_signal_connect (provider, "parsing-error",
                      G_CALLBACK (gl_util_on_css_provider_parsing_error),
                      NULL);
    gtk_css_provider_load_from_file (provider, file, &err);

    if (err != NULL)
    {
        g_critical ("Error parsing CSS styling data: %s", err->message);
        g_error_free (err);
        g_object_unref (file);
        g_object_unref (provider);
        return;
    }

    screen = gdk_screen_get_default ();
    gtk_style_context_add_provider_for_screen (screen,
                                               GTK_STYLE_PROVIDER (provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref (file);
    g_object_unref (provider);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
