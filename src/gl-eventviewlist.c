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

#include "gl-eventviewlist.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>

#include "gl-categorylist.h"
#include "gl-enums.h"
#include "gl-eventtoolbar.h"
#include "gl-eventview.h"
#include "gl-eventviewdetail.h"
#include "gl-eventviewrow.h"
#include "gl-journal-model.h"
#include "gl-util.h"
#include "gl-searchpopover.h"

struct _GlEventViewList
{
    /*< private >*/
    GtkBox parent_instance;
};

typedef struct
{
    GlJournalModel *journal_model;
    GlJournalEntry *entry;
    GlUtilClockFormat clock_format;
    GtkListBox *entries_box;
    GtkSizeGroup *category_sizegroup;
    GtkSizeGroup *message_sizegroup;
    GtkSizeGroup *time_sizegroup;
    GtkWidget *categories;
    GtkWidget *event_search;
    GtkWidget *event_scrolled;
    GtkWidget *search_entry;
    GtkWidget *search_dropdown_button;
    GtkWidget *search_popover;
    GlSearchPopoverJournalFieldFilter journal_search_field;
    GlQuerySearchType search_type;
    GlSearchPopoverJournalTimestampRange journal_timestamp_range;
    gchar *search_text;
    gchar *boot_match;
} GlEventViewListPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewList, gl_event_view_list, GTK_TYPE_BOX)

static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar SETTINGS_SCHEMA[] = "org.gnome.Logs";
static const gchar CLOCK_FORMAT[] = "clock-format";
static const gchar SORT_ORDER[] = "sort-order";

gchar *
gl_event_view_list_get_output_logs (GlEventViewList *view)
{
    gchar *output_buf = NULL;
    gint index = 0;
    GOutputStream *stream;
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    stream = g_memory_output_stream_new_resizable ();

    while (gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->entries_box),
                                          index) != NULL)
    {
        const gchar *comm;
        const gchar *message;
        gchar *output_text;
        gchar *time;
        GDateTime *now;
        guint64 timestamp;
        GtkListBoxRow *row;

        row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->entries_box),
                                             index);

        /* Only output search results.
         * Search results are entries that are visible and child visible */
        if (gtk_widget_get_mapped (GTK_WIDGET (row)) == FALSE
            || gtk_widget_get_visible (GTK_WIDGET (row)) == FALSE)
        {
            index++;
            continue;
        }

        comm = gl_event_view_row_get_command_line (GL_EVENT_VIEW_ROW (row));
        message = gl_event_view_row_get_message (GL_EVENT_VIEW_ROW (row));
        timestamp = gl_event_view_row_get_timestamp (GL_EVENT_VIEW_ROW (row));
        now = g_date_time_new_now_local ();
        time = gl_util_timestamp_to_display (timestamp, now,
                                             priv->clock_format, TRUE);

        output_text = g_strconcat (time, " ",
                                   comm ? comm : "kernel", ": ",
                                   message, "\n", NULL);
        index++;

        g_output_stream_write (stream, output_text, strlen (output_text),
                               NULL, NULL);

        g_date_time_unref (now);
        g_free (time);
        g_free (output_text);
    }

    output_buf = g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (stream));

    g_output_stream_close (stream, NULL, NULL);

    return output_buf;
}



static void
listbox_update_header_func (GtkListBoxRow *row,
                            GtkListBoxRow *before,
                            gpointer user_data)
{
    GtkWidget *current;

    if (before == NULL)
    {
        gtk_list_box_row_set_header (row, NULL);
        return;
    }

    current = gtk_list_box_row_get_header (row);

    if (current == NULL)
    {
        current = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_show (current);
        gtk_list_box_row_set_header (row, current);
    }
}



