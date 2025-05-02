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

#include "gl-window.h"

#include <glib/gi18n.h>

#include "gl-categorylist.h"
#include "gl-eventtoolbar.h"
#include "gl-eventviewlist.h"
#include "gl-enums.h"
#include "gl-util.h"

struct _GlWindow
{
    /*< private >*/
    GtkApplicationWindow parent_instance;
};

typedef struct
{
    GtkWidget *event_toolbar;
    GtkWidget *event_list;
    GtkWidget *info_bar;
    GtkLabel *message_label;
} GlWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlWindow, gl_window, ADW_TYPE_APPLICATION_WINDOW)

static const gchar SETTINGS_SCHEMA[] = "org.gnome.Logs";
static const gchar IGNORE_WARNING[] = "ignore-warning";

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
on_search (GSimpleAction *action,
           GVariant *variant,
           gpointer user_data)
{
    gboolean state;
    GlWindowPrivate *priv;
    GlEventViewList *event_list;

    state = g_variant_get_boolean (variant);
    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);

    gl_event_view_list_set_search_mode (event_list, state);

    g_simple_action_set_state (action, variant);
}

static void
on_save_finish (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
    GlWindowPrivate *priv;
    GlEventViewList *event_list;
    GtkFileDialog *dialog;
    gboolean have_error = FALSE;
    gchar *file_content;
    GFile *output_file;
    GFileOutputStream *file_ostream;
    GError *error = NULL;
    GtkWidget *error_dialog;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);
    dialog = GTK_FILE_DIALOG (source_object);

    output_file = gtk_file_dialog_save_finish (dialog, res, &error);

    if (error != NULL)
    {
        g_warning ("Error while replacing exported log messages file: %s",
                   error->message);

        g_clear_error (&error);
        return;
    }

    file_content = gl_event_view_list_get_output_logs (event_list);
    file_ostream = g_file_replace (output_file, NULL, TRUE,
                                   G_FILE_CREATE_NONE, NULL, &error);
    if (error != NULL)
    {
        have_error = TRUE;

        g_warning ("Error while replacing exported log messages file: %s",
                   error->message);
        g_clear_error (&error);
    }

    /* Check against NULL pointer to avoid a crash when exporting and there
     * are no log entries. */
    if (file_content != NULL)
    {
        g_output_stream_write (G_OUTPUT_STREAM (file_ostream), file_content,
                               strlen (file_content), NULL, &error);
    }

    if (error != NULL)
    {
        have_error = TRUE;

        g_warning ("Error while replacing exported log messages file: %s",
                   error->message);
        g_clear_error (&error);
    }

    g_output_stream_close (G_OUTPUT_STREAM (file_ostream), NULL, &error);
    if (error != NULL)
    {
        have_error = TRUE;

        g_warning ("Error while replacing exported log messages file: %s",
                   error->message);
        g_clear_error (&error);
    }

    if (have_error == TRUE)
    {
        error_dialog = adw_message_dialog_new (GTK_WINDOW (user_data),
                                               _("Export Failed"),
                                               _("Unable to export log messages to a file"));
        adw_message_dialog_add_response (ADW_MESSAGE_DIALOG (error_dialog),
                                         "close", _("_Close"));
        adw_message_dialog_choose (ADW_MESSAGE_DIALOG (error_dialog),
                                   NULL, NULL, NULL);
    }

    g_free (file_content);
    g_object_unref (file_ostream);
    g_object_unref (output_file);
}

static void
on_export (GSimpleAction *action,
           GVariant *variant,
           gpointer user_data)
{
    GtkFileDialog *dialog;

    dialog = gtk_file_dialog_new ();

    gtk_file_dialog_set_initial_name (dialog, _("log messages"));

    gtk_file_dialog_save (dialog,
                          GTK_WINDOW (user_data),
                          NULL,
                          on_save_finish,
                          user_data);
}

