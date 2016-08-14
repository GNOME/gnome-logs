/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2016  Pranav Ganorkar <pranavg189@gmail.com>
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

#include "gl-searchpopover.h"
#include "gl-enums.h"
#include "gl-util.h"

#include <glib/gi18n.h>
#include <stdlib.h>

struct _GlSearchPopover
{
    /*< private >*/
    GtkPopover parent_instance;
};

typedef struct
{
    /* Search popover elements */
    GtkWidget *parameter_stack;
    GtkWidget *parameter_button_label;
    GtkWidget *parameter_label_stack;
    GtkWidget *parameter_treeview;
    GtkListStore *parameter_liststore;
    GtkWidget *search_type_revealer;
    GtkWidget *range_stack;
    GtkWidget *range_label_stack;
    GtkWidget *range_treeview;
    GtkWidget *range_button_label;
    GtkWidget *clear_range_button;
    GtkWidget *range_button_drop_down_image;
    GtkListStore *range_liststore;
    GtkWidget *menu_stack;

    GtkWidget *start_date_stack;
    GtkWidget *start_date_calendar_revealer;
    GtkWidget *start_date_entry;
    GtkWidget *start_date_button_label;

    GtkWidget *start_time_stack;
    GtkWidget *start_time_hour_spin;
    GtkWidget *start_time_minute_spin;
    GtkWidget *start_time_second_spin;
    GtkWidget *start_time_period_spin;
    GtkWidget *start_time_button_label;
    GtkWidget *start_time_period_label;

    GtkWidget *end_date_stack;
    GtkWidget *end_date_calendar_revealer;
    GtkWidget *end_date_entry;
    GtkWidget *end_date_button_label;

    GtkWidget *end_time_stack;
    GtkWidget *end_time_hour_spin;
    GtkWidget *end_time_minute_spin;
    GtkWidget *end_time_second_spin;
    GtkWidget *end_time_period_spin;
    GtkWidget *end_time_button_label;
    GtkWidget *end_time_period_label;

    GlSearchPopoverJournalFieldFilter journal_search_field;
    GlQuerySearchType search_type;
    GlSearchPopoverJournalTimestampRange journal_timestamp_range;

    guint64 custom_start_timestamp;
    guint64 custom_end_timestamp;

    GlUtilClockFormat clock_format;
} GlSearchPopoverPrivate;

enum
{
    PROP_0,
    PROP_JOURNAL_SEARCH_FIELD,
    PROP_SEARCH_TYPE,
    PROP_JOURNAL_TIMESTAMP_RANGE,
    N_PROPERTIES
};

enum
{
    COLUMN_JOURNAL_FIELD_LABEL,
    COLUMN_JOURNAL_FIELD_SHOW_SEPARATOR,
    COLUMN_JOURNAL_FIELD_ENUM_VALUE,
    JOURNAL_FIELD_N_COLUMNS
};

enum
{
    COLUMN_JOURNAL_TIMESTAMP_RANGE_LABEL,
    COLUMN_JOURNAL_TIMESTAMP_RANGE_SHOW_SEPARATOR,
    COLUMN_JOURNAL_TIMESTAMP_RANGE_ENUM_VALUE,
    JOURNAL_TIMESTAMP_RANGE_N_COLUMNS
};

enum
{
    AM,
    PM
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar CLOCK_FORMAT[] = "clock-format";

G_DEFINE_TYPE_WITH_PRIVATE (GlSearchPopover, gl_search_popover, GTK_TYPE_POPOVER)

/* Event handlers for search popover elements */
static void
search_popover_closed (GtkPopover *popover,
                       gpointer user_data)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (GL_SEARCH_POPOVER (user_data));

    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_stack), "parameter-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_label_stack), "what-label");

    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "when-label");
}

static void
select_parameter_button_clicked (GtkButton *button,
                                 gpointer user_data)
{
    GlSearchPopoverPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;

    priv = gl_search_popover_get_instance_private (GL_SEARCH_POPOVER (user_data));

    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_stack), "parameter-list");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_label_stack), "select-parameter-label");

    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "when-label");

    model = GTK_TREE_MODEL (priv->parameter_liststore);

    valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid)
    {
        GlSearchPopoverJournalFieldFilter journal_field_enum_value;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->parameter_liststore), &iter,
                            COLUMN_JOURNAL_FIELD_ENUM_VALUE, &journal_field_enum_value,
                            -1);

        if (priv->journal_search_field == journal_field_enum_value)
        {
            break;
        }

        valid = gtk_tree_model_iter_next (model, &iter);
    }

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->parameter_treeview));

    gtk_tree_selection_select_iter (selection, &iter);
}

