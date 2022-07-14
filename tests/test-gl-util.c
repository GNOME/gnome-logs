/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2015 Ekaterina Gerasimova
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

static void
util_timestamp_to_display (void)
{
    gsize i;
    GDateTime *now;

    static const struct
    {
        guint64 microsecs;
        GlUtilClockFormat format;
        const gchar *time;
    } times[] =
    {
        /* Test three cases for each format (same day, same year, different
           year */
#if GLIB_CHECK_VERSION (2, 73, 1)
        /* The space is a FIGURE SPACE (U+2007) */
        { G_GUINT64_CONSTANT (1423486800000000), GL_UTIL_CLOCK_FORMAT_12HR,
          "\u20071:00 PM" },
        { G_GUINT64_CONSTANT (1423402200000000), GL_UTIL_CLOCK_FORMAT_12HR,
          "Feb \u20078 \u20071:30 PM" },
        { G_GUINT64_CONSTANT (1391952600000000), GL_UTIL_CLOCK_FORMAT_12HR,
          "Feb \u20079 2014 \u20071:30 PM" },
        { G_GUINT64_CONSTANT (1423486800000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "13:00" },
        { G_GUINT64_CONSTANT (1423402200000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "Feb \u20078 13:30" },
        { G_GUINT64_CONSTANT (1391952600000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "Feb \u20079 2014 13:30" }
#else
        { G_GUINT64_CONSTANT (1423486800000000), GL_UTIL_CLOCK_FORMAT_12HR,
          " 1:00 PM" },
        { G_GUINT64_CONSTANT (1423402200000000), GL_UTIL_CLOCK_FORMAT_12HR,
          "Feb  8  1:30 PM" },
        { G_GUINT64_CONSTANT (1391952600000000), GL_UTIL_CLOCK_FORMAT_12HR,
          "Feb  9 2014  1:30 PM" },
        { G_GUINT64_CONSTANT (1423486800000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "13:00" },
        { G_GUINT64_CONSTANT (1423402200000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "Feb  8 13:30" },
        { G_GUINT64_CONSTANT (1391952600000000), GL_UTIL_CLOCK_FORMAT_24HR,
          "Feb  9 2014 13:30" }
#endif
    };

    now = g_date_time_new_utc (2015, 2, 9, 13, 30, 42);

    for (i = 0; i < G_N_ELEMENTS (times); i++)
    {
        gchar *compare;

        compare = gl_util_timestamp_to_display (times[i].microsecs, now,
                                                times[i].format, FALSE);
        g_assert_cmpstr (compare, ==, times[i].time);
        g_free (compare);
    }

    g_date_time_unref (now);
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/util/timestamp_to_display", util_timestamp_to_display);

    return g_test_run ();
}
