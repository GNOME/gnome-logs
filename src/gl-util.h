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

#ifndef GL_UTIL_H_
#define GL_UTIL_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * GlUtilClockFormat:
 * @GL_UTIL_CLOCK_FORMAT_24HR: 24-hour clock format
 * @GL_UTIL_CLOCK_FORMAT_12HR: 12-hour clock format
 *
 * Keep in sync with the enum in the org.gnome.desktop.interface schema. */
typedef enum
{
    GL_UTIL_CLOCK_FORMAT_24HR,
    GL_UTIL_CLOCK_FORMAT_12HR
} GlUtilClockFormat;

/*
 * GlJournalStorage:
 * @GL_JOURNAL_STORAGE_NONE: no log data
 * @GL_JOURNAL_STORAGE_PERSISTENT: log data stored on disk
 * below /var/log/journal hierarchy
 * @GL_JOURNAL_STORAGE_VOLATILE:  log data stored in memory
 * below /run/log/journal hierarchy
 *
 * Determine journal storage type, used in warning logic.*/
typedef enum
{
    GL_JOURNAL_STORAGE_NONE,
    GL_JOURNAL_STORAGE_PERSISTENT,
    GL_JOURNAL_STORAGE_VOLATILE
} GlJournalStorage;

gchar * gl_util_timestamp_to_display (guint64 microsecs,
                                      GDateTime *now,
                                      GlUtilClockFormat format,
                                      gboolean show_second);
gint gl_util_get_uid (void);
gchar * gl_util_boot_time_to_display (guint64 timestamp_first,
                                      guint64 timestamp_last);
GlJournalStorage gl_util_journal_storage_type (void);
gboolean gl_util_can_read_system_journal (GlJournalStorage storage_type);
gboolean gl_util_can_read_user_journal (void);
gchar *pango_font_description_to_css (PangoFontDescription *desc);

G_END_DECLS

#endif /* GL_UTIL_H_ */