static gboolean
parameter_treeview_row_seperator (GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;
    gboolean show_separator;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->parameter_liststore), iter,
                        COLUMN_JOURNAL_FIELD_SHOW_SEPARATOR, &show_separator,
                        -1);

    return show_separator;
}

static void
on_parameter_treeview_row_activated (GtkTreeView *tree_view,
                                     GtkTreePath *path,
                                     GtkTreeViewColumn *column,
                                     gpointer user_data)
{
    GtkTreeIter iter;
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;
    gchar *journal_field_label;
    GlSearchPopoverJournalFieldFilter journal_field_enum_value;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->parameter_liststore), &iter, path);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->parameter_liststore), &iter,
                        COLUMN_JOURNAL_FIELD_LABEL, &journal_field_label,
                        COLUMN_JOURNAL_FIELD_ENUM_VALUE, &journal_field_enum_value,
                        -1);

    gtk_label_set_label (GTK_LABEL (priv->parameter_button_label),
                         _(journal_field_label));

    priv->journal_search_field = journal_field_enum_value;

    g_object_notify_by_pspec (G_OBJECT (popover),
                              obj_properties[PROP_JOURNAL_SEARCH_FIELD]);

    /* Do not Show "Search Type" option if all available fields group is selected */
    if (priv->journal_search_field == GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_ALL_AVAILABLE_FIELDS)
    {
        gtk_revealer_set_reveal_child (GTK_REVEALER (priv->search_type_revealer), FALSE);
        gtk_widget_set_visible (priv->search_type_revealer, FALSE);
    }
    else
    {
        gtk_widget_set_visible (priv->search_type_revealer, TRUE);
        gtk_revealer_set_reveal_child (GTK_REVEALER (priv->search_type_revealer), TRUE);
    }

    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_stack), "parameter-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_label_stack), "what-label");

    g_free (journal_field_label);
}

static void
search_type_changed (GtkToggleButton *togglebutton,
                     gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    if (gtk_toggle_button_get_active (togglebutton))
    {
        priv->search_type = GL_QUERY_SEARCH_TYPE_EXACT;
    }
    else
    {
        priv->search_type = GL_QUERY_SEARCH_TYPE_SUBSTRING;
    }

    /* Inform GlEventViewlist about search type property change */
    g_object_notify_by_pspec (G_OBJECT (popover),
                              obj_properties[PROP_SEARCH_TYPE]);
}

GlSearchPopoverJournalFieldFilter
gl_search_popover_get_journal_search_field (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    return priv->journal_search_field;
}

GlQuerySearchType
gl_search_popover_get_query_search_type (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    return priv->search_type;
}

GlSearchPopoverJournalTimestampRange
gl_search_popover_get_journal_timestamp_range (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    return priv->journal_timestamp_range;
}

static void
gl_search_popover_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (object);
    GlSearchPopoverPrivate *priv = gl_search_popover_get_instance_private (popover);

    switch (prop_id)
    {
        case PROP_JOURNAL_SEARCH_FIELD:
            g_value_set_enum (value, priv->journal_search_field);
            break;
        case PROP_SEARCH_TYPE:
            g_value_set_enum (value, priv->search_type);
            break;
        case PROP_JOURNAL_TIMESTAMP_RANGE:
            g_value_set_enum (value, priv->journal_timestamp_range);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_search_popover_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (object);
    GlSearchPopoverPrivate *priv = gl_search_popover_get_instance_private (popover);

    switch (prop_id)
    {
        case PROP_JOURNAL_SEARCH_FIELD:
            priv->journal_search_field = g_value_get_enum (value);
            break;
        case PROP_SEARCH_TYPE:
            priv->search_type = g_value_get_enum (value);
            break;
        case PROP_JOURNAL_TIMESTAMP_RANGE:
            priv->journal_timestamp_range = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
select_range_button_clicked (GtkButton *button,
                             gpointer user_data)
{
    GlSearchPopoverPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean valid;

    priv = gl_search_popover_get_instance_private (GL_SEARCH_POPOVER (user_data));

    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-list");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "show-log-from-label");

    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_stack), "parameter-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->parameter_label_stack), "what-label");

    model = GTK_TREE_MODEL (priv->range_liststore);

    valid = gtk_tree_model_get_iter_first (model, &iter);

    while (valid)
    {
        GlSearchPopoverJournalTimestampRange journal_range_enum_value;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->range_liststore), &iter,
                            COLUMN_JOURNAL_TIMESTAMP_RANGE_ENUM_VALUE, &journal_range_enum_value,
                            -1);

        if (priv->journal_timestamp_range == journal_range_enum_value)
        {
            break;
        }

        valid = gtk_tree_model_iter_next (model, &iter);
    }

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->range_treeview));

    gtk_tree_selection_select_iter (selection, &iter);
}