static void
on_view_boot (GSimpleAction *action,
              GVariant *variant,
              gpointer user_data)
{
    GlWindowPrivate *priv;
    GlEventViewList *event_list;
    GlEventToolbar *toolbar;
    GArray *boot_ids;
    GlJournalBootID *boot_id;
    const gchar *boot_match;
    gchar *current_boot;
    gchar *latest_boot;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);
    toolbar = GL_EVENT_TOOLBAR (priv->event_toolbar);

    boot_match = g_variant_get_string (variant, NULL);

    gl_event_view_list_view_boot (event_list, boot_match);

    current_boot = gl_event_view_list_get_boot_time (event_list, boot_match);
    if (current_boot == NULL)
    {
        g_debug ("Error fetching the time using boot_match");
    }

    boot_ids = gl_event_view_list_get_boot_ids (event_list);
    boot_id = &g_array_index (boot_ids, GlJournalBootID,
                              boot_ids->len - 1);
    latest_boot = gl_event_view_list_get_boot_time (event_list,
                                                    boot_id->boot_match);

    gl_event_toolbar_change_current_boot (toolbar, current_boot, latest_boot);

    g_simple_action_set_state (action, variant);

    g_free (current_boot);
    g_free (latest_boot);
}

static void
on_category_list_changed (GlCategoryList *list,
                          GParamSpec *pspec,
                          gpointer user_data)
{
    GlWindowPrivate *priv;
    GlEventViewList *event_list;
    GlEventToolbar *toolbar;
    gchar *current_boot;
    const gchar *boot_match;

    priv = gl_window_get_instance_private (GL_WINDOW (user_data));
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);
    toolbar = GL_EVENT_TOOLBAR (priv->event_toolbar);

    boot_match = gl_event_view_list_get_boot_match (event_list);
    current_boot = gl_event_view_list_get_boot_time (event_list, boot_match);

    gl_event_toolbar_change_current_boot (toolbar, current_boot, current_boot);

    g_free (current_boot);
}

static void
on_help_button_clicked (GlWindow *window,
                        gint response_id,
                        gpointer user_data)
{
    GlWindowPrivate *priv;
    GtkWidget *error_dialog;
    g_autoptr (GError) error = NULL;

    priv = gl_window_get_instance_private (GL_WINDOW (window));

    g_app_info_launch_default_for_uri ("help:gnome-logs/permissions",
                                       NULL, &error);

    if (error != NULL)
    {
      error_dialog = adw_message_dialog_new (GTK_WINDOW (window),
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

    gtk_widget_set_visible (priv->info_bar, FALSE);
}

static void
on_ignore_button_clicked (GlWindow *window,
                          gint response_id,
                          gpointer user_data)
{
    GlWindowPrivate *priv;
    GSettings *settings;

    priv = gl_window_get_instance_private (GL_WINDOW (window));

    settings = g_settings_new (SETTINGS_SCHEMA);
    g_settings_set_boolean (settings, IGNORE_WARNING, TRUE);

    gtk_widget_set_visible (priv->info_bar, FALSE);

    g_object_unref (settings);
}

void
gl_window_set_sort_order (GlWindow *window,
                          GlSortOrder sort_order)
{
    GlWindowPrivate *priv;
    GlEventViewList *event_list;

    g_return_if_fail (GL_WINDOW (window));

    priv = gl_window_get_instance_private (window);
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);

    gl_event_view_list_set_sort_order (event_list, sort_order);
}

static GActionEntry actions[] = {
    { "search", on_action_toggle, NULL, "false", on_search },
    { "view-boot", on_action_radio, "s", "''", on_view_boot },
    { "export", on_export },
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
                                                  event_list);
    gtk_widget_class_bind_template_child_private (widget_class, GlWindow,
                                                  info_bar);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_help_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_ignore_button_clicked);
}

void
disable_export (GlWindow *window)
{
    GAction *action_export;

    action_export = g_action_map_lookup_action (G_ACTION_MAP (window),
                                                "export");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action_export), FALSE);
}

void
enable_export (GlWindow *window)
{
    GAction *action_export;

    action_export = g_action_map_lookup_action (G_ACTION_MAP (window),
                                                "export");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action_export), TRUE);
}

