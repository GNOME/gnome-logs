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

/**
 * GlUtilTimestamps:
 * @GL_UTIL_TIMESTAMPS_SAME_DAY: the timestamps have the same year, month and
 * day
 * @GL_UTIL_TIMESTAMPS_SAME_YEAR: the timestamps have the same year, but
 * different months and days
 * @GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR: the timestamps have different years,
 * months and days
 *
 * Date string comparison result, used for formatting a date into an
 * appropriate string.
 */
typedef enum
{
    GL_UTIL_TIMESTAMPS_SAME_DAY,
    GL_UTIL_TIMESTAMPS_SAME_YEAR,
    GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR
} GlUtilTimestamps;

/**
 * gl_util_on_css_provider_parsing_error:
 * @provider: the provider that had a parsing error
 * @section: section the error happened in
 * @error: the parsing error
 * @user_data: user data set when the signal handler was connected
 *
 * Handle the GtkCssProvider::parsing-error signal and print a warning based on
 * @section and @error.
 */
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

/**
 * compare_timestamps:
 * @a: a date
 * @b: a date to compare with
 *
 * Compare @a to @b and return how similar the dates are.
 *
 * Returns: a value from GlUtilTimestamps
 */
static GlUtilTimestamps
compare_timestamps (GDateTime *a,
                    GDateTime *b)
{
    gint ayear, amonth, aday;
    gint byear, bmonth, bday;

    g_date_time_get_ymd (a, &ayear, &amonth, &aday);
    g_date_time_get_ymd (b, &byear, &bmonth, &bday);

    if (ayear != byear)
    {
        return GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR;
    }

    /* Same year, month and day. */
    if ((amonth == bmonth) && (aday == bday))
    {
        return GL_UTIL_TIMESTAMPS_SAME_DAY;
    }

    /* Same year, but differing by month or day. */
    return GL_UTIL_TIMESTAMPS_SAME_YEAR;
}

/**
 * gl_util_timestamp_to_display:
 * @microsecs: number of microseconds since the Unix epoch in UTC
 * @format: clock format (12 or 24 hour)
 *
 * Return a human readable time, corresponding to @microsecs, using an
 * appropriate @format after comparing it to the current time and discarding
 * unnecessary elements (for example, return only time if the date is today).
 *
 * Returns: a newly-allocated human readable string which represents @microsecs
 */
gchar *
gl_util_timestamp_to_display (guint64 microsecs,
                              GlUtilClockFormat format)
{
    GDateTime *datetime;
    GDateTime *local;
    GDateTime *now;
    gchar *time = NULL;

    datetime = g_date_time_new_from_unix_utc (microsecs / G_TIME_SPAN_SECOND);

    if (datetime == NULL)
    {
        g_warning ("Error converting timestamp to time value");
        goto out;
    }

    local = g_date_time_to_local (datetime);
    now = g_date_time_new_now_local ();

    switch (format)
    {
        case GL_UTIL_CLOCK_FORMAT_12HR:
            switch (compare_timestamps (local, now))
            {
                case GL_UTIL_TIMESTAMPS_SAME_DAY:
                    /* Translators: timestamp format for events on the current
                     * day, showing the time in 12-hour format. */
                    time = g_date_time_format (local, _("%l:%M %p"));
                    break;
                case GL_UTIL_TIMESTAMPS_SAME_YEAR:
                    /* Translators: timestamp format for events in the current
                     * year, showing the abbreviated month name, day of the
                     * month and the time in 12-hour format. */
                    time = g_date_time_format (local, _("%b %e %l:%M %p"));
                    break;
                case GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR:
                    /* Translators: timestamp format for events in a different
                     * year, showing the abbreviated month name, day of the
                     * month, year and the time in 12-hour format. */
                    time = g_date_time_format (local, _("%b %e %Y %l:%M %p"));
                    break;
                default:
                    g_assert_not_reached ();
            }

            break;
        case GL_UTIL_CLOCK_FORMAT_24HR:
            switch (compare_timestamps (local, now))
            {
                case GL_UTIL_TIMESTAMPS_SAME_DAY:
                    /* Translators: timestamp format for events on the current
                     * day, showing the time in 24-hour format. */
                    time = g_date_time_format (local, _("%H:%M"));
                    break;
                case GL_UTIL_TIMESTAMPS_SAME_YEAR:
                    /* Translators: timestamp format for events in the current
                     * year, showing the abbreviated month name, day of the
                     * month and the time in 24-hour format. */
                    time = g_date_time_format (local, _("%b %e %H:%M"));
                    break;
                case GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR:
                    /* Translators: timestamp format for events in a different
                     * year, showing the abbreviated month name, day of the
                     * month, year and the time in 24-hour format. */
                    time = g_date_time_format (local, _("%b %e %Y %H:%M"));
                    break;
                default:
                    g_assert_not_reached ();
            }

            break;
        default:
            g_assert_not_reached ();
    }

    g_date_time_unref (datetime);
    g_date_time_unref (local);
    g_date_time_unref (now);

    if (time == NULL)
    {
        g_warning ("Error converting datetime to string");
    }

out:
    return time;
}
