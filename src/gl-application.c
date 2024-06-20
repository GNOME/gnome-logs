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
#include "gl-journal.h"
#include "gl-journal-model.h"
#include "gl-util.h"
#include "gl-window.h"

struct _GlApplication
{
    /*< private >*/
    AdwApplication parent_instance;
};

typedef struct
{
    GlJournal *journal;
    GSettings *desktop;
    GSettings *settings;
    gchar *monospace_font;
} GlApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlApplication, gl_application, ADW_TYPE_APPLICATION)

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
    GlApplicationPrivate *priv;

    application = GTK_APPLICATION (user_data);
    priv = gl_application_get_instance_private (GL_APPLICATION (application));

    window = gl_window_new (GTK_APPLICATION (application));
    gl_window_load_journal (GL_WINDOW (window), priv->journal);
    gtk_window_present (GTK_WINDOW (window));
}

static void
on_help_launch_cb (GtkUriLauncher *launcher,
                   GAsyncResult *res,
                   gpointer user_data)
{
  GtkWidget *error_dialog;
  GtkWindow *active_window = GTK_WINDOW (user_data);
  g_autoptr (GError) error = NULL;

  if (!gtk_uri_launcher_launch_finish (launcher, res, &error))
  {
    error_dialog = adw_message_dialog_new (active_window,
                                           _("Failed To Open Help"),
                                           NULL);
    adw_message_dialog_format_body (ADW_MESSAGE_DIALOG (error_dialog),
                                    _("Failed to open the given help URI: %s"),
                                    error->message);
    adw_message_dialog_add_response (ADW_MESSAGE_DIALOG (error_dialog),
                                     "close", _("_Close"));
    adw_message_dialog_choose (ADW_MESSAGE_DIALOG (error_dialog),
                               NULL, NULL, NULL);
  }
}

static void
on_help (GSimpleAction *action,
         GVariant *parameter,
         gpointer user_data)
{
    GtkUriLauncher *launcher;
    GtkApplication *self = GTK_APPLICATION (user_data);
    GtkWindow *active_window = gtk_application_get_active_window (self);

    launcher = gtk_uri_launcher_new ("help:gnome-logs");
    gtk_uri_launcher_launch (launcher,
                             active_window,
                             NULL,
                             (GAsyncReadyCallback) on_help_launch_cb,
                             NULL);
}

static void
on_about (GSimpleAction *action,
          GVariant *parameter,
          gpointer user_data)
{
    GtkApplication *application;
    GtkWindow *parent;
    static const gchar* designers[] = {
        "Jakub Steiner <jimmac@gmail.com>",
        "Lapo Calamandrei <calamandrei@gmail.com>",
        NULL
    };
    static const gchar* developers[] = {
        "David King <davidk@gnome.org>",
        "Jonathan Kang <jonathan121537@gmail.com>",
        NULL
    };

    application = GTK_APPLICATION (user_data);
    parent = gtk_application_get_active_window (GTK_APPLICATION (application));
    adw_show_about_window (parent,
                           "application-name", _("Logs"),
                           "application-icon", "org.gnome.Logs",
                           "developer-name", _("The GNOME Project"),
                           "developers", developers,
                           "designers", designers,
                           "translator-credits", _("translator-credits"),
                           "copyright", "Copyright © 2013–2015 Red Hat, Inc.\nCopyright © 2014-2015 Jonathan Kang",
                           "license-type", GTK_LICENSE_GPL_3_0,
                           "version", PACKAGE_VERSION,
                           "website", PACKAGE_URL,
                           "issue-url", "https://gitlab.gnome.org/GNOME/gnome-logs/-/issues/",
                           NULL);
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
        gtk_css_provider_load_from_data (provider, css_fragment, -1);

        gtk_style_context_add_provider_for_display (gdk_display_get_default (),
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
    { "about", on_about }
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
    gtk_window_set_default_icon_name ("org.gnome.Logs");
}

static void
launch_window (GApplication *application)
{
    GtkWidget *window;
    GlApplicationPrivate *priv;
    const gchar * const close_accel[] = { "<Primary>w", NULL };
    const gchar * const search_accel[] = { "<Primary>f", NULL };
    const gchar * const export_accel[] = { "<Primary>e", NULL };
    const gchar * const help_accel[] = { "F1", NULL };
    const gchar * const new_window_accel[] = { "<Primary>n", NULL };

    priv = gl_application_get_instance_private (GL_APPLICATION (application));

    window = gl_window_new (GTK_APPLICATION (application));
    gl_window_load_journal (GL_WINDOW (window), priv->journal);
    gtk_window_present (GTK_WINDOW (window));
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "win.close", close_accel);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "win.search", search_accel);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "win.export", export_accel);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "app.help", help_accel);
    gtk_application_set_accels_for_action (GTK_APPLICATION (application),
                                           "app.new-window", new_window_accel);

    on_monospace_font_name_changed (priv->desktop, DESKTOP_MONOSPACE_FONT_NAME,
                                    priv);
}

static void
gl_application_activate (GApplication *application)
{
    GlApplicationPrivate *priv;

    priv = gl_application_get_instance_private (GL_APPLICATION (application));
    priv->journal = gl_journal_new (NULL);
    launch_window (application);
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
gl_application_open (GApplication *application,
                     GFile **files,
                     gint n_files,
                     const gchar *hint)
{
    gint i;
    GPtrArray *array;
    GlApplicationPrivate *priv;

    priv = gl_application_get_instance_private (GL_APPLICATION (application));

    array = g_ptr_array_new ();
    for (i = 0; i < n_files; i++)
    {
        GFile *file;

        file = files[i];
        g_ptr_array_add (array, g_file_get_path (file));
    }

    g_ptr_array_add (array, NULL);

    priv->journal = gl_journal_new (array);
    launch_window (application);
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

    g_clear_object (&priv->journal);

    G_OBJECT_CLASS (gl_application_parent_class)->finalize (object);
}

static void
gl_application_init (GlApplication *application)
{
    GlApplicationPrivate *priv;
    gchar *changed_font;
    GAction *action;

    g_application_set_flags (G_APPLICATION (application), G_APPLICATION_HANDLES_OPEN);

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
    app_class->open = gl_application_open;
    app_class->startup = gl_application_startup;
    app_class->handle_local_options = gl_application_handle_local_options;
}

GtkApplication *
gl_application_new (void)
{
    return g_object_new (GL_TYPE_APPLICATION, "application-id",
                         "org.gnome.Logs", NULL);
}