static gboolean
range_treeview_row_seperator (GtkTreeModel *model,
                              GtkTreeIter *iter,
                              gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;
    gboolean show_seperator;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->range_liststore), iter,
                        COLUMN_JOURNAL_TIMESTAMP_RANGE_SHOW_SEPARATOR, &show_seperator,
                        -1);

    return show_seperator;
}

static void
show_start_date_widgets (GlSearchPopover *popover, gboolean visible)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    gtk_stack_set_visible_child_name (GTK_STACK (priv->start_date_stack),
                                      visible ? "start-date-entry" : "start-date-button");

    gtk_widget_set_visible (priv->start_date_calendar_revealer, visible);
    gtk_revealer_set_reveal_child (GTK_REVEALER (priv->start_date_calendar_revealer), visible);
}

static void
show_start_time_widgets (GlSearchPopover *popover, gboolean visible)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    gtk_stack_set_visible_child_name (GTK_STACK (priv->start_time_stack),
                                      visible ? "start-time-spinbutton" : "start-time-select-button");
}

static void
show_end_date_widgets (GlSearchPopover *popover, gboolean visible)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    gtk_stack_set_visible_child_name (GTK_STACK (priv->end_date_stack),
                                      visible ? "end-date-entry" : "end-date-button");

    gtk_widget_set_visible (priv->end_date_calendar_revealer, visible);
    gtk_revealer_set_reveal_child (GTK_REVEALER (priv->end_date_calendar_revealer), visible);
}

static void
show_end_time_widgets (GlSearchPopover *popover, gboolean visible)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    gtk_stack_set_visible_child_name (GTK_STACK (priv->end_time_stack),
                                      visible ? "end-time-spinbutton" : "end-time-select-button");
}

static void
reset_custom_range_widgets (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    priv->custom_start_timestamp = 0;
    priv->custom_end_timestamp = 0;

    /* Close any previously opened widgets in the submenu */
    show_start_date_widgets (popover, FALSE);
    show_start_time_widgets (popover, FALSE);
    show_end_date_widgets (popover, FALSE);
    show_end_time_widgets (popover, FALSE);

    /* Reset start range elements */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->start_time_minute_spin), 59.0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->start_time_second_spin), 59.0);

    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->start_time_hour_spin), 11.0);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->start_time_period_spin), PM);
    }
    else
    {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->start_time_hour_spin), 23.0);
    }

    gtk_entry_set_text (GTK_ENTRY (priv->start_date_entry), "");
    gtk_label_set_label (GTK_LABEL (priv->start_date_button_label), _("Select Start Date..."));
    gtk_label_set_label (GTK_LABEL (priv->start_time_button_label), _("Select Start Time..."));

    /*Reset end range elements */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->end_time_minute_spin), 0.0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->end_time_second_spin), 0.0);

    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->end_time_hour_spin), 12.0);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->end_time_period_spin), AM);
    }
    else
    {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->end_time_hour_spin), 0.0);
    }

    gtk_entry_set_text (GTK_ENTRY (priv->end_date_entry), "");
    gtk_label_set_label (GTK_LABEL (priv->end_date_button_label), _("Select End Date..."));
    gtk_label_set_label (GTK_LABEL (priv->end_time_button_label), _("Select End Time..."));
}

static void
on_range_treeview_row_activated (GtkTreeView *tree_view,
                                 GtkTreePath *path,
                                 GtkTreeViewColumn *column,
                                 gpointer user_data)
{
    GtkTreeIter iter;
    gchar *range_label;
    GlSearchPopoverJournalTimestampRange journal_range_enum_value;
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->range_liststore), &iter, path);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->range_liststore), &iter,
                        COLUMN_JOURNAL_TIMESTAMP_RANGE_LABEL, &range_label,
                        COLUMN_JOURNAL_TIMESTAMP_RANGE_ENUM_VALUE, &journal_range_enum_value,
                        -1);

    priv->journal_timestamp_range = journal_range_enum_value;

    if (priv->journal_timestamp_range == GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CUSTOM)
    {
        gtk_stack_set_visible_child_name (GTK_STACK (priv->menu_stack), "custom-range-submenu");
    }
    else
    {
        /* Reset the Custom Range elements if set as only one filter can be applied at time */
        reset_custom_range_widgets (popover);

        gtk_label_set_label (GTK_LABEL (priv->range_button_label),
                             _(range_label));

        g_object_notify_by_pspec (G_OBJECT (popover),
                                  obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

        gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-button");
        gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "when-label");
    }

    g_free (range_label);
}

