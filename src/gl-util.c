/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013  Red Hat, Inc.
 *  Copyright (C) 2015  Ekaterina Gerasimova
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

#include "config.h"
#include "gl-util.h"

#include <glib/gi18n.h>
#include <systemd/sd-id128.h>
#include <systemd/sd-journal.h>

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

static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar CLOCK_FORMAT[] = "clock-format";

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
 * @now: the time to compare with
 * @format: clock format (12 or 24 hour)
 *
 * Return a human readable time, corresponding to @microsecs, using an
 * appropriate @format after comparing it to @now and discarding unnecessary
 * elements (for example, return only time if the date is today).
 *
 * Returns: a newly-allocated human readable string which represents @microsecs
 */
gchar *
gl_util_timestamp_to_display (guint64 microsecs,
                              GDateTime *now,
                              GlUtilClockFormat format,
                              gboolean show_second)
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

    switch (format)
    {
        case GL_UTIL_CLOCK_FORMAT_12HR:
            switch (compare_timestamps (local, now))
            {
                case GL_UTIL_TIMESTAMPS_SAME_DAY:
                    if (show_second)
                    {
                        /* Translators: timestamp format for events on the
                         * current day, showing the time with seconds in
                         * 12-hour format. */
                        time = g_date_time_format (local, _("%l:%M:%S %p"));
                    }
                    else
                    {
                        /* Translators: timestamp format for events on the
                         * current day, showing the time without seconds in
                         * 12-hour format. */
                        time = g_date_time_format (local, _("%l:%M %p"));
                    }
                    break;
                case GL_UTIL_TIMESTAMPS_SAME_YEAR:
                    if (show_second)
                    {
                        time = g_date_time_format (local,
                               /* Translators: timestamp format for events in
                                * the current year, showing the abbreviated
                                * month name, day of the month and the time
                                * with seconds in 12-hour format. */
                                                   _("%b %e %l:%M:%S %p"));
                    }
                    else
                    {
                        /* Translators: timestamp format for events in the
                         * current year, showing the abbreviated month name,
                         * day of the month and the time without seconds in
                         * 12-hour format. */
                        time = g_date_time_format (local, _("%b %e %l:%M %p"));
                    }
                    break;
                case GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR:
                    if (show_second)
                    {
                        time = g_date_time_format (local,
                               /* Translators: timestamp format for events in
                                * a different year, showing the abbreviated
                                * month name, day of the month, year and the
                                * time with seconds in 12-hour format. */
                                                   _("%b %e %Y %l:%M:%S %p"));
                    }
                    else
                    {
                        time = g_date_time_format (local,
                               /* Translators: timestamp format for events in
                                * a different year, showing the abbreviated
                                * month name day of the month, year and the
                                * time without seconds in 12-hour format. */
                                                   _("%b %e %Y %l:%M %p"));
                    }
                    break;
                default:
                    g_assert_not_reached ();
            }

            break;
        case GL_UTIL_CLOCK_FORMAT_24HR:
            switch (compare_timestamps (local, now))
            {
                case GL_UTIL_TIMESTAMPS_SAME_DAY:
                    if (show_second)
                    {
                        /* Translators: timestamp format for events on the
                         * current day, showing the time with seconds in
                         * 24-hour format. */
                        time = g_date_time_format (local, _("%H:%M:%S"));
                    }
                    else
                    {
                        /* Translators: timestamp format for events on the
                         * current day, showing the time without seconds in
                         * 24-hour format. */
                        time = g_date_time_format (local, _("%H:%M"));
                    }
                    break;
                case GL_UTIL_TIMESTAMPS_SAME_YEAR:
                    if (show_second)
                    {
                        /* Translators: timestamp format for events in the
                         * current year, showing the abbreviated month name,
                         * day of the month and the time with seconds in
                         * 24-hour format. */
                        time = g_date_time_format (local, _("%b %e %H:%M:%S"));
                    }
                    else
                    {
                        /* Translators: timestamp format for events in the
                         * current year, showing the abbreviated month name,
                         * day of the month and the time without seconds in
                         * 24-hour format. */
                        time = g_date_time_format (local, _("%b %e %H:%M"));
                    }
                    break;
                case GL_UTIL_TIMESTAMPS_DIFFERENT_YEAR:
                    if (show_second)
                    {
                        time = g_date_time_format (local,
                               /* Translators: timestamp format for events in
                                * a different year, showing the abbreviated
                                * month name, day of the month, year and the
                                * time with seconds in 24-hour format. */
                                                   _("%b %e %Y %H:%M:%S"));
                    }
                    else
                    {
                        /* Translators: timestamp format for events in a
                         * different year, showing the abbreviated month name,
                         * day of the month, year and the time without seconds
                         * in 24-hour format. */
                        time = g_date_time_format (local, _("%b %e %Y %H:%M"));
                    }
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

    if (time == NULL)
    {
        g_warning ("Error converting datetime to string");
    }

out:
    return time;
}

gint
gl_util_get_uid (void)
{
    GCredentials *creds;
    uid_t uid;

    creds = g_credentials_new ();
    uid = g_credentials_get_unix_user (creds, NULL);

    g_object_unref (creds);

    return uid;
}

gchar *
gl_util_boot_time_to_display (guint64 realtime_first,
                              guint64 realtime_last)
{
    gchar *time_first;
    gchar *time_last;
    gchar *time_display;
    GDateTime *now;
    GSettings *settings;
    GlUtilClockFormat clock_format;

    /* TODO: Monitor and propagate any GSettings changes. */
    settings = g_settings_new (DESKTOP_SCHEMA);
    clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);

    g_object_unref (settings);

    now = g_date_time_new_now_local ();
    time_first = gl_util_timestamp_to_display (realtime_first,
                                               now, clock_format, FALSE);
    time_last = gl_util_timestamp_to_display (realtime_last,
                                              now, clock_format, FALSE);

    /* Transltors: the first string is the earliest timestamp of the boot,
     * and the second string is the newest timestamp. An example string might
     * be '08:10 - 08:30' */
    time_display = g_strdup_printf (_("%s – %s"), time_first, time_last);

    g_date_time_unref (now);
    g_free (time_first);
    g_free (time_last);

    return time_display;
}

