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
    gchar *search_text;
    const gchar *boot_match;
} GlEventViewListPrivate;

/* We define these two enum values as 2 and 3 to avoid the conflict with TRUE
 * and FALSE */
typedef enum
{
    LOGICAL_OR = 2,
    LOGICAL_AND = 3
} GlEventViewListLogic;

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

static gboolean
gl_event_view_search_is_case_sensitive (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;
    const gchar *search_text;

    priv = gl_event_view_list_get_instance_private (view);

    for (search_text = priv->search_text; search_text && *search_text;
         search_text = g_utf8_next_char (search_text))
    {
        gunichar c;

        c = g_utf8_get_char (search_text);

        if (g_unichar_isupper (c))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
utf8_strcasestr (const gchar *potential_hit,
                 const gchar *search_term)
{
  gchar *folded;
  gboolean matches;

  folded = g_utf8_casefold (potential_hit, -1);
  matches = strstr (folded, search_term) != NULL;

  g_free (folded);
  return matches;
}

static GPtrArray *
tokenize_search_string (gchar *search_text)
{
    gchar *field_name;
    gchar *field_value;
    GPtrArray *token_array;
    GScanner *scanner;

    token_array = g_ptr_array_new_with_free_func (g_free);
    scanner = g_scanner_new (NULL);
    scanner->config->cset_skip_characters = " =\t\n";
    g_scanner_input_text (scanner, search_text, strlen (search_text));

    do
    {
        g_scanner_get_next_token (scanner);
        if (scanner->value.v_identifier == NULL && scanner->token != '+')
        {
            break;
        }
        else if (scanner->token == '+')
        {
            g_ptr_array_add (token_array, g_strdup ("+"));

            g_scanner_get_next_token (scanner);
            if (scanner->value.v_identifier != NULL)
            {
                field_name = g_strdup (scanner->value.v_identifier);
                g_ptr_array_add (token_array, field_name);
            }
            else
            {
                field_name = NULL;
            }
        }
        else if (scanner->token == G_TOKEN_INT)
        {
            field_name = g_strdup_printf ("%lu", scanner->value.v_int);
            g_ptr_array_add (token_array, field_name);
        }
        else if (scanner->token == G_TOKEN_FLOAT)
        {
            field_name = g_strdup_printf ("%g", scanner->value.v_float);
            g_ptr_array_add (token_array, field_name);
        }
        else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
            if (token_array->len != 0)
            {
                g_ptr_array_add (token_array, g_strdup (" "));
            }

            field_name = g_strdup (scanner->value.v_identifier);
            g_ptr_array_add (token_array, field_name);
        }
        else
        {
            field_name = NULL;
        }

        g_scanner_get_next_token (scanner);
        if (scanner->token == G_TOKEN_INT)
        {
            field_value = g_strdup_printf ("%lu", scanner->value.v_int);
            g_ptr_array_add (token_array, field_value);
        }
        else if (scanner->token == G_TOKEN_FLOAT)
        {
            field_value = g_strdup_printf ("%g", scanner->value.v_float);
            g_ptr_array_add (token_array, field_value);
        }
        else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
            field_value = g_strdup (scanner->value.v_identifier);
            g_ptr_array_add (token_array, field_value);
        }
        else
        {
            field_value = NULL;
        }
    } while (field_name != NULL && field_value != NULL);

    g_scanner_destroy (scanner);

    return token_array;
}

