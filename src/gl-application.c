/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014, 2015  Red Hat, Inc.
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

#include "gl-categorylist.h"
#include "gl-eventtoolbar.h"
#include "gl-eventviewlist.h"
#include "gl-util.h"
#include "gl-window.h"

struct _GlApplication
{
    /*< private >*/
    GtkApplication parent_instance;
};

typedef struct
{
    GSettings *desktop;
    GSettings *settings;
    gchar *monospace_font;
} GlApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlApplication, gl_application, GTK_TYPE_APPLICATION)

static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar SETTINGS_SCHEMA[] = "org.gnome.Logs";
static const gchar DESKTOP_MONOSPACE_FONT_NAME[] = "monospace-font-name";
static const gchar SORT_ORDER[] = "sort-order";

static void
on_new_window (GSimpleAction *action,
               GVariant *parameter,
               gpointer user_data)
{
    GtkApplication *application;
    GtkWidget *window;

    application = GTK_APPLICATION (user_data);

    window = gl_window_new (GTK_APPLICATION (application));
    gtk_widget_show (window);
}

static void
on_help (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    GtkApplication *application;
    GtkWindow *parent;
    GError *error = NULL;

    application = GTK_APPLICATION (user_data);
    parent = gtk_application_get_active_window (application);

    gtk_show_uri_on_window (parent, "help:gnome-logs",
                            GDK_CURRENT_TIME, &error);

    if (error)
    {
        g_debug ("Error while opening help: %s", error->message);
        g_error_free (error);
    }
}

static void
on_about (GSimpleAction *action,
          GVariant *parameter,
          gpointer user_data)
{
    GtkApplication *application;
    GtkWindow *parent;
    static const gchar* artists[] = {
        "Jakub Steiner <jimmac@gmail.com>",
        "Lapo Calamandrei <calamandrei@gmail.com>",
        NULL
    };
    static const gchar* authors[] = {
        "David King <davidk@gnome.org>",
        "Jonathan Kang <jonathan121537@gmail.com>",
        NULL
    };

    application = GTK_APPLICATION (user_data);
    parent = gtk_application_get_active_window (GTK_APPLICATION (application));
    gtk_show_about_dialog (parent,
                           "authors", authors,
                           "artists", artists,
                           "translator-credits", _("translator-credits"),
                           "comments", _("View and search logs"),
                           "copyright", "Copyright © 2013–2015 Red Hat, Inc.\nCopyright © 2014-2015 Jonathan Kang",
                           "license-type", GTK_LICENSE_GPL_3_0,
                           "logo-icon-name", PACKAGE_TARNAME,
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

static void
on_sort_order_changed (GSettings *settings,
                       const gchar *key,
                       GlApplication *application)
{
    GList *window;
    gint sort_order;

    window = gtk_application_get_windows (GTK_APPLICATION (application));
    sort_order = g_settings_get_enum (settings, SORT_ORDER);

    /* refresh the event view list for every logs window */
    while (window != NULL)
    {
        gl_window_set_sort_order (window->data, sort_order);
        window = g_list_next (window);
    }
}

static void
on_monospace_font_name_changed (GSettings *settings,
                                const gchar *key,
                                GlApplicationPrivate *priv)
{
    gchar *font_name;

    font_name = g_settings_get_string (settings, key);

    if (g_strcmp0 (font_name, priv->monospace_font) != 0)
    {
        GtkCssProvider *provider;
        gchar *css_fragment;
        gchar *css_desc;
        PangoFontDescription *font_desc;

        g_free (priv->monospace_font);
        priv->monospace_font = font_name;

        font_desc = pango_font_description_from_string (font_name);
        css_desc = pango_font_description_to_css (font_desc);
        css_fragment = g_strconcat (".event-monospace ", css_desc, NULL);

        provider = gtk_css_provider_new ();
        g_signal_connect (provider, "parsing-error",
                          G_CALLBACK (gl_util_on_css_provider_parsing_error),
                          NULL);
        gtk_css_provider_load_from_data (provider, css_fragment, -1, NULL);

        gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        g_free (css_desc);
        g_free (css_fragment);
        g_object_unref (provider);
        pango_font_description_free (font_desc);
    }
    else
    {
        g_free (font_name);
    }
}

static GActionEntry actions[] = {
    { "new-window", on_new_window },
    { "help", on_help },
    { "about", on_about },
    { "quit", on_quit }
};

static void
gl_application_startup (GApplication *application)
{
    g_action_map_add_action_entries (G_ACTION_MAP (application), actions,
                                     G_N_ELEMENTS (actions), application);

    /* Calls gtk_init() with no arguments. */
    G_APPLICATION_CLASS (gl_application_parent_class)->startup (application);

    /* gtk_init() calls setlocale(), so gettext must be called after that. */
    g_set_application_name (_(PACKAGE_NAME));
    gtk_window_set_default_icon_name (PACKAGE_TARNAME);

    /* Must register custom types before using them from GtkBuilder. */
    gl_window_get_type ();
    gl_category_list_get_type ();
    gl_event_toolbar_get_type ();
    gl_event_view_list_get_type ();
}

static void
gl_application_activate (GApplication *application)
{
    GtkWidget *window;
    GlApplicationPrivate *priv;
    const gchar * const close_accel[] = { "<Primary>w", NULL };
    const gchar * const search_accel[] = { "<Primary>f", NULL };

    window = gl_window_new (GTK_APPLICATION (application));
    gtk_widget_show (window);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "win.close", close_accel);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "win.search", search_accel);

    priv = gl_application_get_instance_private (GL_APPLICATION (application));

    on_monospace_font_name_changed (priv->desktop, DESKTOP_MONOSPACE_FONT_NAME,
                                    priv);
}

