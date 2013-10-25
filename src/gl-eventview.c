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

#include "gl-eventview.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>

#include "gl-enums.h"
#include "gl-eventtoolbar.h"
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
    GlEventViewFilter filter;
    GtkListBox *active_listbox;
    GlEventViewMode mode;
    gchar *search_text;
} GlEventViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventView, gl_event_view, GTK_TYPE_STACK)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static const guint N_RESULTS = 50;

static gboolean
listbox_search_filter_func (GtkListBoxRow *row,
                            GlEventView *view)
{
    GlEventViewPrivate *priv;

    priv = gl_event_view_get_instance_private (view);

    if (!priv->search_text || !*(priv->search_text))
    {
        return TRUE;
    }
    else
    {
        gchar *cursor;
        GlJournalResult *result;

        cursor = g_object_get_data (G_OBJECT (row), "cursor");

        if (cursor == NULL)
        {
            g_warning ("Error getting cursor from row");
            goto out;
        }

        result = gl_journal_query_cursor (priv->journal, cursor);

        if ((result->comm ? strstr (result->comm, priv->search_text) : NULL)
            || (result->message ? strstr (result->message, priv->search_text)
                                : NULL)
            || (result->kernel_device ? strstr (result->kernel_device,
                                                priv->search_text) : NULL)
            || (result->audit_session ? strstr (result->audit_session,
                                                priv->search_text) : NULL))
        {
            gl_journal_result_free (priv->journal, result);

            return TRUE;
        }

        gl_journal_result_free (priv->journal, result);
    }

out:
    return FALSE;
}

static void
on_listbox_row_activated (GtkListBox *listbox,
                          GtkListBoxRow *row,
                          GlEventView *view)
{
    GlEventViewPrivate *priv;
    gchar *cursor;
    GlJournalResult *result;
    gchar *time;
    gboolean rtl;
    GtkWidget *grid;
    GtkWidget *label;
    GtkStyleContext *context;
    GtkStack *stack;

    priv = gl_event_view_get_instance_private (view);
    cursor = g_object_get_data (G_OBJECT (row), "cursor");

    if (cursor == NULL)
    {
        g_warning ("Error getting cursor from row");
        return;
    }

    rtl = gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL;

    result = gl_journal_query_cursor (priv->journal, cursor);

    grid = gtk_grid_new ();
    label = gtk_label_new (result->comm);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-comm");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

    time = gl_util_timestamp_to_display (result->timestamp);
    label = gtk_label_new (gl_util_timestamp_to_display (result->timestamp));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-time");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 0, 1, 1);
    g_free (time);

    label = gtk_label_new (result->message);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-message");
    gtk_style_context_add_class (context, "event-monospace");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);

    label = gtk_label_new (result->catalog);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-catalog");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 2, 1);

    gtk_widget_show_all (grid);
    stack = GTK_STACK (view);
    gtk_stack_add_named (stack, grid, "detail");
    gl_event_view_set_mode (view, GL_EVENT_VIEW_MODE_DETAIL);

    gl_journal_result_free (priv->journal, result);
    return;
}

static void
on_notify_filter (GlEventView *view,
                  G_GNUC_UNUSED GParamSpec *pspec,
                  G_GNUC_UNUSED gpointer user_data)
{
    GlEventViewPrivate *priv;
    GtkStack *stack;
    GtkWidget *scrolled;
    GtkWidget *viewport;

    priv = gl_event_view_get_instance_private (view);
    stack = GTK_STACK (view);

    switch (priv->filter)
    {
        case GL_EVENT_VIEW_FILTER_IMPORTANT:
            gtk_stack_set_visible_child_name (stack, "listbox-important");
            break;
        case GL_EVENT_VIEW_FILTER_ALERTS:
            gtk_stack_set_visible_child_name (stack, "listbox-alerts");
            break;
        case GL_EVENT_VIEW_FILTER_STARRED:
            gtk_stack_set_visible_child_name (stack, "listbox-starred");
            break;
        case GL_EVENT_VIEW_FILTER_ALL:
            gtk_stack_set_visible_child_name (stack, "listbox-all");
            break;
        case GL_EVENT_VIEW_FILTER_APPLICATIONS:
            gtk_stack_set_visible_child_name (stack, "listbox-applications");
            break;
        case GL_EVENT_VIEW_FILTER_SYSTEM:
            gtk_stack_set_visible_child_name (stack, "listbox-system");
            break;
        case GL_EVENT_VIEW_FILTER_HARDWARE:
            gtk_stack_set_visible_child_name (stack, "listbox-hardware");
            break;
        case GL_EVENT_VIEW_FILTER_SECURITY:
            gtk_stack_set_visible_child_name (stack, "listbox-security");
            break;
        case GL_EVENT_VIEW_FILTER_UPDATES:
            gtk_stack_set_visible_child_name (stack, "listbox-updates");
            break;
        case GL_EVENT_VIEW_FILTER_USAGE:
            gtk_stack_set_visible_child_name (stack, "listbox-usage");
            break;
        default:
            break;
    }

    scrolled = gtk_stack_get_visible_child (stack);
    viewport = gtk_bin_get_child (GTK_BIN (scrolled));
    priv->active_listbox = GTK_LIST_BOX (gtk_bin_get_child (GTK_BIN (viewport)));

    gl_event_view_set_mode (view, GL_EVENT_VIEW_MODE_LIST);
}