static gboolean
calculate_match (GlJournalEntry *entry,
                 GPtrArray *token_array,
                 gboolean case_sensetive)
{
    const gchar *comm;
    const gchar *message;
    const gchar *kernel_device;
    const gchar *audit_session;
    gboolean matches;
    gchar *field_name;
    gchar *field_value;
    gint i;
    gint match_stack[10];
    guint match_count = 0;
    guint token_index = 0;

    comm = gl_journal_entry_get_command_line (entry);
    message = gl_journal_entry_get_message (entry);
    kernel_device = gl_journal_entry_get_kernel_device (entry);
    audit_session = gl_journal_entry_get_audit_session (entry);

    /* No logical AND or OR used in search text */
    if (token_array->len == 1 && case_sensetive == TRUE)
    {
        gchar *search_text;

        search_text = g_ptr_array_index (token_array, 0);

        if ((comm ? strstr (comm, search_text) : NULL)
            || (message ? strstr (message, search_text) : NULL)
            || (kernel_device ? strstr (kernel_device, search_text) : NULL)
            || (audit_session ? strstr (audit_session, search_text) : NULL))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (token_array->len == 1 && case_sensetive == FALSE)
    {
        gchar *search_text;

        search_text = g_ptr_array_index (token_array, 0);

        matches = (comm && utf8_strcasestr (comm, search_text)) ||
                  (message && utf8_strcasestr (message, search_text)) ||
                  (kernel_device && utf8_strcasestr (kernel_device, search_text)) ||
                  (audit_session && utf8_strcasestr (audit_session, search_text));

        return matches;
    }

    while (token_index < token_array->len)
    {
        field_name = g_ptr_array_index (token_array, token_index);
        token_index++;

        if (token_index == token_array->len)
        {
            break;
        }

        field_value = g_ptr_array_index (token_array, token_index);
        token_index++;

        if (case_sensetive)
        {
            matches = (strstr ("_COMM", field_name) &&
                       comm &&
                       strstr (comm, field_value)) ||
                      (strstr ("_MESSAGE", field_name) &&
                       message &&
                       strstr (message, field_value)) ||
                      (strstr ("_KERNEL_DEVICE", field_name) &&
                       kernel_device &&
                       strstr (kernel_device, field_value)) ||
                      (strstr ("_AUDIT_SESSION", field_name) &&
                       audit_session &&
                       strstr (audit_session, field_value));
        }
        else
        {
            matches = (utf8_strcasestr ("_comm", field_name) &&
                       comm &&
                       strstr (comm, field_value)) ||
                      (utf8_strcasestr ("_message", field_name) &&
                       message &&
                       strstr (message, field_value)) ||
                      (utf8_strcasestr ("_kernel_device", field_name) &&
                       kernel_device &&
                       strstr (kernel_device, field_value)) ||
                      (utf8_strcasestr ("_audit_session", field_name) &&
                       audit_session &&
                       strstr (audit_session, field_value));
        }

        match_stack[match_count] = matches;
        match_count++;

        if (token_index == token_array->len)
        {
            break;
        }

        if (g_strcmp0 (g_ptr_array_index (token_array, token_index), " ") == 0)
        {
            match_stack[match_count] = LOGICAL_AND;
            match_count++;
            token_index++;
        }
        else if (g_strcmp0 (g_ptr_array_index (token_array, token_index),
                            "+") == 0)
        {
            match_stack[match_count] = LOGICAL_OR;
            match_count++;
            token_index++;
        }
    }

    /* match_count > 2 means there are still matches to be calculated in the
     * stack */
    if (match_count > 2)
    {
        /* calculate the expression with logical AND */
        for (i = 0; i < match_count; i++)
        {
            if (match_stack[i] == LOGICAL_AND)
            {
                int j;

                match_stack[i - 1] = match_stack[i - 1] && match_stack[i + 1];

                for (j = i; j < match_count - 2; j++)
                {
                    if (j == match_count - 3)
                    {
                        match_stack[j] = match_stack[j + 2];
                        /* We use -1 to represent the values that are not
                         * useful */
                        match_stack[j + 1] = -1;

                        break;
                    }

                    match_stack[j] = match_stack[j + 2];
                    match_stack[j + 2] = -1;
                }
            }
        }

        /* calculate the expression with logical OR */
        for (i = 0; i < match_count; i++)
        {
            /* We use -1 to represent the values that are not useful */
            if ((match_stack[i] == LOGICAL_OR) && (i != token_index - 1) &&
                (match_stack[i + 1] != -1))
            {
                int j;

                match_stack[i - 1] = match_stack[i - 1] || match_stack[i + 1];

                for (j = i; j < match_count - 2; j++)
                {
                    match_stack[j] = match_stack[j + 2];
                    match_stack[j + 2] = -1;
                }
            }
        }
    }

    matches = match_stack[0];

    return matches;
}

static gboolean
search_in_result (GlJournalEntry *entry,
                  const gchar *search_text)
{
    gboolean matches;
    gchar *search_text_copy;
    GPtrArray *token_array;

    search_text_copy = g_strdup (search_text);

    token_array = tokenize_search_string (search_text_copy);
    matches = calculate_match (entry, token_array, TRUE);

    g_ptr_array_free (token_array, TRUE);

    return matches;
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

static gboolean
listbox_search_filter_func (GtkListBoxRow *row,
                            GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (!priv->search_text || !*(priv->search_text))
    {
        return TRUE;
    }
    else
    {
        GlJournalEntry *entry;

        entry = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row));

        if (gl_event_view_search_is_case_sensitive (view))
        {
            if (search_in_result (entry, priv->search_text))
            {
                return TRUE;
            }
        }
        else
        {
            gboolean matches;
            gchar *search_text_copy;
            GPtrArray *token_array;

            search_text_copy = g_strdup (priv->search_text);

            token_array = tokenize_search_string (search_text_copy);
            matches = calculate_match (entry, token_array, FALSE);

            g_ptr_array_free (token_array, TRUE);

            return matches;
        }
    }

    return FALSE;
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
gl_event_view_list_get_current_boot_time (GlEventViewList *view,
                                          const gchar *boot_match)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    return gl_journal_model_get_current_boot_time (priv->journal_model,
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
    gchar *boot_value;

    boot_value = strchr (boot_match, '=') + 1;

    return g_strdup (boot_value);
}

/* Create query object according to category and set it on journal model */
static GlQuery *
create_query_object (GlJournalModel *model,
                     GlCategoryList *list,
                     const gchar *current_boot_match)
{
    GlQuery *query;
    gchar *boot_id;
    GlCategoryListFilter filter;

    /* Create new query object */
    query = gl_query_new ();

    /* Get current boot id */
    boot_id = get_current_boot_id (current_boot_match);

    /* Add boot match for all the categories */
    gl_query_add_match (query, "_BOOT_ID", boot_id);

    /* Add exact matches according to selected category */
    filter = gl_category_list_get_category (list);

    switch (filter)
    {
        case GL_CATEGORY_LIST_FILTER_IMPORTANT:
            {
              /* Alert or emergency priority. */
              gl_query_add_match (query, "PRIORITY", "0");
              gl_query_add_match (query, "PRIORITY", "1");
              gl_query_add_match (query, "PRIORITY", "2");
              gl_query_add_match (query, "PRIORITY", "3");
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

                gl_query_add_match (query, "_TRANSPORT", "journal");
                gl_query_add_match (query, "_TRANSPORT", "stdout");
                gl_query_add_match (query, "_TRANSPORT", "syslog");
                gl_query_add_match (query, "_UID", uid_str);

                g_free (uid_str);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_SYSTEM:
            {
                gl_query_add_match (query, "_TRANSPORT", "kernel");
            }
            break;

        case GL_CATEGORY_LIST_FILTER_HARDWARE:
            {
                gl_query_add_match (query, "_TRANSPORT", "kernel");
                gl_query_add_match ( query, "_KERNEL_DEVICE", NULL);
            }
            break;

        case GL_CATEGORY_LIST_FILTER_SECURITY:
            {
                gl_query_add_match (query, "_AUDIT_SESSION", NULL);
            }
            break;

        default:
            g_assert_not_reached ();
    }

    g_free (boot_id);

    return query;
}

static void
on_notify_category (GlCategoryList *list,
                    GParamSpec *pspec,
                    gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GSettings *settings;
    gint sort_order;
    GlQuery *query;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);

    /* Create the query object */
    query = create_query_object (priv->journal_model, list, priv->boot_match);

    /* Set the created query on the journal model */
    gl_journal_model_take_query (priv->journal_model, query);

    settings = g_settings_new (SETTINGS_SCHEMA);
    sort_order = g_settings_get_enum (settings, SORT_ORDER);
    g_object_unref (settings);
    gl_event_view_list_set_sort_order (view, sort_order);
}

void
gl_event_view_list_view_boot (GlEventViewList *view, const gchar *match)
{
    GlEventViewListPrivate *priv;
    GlCategoryList *categories;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);
    categories = GL_CATEGORY_LIST (priv->categories);
    priv->boot_match = match;

    on_notify_category (categories, NULL, view);
}

