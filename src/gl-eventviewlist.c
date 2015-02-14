/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014  Red Hat, Inc.
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
#include "gl-journal.h"
#include "gl-util.h"

typedef struct
{
    GlJournal *journal;
    GlJournalResult *result;
    GlUtilClockFormat clock_format;
    GtkListBox *active_listbox;
    GtkWidget *categories;
    GtkWidget *event_search;
    GtkWidget *event_scrolled;
    GtkWidget *search_entry;
    gchar *search_text;

    GQueue *pending_results;
    GList *results;
    guint insert_idle_id;
} GlEventViewListPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewList, gl_event_view_list, GTK_TYPE_BOX)

static const gssize N_RESULTS = -1;
static const gssize N_RESULTS_IDLE = 25;
static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar SETTINGS_SCHEMA[] = "org.gnome.Logs";
static const gchar CLOCK_FORMAT[] = "clock-format";
static const gchar SORT_ORDER[] = "sort-order";

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
search_in_result (GlJournalResult *result,
                  const gchar *search_text)
{
    if ((result->comm ? strstr (result->comm, search_text) : NULL)
        || (result->message ? strstr (result->message, search_text) : NULL)
        || (result->kernel_device ? strstr (result->kernel_device, search_text)
                                  : NULL)
        || (result->audit_session ? strstr (result->audit_session, search_text)
                                  : NULL))
    {
        return TRUE;
    }

    return FALSE;
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
        GlJournalResult *result;

        result = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row));

        if (gl_event_view_search_is_case_sensitive (view))
        {
            if (search_in_result (result, priv->search_text))
            {
                return TRUE;
            }
        }
        else
        {
            gchar *casefolded_text;
            GlJournalResult casefolded;

            /* Case-insensitive search. */
            casefolded_text = g_utf8_casefold (priv->search_text, -1);

            casefolded.comm = result->comm ? g_utf8_casefold (result->comm, -1)
                                           : NULL;
            casefolded.message = result->message ? g_utf8_casefold (result->message, -1)
                                                 : NULL;
            casefolded.kernel_device = result->kernel_device ? g_utf8_casefold (result->kernel_device, -1)
                                                             : NULL;
            casefolded.audit_session = result->audit_session ? g_utf8_casefold (result->audit_session, -1)
                                                             : NULL;

            if (search_in_result (&casefolded, casefolded_text))
            {
                g_free (casefolded.comm);
                g_free (casefolded.message);
                g_free (casefolded.kernel_device);
                g_free (casefolded.audit_session);
                g_free (casefolded_text);

                return TRUE;
            }

            g_free (casefolded.comm);
            g_free (casefolded.message);
            g_free (casefolded.kernel_device);
            g_free (casefolded.audit_session);
            g_free (casefolded_text);
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
    priv->result = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row));

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

GlJournalResult *
gl_event_view_list_get_detail_result (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    return priv->result;
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
gl_event_view_list_box_new (GlEventViewList *view)
{
    GtkWidget *listbox;

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    gtk_list_box_set_placeholder (GTK_LIST_BOX (listbox),
                                  gl_event_view_create_empty (view));
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox),
                                     GTK_SELECTION_NONE);
    g_signal_connect (listbox, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), GTK_BOX (view));

    return listbox;
}

static gboolean
insert_devices_idle (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->pending_results)
    {
        gssize i;

        for (i = 0; i < N_RESULTS_IDLE; i++)
        {
            GlJournalResult *result;
            GtkWidget *row;

            result = g_queue_pop_head (priv->pending_results);

            if (result)
            {
                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_SIMPLE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->active_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results = NULL;

                priv->insert_idle_id = 0;
                return G_SOURCE_REMOVE;
            }
        }

        return G_SOURCE_CONTINUE;
    }
    else
    {
        priv->insert_idle_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static void
query_devices_ready (GObject *source_object,
                     GAsyncResult *res,
                     gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlJournal *journal;
    GError *error = NULL;
    GList *l;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);
    journal = GL_JOURNAL (source_object);

    priv->results = gl_journal_query_finish (journal, res, &error);

    if (!priv->results)
    {
        /* TODO: Check for error. */
        g_error_free (error);
    }

    priv->pending_results = g_queue_new ();

    for (l = priv->results; l != NULL; l = g_list_next (l))
    {
        g_queue_push_tail (priv->pending_results, l->data);
    }

    priv->insert_idle_id = g_idle_add ((GSourceFunc) insert_devices_idle,
                                       view);
    g_source_set_name_by_id (priv->insert_idle_id, G_STRFUNC);
}