static void
on_notify_mode (GlEventView *view,
                GParamSpec *pspec,
                gpointer user_data)
{
    GlEventViewPrivate *priv;
    GtkStack *stack;
    GtkWidget *toplevel;

    priv = gl_event_view_get_instance_private (view);
    stack = GTK_STACK (view);

    switch (priv->mode)
    {
        case GL_EVENT_VIEW_MODE_LIST:
            {
                GtkContainer *container;
                GList *children;
                GList *l;
                GtkWidget *viewport;
                GtkWidget *scrolled_window;

                container = GTK_CONTAINER (stack);
                children = gtk_container_get_children (container);

                for (l = children; l != NULL; l = g_list_next (l))
                {
                    GtkWidget *child;
                    gchar *name;

                    child = (GtkWidget *)l->data;
                    gtk_container_child_get (container, child, "name", &name,
                                             NULL);

                    if (g_strcmp0 (name, "detail") == 0)
                    {
                        gtk_container_remove (container, child);

                        g_free (name);
                        break;
                    }

                    g_free (name);
                }

                g_list_free (children);

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
gl_event_view_finalize (GObject *object)
{
    GlEventView *view = GL_EVENT_VIEW (object);
    GlEventViewPrivate *priv = gl_event_view_get_instance_private (view);

    g_clear_pointer (&priv->search_text, g_free);
}

static void
gl_event_view_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    GlEventView *view = GL_EVENT_VIEW (object);
    GlEventViewPrivate *priv = gl_event_view_get_instance_private (view);

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
gl_event_view_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    GlEventView *view = GL_EVENT_VIEW (object);
    GlEventViewPrivate *priv = gl_event_view_get_instance_private (view);

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
gl_event_view_class_init (GlEventViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_event_view_finalize;
    gobject_class->get_property = gl_event_view_get_property;
    gobject_class->set_property = gl_event_view_set_property;

    obj_properties[PROP_FILTER] = g_param_spec_enum ("filter", "Filter",
                                                     "Filter events by",
                                                     GL_TYPE_EVENT_VIEW_FILTER,
                                                     GL_EVENT_VIEW_FILTER_IMPORTANT,
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
insert_journal_query_devices (GlJournal *journal,
                              const GlJournalQuery *query,
                              GtkListBox *listbox)
{
    GList *results = NULL;
    GList *l;
    gsize n_results;

    results = gl_journal_query (journal, query);

    n_results = g_list_length (results);

    if (n_results != N_RESULTS)
    {
        g_debug ("Number of results different than requested");
    }

    for (l = results; l != NULL; l = g_list_next (l))
    {
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *time;
        gboolean rtl;
        GtkWidget *image;
        GlJournalResult result = *(GlJournalResult *)(l->data);

        /* Skip if the log entry does not refer to a hardware device. */
        if (result.kernel_device == NULL)
        {
            continue;
        }

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        g_object_set_data_full (G_OBJECT (row), "cursor",
                                g_strdup (result.cursor), g_free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        label = gtk_label_new (result.message);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "event-monospace");
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

        time = gl_util_timestamp_to_display (result.timestamp);
        label = gtk_label_new (time);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "dim-label");
        gtk_style_context_add_class (context, "event-time");
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        g_free (time);
    }

    gl_journal_results_free (journal, results);
}

static void
insert_journal_query_security (GlJournal *journal,
                               const GlJournalQuery *query,
                               GtkListBox *listbox)
{
    GList *results = NULL;
    GList *l;
    gsize n_results;

    results = gl_journal_query (journal, query);

    n_results = g_list_length (results);

    if (n_results != N_RESULTS)
    {
        g_debug ("Number of results different than requested");
    }

    for (l = results; l != NULL; l = g_list_next (l))
    {
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *markup;
        gchar *time;
        gboolean rtl;
        GtkWidget *image;
        GlJournalResult result = *(GlJournalResult *)(l->data);

        /* Skip if the journal entry does not have an associated audit
         * session. */
        if (result.audit_session == NULL)
        {
            continue;
        }

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

        row = gtk_list_box_row_new ();
        gtk_widget_set_direction (row, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        g_object_set_data_full (G_OBJECT (row), "cursor",
                                g_strdup (result.cursor), g_free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        markup = g_markup_printf_escaped ("<b>%s</b>",
                                          result.comm ? result.comm : "");
        label = gtk_label_new (NULL);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

        label = gtk_label_new (result.message);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "event-monospace");
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);

        time = gl_util_timestamp_to_display (result.timestamp);
        label = gtk_label_new (time);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "dim-label");
        gtk_style_context_add_class (context, "event-time");
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 0, 1, 1);

        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 2, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        g_free (time);
    }

    gl_journal_results_free (journal, results);
}

static void
insert_journal_query_simple (GlJournal *journal,
                             const GlJournalQuery *query,
                             GtkListBox *listbox)
{
    GList *results = NULL;
    GList *l;
    gsize n_results;

    results = gl_journal_query (journal, query);

    n_results = g_list_length (results);

    if (n_results != N_RESULTS)
    {
        g_debug ("Number of results different than requested");
    }

    for (l = results; l != NULL; l = g_list_next (l))
    {
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *time;
        gboolean rtl;
        GtkWidget *image;
        GlJournalResult result = *(GlJournalResult *)(l->data);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        g_object_set_data_full (G_OBJECT (row), "cursor",
                                g_strdup (result.cursor), g_free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        label = gtk_label_new (result.message);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "event-monospace");
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

        time = gl_util_timestamp_to_display (result.timestamp);
        label = gtk_label_new (time);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "dim-label");
        gtk_style_context_add_class (context, "event-time");
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        g_free (time);
    }

    gl_journal_results_free (journal, results);
}

static void
insert_journal_query_cmdline (GlJournal *journal,
                              const GlJournalQuery *query,
                              GtkListBox *listbox)
{
    GList *results = NULL;
    GList *l;
    gsize n_results;

    results = gl_journal_query (journal, query);

    n_results = g_list_length (results);

    if (n_results != N_RESULTS)
    {
        g_debug ("Number of results different than requested");
    }

    for (l = results; l != NULL; l = g_list_next (l))
    {
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        gchar *markup;
        GtkWidget *label;
        gchar *time;
        gboolean rtl;
        GtkWidget *image;
        GlJournalResult result = *(GlJournalResult *)(l->data);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        g_object_set_data_full (G_OBJECT (row), "cursor",
                                g_strdup (result.cursor), g_free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        markup = g_markup_printf_escaped ("<b>%s</b>",
                                          result.comm ? result.comm : "");
        label = gtk_label_new (NULL);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

        label = gtk_label_new (result.message);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "event-monospace");
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);

        time = gl_util_timestamp_to_display (result.timestamp);
        label = gtk_label_new (time);
        context = gtk_widget_get_style_context (GTK_WIDGET (label));
        gtk_style_context_add_class (context, "dim-label");
        gtk_style_context_add_class (context, "event-time");
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 0, 1, 1);

        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 2, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        g_free (time);
    }

    gl_journal_results_free (journal, results);
}

