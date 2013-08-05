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
#include <stdlib.h>
#include <systemd/sd-journal.h>

G_DEFINE_TYPE (GlEventView, gl_event_view, GTK_TYPE_STACK)

static void
on_detailed_button_clicked (GtkButton *button,
                            GlEventView *view)
{
    GtkWidget *detailed;
    GtkStack *stack = GTK_STACK (view);

    detailed = gtk_stack_get_visible_child (stack);
    gtk_stack_set_visible_child_name (stack, "listbox");
    gtk_container_remove (GTK_CONTAINER (stack), detailed);
}

static void
on_listbox_row_activated (GtkListBox *listbox,
                          GtkListBoxRow *row,
                          GtkWidget *view)
{
    sd_journal *journal;
    gint ret;
    gchar *cursor;
    gchar *comm;
    gchar *time;
    gsize length;
    guint64 microsec;
    GDateTime *datetime;
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *image;
    gboolean rtl;
    GtkStack *stack;

    ret = sd_journal_open (&journal, 0);

    if (ret < 0)
    {
        g_warning ("Error opening systemd journal: %s", g_strerror (-ret));
        return;
    }

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
        g_warning ("Error getting command line from systemd journal: %s",
                   g_strerror (-ret));
        comm = "_COMM=";
    }

    grid = gtk_grid_new ();
    label = gtk_label_new (strchr (comm, '=') + 1);
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
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
    g_free (time);

    button = gtk_button_new ();
    g_signal_connect (button, "clicked",
                      G_CALLBACK (on_detailed_button_clicked), view);
    rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
    image = gtk_image_new_from_icon_name (rtl ? "go-previous-rtl-symbolic"
                                              : "go-previous-symbolic",
                                          GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_grid_attach (GTK_GRID (grid), button, 0, 2, 1, 1);

    gtk_widget_show_all (grid);
    stack = GTK_STACK (view);
    gtk_stack_add_named (stack, grid, "detailed");
    gtk_stack_set_visible_child_name (stack, "detailed");

out:
    sd_journal_close (journal);
}

static void
gl_event_view_class_init (GlEventViewClass *klass)
{
}

static void
gl_event_view_init (GlEventView *view)
{
    GtkWidget *stack;
    GtkWidget *listbox;
    sd_journal *journal;
    gint ret;
    gsize i;
    GtkWidget *scrolled;

    stack = GTK_WIDGET (view);

    listbox = gtk_list_box_new ();
    ret = sd_journal_open (&journal, 0);

    if (ret < 0)
    {
        g_warning ("Error opening systemd journal: %s", g_strerror (-ret));
    }

    ret = sd_journal_seek_tail (journal);

    if (ret < 0)
    {
        g_warning ("Error seeking to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    ret = sd_journal_next (journal);

    if (ret < 0)
    {
        g_warning ("Error setting cursor to end of systemd journal: %s",
                   g_strerror (-ret));
    }

    for (i = 0; i < 10; i++)
    {
        const gchar *message;
        const gchar *comm;
        gchar *cursor;
        gsize length;
        GtkWidget *row;
        GtkWidget *grid;
        GtkWidget *label;
        gchar *markup;
        gboolean rtl;
        GtkWidget *image;

        ret = sd_journal_get_data (journal, "_COMM", (const void **)&comm,
                                   &length);

        if (ret < 0)
        {
            g_warning ("Error getting commandline from systemd journal: %s",
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
    }

    g_signal_connect (listbox, "row-activated",
                      G_CALLBACK (on_listbox_row_activated), stack);

    sd_journal_close (journal);

    gtk_widget_show_all (listbox);
    gtk_stack_add_named (GTK_STACK (stack), listbox, "listbox");
}

GtkWidget *
gl_event_view_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW, NULL);
}
