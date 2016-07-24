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

#ifndef GL_SEARCH_POPOVER_H_
#define GL_SEARCH_POPOVER_H_

#include <gtk/gtk.h>
#include "gl-journal-model.h"

G_BEGIN_DECLS

/* Rows in parameter treeview */
typedef enum
{
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_ALL_AVAILABLE_FIELDS,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_PID,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_UID,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_GID,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_MESSAGE,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_PROCESS_NAME,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_SYSTEMD_UNIT,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_KERNEL_DEVICE,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_AUDIT_SESSION,
    GL_SEARCH_POPOVER_JOURNAL_FIELD_FILTER_EXECUTABLE_PATH
} GlSearchPopoverJournalFieldFilter;

typedef enum
{
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_CURRENT_BOOT,
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_PREVIOUS_BOOT,
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_TODAY,
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_YESTERDAY,
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_LAST_3_DAYS,
    GL_SEARCH_POPOVER_JOURNAL_TIMESTAMP_RANGE_ENTIRE_JOURNAL
} GlSearchPopoverJournalTimestampRange;

#define GL_TYPE_SEARCH_POPOVER (gl_search_popover_get_type ())
G_DECLARE_FINAL_TYPE (GlSearchPopover, gl_search_popover, GL, SEARCH_POPOVER, GtkPopover)

GtkWidget * gl_search_popover_new (void);
GlSearchPopoverJournalFieldFilter gl_search_popover_get_journal_search_field (GlSearchPopover *popover);
GlQuerySearchType gl_search_popover_get_query_search_type (GlSearchPopover *popover);
GlSearchPopoverJournalTimestampRange gl_search_popover_get_journal_timestamp_range (GlSearchPopover *popover);
void gl_search_popover_set_journal_timestamp_range_current_boot (GlSearchPopover *popover);

G_END_DECLS

#endif /* GL_SEARCH_POPOVER_H_ */