static void
on_listbox_row_activated (GtkListBox *listbox,
                          GtkListBoxRow *row,
                          GlEventViewList *view)
{
    GlEventViewListPrivate *priv;
    GtkWidget *toplevel;

    priv = gl_event_view_list_get_instance_private (view);
    priv->entry = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row));

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

    if (gtk_widget_is_toplevel (toplevel))
    {
        GAction *mode;
        GEnumClass *eclass;
        GEnumValue *evalue;

        mode = g_action_map_lookup_action (G_ACTION_MAP (toplevel), "view-mode");
        eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_MODE);
        evalue = g_enum_get_value (eclass, GL_EVENT_VIEW_MODE_DETAIL);

        g_action_activate (mode, g_variant_new_string (evalue->value_nick));

        g_type_class_unref (eclass);
    }
    else
    {
        g_debug ("Widget not in toplevel window, not switching toolbar mode");
    }
}

GlJournalEntry *
gl_event_view_list_get_detail_entry (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    return priv->entry;
}

gchar *
gl_event_view_list_get_boot_time (GlEventViewList *view,
                                  const gchar *boot_match)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    return gl_journal_model_get_boot_time (priv->journal_model,
                                           boot_match);
}

GArray *
gl_event_view_list_get_boot_ids (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    return gl_journal_model_get_boot_ids (priv->journal_model);
}