static gboolean
insert_security_idle (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->pending_results)
    {
        gssize i;

        for (i = 0; i < N_RESULTS_IDLE; i++)
        {
            GlJournalResult *result;
            GtkWidget *row;

            result = g_queue_pop_head (priv->pending_results);

            if (result)
            {
                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_CMDLINE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->active_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results = NULL;

                priv->insert_idle_id = 0;
                return G_SOURCE_REMOVE;
            }
        }

        return G_SOURCE_CONTINUE;
    }
    else
    {
        priv->insert_idle_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static void
query_security_ready (GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlJournal *journal;
    GError *error = NULL;
    GList *l;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);
    journal = GL_JOURNAL (source_object);

    priv->results = gl_journal_query_finish (journal, res, &error);

    if (!priv->results)
    {
        /* TODO: Check for error. */
        g_error_free (error);
    }

    priv->pending_results = g_queue_new ();

    for (l = priv->results; l != NULL; l = g_list_next (l))
    {
        g_queue_push_tail (priv->pending_results, l->data);
    }

    priv->insert_idle_id = g_idle_add ((GSourceFunc) insert_security_idle,
                                       view);
    g_source_set_name_by_id (priv->insert_idle_id, G_STRFUNC);
}

static gboolean
insert_simple_idle (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->pending_results)
    {
        gssize i;

        for (i = 0; i < N_RESULTS_IDLE; i++)
        {
            GlJournalResult *result;
            GtkWidget *row;

            result = g_queue_pop_head (priv->pending_results);

            if (result)
            {
                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_SIMPLE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->active_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results = NULL;

                priv->insert_idle_id = 0;
                return G_SOURCE_REMOVE;
            }
        }

        return G_SOURCE_CONTINUE;
    }
    else
    {
        priv->insert_idle_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static void
query_simple_ready (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlJournal *journal;
    GError *error = NULL;
    GList *l;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);
    journal = GL_JOURNAL (source_object);

    priv->results = gl_journal_query_finish (journal, res, &error);

    if (!priv->results)
    {
        /* TODO: Check for error. */
        g_error_free (error);
    }

    priv->pending_results = g_queue_new ();

    for (l = priv->results; l != NULL; l = g_list_next (l))
    {
        g_queue_push_tail (priv->pending_results, l->data);
    }

    priv->insert_idle_id = g_idle_add ((GSourceFunc) insert_simple_idle, view);
    g_source_set_name_by_id (priv->insert_idle_id, G_STRFUNC);
}

static gboolean
insert_cmdline_idle (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->pending_results)
    {
        gssize i;

        for (i = 0; i < N_RESULTS_IDLE; i++)
        {
            GlJournalResult *result;
            GtkWidget *row;

            result = g_queue_pop_head (priv->pending_results);

            if (result)
            {
                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_CMDLINE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->active_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results = NULL;

                priv->insert_idle_id = 0;
                return G_SOURCE_REMOVE;
            }
        }

        return G_SOURCE_CONTINUE;
    }
    else
    {
        priv->insert_idle_id = 0;
        return G_SOURCE_REMOVE;
    }
}

static void
query_cmdline_ready (GObject *source_object,
                     GAsyncResult *res,
                     gpointer user_data)
{
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GlJournal *journal;
    GError *error = NULL;
    GList *l;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);
    journal = GL_JOURNAL (source_object);

    priv->results = gl_journal_query_finish (journal, res, &error);

    if (!priv->results)
    {
        /* TODO: Check for error. */
        g_error_free (error);
    }

    priv->pending_results = g_queue_new ();

    for (l = priv->results; l != NULL; l = g_list_next (l))
    {
        g_queue_push_tail (priv->pending_results, l->data);
    }

    priv->insert_idle_id = g_idle_add ((GSourceFunc) insert_cmdline_idle,
                                       view);
    g_source_set_name_by_id (priv->insert_idle_id, G_STRFUNC);
}

static void
gl_event_view_list_add_listbox_important (GlEventViewList *view)
{
    /* Alert or emergency priority. */
    const GlJournalQuery query = { N_RESULTS,
                                   (gchar*[5]){ "PRIORITY=0",
                                                "PRIORITY=1",
                                                "PRIORITY=2",
                                                "PRIORITY=3",
                                                NULL } };
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    gl_journal_query_async (priv->journal, &query, NULL, query_cmdline_ready, view);
}

static void
gl_event_view_list_add_listbox_all (GlEventViewList *view)
{
    const GlJournalQuery query = { N_RESULTS, NULL };
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    gl_journal_query_async (priv->journal, &query, NULL, query_cmdline_ready, view);
}

static void
gl_event_view_list_add_listbox_applications (GlEventViewList *view)
{
    GCredentials *creds;
    uid_t uid;
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);
    creds = g_credentials_new ();
    uid = g_credentials_get_unix_user (creds, NULL);

    /* Allow all _TRANSPORT != kernel. Attempt to filter by only processes
     * owned by the same UID. */
    if (uid != -1)
    {
        gchar *uid_str;

        uid_str = g_strdup_printf ("_UID=%d", uid);

        {
            GlJournalQuery query = { N_RESULTS,
                                     (gchar *[5]){ "_TRANSPORT=journal",
                                                   "_TRANSPORT=stdout",
                                                   "_TRANSPORT=syslog",
                                                   uid_str, NULL } };

            gl_journal_query_async (priv->journal, &query, NULL, query_cmdline_ready, view);
        }

        g_free (uid_str);
    }
    else
    {
        GlJournalQuery query = { N_RESULTS,
                                 (gchar *[4]){ "_TRANSPORT=journal",
                                               "_TRANSPORT=stdout",
                                               "_TRANSPORT=syslog", NULL } };

        gl_journal_query_async (priv->journal, &query, NULL, query_cmdline_ready, view);
    }

    g_object_unref (creds);
}