/**
 * Determine journal storage type:
 *
 * Test existence of possible journal storage paths.
 *
 * Returns: a value from GlJournalStorage
 */
GlJournalStorage
gl_util_journal_storage_type (void)
{
    g_autofree gchar *run_path = NULL;
    g_autofree gchar *var_path = NULL;
    gchar ids[33];
    gint ret;
    sd_id128_t machine_id;

    ret = sd_id128_get_machine (&machine_id);
    if (ret < 0)
    {
        g_critical ("Error getting machine id: %s", g_strerror (-ret));
    }
    sd_id128_to_string (machine_id, ids);

    run_path = g_build_filename ("/run/log/journal/", ids, NULL);
    var_path = g_build_filename ("/var/log/journal/", ids, NULL);

    if (g_file_test (run_path, G_FILE_TEST_EXISTS))
    {
        return GL_JOURNAL_STORAGE_VOLATILE;
    }
    else if (g_file_test (var_path, G_FILE_TEST_EXISTS))
    {
        return GL_JOURNAL_STORAGE_PERSISTENT;
    }
    else
    {
        return GL_JOURNAL_STORAGE_NONE;
    }
}

gboolean
gl_util_can_read_system_journal (GlJournalStorage storage_type)
{
    GFile *file;
    GFileInfo *info;
    gint ret;
    gchar *path;
    gchar ids[33];
    sd_id128_t machine;

    ret = sd_id128_get_machine (&machine);
    if (ret < 0)
    {
        g_critical ("Error getting machine id: %s", g_strerror (-ret));
    }
    sd_id128_to_string (machine, ids);

    if (storage_type == GL_JOURNAL_STORAGE_PERSISTENT)
    {
        path = g_build_filename ("/var/log/journal", ids, "system.journal", NULL);
    }
    else if (storage_type == GL_JOURNAL_STORAGE_VOLATILE)
    {
        path = g_build_filename ("/run/log/journal", ids, "system.journal", NULL);
    }
    else
    {
        path = "/dev/null";
    }

    file = g_file_new_for_path (path);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    g_free (path);
    g_object_unref (file);

    if (g_file_info_get_attribute_boolean (info,
                                           G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
    {
        g_object_unref (info);

        return TRUE;
    }
    else
    {
        g_object_unref (info);

        return FALSE;
    }
}

gboolean
gl_util_can_read_user_journal (void)
{
    GFile *file;
    GFileInfo *info;
    gint ret;
    gchar *path;
    gchar ids[33];
    gchar *filename;
    gchar *uid;
    uid_t user_id;
    sd_id128_t machine;
    GError *error = NULL;
    GCredentials *credentials;

    credentials = g_credentials_new ();
    user_id = g_credentials_get_unix_user (credentials, &error);
    if (error != NULL)
    {
        g_debug ("Unable to get uid: %s", error->message);
        g_error_free (error);
    }
    uid = g_strdup_printf ("%d", user_id);
    filename = g_strconcat ("/user-", uid, ".journal", NULL);

    ret = sd_id128_get_machine (&machine);
    if (ret < 0)
    {
        g_critical ("Error getting machine id: %s", g_strerror (-ret));
    }
    sd_id128_to_string (machine, ids);

    path = g_build_filename ("/var/log/journal", ids, filename, NULL);

    file = g_file_new_for_path (path);
    info = g_file_query_info (file, G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                              G_FILE_QUERY_INFO_NONE, NULL, NULL);

    g_free (uid);
    g_free (path);
    g_free (filename);
    g_object_unref (file);
    g_object_unref (credentials);

    if (g_file_info_get_attribute_boolean (info,
                                           G_FILE_ATTRIBUTE_ACCESS_CAN_READ))
    {
        g_object_unref (info);

        return TRUE;
    }
    else
    {
        g_object_unref (info);

        return FALSE;
    }
}

static void
add_css_variations (GString    *s,
                    const char *variations)
{
  const char *p;
  const char *sep = "";

  if (variations == NULL || variations[0] == '\0')
    {
      g_string_append (s, "normal");
      return;
    }

  p = variations;
  while (p && *p)
    {
      const char *start;
      const char *end, *end2;
      double value;
      char name[5];

      while (g_ascii_isspace (*p)) p++;

      start = p;
      end = strchr (p, ',');
      if (end && (end - p < 6))
        goto skip;

      name[0] = p[0];
      name[1] = p[1];
      name[2] = p[2];
      name[3] = p[3];
      name[4] = '\0';

      p += 4;
      while (g_ascii_isspace (*p)) p++;
      if (*p == '=') p++;

      if (p - start < 5)
        goto skip;

      value = g_ascii_strtod (p, (char **) &end2);

      while (end2 && g_ascii_isspace (*end2)) end2++;

      if (end2 && (*end2 != ',' && *end2 != '\0'))
        goto skip;

      g_string_append_printf (s, "%s\"%s\" %g", sep, name, value);
      sep = ", ";

skip:
      p = end ? end + 1 : NULL;
    }
}

/**
 * This function is orignally written in gtk/gtkfontbutton.c of gtk+ project.
 */
gchar *
pango_font_description_to_css (PangoFontDescription *desc)
{
  GString *s;
  PangoFontMask set;

  /* This line is modified with respect to gtk/gtkfontbutton.c */
  s = g_string_new ("{ ");

  set = pango_font_description_get_set_fields (desc);
  if (set & PANGO_FONT_MASK_FAMILY)
    {
      g_string_append (s, "font-family: \"");
      g_string_append (s, pango_font_description_get_family (desc));
      g_string_append (s, "\"; ");
    }
  if (set & PANGO_FONT_MASK_STYLE)
    {
      switch (pango_font_description_get_style (desc))
        {
        case PANGO_STYLE_NORMAL:
          g_string_append (s, "font-style: normal; ");
          break;
        case PANGO_STYLE_OBLIQUE:
          g_string_append (s, "font-style: oblique; ");
          break;
        case PANGO_STYLE_ITALIC:
          g_string_append (s, "font-style: italic; ");
          break;
        default:
          break;
        }
    }
  if (set & PANGO_FONT_MASK_VARIANT)
    {
      switch (pango_font_description_get_variant (desc))
        {
        case PANGO_VARIANT_NORMAL:
          g_string_append (s, "font-variant: normal; ");
          break;
        case PANGO_VARIANT_SMALL_CAPS:
          g_string_append (s, "font-variant: small-caps; ");
          break;
        case PANGO_VARIANT_ALL_SMALL_CAPS:
          g_string_append (s, "font-variant: all-small-caps; ");
          break;
        case PANGO_VARIANT_PETITE_CAPS:
          g_string_append (s, "font-variant: petite-caps; ");
          break;
        case PANGO_VARIANT_ALL_PETITE_CAPS:
          g_string_append (s, "font-variant: all-petite-caps; ");
          break;
        case PANGO_VARIANT_UNICASE:
          g_string_append (s, "font-variant: unicase; ");
          break;
        case PANGO_VARIANT_TITLE_CAPS:
          g_string_append (s, "font-variant: titling-caps; ");
          break;
        default:
          break;
        }
    }
  if (set & PANGO_FONT_MASK_WEIGHT)
    {
      switch (pango_font_description_get_weight (desc))
        {
        case PANGO_WEIGHT_THIN:
          g_string_append (s, "font-weight: 100; ");
          break;
        case PANGO_WEIGHT_ULTRALIGHT:
          g_string_append (s, "font-weight: 200; ");
          break;
        case PANGO_WEIGHT_LIGHT:
        case PANGO_WEIGHT_SEMILIGHT:
          g_string_append (s, "font-weight: 300; ");
          break;
        case PANGO_WEIGHT_BOOK:
        case PANGO_WEIGHT_NORMAL:
          g_string_append (s, "font-weight: 400; ");
          break;
        case PANGO_WEIGHT_MEDIUM:
          g_string_append (s, "font-weight: 500; ");
          break;
        case PANGO_WEIGHT_SEMIBOLD:
          g_string_append (s, "font-weight: 600; ");
          break;
        case PANGO_WEIGHT_BOLD:
          g_string_append (s, "font-weight: 700; ");
          break;
        case PANGO_WEIGHT_ULTRABOLD:
          g_string_append (s, "font-weight: 800; ");
          break;
        case PANGO_WEIGHT_HEAVY:
        case PANGO_WEIGHT_ULTRAHEAVY:
          g_string_append (s, "font-weight: 900; ");
          break;
        default:
          break;
        }
    }
  if (set & PANGO_FONT_MASK_STRETCH)
    {
      switch (pango_font_description_get_stretch (desc))
        {
        case PANGO_STRETCH_ULTRA_CONDENSED:
          g_string_append (s, "font-stretch: ultra-condensed; ");
          break;
        case PANGO_STRETCH_EXTRA_CONDENSED:
          g_string_append (s, "font-stretch: extra-condensed; ");
          break;
        case PANGO_STRETCH_CONDENSED:
          g_string_append (s, "font-stretch: condensed; ");
          break;
        case PANGO_STRETCH_SEMI_CONDENSED:
          g_string_append (s, "font-stretch: semi-condensed; ");
          break;
        case PANGO_STRETCH_NORMAL:
          g_string_append (s, "font-stretch: normal; ");
          break;
        case PANGO_STRETCH_SEMI_EXPANDED:
          g_string_append (s, "font-stretch: semi-expanded; ");
          break;
        case PANGO_STRETCH_EXPANDED:
          g_string_append (s, "font-stretch: expanded; ");
          break;
        case PANGO_STRETCH_EXTRA_EXPANDED:
          g_string_append (s, "font-stretch: extra-expanded; ");
          break;
        case PANGO_STRETCH_ULTRA_EXPANDED:
          g_string_append (s, "font-stretch: ultra-expanded; ");
          break;
        default:
          break;
        }
    }
  if (set & PANGO_FONT_MASK_SIZE)
    {
      g_string_append_printf (s, "font-size: %dpt; ", pango_font_description_get_size (desc) / PANGO_SCALE);
    }

  if (set & PANGO_FONT_MASK_VARIATIONS)
    {
      const char *variations;

      g_string_append (s, "font-variation-settings: ");
      variations = pango_font_description_get_variations (desc);
      add_css_variations (s, variations);
      g_string_append (s, "; ");
    }

  g_string_append (s, "}");

  return g_string_free (s, FALSE);
}
