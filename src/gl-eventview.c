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
        sd_journal *journal;
        gint ret;
        gsize length;
        gchar *comm;
        gchar *message;

        cursor = g_object_get_data (G_OBJECT (row), "cursor");

        if (cursor == NULL)
        {
            g_warning ("Error getting cursor from row");
            goto out;
        }

        journal = gl_journal_get_journal (priv->journal);

        ret = sd_journal_seek_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error seeking to cursor position: %s", g_strerror (-ret));
            goto out;
        }

        ret = sd_journal_next (journal);

        if (ret < 0)
        {
            g_warning ("Error positioning cursor in systemd journal: %s",
                       g_strerror (-ret));
        }

        ret = sd_journal_test_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error testing cursor string: %s", g_strerror (-ret));
            goto out;
        }
        else if (ret == 0)
        {
            g_warning ("Cursor string does not match journal entry");
            goto out;
        }

        ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                                   &length);

        if (ret < 0)
        {
            g_debug ("Unable to get command line from systemd journal: %s",
                     g_strerror (-ret));
            comm = "_COMM=";
        }

        ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            goto out;
        }

        if (strstr (comm, priv->search_text)
            || strstr (message, priv->search_text))
        {
            return TRUE;
        }
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
    sd_journal *journal;
    gint ret;
    gchar *cursor;
    gchar *comm;
    gchar *message;
    gchar *time;
    gchar *catalog;
    gsize length;
    guint64 microsec;
    GDateTime *datetime;
    GtkWidget *grid;
    GtkWidget *label;
    GtkStyleContext *style;
    GtkStack *stack;
    GtkWidget *toplevel;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);
    cursor = g_object_get_data (G_OBJECT (row), "cursor");

    if (cursor == NULL)
    {
        g_warning ("Error getting cursor from row");
        goto out;
    }

    ret = sd_journal_seek_cursor (journal, cursor);

    if (ret < 0)
    {
        g_warning ("Error seeking to cursor position: %s", g_strerror (-ret));
        goto out;
    }

    ret = sd_journal_next (journal);

    if (ret < 0)
    {
        g_warning ("Error positioning cursor in systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_test_cursor (journal, cursor);

    if (ret < 0)
    {
        g_warning ("Error testing cursor string: %s", g_strerror (-ret));
        goto out;
    }
    else if (ret == 0)
    {
        g_warning ("Cursor string does not match journal entry");
        goto out;
    }

    ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                               &length);

    if (ret < 0)
    {
        g_debug ("Unable to get command line from systemd journal: %s",
                 g_strerror (-ret));
        comm = "_COMM=";
    }

    ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                               &length);

    if (ret < 0)
    {
        g_warning ("Unable to get message from systemd journal: %s",
                   g_strerror (-ret));
        goto out;
    }

    ret = sd_journal_get_catalog (journal, &catalog);

    if (ret == -ENOENT)
    {
        g_debug ("No message for this log entry was found in the catalog");
        catalog = NULL;
    }
    else if (ret < 0)
    {
        g_warning ("Error while getting message from catalog: %s",
                   g_strerror (-ret));
        goto out;
    }

    grid = gtk_grid_new ();
    label = gtk_label_new (strchr (comm, '=') + 1);
    style = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (style, "detail-comm");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

    ret = sd_journal_get_realtime_usec (journal, &microsec);

    if (ret < 0)
    {
        g_warning ("Error getting timestamp from systemd journal: %s",
                   g_strerror (-ret));
        goto out;
    }

    datetime = g_date_time_new_from_unix_utc (microsec / G_TIME_SPAN_SECOND);

    if (datetime == NULL)
    {
        g_warning ("Error converting timestamp to time value");
        goto out;
    }

    /* TODO: Localize? */
    time = g_date_time_format (datetime, "%F %T");
    g_date_time_unref (datetime);

    if (time == NULL)
    {
        g_warning ("Error converting datetime to string");
        goto out;
    }

    label = gtk_label_new (time);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    style = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (style, "detail-time");
    gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);
    g_free (time);

    label = gtk_label_new (strchr (message, '=') + 1);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    style = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (style, "detail-message");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);

    label = gtk_label_new (catalog);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    style = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (style, "detail-catalog");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 2, 1);

    gtk_widget_show_all (grid);
    stack = GTK_STACK (view);
    gtk_stack_add_named (stack, grid, "detailed");
    gtk_stack_set_visible_child_name (stack, "detailed");
    gl_event_view_set_mode (view, GL_EVENT_VIEW_MODE_DETAIL);

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

    if (gtk_widget_is_toplevel (toplevel))
    {
        GAction *mode;
        GEnumClass *eclass;
        GEnumValue *evalue;

        mode = g_action_map_lookup_action (G_ACTION_MAP (toplevel), "mode");
        eclass = g_type_class_ref (GL_TYPE_EVENT_TOOLBAR_MODE);
        evalue = g_enum_get_value (eclass, GL_EVENT_TOOLBAR_MODE_DETAIL);

        g_action_activate (mode, g_variant_new_string (evalue->value_nick));

        g_type_class_unref (eclass);
    }
    else
    {
        g_error ("Widget not in toplevel window, not switching toolbar mode");
    }

