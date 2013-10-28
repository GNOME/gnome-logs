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

#include "gl-util.h"

#include <glib/gi18n.h>

void
gl_util_on_css_provider_parsing_error (GtkCssProvider *provider,
                                       GtkCssSection *section,
                                       GError *error,
                                       G_GNUC_UNUSED gpointer user_data)
{
    g_critical ("Error while parsing CSS style (line: %u, character: %u): %s",
                gtk_css_section_get_end_line (section) + 1,
                gtk_css_section_get_end_position (section) + 1,
                error->message);
}

gchar *
gl_util_timestamp_to_display (guint64 microsecs,
                              GlUtilClockFormat format)
{
    GDateTime *datetime;
    GDateTime *local;
    gchar *time = NULL;

    datetime = g_date_time_new_from_unix_utc (microsecs / G_TIME_SPAN_SECOND);

    if (datetime == NULL)
    {
        g_warning ("Error converting timestamp to time value");
        goto out;
    }

    local = g_date_time_to_local (datetime);

    /* TODO: Add logic to show day/date on timestamps where the day is not
     * today. */

    switch (format)
    {
        case GL_UTIL_CLOCK_FORMAT_12HR:
            /* Translators: timestamp format for events on the current day,
             * showing the time (including seconds) in 12-hour format. */
            time = g_date_time_format (local, _("%l:%M:%S %p"));
            break;
        case GL_UTIL_CLOCK_FORMAT_24HR:
            /* Translators: timestamp format for events on the current day,
             * showing the time (including seconds) in 24-hour format. */
            time = g_date_time_format (local, _("%T"));
            break;
        default:
            g_assert_not_reached ();
    }

    g_date_time_unref (datetime);
    g_date_time_unref (local);

    if (time == NULL)
    {
        g_warning ("Error converting datetime to string");
    }

out:
    return time;
}
