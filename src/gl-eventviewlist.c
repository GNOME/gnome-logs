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

#include "gl-eventviewlist.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>

#include "gl-enums.h"
#include "gl-eventtoolbar.h"
#include "gl-eventviewdetail.h"
#include "gl-eventviewrow.h"
#include "gl-journal.h"
#include "gl-util.h"

enum
{
    PROP_0,
    PROP_FILTER,
    PROP_MODE,
    N_PROPERTIES
};

typedef struct
{
    GlJournal *journal;
    GlUtilClockFormat clock_format;
    GlEventViewListFilter filter;
    GtkListBox *active_listbox;
    GlEventViewMode mode;
    gchar *search_text;

    GtkListBox *results_listbox;
    GQueue *pending_results;
    GList *results;
    guint insert_idle_id;
} GlEventViewListPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewList, gl_event_view_list, GTK_TYPE_STACK)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static const gssize N_RESULTS = -1;
static const gssize N_RESULTS_IDLE = 25;
static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar CLOCK_FORMAT[] = "clock-format";

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
    GlJournalResult *result;
    GtkWidget *detail;
    GtkStack *stack;

    priv = gl_event_view_list_get_instance_private (view);
    result = gl_event_view_row_get_result (GL_EVENT_VIEW_ROW (row));

    detail = gl_event_view_detail_new (result, priv->clock_format);

    gtk_widget_show_all (detail);

    stack = GTK_STACK (view);
    gtk_stack_add_named (stack, detail, "detail");
    gl_event_view_list_set_mode (view, GL_EVENT_VIEW_MODE_DETAIL);
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
                      G_CALLBACK (on_listbox_row_activated), GTK_STACK (view));

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
                if (result->kernel_device == NULL)
                {
                    continue;
                }

                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_SIMPLE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->results_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results_listbox = NULL;
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
insert_journal_query_devices (GlEventViewList *view,
                              const GlJournalQuery *query,
                              GtkListBox *listbox)
{
    GlEventViewListPrivate *priv;
    GList *l;
    gsize n_results;

    priv = gl_event_view_list_get_instance_private (view);
    priv->results = gl_journal_query (priv->journal, query);
    priv->results_listbox = listbox;

    n_results = g_list_length (priv->results);

    if ((n_results != -1) && (n_results != N_RESULTS))
    {
        g_debug ("Number of results different than requested");
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
                if (result->audit_session == NULL)
                {
                    continue;
                }

                row = gl_event_view_row_new (result,
                                             GL_EVENT_VIEW_ROW_STYLE_CMDLINE,
                                             priv->clock_format);
                gtk_container_add (GTK_CONTAINER (priv->results_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results_listbox = NULL;
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
insert_journal_query_security (GlEventViewList *view,
                               const GlJournalQuery *query,
                               GtkListBox *listbox)
{
    GlEventViewListPrivate *priv;
    GList *l;
    gsize n_results;

    priv = gl_event_view_list_get_instance_private (view);
    priv->results = gl_journal_query (priv->journal, query);
    priv->results_listbox = listbox;

    n_results = g_list_length (priv->results);

    if ((n_results != -1) && (n_results != N_RESULTS))
    {
        g_debug ("Number of results different than requested");
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
                gtk_container_add (GTK_CONTAINER (priv->results_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results_listbox = NULL;
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
insert_journal_query_simple (GlEventViewList *view,
                             const GlJournalQuery *query,
                             GtkListBox *listbox)
{
    GlEventViewListPrivate *priv;
    GList *l;
    gsize n_results;

    priv = gl_event_view_list_get_instance_private (view);
    priv->results = gl_journal_query (priv->journal, query);
    priv->results_listbox = listbox;

    n_results = g_list_length (priv->results);

    if ((n_results != -1) && (n_results != N_RESULTS))
    {
        g_debug ("Number of results different than requested");
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
                gtk_container_add (GTK_CONTAINER (priv->results_listbox), row);
                gtk_widget_show_all (row);
            }
            else
            {
                g_queue_free (priv->pending_results);
                gl_journal_results_free (priv->results);
                priv->pending_results = NULL;
                priv->results_listbox = NULL;
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
insert_journal_query_cmdline (GlEventViewList *view,
                              const GlJournalQuery *query,
                              GtkListBox *listbox)
{
    GlEventViewListPrivate *priv;
    GList *l;
    gsize n_results;

    priv = gl_event_view_list_get_instance_private (view);
    priv->results = gl_journal_query (priv->journal, query);
    priv->results_listbox = listbox;

    n_results = g_list_length (priv->results);

    if ((n_results != -1) && (n_results != N_RESULTS))
    {
        g_debug ("Number of results different than requested");
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
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_cmdline (view, &query,
                                  GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-important");
}

static void
gl_event_view_list_add_listbox_alerts (GlEventViewList *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-alerts");
}

static void
gl_event_view_list_add_listbox_starred (GlEventViewList *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-starred");
}

static void
gl_event_view_list_add_listbox_all (GlEventViewList *view)
{
    const GlJournalQuery query = { N_RESULTS, NULL };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_cmdline (view, &query, GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-all");
}

static void
gl_event_view_list_add_listbox_applications (GlEventViewList *view)
{
    GCredentials *creds;
    uid_t uid;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);
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

            insert_journal_query_cmdline (view, &query,
                                          GTK_LIST_BOX (listbox));
        }

        g_free (uid_str);
    }
    else
    {
        GlJournalQuery query = { N_RESULTS,
                                 (gchar *[4]){ "_TRANSPORT=journal",
                                               "_TRANSPORT=stdout",
                                               "_TRANSPORT=syslog", NULL } };

        insert_journal_query_cmdline (view, &query, GTK_LIST_BOX (listbox));
    }

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-applications");

    g_object_unref (creds);
}

static void
gl_event_view_list_add_listbox_system (GlEventViewList *view)
{
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[2]){ "_TRANSPORT=kernel", NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_simple (view, &query, GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-system");
}

static void
gl_event_view_list_add_listbox_hardware (GlEventViewList *view)
{
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[2]){ "_TRANSPORT=kernel", NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_devices (view, &query, GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-hardware");
}

static void
gl_event_view_list_add_listbox_security (GlEventViewList *view)
{
    const GlJournalQuery query = { N_RESULTS, NULL };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_security (view, &query, GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-security");
}

static void
gl_event_view_list_add_listbox_updates (GlEventViewList *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-updates");
}

static void
gl_event_view_list_add_listbox_usage (GlEventViewList *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-usage");
}

static void
on_notify_filter (GlEventViewList *view,
                  G_GNUC_UNUSED GParamSpec *pspec,
                  G_GNUC_UNUSED gpointer user_data)
{
    GlEventViewListPrivate *priv;
    GtkStack *stack;
    GtkWidget *scrolled;
    GtkWidget *viewport;

    priv = gl_event_view_list_get_instance_private (view);
    stack = GTK_STACK (view);

    if (priv->active_listbox)
    {
        GtkWidget *child;

        child = gtk_stack_get_visible_child (stack);
        gtk_widget_destroy (child);
    }

    switch (priv->filter)
    {
        case GL_EVENT_VIEW_LIST_FILTER_IMPORTANT:
            gl_event_view_list_add_listbox_important (view);
            gtk_stack_set_visible_child_name (stack, "listbox-important");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_ALERTS:
            gl_event_view_list_add_listbox_alerts (view);
            gtk_stack_set_visible_child_name (stack, "listbox-alerts");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_STARRED:
            gl_event_view_list_add_listbox_starred (view);
            gtk_stack_set_visible_child_name (stack, "listbox-starred");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_ALL:
            gl_event_view_list_add_listbox_all (view);
            gtk_stack_set_visible_child_name (stack, "listbox-all");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_APPLICATIONS:
            gl_event_view_list_add_listbox_applications (view);
            gtk_stack_set_visible_child_name (stack, "listbox-applications");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_SYSTEM:
            gl_event_view_list_add_listbox_system (view);
            gtk_stack_set_visible_child_name (stack, "listbox-system");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_HARDWARE:
            gl_event_view_list_add_listbox_hardware (view);
            gtk_stack_set_visible_child_name (stack, "listbox-hardware");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_SECURITY:
            gl_event_view_list_add_listbox_security (view);
            gtk_stack_set_visible_child_name (stack, "listbox-security");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_UPDATES:
            gl_event_view_list_add_listbox_updates (view);
            gtk_stack_set_visible_child_name (stack, "listbox-updates");
            break;
        case GL_EVENT_VIEW_LIST_FILTER_USAGE:
            gl_event_view_list_add_listbox_usage (view);
            gtk_stack_set_visible_child_name (stack, "listbox-usage");
            break;
        default:
            break;
    }

    scrolled = gtk_stack_get_visible_child (stack);
    viewport = gtk_bin_get_child (GTK_BIN (scrolled));
    priv->active_listbox = GTK_LIST_BOX (gtk_bin_get_child (GTK_BIN (viewport)));

    gl_event_view_list_set_mode (view, GL_EVENT_VIEW_MODE_LIST);
}

static void
on_notify_mode (GlEventViewList *view,
                GParamSpec *pspec,
                gpointer user_data)
{
    GlEventViewListPrivate *priv;
    GtkStack *stack;
    GtkWidget *toplevel;

    priv = gl_event_view_list_get_instance_private (view);
    stack = GTK_STACK (view);

    switch (priv->mode)
    {
        case GL_EVENT_VIEW_MODE_LIST:
            {
                GtkWidget *child;
                GtkWidget *viewport;
                GtkWidget *scrolled_window;

                child = gtk_stack_get_child_by_name (stack, "detail");

                if (child)
                {
                    gtk_container_remove (GTK_CONTAINER (stack), child);
                }

                viewport = gtk_widget_get_parent (GTK_WIDGET (priv->active_listbox));
                scrolled_window = gtk_widget_get_parent (viewport);
                gtk_stack_set_visible_child (stack, scrolled_window);
            }
            break;
        case GL_EVENT_VIEW_MODE_DETAIL:
            gtk_stack_set_visible_child_name (stack, "detail");
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

    if (gtk_widget_is_toplevel (toplevel))
    {
        GAction *mode;
        GEnumClass *eclass;
        GEnumValue *evalue;

        mode = g_action_map_lookup_action (G_ACTION_MAP (toplevel), "view-mode");
        eclass = g_type_class_ref (GL_TYPE_EVENT_VIEW_MODE);
        evalue = g_enum_get_value (eclass, priv->mode);

        g_action_activate (mode, g_variant_new_string (evalue->value_nick));

        g_type_class_unref (eclass);
    }
    else
    {
        g_debug ("Widget not in toplevel window, not switching toolbar mode");
    }
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
gl_event_view_list_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    GlEventViewList *view = GL_EVENT_VIEW_LIST (object);
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    switch (prop_id)
    {
        case PROP_FILTER:
            g_value_set_enum (value, priv->filter);
            break;
        case PROP_MODE:
            g_value_set_enum (value, priv->mode);
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_list_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    GlEventViewList *view = GL_EVENT_VIEW_LIST (object);
    GlEventViewListPrivate *priv = gl_event_view_list_get_instance_private (view);

    switch (prop_id)
    {
        case PROP_FILTER:
            priv->filter = g_value_get_enum (value);
            break;
        case PROP_MODE:
            priv->mode = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_list_class_init (GlEventViewListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_event_view_list_finalize;
    gobject_class->get_property = gl_event_view_list_get_property;
    gobject_class->set_property = gl_event_view_list_set_property;

    obj_properties[PROP_FILTER] = g_param_spec_enum ("filter", "Filter",
                                                     "Filter events by",
                                                     GL_TYPE_EVENT_VIEW_LIST_FILTER,
                                                     GL_EVENT_VIEW_LIST_FILTER_IMPORTANT,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_MODE] = g_param_spec_enum ("mode", "Mode",
                                                   "Event display mode",
                                                   GL_TYPE_EVENT_VIEW_MODE,
                                                   GL_EVENT_VIEW_MODE_LIST,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);
}

static void
gl_event_view_list_init (GlEventViewList *view)
{
    GlEventViewListPrivate *priv;
    GSettings *settings;

    priv = gl_event_view_list_get_instance_private (view);
    priv->search_text = NULL;
    priv->active_listbox = NULL;
    priv->insert_idle_id = 0;
    priv->journal = gl_journal_new ();

    /* TODO: Monitor and propagate any GSettings changes. */
    settings = g_settings_new (DESKTOP_SCHEMA);
    priv->clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);
    g_object_unref (settings);

    g_signal_connect (view, "notify::filter", G_CALLBACK (on_notify_filter),
                      NULL);
    g_signal_connect (view, "notify::mode", G_CALLBACK (on_notify_mode),
                      NULL);

    /* Force an update of the active filter. */
    on_notify_filter (view, NULL, NULL);
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

void
gl_event_view_list_set_filter (GlEventViewList *view,
                               GlEventViewListFilter filter)
{
    GlEventViewListPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->filter != filter)
    {
        priv->filter = filter;
        g_object_notify_by_pspec (G_OBJECT (view),
                                  obj_properties[PROP_FILTER]);
    }
}

void
gl_event_view_list_set_mode (GlEventViewList *view,
                             GlEventViewMode mode)
{
    GlEventViewListPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW_LIST (view));

    priv = gl_event_view_list_get_instance_private (view);

    if (priv->mode != mode)
    {
        priv->mode = mode;
        g_object_notify_by_pspec (G_OBJECT (view),
                                  obj_properties[PROP_MODE]);
    }
}

GtkWidget *
gl_event_view_list_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_LIST, NULL);
}