static gint
gl_event_view_sort_by_ascending_time (GtkListBoxRow *row1,
                                      GtkListBoxRow *row2)
{
    GlJournalEntry *entry1;
    GlJournalEntry *entry2;
    guint64 time1;
    guint64 time2;

    entry1 = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row1));
    entry2 = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row2));
    time1 = gl_journal_entry_get_timestamp (entry1);
    time2 = gl_journal_entry_get_timestamp (entry2);

    if (time1 > time2)
    {
        return 1;
    }
    else if (time1 < time2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static gint
gl_event_view_sort_by_descending_time (GtkListBoxRow *row1,
                                       GtkListBoxRow *row2)
{
    GlJournalEntry *entry1;
    GlJournalEntry *entry2;
    guint64 time1;
    guint64 time2;

    entry1 = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row1));
    entry2 = gl_event_view_row_get_entry (GL_EVENT_VIEW_ROW (row2));
    time1 = gl_journal_entry_get_timestamp (entry1);
    time2 = gl_journal_entry_get_timestamp (entry2);

    if (time1 > time2)
    {
        return -1;
    }
    else if (time1 < time2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void
gl_event_view_list_set_sort_order (GlEventViewList *view,
                                   GlSortOrder sort_order)
{
    GlEventViewListPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

    switch (sort_order)
    {
        case GL_SORT_ORDER_ASCENDING_TIME:
            gtk_list_box_set_sort_func (GTK_LIST_BOX (priv->entries_box),
                                        (GtkListBoxSortFunc) gl_event_view_sort_by_ascending_time,
                                        NULL, NULL);
            break;
        case GL_SORT_ORDER_DESCENDING_TIME:
            gtk_list_box_set_sort_func (GTK_LIST_BOX (priv->entries_box),
                                        (GtkListBoxSortFunc) gl_event_view_sort_by_descending_time,
                                        NULL, NULL);
            break;
        default:
            g_assert_not_reached ();
            break;
    }

}

static void
on_search_entry_changed (GtkSearchEntry *entry,
                         gpointer user_data)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (GL_EVENT_VIEW_LIST (user_data));

    gl_event_view_list_search (GL_EVENT_VIEW_LIST (user_data),
                               gtk_entry_get_text (GTK_ENTRY (priv->search_entry)));
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
gl_event_view_list_finalize (GObject *object)
{
    GlEventViewList *view = GL_EVENT_VIEW_LIST (object);
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

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
    gtk_list_box_set_filter_func (GTK_LIST_BOX (priv->entries_box),
                                  (GtkListBoxFilterFunc) listbox_search_filter_func,
                                  view, NULL);
    gtk_list_box_set_placeholder (GTK_LIST_BOX (priv->entries_box),
                                  gl_event_view_create_empty (view));
    g_signal_connect (priv->entries_box, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), GTK_BOX (view));

    /* TODO: Monitor and propagate any GSettings changes. */
    settings = g_settings_new (DESKTOP_SCHEMA);
    priv->clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);
    g_object_unref (settings);

    g_signal_connect (categories, "notify::category", G_CALLBACK (on_notify_category),
                      view);
}

void
gl_event_view_list_search (GlEventViewList *view,
                           const gchar *needle)
{
    GlEventViewListPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

    g_free (priv->search_text);
    priv->search_text = g_strdup (needle);

    /* for search, we need all entries - tell the model to fetch them */
    gl_journal_model_fetch_more_entries (priv->journal_model, TRUE);

    gtk_list_box_invalidate_filter (priv->entries_box);
}

GtkWidget *
gl_event_view_list_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_LIST, NULL);
}