static GtkWidget *
gl_event_view_create_empty (G_GNUC_UNUSED GlEventView *view)
{
    GtkWidget *box;
    GtkStyleContext *context;
    GtkWidget *image;
    GtkWidget *label;
    gchar *markup;

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
    context = gtk_widget_get_style_context (box);
    gtk_style_context_add_class (context, "dim-label");

    image = gtk_image_new_from_icon_name ("action-unavailable-symbolic", 0);
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
gl_event_view_list_box_new (GlEventView *view)
{
    GtkWidget *listbox;

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    gtk_list_box_set_placeholder (GTK_LIST_BOX (listbox),
                                  gl_event_view_create_empty (view));
    g_signal_connect (listbox, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), GTK_STACK (view));

    return listbox;
}

static void
gl_event_view_add_listbox_important (GlEventView *view)
{
    GlEventViewPrivate *priv;
    /* Alert or emergency priority. */
    const GlJournalQuery query = { N_RESULTS,
                                   (gchar*[5]){ "PRIORITY=0",
                                                "PRIORITY=1",
                                                "PRIORITY=2",
                                                "PRIORITY=3",
                                                NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_cmdline (priv->journal, &query,
                                  GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-important");
}

static void
gl_event_view_add_listbox_alerts (GlEventView *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-alerts");
}

static void
gl_event_view_add_listbox_starred (GlEventView *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-starred");
}

static void
gl_event_view_add_listbox_all (GlEventView *view)
{
    GlEventViewPrivate *priv;
    const GlJournalQuery query = { N_RESULTS, NULL };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_cmdline (priv->journal, &query,
                                  GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-all");
}

static void
gl_event_view_add_listbox_applications (GlEventView *view)
{
    GlEventViewPrivate *priv;
    /* Allow all _TRANSPORT != kernel. */
    const GlJournalQuery query = { N_RESULTS,
                                   (gchar *[4]){ "_TRANSPORT=journal",
                                                 "_TRANSPORT=stdout",
                                                 "_TRANSPORT=syslog", NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_cmdline (priv->journal, &query,
                                  GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-applications");
}

static void
gl_event_view_add_listbox_system (GlEventView *view)
{
    GlEventViewPrivate *priv;
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[2]){ "_TRANSPORT=kernel", NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_simple (priv->journal, &query,
                                 GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-system");
}

static void
gl_event_view_add_listbox_hardware (GlEventView *view)
{
    GlEventViewPrivate *priv;
    GlJournalQuery query = { N_RESULTS,
                             (gchar *[2]){ "_TRANSPORT=kernel", NULL } };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_devices (priv->journal, &query,
                                  GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-hardware");
}

static void
gl_event_view_add_listbox_security (GlEventView *view)
{
    GlEventViewPrivate *priv;
    const GlJournalQuery query = { N_RESULTS, NULL };
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);

    listbox = gl_event_view_list_box_new (view);

    insert_journal_query_security (priv->journal, &query,
                                   GTK_LIST_BOX (listbox));

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-security");
}

static void
gl_event_view_add_listbox_updates (GlEventView *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-updates");
}

static void
gl_event_view_add_listbox_usage (GlEventView *view)
{
    GtkWidget *label;

    label = gtk_label_new (_("Not implemented"));
    gtk_widget_show_all (label);
    gtk_stack_add_named (GTK_STACK (view), label, "listbox-usage");
}

static void
gl_event_view_init (GlEventView *view)
{
    GlEventViewPrivate *priv;

    priv = gl_event_view_get_instance_private (view);
    priv->search_text = NULL;
    priv->active_listbox = NULL;
    priv->journal = gl_journal_new ();

    gl_event_view_add_listbox_important (view);
    gl_event_view_add_listbox_alerts (view);
    gl_event_view_add_listbox_starred (view);
    gl_event_view_add_listbox_all (view);
    gl_event_view_add_listbox_applications (view);
    gl_event_view_add_listbox_system (view);
    gl_event_view_add_listbox_hardware (view);
    gl_event_view_add_listbox_security (view);
    gl_event_view_add_listbox_updates (view);
    gl_event_view_add_listbox_usage (view);

    g_signal_connect (view, "notify::filter", G_CALLBACK (on_notify_filter),
                      NULL);
    g_signal_connect (view, "notify::mode", G_CALLBACK (on_notify_mode),
                      NULL);

    /* Force an update of the active filter. */
    on_notify_filter (view, NULL, NULL);
}

void
gl_event_view_search (GlEventView *view,
                      const gchar *needle)
{
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);

    g_free (priv->search_text);
    priv->search_text = g_strdup (needle);

    gtk_list_box_invalidate_filter (priv->active_listbox);
}

void
gl_event_view_set_filter (GlEventView *view, GlEventViewFilter filter)
{
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);

    if (priv->filter != filter)
    {
        priv->filter = filter;
        g_object_notify_by_pspec (G_OBJECT (view),
                                  obj_properties[PROP_FILTER]);
    }
}

void
gl_event_view_set_mode (GlEventView *view, GlEventViewMode mode)
{
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);

    if (priv->mode != mode)
    {
        priv->mode = mode;
        g_object_notify_by_pspec (G_OBJECT (view),
                                  obj_properties[PROP_MODE]);
    }
}

GtkWidget *
gl_event_view_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW, NULL);
}