static void
gl_event_view_list_add_listbox_system (GlEventViewList *view)
{
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[2]){ "_TRANSPORT=kernel", NULL } };
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    gl_journal_query_async (priv->journal, &query, NULL, query_simple_ready, view);
}

static void
gl_event_view_list_add_listbox_hardware (GlEventViewList *view)
{
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[3]){ "_TRANSPORT=kernel", "_KERNEL_DEVICE", NULL } };
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    gl_journal_query_async (priv->journal, &query, NULL, query_devices_ready, view);
}

static void
gl_event_view_list_add_listbox_security (GlEventViewList *view)
{
    const GlJournalQuery query = { N_RESULTS, (gchar *[2]){ "_AUDIT_SESSION", NULL } };
    GlEventViewListPrivate *priv;

    priv = gl_event_view_list_get_instance_private (view);

    gl_journal_query_async (priv->journal, &query, NULL, query_security_ready, view);
}

static void
on_notify_category (GlCategoryList *list,
                    GParamSpec *pspec,
                    gpointer user_data)
{
    GlCategoryListFilter filter;
    GlEventViewList *view;
    GlEventViewListPrivate *priv;
    GSettings *settings;
    gint sort_order;

    view = GL_EVENT_VIEW_LIST (user_data);
    priv = gl_event_view_list_get_instance_private (view);
    filter = gl_category_list_get_category (list);

    if (priv->active_listbox)
      {
        GtkWidget *child;

        child = gtk_bin_get_child (GTK_BIN (priv->event_scrolled));
        gtk_widget_destroy (child);
      }

    priv->active_listbox = GTK_LIST_BOX (gl_event_view_list_box_new (view));
    gtk_container_add (GTK_CONTAINER (priv->event_scrolled), GTK_WIDGET (priv->active_listbox));

    switch (filter)
    {
        case GL_CATEGORY_LIST_FILTER_IMPORTANT:
            gl_event_view_list_add_listbox_important (view);
            break;
        case GL_CATEGORY_LIST_FILTER_ALL:
            gl_event_view_list_add_listbox_all (view);
            break;
        case GL_CATEGORY_LIST_FILTER_APPLICATIONS:
            gl_event_view_list_add_listbox_applications (view);
            break;
        case GL_CATEGORY_LIST_FILTER_SYSTEM:
            gl_event_view_list_add_listbox_system (view);
            break;
        case GL_CATEGORY_LIST_FILTER_HARDWARE:
            gl_event_view_list_add_listbox_hardware (view);
            break;
        case GL_CATEGORY_LIST_FILTER_SECURITY:
            gl_event_view_list_add_listbox_security (view);
            break;
        default:
            g_assert_not_reached ();
    }

    gtk_widget_show_all (GTK_WIDGET (priv->active_listbox));

    settings = g_settings_new (SETTINGS_SCHEMA);
    sort_order = g_settings_get_enum (settings, SORT_ORDER);
    g_object_unref (settings);
    gl_event_view_list_set_sort_order (view, sort_order);
}

static gint
gl_event_view_sort_by_ascending_time (GtkListBoxRow *row1,
                                      GtkListBoxRow *row2)
{
    GlJournalResult *result1;
    GlJournalResult *result2;
    guint64 time1;
    guint64 time2;

    result1 = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row1));
    result2 = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row2));
    time1 = result1->timestamp;
    time2 = result2->timestamp;

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
    GlJournalResult *result1;
    GlJournalResult *result2;
    guint64 time1;
    guint64 time2;

    result1 = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row1));
    result2 = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row2));
    time1 = result1->timestamp;
    time2 = result2->timestamp;

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
            gtk_list_box_set_sort_func (GTK_LIST_BOX (priv->active_listbox),
                                        (GtkListBoxSortFunc) gl_event_view_sort_by_ascending_time,
                                        NULL, NULL);
            break;
        case GL_SORT_ORDER_DESCENDING_TIME:
            gtk_list_box_set_sort_func (GTK_LIST_BOX (priv->active_listbox),
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
gl_event_view_list_finalize (GObject *object)
{
    GlEventViewList *view = GL_EVENT_VIEW_LIST (object);
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    if (priv->insert_idle_id)
    {
        g_source_remove (priv->insert_idle_id);
    }

    g_clear_pointer (&priv->search_text, g_free);
    g_clear_pointer (&priv->pending_results, g_queue_free);
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
    priv->active_listbox = NULL;
    priv->insert_idle_id = 0;
    priv->journal = gl_journal_new ();
    categories = GL_CATEGORY_LIST (priv->categories);

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

    gtk_list_box_invalidate_filter (priv->active_listbox);
}

GtkWidget *
gl_event_view_list_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_LIST, NULL);
}