static void
start_date_button_clicked (GtkButton *button,
                           gpointer user_data)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (user_data);

    show_start_date_widgets (popover, TRUE);
    show_start_time_widgets (popover, FALSE);
    show_end_date_widgets (popover, FALSE);
    show_end_time_widgets (popover, FALSE);
}

/* Utility function for converting hours from 12 hour format to 24 hour format */
static void
convert_hour (gint *hour, gint time_period)
{
    if (*hour == 12 && time_period == AM)
    {
        *hour = 0;
    }
    else if (*hour != 12 && time_period == PM)
    {
        *hour = *hour + 12;
    }
}

static GDateTime *
get_start_date_time (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;
    const gchar *entry_date;
    GDateTime *now;
    GDateTime *start_date_time;
    GDate *start_date;
    gint hour;
    gint minute;
    gint second;

    priv = gl_search_popover_get_instance_private (popover);

    entry_date = gtk_entry_get_text (GTK_ENTRY (priv->start_date_entry));

    hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->start_time_hour_spin));
    minute = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->start_time_minute_spin));
    second = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->start_time_second_spin));

    /* Convert to 24 Hour format */
    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        gint time_period;

        time_period = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->start_time_period_spin));

        convert_hour (&hour, time_period);
    }

    /* Parse the date entered into the Text Entry */
    start_date = g_date_new ();
    g_date_set_parse (start_date, entry_date);

    now = g_date_time_new_now_local ();

    /* If Invalid date, then take today's date as default */
    if (!g_date_valid (start_date))
    {
        start_date_time = g_date_time_new_local (g_date_time_get_year (now),
                                                 g_date_time_get_month (now),
                                                 g_date_time_get_day_of_month (now),
                                                 hour,
                                                 minute,
                                                 second);
    }
    else
    {
        start_date_time = g_date_time_new_local (g_date_get_year (start_date),
                                                 g_date_get_month (start_date),
                                                 g_date_get_day (start_date),
                                                 hour,
                                                 minute,
                                                 second);
    }

    g_date_time_unref (now);
    g_date_free (start_date);

    return start_date_time;
}

static void
update_range_button (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;
    gchar *range_button_label;
    gchar *display_time;
    GDateTime *now;

    priv = gl_search_popover_get_instance_private (popover);

    now = g_date_time_new_now_local ();

    /* Update range button label according to timestamps set in the custom range submenu */
    if (priv->custom_end_timestamp && priv->custom_start_timestamp)
    {
        range_button_label = gl_util_boot_time_to_display (priv->custom_start_timestamp,
                                                           priv->custom_end_timestamp);
    }
    else if (priv->custom_start_timestamp && !priv->custom_end_timestamp)
    {
        display_time = gl_util_timestamp_to_display (priv->custom_start_timestamp,
                                                     now, priv->clock_format, FALSE);

        /* Translators: if only custom start timestamp is set, then we update
         * the timestamp range button label in popover to show that
         * logs are shown in the window starting from this timestamp
         * until the ending timestamp of journal. */
        range_button_label = g_strdup_printf (_("From %s"), display_time);

        g_free (display_time);
    }
    else
    {
        display_time = gl_util_timestamp_to_display (priv->custom_end_timestamp,
                                                     now, priv->clock_format, FALSE);

        /* Translators: if only custom end timestamp is set, then we update
         * the timestamp range button label in popover to show that
         * logs are shown in the window upto this timestamp
         * with the starting timestamp being the current time. */
        range_button_label = g_strdup_printf (_("Until %s"), display_time);

        g_free (display_time);
    }

    gtk_label_set_label (GTK_LABEL (priv->range_button_label), range_button_label);

    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "when-label");

    g_date_time_unref (now);
    g_free (range_button_label);
}

