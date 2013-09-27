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
#include "gl-eventview.h"
#include "gl-enums.h"

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
on_mode (GSimpleAction *action,
         GVariant *variant,
         gpointer user_data)
{
    /* TODO: Switch toolbar mode. */
    g_simple_action_set_state (action, variant);
}

static void
on_provider_parsing_error (GtkCssProvider *provider,
                           GtkCssSection *section,
                           GError *error,
                           gpointer user_data)
{
    g_critical ("Error while parsing CSS style (line: %u, character: %u): %s",
                gtk_css_section_get_end_line (section) + 1,
                gtk_css_section_get_end_position (section) + 1,
                error->message);
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
    GBytes *data;
    GError *err = NULL;
    GtkCssProvider *provider;
    GdkScreen *screen;

    gtk_widget_init_template (GTK_WIDGET (window));

    g_action_map_add_action_entries (G_ACTION_MAP (window), actions,
                                     G_N_ELEMENTS (actions), window);

    data = g_resources_lookup_data ("/org/gnome/Logs/gl-style.css",
                                    G_RESOURCE_LOOKUP_FLAGS_NONE, &err);

    if (err != NULL)
    {
        g_critical ("Error loading CSS styling data: %s", err->message);
        g_error_free (err);
        return;
    }

    provider = gtk_css_provider_get_default ();
    g_signal_connect (provider, "parsing-error",
                      G_CALLBACK (on_provider_parsing_error), NULL);
    gtk_css_provider_load_from_data (provider, g_bytes_get_data (data, NULL),
                                     g_bytes_get_size (data), &err);

    if (err != NULL)
    {
        g_critical ("Error parsing CSS styling data: %s", err->message);
        g_error_free (err);
        g_object_unref (provider);
        return;
    }

    screen = gdk_screen_get_default ();
    gtk_style_context_add_provider_for_screen (screen,
                                               GTK_STYLE_PROVIDER (provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
