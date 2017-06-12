/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014, 2015  Red Hat, Inc.
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

#ifndef GL_EVENT_VIEW_DETAIL_H_
#define GL_EVENT_VIEW_DETAIL_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "gl-journal.h"
#include "gl-journal-model.h"
#include "gl-util.h"

#define GL_TYPE_EVENT_VIEW_DETAIL (gl_event_view_detail_get_type ())
G_DECLARE_FINAL_TYPE (GlEventViewDetail, gl_event_view_detail, GL, EVENT_VIEW_DETAIL, GtkPopover)

GtkWidget * gl_event_view_detail_new (GlRowEntry *result, GlUtilClockFormat clock_format);

G_END_DECLS

#endif /* GL_EVENT_VIEW_DETAIL_H_ */