static void
start_date_calendar_day_selected (GtkCalendar *calendar,
                                  gpointer user_data)
{
    GDateTime *date;
    GDateTime *now;
    guint year, month, day;
    gchar *date_label;
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_calendar_get_date (calendar, &year, &month, &day);

    date = g_date_time_new_local (year, month + 1, day, 0, 0, 0);

    /* Translators: date format for the start date entry
     * and start date button label in the custom range submenu,
     * showing the day of month in decimal number, full month
     * name as string, the year as a decimal number including the century. */
    date_label = g_date_time_format (date, _("%e %B %Y"));

    now = g_date_time_new_now_local ();

    /* If a future date, fail silently */
    if (g_date_time_compare (date, now) != 1)
    {
        GDateTime *start_date_time;

        gtk_entry_set_text (GTK_ENTRY (priv->start_date_entry), date_label);

        gtk_label_set_label (GTK_LABEL (priv->start_date_button_label), date_label);

        start_date_time = get_start_date_time (popover);

        priv->custom_start_timestamp = g_date_time_to_unix (start_date_time) * G_USEC_PER_SEC;

        update_range_button (popover);

        g_object_notify_by_pspec (G_OBJECT (popover),
                                  obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

        g_date_time_unref (start_date_time);
    }

    g_date_time_unref (date);
}

static void
start_date_entry_activate (GtkEntry *entry,
                           gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    if (gtk_entry_get_text_length (entry) > 0)
    {
        GDateTime *now;
        GDateTime *date_time;
        GDate *date;

        date = g_date_new ();
        g_date_set_parse (date, gtk_entry_get_text (entry));

        /* Invalid date silently does nothing */
        if (!g_date_valid (date))
        {
            g_date_free (date);
            return;
        }

        now = g_date_time_new_now_local ();
        date_time = g_date_time_new_local (g_date_get_year (date),
                                           g_date_get_month (date),
                                           g_date_get_day (date),
                                           0,
                                           0,
                                           0);

        /* Future dates silently fail */
        if (g_date_time_compare (date_time, now) != 1)
        {
            GDateTime *start_date_time;

            gtk_label_set_label (GTK_LABEL (priv->start_date_button_label), gtk_entry_get_text(entry));

            show_start_date_widgets (popover, FALSE);

            start_date_time = get_start_date_time (popover);

            priv->custom_start_timestamp = g_date_time_to_unix (start_date_time) * G_USEC_PER_SEC;

            update_range_button (popover);

            g_object_notify_by_pspec (G_OBJECT (popover),
                                      obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

            g_date_time_unref (start_date_time);
        }

        g_date_time_unref (now);
        g_date_time_unref (date_time);
        g_date_free (date);
    }
}

static void
start_time_button_clicked (GtkButton *button,
                           gpointer user_data)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (user_data);

    show_start_time_widgets (popover, TRUE);
    show_start_date_widgets (popover, FALSE);
    show_end_time_widgets (popover, FALSE);
    show_end_date_widgets (popover, FALSE);
}

static void
roundoff_invalid_time_value (GtkSpinButton *spin_button,
                             gdouble *new_val,
                             gint lower_limit,
                             gint upper_limit)
{
    gint time;

    time = atoi (gtk_entry_get_text (GTK_ENTRY (spin_button)));

    /* Roundoff to the nearest limit if out of limits*/
    if (time < lower_limit)
    {
        *new_val = lower_limit;
    }
    else if (time > upper_limit)
    {
        *new_val = upper_limit;
    }
    else
    {
        *new_val = time;
    }
}

static gboolean
spinbox_format_time_period_to_text (GtkSpinButton *spin_button,
                                    gpointer user_data)
{
    gchar *time_period_string;
    gint time_period;

    time_period = gtk_spin_button_get_value_as_int (spin_button);

    if (time_period == AM)
    {
        time_period_string = g_strdup_printf (_("AM"));
    }
    else
    {
        time_period_string = g_strdup_printf (_("PM"));
    }

    gtk_entry_set_text (GTK_ENTRY (spin_button), time_period_string);

    g_free (time_period_string);

    return TRUE;
}

static gint
spinbox_format_time_period_to_int (GtkSpinButton *spin_button,
                                   gdouble *time_period,
                                   gpointer user_data)
{
    const gchar *time_period_string;

    time_period_string = gtk_entry_get_text (GTK_ENTRY (spin_button));

    if ( g_strcmp0 ("PM", time_period_string) == 0)
    {
        *time_period = PM;
    }
    else
    {
        /* Reset invalid values to "AM" */
        *time_period = AM;
    }

    return TRUE;
}

/* Common event handler for validating spinbutton entry values */
static gint
spinbox_entry_validate_hour_min_sec (GtkSpinButton *spin_button,
                                     gdouble *new_val,
                                     gpointer user_data)
{
    const gchar *spinbutton_id;
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    spinbutton_id = gtk_buildable_get_name (GTK_BUILDABLE (spin_button));

    /* Check if called from hour spinboxes */
    if (g_strcmp0 (spinbutton_id, "end_time_hour_spin") == 0
        || g_strcmp0 (spinbutton_id, "start_time_hour_spin") == 0)
    {
        if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_24HR)
        {
            roundoff_invalid_time_value (spin_button, new_val, 0, 23);
        }
        else
        {
            roundoff_invalid_time_value (spin_button, new_val, 1, 12);
        }
    }
    else
    {
        roundoff_invalid_time_value (spin_button, new_val, 0, 59);
    }

    return TRUE;
}