gboolean
gl_event_view_list_handle_search_event (GlEventViewList *view,
                                        GAction *action,
                                        GdkEvent *event)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (g_action_get_enabled (action))
    {
        if (gtk_search_bar_handle_event (GTK_SEARCH_BAR (priv->event_search),
                                         event) == GDK_EVENT_STOP)
        {
            g_action_change_state (action, g_variant_new_boolean (TRUE));

            return GDK_EVENT_STOP;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

void
gl_event_view_list_set_search_mode (GlEventViewList *view,
                                    gboolean state)
{
    GlEventViewListPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

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
}

static GtkWidget *
gl_event_view_create_empty (G_GNUC_UNUSED GlEventViewList *view)
{
    GtkWidget *box;
    GtkStyleContext *context;
    GtkWidget *image;
    GtkWidget *label;
    gchar *markup;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand (box, TRUE);
    gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand (box, TRUE);
    context = gtk_widget_get_style_context (box);
    gtk_style_context_add_class (context, "dim-label");

    image = gtk_image_new_from_icon_name ("action-unavailable-symbolic", 0);
    context = gtk_widget_get_style_context (image);
    gtk_style_context_add_class (context, "dim-label");
    gtk_image_set_pixel_size (GTK_IMAGE (image), 128);
    gtk_container_add (GTK_CONTAINER (box), image);

    label = gtk_label_new (NULL);
    /* Translators: Shown when there are no (zero) results in the current
     * view. */
    markup = g_markup_printf_escaped ("<big>%s</big>", _("No results"));
    gtk_label_set_markup (GTK_LABEL (label), markup);
    gtk_container_add (GTK_CONTAINER (box), label);
    g_free (markup);

    gtk_widget_show_all (box);

    return box;
}

static GtkWidget *
gl_event_list_view_create_row_widget (gpointer item,
                                      gpointer user_data)
{
    GtkWidget *rtn;
    GtkWidget *message_label;
    GtkWidget *time_label;
    GlCategoryList *list;
    GlCategoryListFilter filter;
    GlEventViewList *view = user_data;

    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    list = GL_CATEGORY_LIST (priv->categories);
    filter = gl_category_list_get_category (list);

    if (filter == GL_CATEGORY_LIST_FILTER_IMPORTANT)
    {
        GtkWidget *category_label;

        rtn = gl_event_view_row_new (item,
                                     priv->clock_format,
                                     GL_EVENT_VIEW_ROW_CATEGORY_IMPORTANT);

        category_label = gl_event_view_row_get_category_label (GL_EVENT_VIEW_ROW (rtn));
        gtk_size_group_add_widget (GTK_SIZE_GROUP (priv->category_sizegroup),
                                   category_label);
    }
    else
    {
        rtn = gl_event_view_row_new (item,
                                     priv->clock_format,
                                     GL_EVENT_VIEW_ROW_CATEGORY_NONE);
    }

    message_label = gl_event_view_row_get_message_label (GL_EVENT_VIEW_ROW (rtn));
    time_label = gl_event_view_row_get_time_label (GL_EVENT_VIEW_ROW (rtn));

    gtk_size_group_add_widget (GTK_SIZE_GROUP (priv->message_sizegroup),
                               message_label);
    gtk_size_group_add_widget (GTK_SIZE_GROUP (priv->time_sizegroup),
                               time_label);

    return rtn;
}

static gchar *
get_uid_match_field_value (void)
{
    GCredentials *creds;
    uid_t uid;
    gchar *str = NULL;

    creds = g_credentials_new ();
    uid = g_credentials_get_unix_user (creds, NULL);

    if (uid != -1)
        str = g_strdup_printf ("%d", uid);

    g_object_unref (creds);
    return str;
}

/* Get Boot ID for current boot match */
static gchar *
get_current_boot_id (const gchar *boot_match)
{
    g_return_val_if_fail (boot_match != NULL, NULL);

    gchar *boot_value;

    boot_value = strchr (boot_match, '=') + 1;

    return g_strdup (boot_value);
}

static void
query_add_category_matches (GlQuery *query,
                            GlCategoryList *list)
{
    GlCategoryListFilter filter;

    /* Add exact matches according to selected category */
    filter = gl_category_list_get_category (list);

    switch (filter)
    {
        case GL_CATEGORY_LIST_FILTER_IMPORTANT:
            {
              /* Alert or emergency priority. */
              gl_query_add_match (query, "PRIORITY", "0", GL_QUERY_SEARCH_TYPE_EXACT);
              gl_query_add_match (query, "PRIORITY", "1", GL_QUERY_SEARCH_TYPE_EXACT);
              gl_query_add_match (query, "PRIORITY", "2", GL_QUERY_SEARCH_TYPE_EXACT);
              gl_query_add_match (query, "PRIORITY", "3", GL_QUERY_SEARCH_TYPE_EXACT);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_ALL:
            {

            }
            break;

        case GL_CATEGORY_LIST_FILTER_APPLICATIONS:
            /* Allow all _TRANSPORT != kernel. Attempt to filter by only processes
             * owned by the same UID. */
            {
                gchar *uid_str;

                uid_str = get_uid_match_field_value ();

                gl_query_add_match (query, "_TRANSPORT", "journal", GL_QUERY_SEARCH_TYPE_EXACT);
                gl_query_add_match (query, "_TRANSPORT", "stdout", GL_QUERY_SEARCH_TYPE_EXACT);
                gl_query_add_match (query, "_TRANSPORT", "syslog", GL_QUERY_SEARCH_TYPE_EXACT);
                gl_query_add_match (query, "_UID", uid_str, GL_QUERY_SEARCH_TYPE_EXACT);

                g_free (uid_str);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_SYSTEM:
            {
                gl_query_add_match (query, "_TRANSPORT", "kernel", GL_QUERY_SEARCH_TYPE_EXACT);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_HARDWARE:
            {
                gl_query_add_match (query, "_TRANSPORT", "kernel", GL_QUERY_SEARCH_TYPE_EXACT);
                gl_query_add_match ( query, "_KERNEL_DEVICE", NULL, GL_QUERY_SEARCH_TYPE_EXACT);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_SECURITY:
            {
                gl_query_add_match (query, "_AUDIT_SESSION", NULL, GL_QUERY_SEARCH_TYPE_EXACT);
            }
            break;

        default:
            g_assert_not_reached ();
    }
}

static void
query_add_search_matches (GlQuery *query,
                          const gchar *search_text,
                          GlSearchPopoverJournalFieldFilter journal_search_field,
                          GlQuerySearchType search_type)
{
    switch (journal_search_field)
    {
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_ALL_AVAILABLE_FIELDS:
            gl_query_add_match (query, "_PID", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_UID", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_GID", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "MESSAGE", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_COMM", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_SYSTEMD_UNIT", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_KERNEL_DEVICE", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_AUDIT_SESSION", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            gl_query_add_match (query, "_EXE", search_text, GL_QUERY_SEARCH_TYPE_SUBSTRING);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_PID:
            gl_query_add_match (query, "_PID", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_UID:
            gl_query_add_match (query, "_UID", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_GID:
            gl_query_add_match (query, "_GID", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_MESSAGE:
            gl_query_add_match (query, "MESSAGE", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_PROCESS_NAME:
            gl_query_add_match (query, "_COMM", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_SYSTEMD_UNIT:
            gl_query_add_match (query, "_SYSTEMD_UNIT", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_KERNEL_DEVICE:
            gl_query_add_match (query, "_KERNEL_DEVICE", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_AUDIT_SESSION:
            gl_query_add_match (query, "_AUDIT_SESSION", search_text, search_type);
            break;
        case GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_EXECUTABLE_PATH:
            gl_query_add_match (query, "_EXE", search_text, search_type);
            break;
    }
}

static void
query_set_day_timestamps (GlQuery *query,
                          gint start_day_offset,
                          gint end_day_offset)
{
    GDateTime *now;
    GDateTime *today_start;
    GDateTime *today_end;
    guint64 start_timestamp;
    guint64 end_timestamp;

    now = g_date_time_new_now_local();

    today_start = g_date_time_new_local (g_date_time_get_year (now),
                                         g_date_time_get_month (now),
                                         g_date_time_get_day_of_month (now) - start_day_offset,
                                         23,
                                         59,
                                         59.0);

    start_timestamp = g_date_time_to_unix (today_start) * G_USEC_PER_SEC;

    today_end = g_date_time_new_local (g_date_time_get_year (now),
                                       g_date_time_get_month (now),
                                       g_date_time_get_day_of_month (now) - end_day_offset,
                                       0,
                                       0,
                                       0.0);

    end_timestamp = g_date_time_to_unix (today_end) * G_USEC_PER_SEC;

    gl_query_set_journal_timestamp_range (query, start_timestamp, end_timestamp);

    g_date_time_unref (now);
    g_date_time_unref (today_start);
    g_date_time_unref (today_end);
}

static void
query_add_journal_range_filter (GlQuery *query,
                                GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);
    /* Add range filters */
    switch (priv->journal_timestamp_range)
    {
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CURRENT_BOOT:
        {
            /* Get current boot id */
            gchar *boot_match = NULL;

            /* Don't add match when priv->boot_match equals NULL. This
             * happens when users don't have permissions to view both
             * system and user logs. */
            if (priv->boot_match != NULL)
            {
                boot_match = get_current_boot_id (priv->boot_match);
                gl_query_add_match (query, "_BOOT_ID", boot_match,
                                    GL_QUERY_SEARCH_TYPE_EXACT);
            }

            g_free (boot_match);
        }
            break;
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_PREVIOUS_BOOT:
        {
            GArray *boot_ids;
            GlJournalBootID *boot_id;

            boot_ids = gl_event_view_list_get_boot_ids (view);

            boot_id = &g_array_index (boot_ids, GlJournalBootID, boot_ids->len - 2);

            gl_query_set_journal_timestamp_range (query, boot_id->realtime_last, boot_id->realtime_first);
        }
            break;
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_TODAY:

            query_set_day_timestamps (query, 0, 0);

            break;
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_YESTERDAY:

            query_set_day_timestamps (query, 1, 1);

            break;
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_LAST_3_DAYS:

            query_set_day_timestamps (query, 0, 2);

            break;
        case GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CUSTOM:
        {
            GlSearchPopover *popover;
            guint64 custom_start_timestamp;
            guint64 custom_end_timestamp;

            popover = GL_SEARCH_POPOVER (priv->search_popover);

            custom_start_timestamp = gl_search_popover_get_custom_start_timestamp (popover);
            custom_end_timestamp = gl_search_popover_get_custom_end_timestamp (popover);

            gl_query_set_journal_timestamp_range (query, custom_start_timestamp, custom_end_timestamp);
            break;
        }
        default:

            /* By default, search the entire journal */
            break;
    }
}

/* Create query object according to selected category */
static GlQuery *
create_query_object (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;
    GlQuery *query;
    GlCategoryList *list;
    GSettings *settings;
    gint sort_order;

    priv = gl_event_view_list_get_instance_private (view);
    list = GL_CATEGORY_LIST (priv->categories);

    /* Get the sorting order from GSettings key */
    settings = g_settings_new (SETTINGS_SCHEMA);
    sort_order = g_settings_get_enum (settings, SORT_ORDER);

    /* Create new query object */
    query = gl_query_new ();

    /* Set journal timestamp range */
    query_add_journal_range_filter (query, view);

    query_add_category_matches (query, list);

    query_add_search_matches (query, priv->search_text, priv->journal_search_field, priv->search_type);

    gl_query_set_search_type (query, priv->search_type);

    gl_query_set_sort_order (query, sort_order);

    g_object_unref (settings);

    return query;
}

static void
on_notify_category (GlCategoryList *list,
                    GParamSpec *pspec,
                    gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlQuery *query;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);

    /* Create the query object */
    query = create_query_object (view);

    /* Set the created query on the journal model */
    gl_journal_model_take_query (priv->journal_model, query);
}

void
gl_event_view_list_view_boot (GlEventViewList *view, const gchar *match)
{
    GlEventViewListPrivate *priv;
    GlSearchPopover *popover;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);
    popover = GL_SEARCH_POPOVER (priv->search_popover);
    g_free (priv->boot_match);
    priv->boot_match = g_strdup (match);

    /* Make search popover journal timestamp range label consistent with
       event-toolbar boot selection menu */
    gl_search_popover_set_journal_timestamp_range_current_boot (popover);
}

void
gl_event_view_list_set_sort_order (GlEventViewList *view,
                                   GlSortOrder sort_order)
{
    GlEventViewListPrivate *priv;
    GlQuery *query;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

    /* Create the query object */
    query = create_query_object (view);

    /* The sort order is taken from the GSettings key irrespective
     * of current model sort order and the created query is passed
     * on to model */
    gl_journal_model_take_query (priv->journal_model, query);
}

static void
on_search_entry_changed (GtkSearchEntry *entry,
                         gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlQuery *query;

    view = GL_EVENT_VIEW_LIST (user_data);

    priv = gl_event_view_list_get_instance_private (view);

    g_free (priv->search_text);

    priv->search_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));

    /* Create the query object */
    query = create_query_object (view);

    /* Set the created query on the journal model */
    gl_journal_model_take_query (priv->journal_model, query);
}

static void
on_search_bar_notify_search_mode_enabled (GtkSearchBar *search_bar,
                                          GParamSpec *pspec,
                                          gpointer user_data)
{
    GAction *search;
    GtkWidget *toplevel;
    GActionMap *appwindow;

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (user_data));

    if (gtk_widget_is_toplevel (toplevel))
    {
        appwindow = G_ACTION_MAP (toplevel);
        search = g_action_map_lookup_action (appwindow, "search");
    }
    else
    {
        /* TODO: Investigate whether this only happens during dispose. */
        g_debug ("%s",
                 "Search bar activated while not in a toplevel");
        return;
    }

    g_action_change_state (search,
                           g_variant_new_boolean (gtk_search_bar_get_search_mode (search_bar)));
}

static void
gl_event_list_view_edge_reached (GtkScrolledWindow *scrolled,
                                 GtkPositionType    pos,
                                 gpointer           user_data)
{
    GlEventViewList *view = user_data;
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    if (pos == GTK_POS_BOTTOM)
        gl_journal_model_fetch_more_entries (priv->journal_model, FALSE);
}

static void
search_popover_journal_search_field_changed (GlSearchPopover *popover,
                                             GParamSpec *psec,
                                             GlEventViewList *view)
{
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);
    GlQuery *query;

    priv->journal_search_field = gl_search_popover_get_journal_search_field (popover);

    query = create_query_object (view);

    gl_journal_model_take_query (priv->journal_model, query);
}

static void
search_popover_search_type_changed (GlSearchPopover *popover,
                                    GParamSpec *psec,
                                    GlEventViewList *view)
{
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);
    GlQuery *query;

    priv->search_type = gl_search_popover_get_query_search_type (popover);

    query = create_query_object (view);

    gl_journal_model_take_query (priv->journal_model, query);
}

static void
search_popover_journal_timestamp_range_changed (GlSearchPopover *popover,
                                                GParamSpec *psec,
                                                GlEventViewList *view)
{
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);
    GlQuery *query;

    priv->journal_timestamp_range = gl_search_popover_get_journal_timestamp_range (popover);

    query = create_query_object (view);

    gl_journal_model_take_query (priv->journal_model, query);
}

/* Get the view elements from ui file and link it with the drop down button */
static void
set_up_search_popover (GlEventViewList *view)
{

    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    priv->search_popover = gl_search_popover_new ();

    /* Grab/Remove keyboard focus from popover menu when it is opened or closed */
    g_signal_connect (priv->search_popover, "show", (GCallback) gtk_widget_grab_focus, NULL);
    g_signal_connect_swapped (priv->search_popover, "closed", (GCallback) gtk_widget_grab_focus, view);

    g_signal_connect (priv->search_popover, "notify::journal-search-field",
                      G_CALLBACK (search_popover_journal_search_field_changed), view);
    g_signal_connect (priv->search_popover, "notify::search-type",
                      G_CALLBACK (search_popover_search_type_changed), view);
    g_signal_connect (priv->search_popover, "notify::journal-timestamp-range",
                      G_CALLBACK (search_popover_journal_timestamp_range_changed), view);

    /* Link the drop down button with search popover */
    gtk_menu_button_set_popover (GTK_MENU_BUTTON (priv->search_dropdown_button),
                                 priv->search_popover);
}

static void
gl_event_view_list_finalize (GObject *object)
{
    GlEventViewList *view = GL_EVENT_VIEW_LIST (object);
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    g_free (priv->boot_match);
    g_clear_object (&priv->journal_model);
    g_clear_pointer (&priv->search_text, g_free);
    g_object_unref (priv->category_sizegroup);
    g_object_unref (priv->message_sizegroup);
    g_object_unref (priv->time_sizegroup);
}

static void
gl_event_view_list_class_init (GlEventViewListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->finalize = gl_event_view_list_finalize;

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-eventviewlist.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  entries_box);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  categories);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  event_search);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  event_scrolled);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  search_entry);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewList,
                                                  search_dropdown_button);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_search_entry_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_search_bar_notify_search_mode_enabled);
}