static const GOptionEntry options[] =
{
    { "version", 'v', 0, G_OPTION_ARG_NONE, NULL,
      N_("Print version information and exit"), NULL },
    { NULL }
};

static gint
gl_application_handle_local_options (GApplication *application,
                                     GVariantDict *options)
{
    if (g_variant_dict_contains (options, "version"))
    {
        g_print ("%s - Version %s\n", g_get_application_name (), PACKAGE_VERSION);
        return 0;
    }

    return -1;
}

static void
gl_application_finalize (GObject *object)
{
    GlApplication *application;
    GlApplicationPrivate *priv;

    application = GL_APPLICATION (object);
    priv = gl_application_get_instance_private (application);

    g_clear_object (&priv->desktop);
    g_clear_object (&priv->settings);
    g_clear_pointer (&priv->monospace_font, g_free);
}

static void
gl_application_init (GlApplication *application)
{
    GlApplicationPrivate *priv;
    gchar *changed_font;
    GAction *action;

    priv = gl_application_get_instance_private (application);

    priv->monospace_font = NULL;
    priv->desktop = g_settings_new (DESKTOP_SCHEMA);

    g_application_add_main_option_entries (G_APPLICATION (application), options);

    changed_font = g_strconcat ("changed::", DESKTOP_MONOSPACE_FONT_NAME, NULL);
    g_signal_connect (priv->desktop, changed_font,
                      G_CALLBACK (on_monospace_font_name_changed),
                      priv);

    priv->settings = g_settings_new (SETTINGS_SCHEMA);
    action = g_settings_create_action (priv->settings, SORT_ORDER);
    g_action_map_add_action (G_ACTION_MAP (application), action);

    g_signal_connect (priv->settings, "changed::sort-order",
                      G_CALLBACK (on_sort_order_changed),
                      application);

    g_object_unref (action);
    g_free (changed_font);
}

static void
gl_application_class_init (GlApplicationClass *klass)
{
    GObjectClass *gobject_class;
    GApplicationClass *app_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = gl_application_finalize;

    app_class = G_APPLICATION_CLASS (klass);
    app_class->activate = gl_application_activate;
    app_class->startup = gl_application_startup;
    app_class->handle_local_options = gl_application_handle_local_options;
}

GtkApplication *
gl_application_new (void)
{
    return g_object_new (GL_TYPE_APPLICATION, "application-id",
                         "org.gnome.Logs", NULL);
}