static gboolean
spinbox_entry_format_two_digits (GtkSpinButton *spin_button,
                                 gpointer user_data)
{
    gchar *time_string;
    gint value;

    value = gtk_spin_button_get_value_as_int (spin_button);

    time_string = g_strdup_printf ("%02d", value);

    gtk_entry_set_text (GTK_ENTRY (spin_button), time_string);

    g_free (time_string);

    return TRUE;
}

/* Common event handler for setting query when any one of start time spinbutton changes */
static void
start_time_spinbox_value_changed (GtkSpinButton *spin_button,
                                  gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;
    GDateTime *start_date_time;
    gchar *button_label;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    start_date_time = get_start_date_time (popover);

    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        /* Translators: timestamp format for the custom start time button
         * label in the custom range submenu, showing the time with seconds
         * in 12-hour format. */
        button_label = g_date_time_format (start_date_time, _("%I:%M:%S %p"));
    }
    else
    {
        /* Translators: timestamp format for the custom start time button
         * label in the custom range submenu, showing the time with seconds
         * in 24-hour format. */
        button_label = g_date_time_format (start_date_time, _("%T"));
    }

    gtk_label_set_label (GTK_LABEL (priv->start_time_button_label), button_label);

    priv->custom_start_timestamp = g_date_time_to_unix (start_date_time) * G_USEC_PER_SEC;

    update_range_button (popover);

    g_object_notify_by_pspec (G_OBJECT (popover),
                              obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);
}

static void
end_date_button_clicked (GtkButton *button,
                         gpointer user_data)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (user_data);

    show_end_date_widgets (popover, TRUE);
    show_end_time_widgets (popover, FALSE);
    show_start_date_widgets (popover, FALSE);
    show_start_time_widgets (popover, FALSE);
}

static GDateTime *
get_end_date_time (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;
    const gchar *entry_date;
    GDateTime *now;
    GDateTime *end_date_time;
    GDate *end_date;
    gint hour;
    gint minute;
    gint second;

    priv = gl_search_popover_get_instance_private (popover);

    entry_date = gtk_entry_get_text (GTK_ENTRY (priv->end_date_entry));

    hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->end_time_hour_spin));
    minute = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->end_time_minute_spin));
    second = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->end_time_second_spin));

    /* Convert to 24 Hour format */
    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        gint time_period;

        time_period = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->end_time_period_spin));

        convert_hour (&hour, time_period);
    }

    /* Parse the date entered into the Text Entry */
    end_date = g_date_new ();
    g_date_set_parse (end_date, entry_date);

    now = g_date_time_new_now_local ();

    /* If Invalid date, then take today's date as default */
    if (!g_date_valid (end_date))
    {
        end_date_time = g_date_time_new_local (g_date_time_get_year (now),
                                               g_date_time_get_month (now),
                                               g_date_time_get_day_of_month (now),
                                               hour,
                                               minute,
                                               second);
    }
    else
    {
        end_date_time = g_date_time_new_local (g_date_get_year (end_date),
                                               g_date_get_month (end_date),
                                               g_date_get_day (end_date),
                                               hour,
                                               minute,
                                               second);
    }

    g_date_time_unref (now);
    g_date_free (end_date);

    return end_date_time;
}