static void
gl_event_view_list_init (GlEventViewList *view)
{
    GlCategoryList *categories;
    GlEventViewListPrivate *priv;
    GSettings *settings;

    gtk_widget_init_template (GTK_WIDGET (view));

    priv = gl_event_view_list_get_instance_private (view);

    priv->search_text = NULL;
    priv->boot_match = NULL;
    priv->category_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    priv->message_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    priv->time_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    categories = GL_CATEGORY_LIST (priv->categories);

    priv->journal_model = gl_journal_model_new ();
    g_application_bind_busy_property (g_application_get_default (), priv->journal_model, "loading");

    g_signal_connect (priv->event_scrolled, "edge-reached",
                      G_CALLBACK (gl_event_list_view_edge_reached), view);

    gtk_list_box_bind_model (GTK_LIST_BOX (priv->entries_box),
                             G_LIST_MODEL (priv->journal_model),
                             gl_event_list_view_create_row_widget,
                             view, NULL);

    gtk_list_box_set_header_func (GTK_LIST_BOX (priv->entries_box),
                                  (GtkListBoxUpdateHeaderFunc) listbox_update_header_func,
                                  NULL, NULL);
    gtk_list_box_set_placeholder (GTK_LIST_BOX (priv->entries_box),
                                  gl_event_view_create_empty (view));
    g_signal_connect (priv->entries_box, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), GTK_BOX (view));

    gtk_search_bar_connect_entry (GTK_SEARCH_BAR (priv->event_search),
                                  GTK_ENTRY (priv->search_entry));

    /* TODO: Monitor and propagate any GSettings changes. */
    settings = g_settings_new (DESKTOP_SCHEMA);
    priv->clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);
    g_object_unref (settings);

    set_up_search_popover (view);

    g_signal_connect (categories, "notify::category", G_CALLBACK (on_notify_category),
                      view);
}

GtkWidget *
gl_event_view_list_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_LIST, NULL);
}