out:
    return;
}

static void
on_notify_filter (GlEventView *view,
                  GParamSpec *pspec,
                  gpointer user_data)
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
}

static void
on_notify_mode (GlEventView *view,
                GParamSpec *pspec,
                gpointer user_data)
{
    GlEventViewPrivate *priv;

    priv = gl_event_view_get_instance_private (view);

    switch (priv->mode)
    {

        case GL_EVENT_VIEW_MODE_LIST:
            {
                GtkStack *stack;
                GtkWidget *visible_child;

                stack = GTK_STACK (view);
                visible_child = gtk_stack_get_visible_child (stack);
                gtk_container_remove (GTK_CONTAINER (stack), visible_child);
            }
            break;
        case GL_EVENT_VIEW_MODE_DETAIL:
            /* Ignore. */
            break;
        default:
            g_assert_not_reached ();
            break;
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
                                                     GL_EVENT_VIEW_FILTER_ALL,
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
insert_journal_items_devices (sd_journal *journal, GtkListBox *listbox)
{
    gsize i;

    for (i = 0; i < 10; i++)
    {
        gint ret;
        const gchar *device;
        const gchar *message;
        gchar *cursor;
        gsize length;
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gboolean rtl;
        GtkWidget *image;

        ret = sd_journal_get_data (journal, "_KERNEL_DEVICE",
                                   (const void **)&device, &length);

        if (ret == -ENOENT)
        {
            g_debug ("Skipping non-device journal entry");
            --i;
            goto out;
        }
        else if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_cursor (journal, &cursor);

        if (ret < 0)
        {
            g_warning ("Error getting cursor for current journal entry: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_test_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error testing cursor string: %s", g_strerror (-ret));
            free (cursor);
            break;
        }
        else if (ret == 0)
        {
            g_warning ("Cursor string does not match journal entry");
            /* Not a problem at this point, but would be when seeking to the
             * cursor later on. */
        }

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        /* sd_journal_get_cursor allocates the cursor with libc malloc. */
        g_object_set_data_full (G_OBJECT (row), "cursor", cursor, free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        label = gtk_label_new (strchr (message, '=') + 1);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 1);

        gtk_container_add (GTK_CONTAINER (listbox), row);

out:
        ret = sd_journal_previous (journal);

        if (ret < 0)
        {
            g_warning ("Error setting cursor to previous systemd journal entry %s",
                       g_strerror (-ret));
            break;
        }
        else if (ret == 0)
        {
            g_debug ("End of systemd journal reached");
        }
    }
}

static void
insert_journal_items_security (sd_journal *journal, GtkListBox *listbox)
{
    gsize i;

    for (i = 0; i < 10; i++)
    {
        gint ret;
        const gchar *audit;
        const gchar *message;
        const gchar *comm;
        gchar *cursor;
        gsize length;
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *markup;
        gboolean rtl;
        GtkWidget *image;

        ret = sd_journal_get_data (journal, "_AUDIT_SESSION",
                                   (const void **)&audit, &length);

        if (ret == -ENOENT)
        {
            g_debug ("Skipping journal entry without audit session");
            --i;
            goto out;
        }
        else if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                                   &length);

        if (ret < 0)
        {
            g_debug ("Unable to get commandline from systemd journal: %s",
                     g_strerror (-ret));
            comm = "_COMM=";
        }

        ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_cursor (journal, &cursor);

        if (ret < 0)
        {
            g_warning ("Error getting cursor for current journal entry: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_test_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error testing cursor string: %s", g_strerror (-ret));
            free (cursor);
            break;
        }
        else if (ret == 0)
        {
            g_warning ("Cursor string does not match journal entry");
            /* Not a problem at this point, but would be when seeking to the
             * cursor later on. */
        }

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        /* sd_journal_get_cursor allocates the cursor with libc malloc. */
        g_object_set_data_full (G_OBJECT (row), "cursor", cursor, free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        markup = g_markup_printf_escaped ("<b>%s</b>", strchr (comm, '=') + 1);
        label = gtk_label_new (NULL);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

        label = gtk_label_new (strchr (message, '=') + 1);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

out:
        ret = sd_journal_previous (journal);

        if (ret < 0)
        {
            g_warning ("Error setting cursor to previous systemd journal entry %s",
                       g_strerror (-ret));
            break;
        }
        else if (ret == 0)
        {
            g_debug ("End of systemd journal reached");
        }
    }
}

static void
insert_journal_items_simple (sd_journal *journal, GtkListBox *listbox)
{
    gsize i;

    for (i = 0; i < 10; i++)
    {
        gint ret;
        const gchar *message;
        gchar *cursor;
        gsize length;
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gboolean rtl;
        GtkWidget *image;

        ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_cursor (journal, &cursor);

        if (ret < 0)
        {
            g_warning ("Error getting cursor for current journal entry: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_test_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error testing cursor string: %s", g_strerror (-ret));
            free (cursor);
            break;
        }
        else if (ret == 0)
        {
            g_warning ("Cursor string does not match journal entry");
            /* Not a problem at this point, but would be when seeking to the
             * cursor later on. */
        }

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        /* sd_journal_get_cursor allocates the cursor with libc malloc. */
        g_object_set_data_full (G_OBJECT (row), "cursor", cursor, free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        label = gtk_label_new (strchr (message, '=') + 1);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 1);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        ret = sd_journal_previous (journal);

        if (ret < 0)
        {
            g_warning ("Error setting cursor to previous systemd journal entry %s",
                       g_strerror (-ret));
            break;
        }
        else if (ret == 0)
        {
            g_debug ("End of systemd journal reached");
        }
    }
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
        GtkStyleContext *style;
        GtkWidget *grid;
        gchar *markup;
        GtkWidget *label;
        gboolean rtl;
        GtkWidget *image;
        GlJournalResult result = *(GlJournalResult *)(l->data);

        row = gtk_list_box_row_new ();
        style = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (style, "event");
        g_object_set_data_full (G_OBJECT (row), "cursor",
                                g_strdup (result.cursor), g_free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        markup = g_markup_printf_escaped ("<b>%s</b>", result.comm);
        label = gtk_label_new (NULL);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

        label = gtk_label_new (result.message);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);
    }

    gl_journal_results_free (journal, results);
}

static void
insert_journal_items_cmdline (sd_journal *journal, GtkListBox *listbox)
{
    gsize i;

    for (i = 0; i < 10; i++)
    {
        gint ret;
        const gchar *message;
        const gchar *comm;
        gchar *cursor;
        gsize length;
        GtkWidget *row;
        GtkStyleContext *context;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *markup;
        gboolean rtl;
        GtkWidget *image;

        ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                                   &length);

        if (ret < 0)
        {
            g_debug ("Unable to get commandline from systemd journal: %s",
                     g_strerror (-ret));
            comm = "_COMM=";
        }

        ret = sd_journal_get_data (journal, "MESSAGE", (const void **)&message,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting message from systemd journal: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_get_cursor (journal, &cursor);

        if (ret < 0)
        {
            g_warning ("Error getting cursor for current journal entry: %s",
                       g_strerror (-ret));
            break;
        }

        ret = sd_journal_test_cursor (journal, cursor);

        if (ret < 0)
        {
            g_warning ("Error testing cursor string: %s", g_strerror (-ret));
            free (cursor);
            break;
        }
        else if (ret == 0)
        {
            g_warning ("Cursor string does not match journal entry");
            /* Not a problem at this point, but would be when seeking to the
             * cursor later on. */
        }

        row = gtk_list_box_row_new ();
        context = gtk_widget_get_style_context (GTK_WIDGET (row));
        gtk_style_context_add_class (context, "event");
        /* sd_journal_get_cursor allocates the cursor with libc malloc. */
        g_object_set_data_full (G_OBJECT (row), "cursor", cursor, free);
        grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
        gtk_container_add (GTK_CONTAINER (row), grid);

        markup = g_markup_printf_escaped ("<b>%s</b>", strchr (comm, '=') + 1);
        label = gtk_label_new (NULL);
        gtk_widget_set_hexpand (label, TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

        label = gtk_label_new (strchr (message, '=') + 1);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

        rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
        image = gtk_image_new_from_icon_name (rtl ? "go-next-rtl-symbolic"
                                                  : "go-next-symbolic",
                                              GTK_ICON_SIZE_MENU);
        gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

        gtk_container_add (GTK_CONTAINER (listbox), row);

        ret = sd_journal_previous (journal);

        if (ret < 0)
        {
            g_warning ("Error setting cursor to previous systemd journal entry %s",
                       g_strerror (-ret));
            break;
        }
        else if (ret == 0)
        {
            g_debug ("End of systemd journal reached");
        }
    }
}

static void
gl_event_view_add_listbox_important (GlEventView *view)
{
    GlEventViewPrivate *priv;
    gint ret;
    sd_journal *journal;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);

    /* Alert or emergency priority. */
    ret = sd_journal_add_match (journal, "PRIORITY=0", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for emergency priority: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_add_match (journal, "PRIORITY=1", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for alert priority: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_previous (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("End of systemd journal reached");
    }

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    insert_journal_items_cmdline (journal, GTK_LIST_BOX (listbox));

    sd_journal_flush_matches (journal);

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
gl_event_view_add_listbox_applications (GlEventView *view)
{
    GlEventViewPrivate *priv;
    gint ret;
    sd_journal *journal;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);

    /* Allow all _TRANSPORT != kernel. */
    ret = sd_journal_add_match (journal, "_TRANSPORT=journal", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for journal transport: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_add_match (journal, "_TRANSPORT=stdout", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for stdout transport: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_add_match (journal, "_TRANSPORT=syslog", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for syslog transport: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_previous (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("End of systemd journal reached");
    }

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    insert_journal_items_cmdline (journal, GTK_LIST_BOX (listbox));

    sd_journal_flush_matches (journal);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-applications");
}

static void
gl_event_view_add_listbox_system (GlEventView *view)
{
    GlEventViewPrivate *priv;
    gint ret;
    sd_journal *journal;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);

    ret = sd_journal_add_match (journal, "_TRANSPORT=kernel", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for kernel transport: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_previous (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("End of systemd journal reached");
    }

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    insert_journal_items_simple (journal, GTK_LIST_BOX (listbox));

    sd_journal_flush_matches (journal);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-system");
}

static void
gl_event_view_add_listbox_hardware (GlEventView *view)
{
    GlEventViewPrivate *priv;
    gint ret;
    sd_journal *journal;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);

    ret = sd_journal_add_match (journal, "_TRANSPORT=kernel", 0);

    if (ret < 0)
    {
        g_warning ("Error adding match for kernel transport: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_previous (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("End of systemd journal reached");
    }

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    insert_journal_items_devices (journal, GTK_LIST_BOX (listbox));

    sd_journal_flush_matches (journal);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (view), scrolled, "listbox-hardware");
}

static void
gl_event_view_add_listbox_security (GlEventView *view)
{
    GlEventViewPrivate *priv;
    gint ret;
    sd_journal *journal;
    GtkWidget *listbox;
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    journal = gl_journal_get_journal (priv->journal);

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_previous (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("End of systemd journal reached");
    }

    listbox = gtk_list_box_new ();

    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);
    insert_journal_items_security (journal, GTK_LIST_BOX (listbox));

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
    GtkWidget *stack;
    GtkWidget *listbox;
    const GlJournalQuery query = { N_RESULTS, NULL };
    GtkWidget *scrolled;

    priv = gl_event_view_get_instance_private (view);
    priv->search_text = NULL;
    stack = GTK_WIDGET (view);

    listbox = gtk_list_box_new ();
    gtk_list_box_set_filter_func (GTK_LIST_BOX (listbox),
                                  (GtkListBoxFilterFunc)listbox_search_filter_func,
                                  view, NULL);

    priv->journal = gl_journal_new ();

    insert_journal_query_cmdline (priv->journal, &query,
                                  GTK_LIST_BOX (listbox));

    g_signal_connect (listbox, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), stack);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), listbox);
    gtk_widget_show_all (scrolled);
    gtk_stack_add_named (GTK_STACK (stack), scrolled, "listbox-all");
    priv->active_listbox = GTK_LIST_BOX (listbox);

    gl_event_view_add_listbox_important (view);
    gl_event_view_add_listbox_alerts (view);
    gl_event_view_add_listbox_starred (view);
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