static void
end_date_calendar_day_selected (GtkCalendar *calendar,
                                gpointer user_data)
{
    GDateTime *date;
    GDateTime *now;
    guint year, month, day;
    gchar *date_label;
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    gtk_calendar_get_date (calendar, &year, &month, &day);

    date = g_date_time_new_local (year, month + 1, day, 0, 0, 0);

    /* Translators: date format for the end date entry
     * and end date button label in the custom range submenu,
     * showing the day of month in decimal number, full month
     * name as string, the year as a decimal number including the century. */
    date_label = g_date_time_format (date, _("%e %B %Y"));

    now = g_date_time_new_now_local ();

    /* If a future date, fail silently */
    if (g_date_time_compare (date, now) != 1)
    {
        GDateTime *end_date_time;

        gtk_entry_set_text (GTK_ENTRY (priv->end_date_entry), date_label);

        gtk_label_set_label (GTK_LABEL (priv->end_date_button_label), date_label);

        end_date_time = get_end_date_time (popover);

        priv->custom_end_timestamp = g_date_time_to_unix (end_date_time) * G_USEC_PER_SEC;

        update_range_button (popover);

        g_object_notify_by_pspec (G_OBJECT (popover),
                                  obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

        g_date_time_unref (end_date_time);
    }

    g_date_time_unref (date);
}

static void
end_date_entry_activate (GtkEntry *entry,
                         gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    if (gtk_entry_get_text_length (entry) > 0)
    {
        GDateTime *now;
        GDateTime *date_time;
        GDate *date;

        date = g_date_new ();
        g_date_set_parse (date, gtk_entry_get_text (entry));

        /* Invalid date silently does nothing */
        if (!g_date_valid (date))
        {
            g_date_free (date);
            return;
        }

        now = g_date_time_new_now_local ();
        date_time = g_date_time_new_local (g_date_get_year (date),
                                           g_date_get_month (date),
                                           g_date_get_day (date),
                                           0,
                                           0,
                                           0);

        /* Future dates silently fails */
        if (g_date_time_compare (date_time, now) != 1)
        {
            GDateTime *end_date_time;

            gtk_label_set_label (GTK_LABEL (priv->end_date_button_label), gtk_entry_get_text(entry));

            show_end_date_widgets (popover, FALSE);

            end_date_time = get_end_date_time (popover);

            priv->custom_end_timestamp = g_date_time_to_unix (end_date_time) * G_USEC_PER_SEC;

            update_range_button (popover);

            g_object_notify_by_pspec (G_OBJECT (popover),
                                      obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

            g_date_time_unref (end_date_time);
        }

        g_date_time_unref (now);
        g_date_time_unref (date_time);
        g_date_free (date);
    }
}

static void
end_time_button_clicked (GtkButton *button,
                         gpointer user_data)
{
    GlSearchPopover *popover = GL_SEARCH_POPOVER (user_data);

    show_end_time_widgets (popover, TRUE);
    show_end_date_widgets (popover, FALSE);
    show_start_time_widgets (popover, FALSE);
    show_start_date_widgets (popover, FALSE);
}

static void
end_time_spinbox_value_changed (GtkSpinButton *spin_button,
                                gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;
    GDateTime *end_date_time;
    gchar *button_label;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    end_date_time = get_end_date_time (popover);

    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_12HR)
    {
        /* Translators: timestamp format for the custom end time button
         * label in the custom range submenu, showing the time with seconds
         * in 12-hour format. */
        button_label = g_date_time_format (end_date_time, _("%I:%M:%S %p"));
    }
    else
    {
        /* Translators: timestamp format for the custom end time button
         * label in the custom range submenu, showing the time with seconds
         * in 24-hour format. */
        button_label = g_date_time_format (end_date_time, _("%T"));
    }

    gtk_label_set_label (GTK_LABEL (priv->end_time_button_label), button_label);

    priv->custom_end_timestamp = g_date_time_to_unix (end_date_time) * G_USEC_PER_SEC;

    update_range_button (popover);

    g_object_notify_by_pspec (G_OBJECT (popover),
                              obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);
}

static void
custom_range_submenu_back_button_clicked (GtkButton *button,
                                          gpointer user_data)
{
    GlSearchPopover *popover;
    GlSearchPopoverPrivate *priv;

    popover = GL_SEARCH_POPOVER (user_data);

    priv = gl_search_popover_get_instance_private (popover);

    /* Default to Current boot if none of the timestamp was set */
    if (!priv->custom_start_timestamp && !priv->custom_end_timestamp)
    {
        gl_search_popover_set_journal_timestamp_range_current_boot (popover);
    }

    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_stack), "range-button");
    gtk_stack_set_visible_child_name (GTK_STACK (priv->range_label_stack), "when-label");
}

guint64
gl_search_popover_get_custom_start_timestamp (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    return priv->custom_start_timestamp;
}

guint64
gl_search_popover_get_custom_end_timestamp (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    return priv->custom_end_timestamp;
}

void
gl_search_popover_set_journal_timestamp_range_current_boot (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;

    priv = gl_search_popover_get_instance_private (popover);

    /* Reset the Custom Range elements if set as only one filter can be applied at time */
    reset_custom_range_widgets (popover);

    priv->journal_timestamp_range = GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CURRENT_BOOT;

    g_object_notify_by_pspec (G_OBJECT (popover),
                              obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE]);

    gtk_label_set_label (GTK_LABEL (priv->range_button_label),
                         _("Current Boot"));
}