void
gl_window_load_journal (GlWindow *window,
                        GlJournal *journal)
{
    gchar *boot_match;
    GAction *action_view_boot;
    GArray *boot_ids;
    GVariant *variant;
    GlJournalBootID *boot_id;
    GlEventViewList *event_list;
    GlEventToolbar *toolbar;
    GlJournalModel *journal_model;
    GlWindowPrivate *priv;

    priv = gl_window_get_instance_private (window);
    toolbar = GL_EVENT_TOOLBAR (priv->event_toolbar);
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);

    journal_model = gl_event_view_list_get_journal_model (event_list);
    gl_journal_model_load_journal (journal_model, journal);

    boot_ids = gl_event_view_list_get_boot_ids (event_list);
    gl_event_toolbar_add_boots (toolbar, boot_ids);

    if (boot_ids->len > 0)
    {
        boot_id = &g_array_index (boot_ids, GlJournalBootID,
                                  boot_ids->len - 1);
        boot_match = boot_id->boot_match;

        action_view_boot = g_action_map_lookup_action (G_ACTION_MAP (window),
                                                       "view-boot");
        variant = g_variant_new_string (boot_match);
        g_action_change_state (action_view_boot, variant);
    }
}

static void
gl_window_init (GlWindow *window)
{
    GdkDisplay *display;
    GlWindowPrivate *priv;
    GlEventViewList *event_list;
    GtkWidget *categories;
    GlJournalStorage storage_type;
    GSettings *settings;
    gboolean ignore;
    GlJournalModel *model;
    GtkWindowGroup *window_group;

    /* Ensure these types that are used by the template have been
     * registered before calling gtk_widget_init_template().
     */
    g_type_ensure(GL_TYPE_CATEGORY_LIST);
    g_type_ensure(GL_TYPE_EVENT_TOOLBAR);
    g_type_ensure(GL_TYPE_EVENT_VIEW_LIST);

    gtk_widget_init_template (GTK_WIDGET (window));

    window_group = gtk_window_group_new ();
    gtk_window_group_add_window (window_group, GTK_WINDOW (window));
    g_object_unref (window_group);

    priv = gl_window_get_instance_private (window);
    event_list = GL_EVENT_VIEW_LIST (priv->event_list);

    g_action_map_add_action_entries (G_ACTION_MAP (window), actions,
                                     G_N_ELEMENTS (actions), window);

    categories = gl_event_view_list_get_category_list (event_list);
    g_signal_connect (GL_CATEGORY_LIST (categories), "notify::category",
                      G_CALLBACK (on_category_list_changed), window);

    model = gl_event_view_list_get_journal_model (event_list);

    g_signal_connect_swapped (model, "disable_export",
                              G_CALLBACK (disable_export), window);
    g_signal_connect_swapped (model, "enable_export",
                              G_CALLBACK (enable_export), window);

    settings = g_settings_new (SETTINGS_SCHEMA);
    ignore = g_settings_get_boolean (settings, IGNORE_WARNING);
    /* Don't show info_bar again if users have ever ignored the warning. */
    if (ignore)
    {
        g_object_unref (settings);
        return;
    }

    /* Show warnings based on storage type. */
    storage_type = gl_util_journal_storage_type ();
    switch (storage_type)
    {
        case GL_JOURNAL_STORAGE_PERSISTENT:
        {
            if (!gl_util_can_read_system_journal (GL_JOURNAL_STORAGE_PERSISTENT))
            {
                gtk_label_set_label (priv->message_label, _("Unable to read system logs"));

                gtk_widget_set_visible (priv->info_bar, TRUE);
            }

            if (!gl_util_can_read_user_journal ())
            {
                gtk_label_set_label (priv->message_label, _("Unable to read user logs"));

                gtk_widget_set_visible (priv->info_bar, TRUE);
            }
            break;
        }
        case GL_JOURNAL_STORAGE_VOLATILE:
        {
            if (!gl_util_can_read_system_journal (GL_JOURNAL_STORAGE_VOLATILE))
            {
                gtk_label_set_label (priv->message_label, _("Unable to read system logs"));

                gtk_widget_set_visible (priv->info_bar, TRUE);
            }
            break;
        }
        case GL_JOURNAL_STORAGE_NONE:
        {
            gtk_label_set_label (priv->message_label, _("No logs available"));

            gtk_widget_set_visible (priv->info_bar, TRUE);
            break;
        }
        default:
            g_assert_not_reached ();
    }

    g_object_unref (settings);
}

GtkWidget *
gl_window_new (GtkApplication *application)
{
    g_return_val_if_fail (GTK_APPLICATION (application), NULL);

    return g_object_new (GL_TYPE_WINDOW, "application", application, NULL);
}