static void
gl_search_popover_class_init (GlSearchPopoverClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->get_property = gl_search_popover_get_property;
    gobject_class->set_property = gl_search_popover_set_property;

    obj_properties[PROP_JOURNAL_SEARCH_FIELD] = g_param_spec_enum ("journal-search-field", "Journal Search Field",
                                                                    "The Journal search field by which to filter the logs",
                                                                    GL_TYPE_SEARCH_POPOVER_JOURNAL_FIELD_FILTER,
                                                                    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_ALL_AVAILABLE_FIELDS,
                                                                    G_PARAM_READWRITE |
                                                                    G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_SEARCH_TYPE] = g_param_spec_enum ("search-type", "Search Type",
                                                          "Do exact or substring search",
                                                          GL_TYPE_QUERY_SEARCH_TYPE,
                                                          GL_QUERY_SEARCH_TYPE_SUBSTRING,
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_JOURNAL_TIMESTAMP_RANGE] = g_param_spec_enum ("journal-timestamp-range", "Journal Timestamp Range",
                                                                      "The Timestamp range of the logs to be shown",
                                                                      GL_TYPE_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE,
                                                                      GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CURRENT_BOOT,
                                                                      G_PARAM_READWRITE |
                                                                      G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-searchpopover.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  parameter_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  parameter_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  parameter_label_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  parameter_treeview);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  parameter_liststore);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  search_type_revealer);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_label_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_treeview);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_button_drop_down_image);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  range_liststore);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  menu_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_date_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_date_calendar_revealer);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_date_entry);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_date_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_hour_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_minute_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_second_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_period_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_date_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_date_calendar_revealer);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_date_entry);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_date_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_stack);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_hour_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_minute_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_second_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_period_spin);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_button_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  start_time_period_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlSearchPopover,
                                                  end_time_period_label);


    gtk_widget_class_bind_template_callback (widget_class,
                                             search_popover_closed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             select_parameter_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_parameter_treeview_row_activated);
    gtk_widget_class_bind_template_callback (widget_class,
                                             search_type_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             select_range_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             on_range_treeview_row_activated);
    gtk_widget_class_bind_template_callback (widget_class,
                                             custom_range_submenu_back_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             end_time_spinbox_value_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             end_time_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             end_date_entry_activate);
    gtk_widget_class_bind_template_callback (widget_class,
                                             end_date_calendar_day_selected);
    gtk_widget_class_bind_template_callback (widget_class,
                                             end_date_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             start_time_spinbox_value_changed);
    gtk_widget_class_bind_template_callback (widget_class,
                                             spinbox_entry_format_two_digits);
    gtk_widget_class_bind_template_callback (widget_class,
                                             spinbox_entry_validate_hour_min_sec);
    gtk_widget_class_bind_template_callback (widget_class,
                                             spinbox_format_time_period_to_int);
    gtk_widget_class_bind_template_callback (widget_class,
                                             spinbox_format_time_period_to_text);
    gtk_widget_class_bind_template_callback (widget_class,
                                             start_time_button_clicked);
    gtk_widget_class_bind_template_callback (widget_class,
                                             start_date_entry_activate);
    gtk_widget_class_bind_template_callback (widget_class,
                                             start_date_calendar_day_selected);
    gtk_widget_class_bind_template_callback (widget_class,
                                             start_date_button_clicked);
}

static void
gl_search_popover_init (GlSearchPopover *popover)
{
    GlSearchPopoverPrivate *priv;
    GSettings *settings;

    gtk_widget_init_template (GTK_WIDGET (popover));

    priv = gl_search_popover_get_instance_private (popover);

    gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (priv->parameter_treeview),
                                          (GtkTreeViewRowSeparatorFunc) parameter_treeview_row_seperator,
                                          popover,
                                          NULL);

    gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (priv->range_treeview),
                                          (GtkTreeViewRowSeparatorFunc) range_treeview_row_seperator,
                                          popover,
                                          NULL);

    settings = g_settings_new (DESKTOP_SCHEMA);
    priv->clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);

    /* Show only hour-minute-second spinboxes when time format is 24hr */
    if (priv->clock_format == GL_UTIL_CLOCK_FORMAT_24HR)
    {
        GtkAdjustment *start_hour_adjustment;
        GtkAdjustment *end_hour_adjustment;

        /* Hide the AM/PM time period spinbuttons */
        gtk_widget_hide (priv->start_time_period_spin);
        gtk_widget_hide (priv->start_time_period_label);
        gtk_widget_hide (priv->end_time_period_spin);
        gtk_widget_hide (priv->end_time_period_label);

        /* Set 0-23 as range for hour spinbutton */
        start_hour_adjustment = gtk_adjustment_new (23.0, 0.0, 23.0, 5.0, 0.0, 0.0);
        end_hour_adjustment = gtk_adjustment_new (0.0, 0.0, 23.0, 5.0, 0.0, 0.0);

        gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (priv->start_time_hour_spin),
                                        start_hour_adjustment);
        gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (priv->end_time_hour_spin),
                                        end_hour_adjustment);
    }

    priv->custom_start_timestamp = 0;
    priv->custom_end_timestamp = 0;

    g_object_unref (settings);
}

GtkWidget *
gl_search_popover_new (void)
{
    return g_object_new (GL_TYPE_SEARCH_POPOVER, NULL);
}
