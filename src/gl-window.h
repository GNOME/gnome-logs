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

#ifndef GL_WINDOW_H_
#define GL_WINDOW_H_

#include <gtk/gtk.h>
#include <libadwaita-1/adwaita.h>

#include "gl-application.h"
#include "gl-journal-model.h"

G_BEGIN_DECLS

#define GL_TYPE_WINDOW (gl_window_get_type ())
G_DECLARE_FINAL_TYPE (GlWindow, gl_window, GL, WINDOW, AdwApplicationWindow)

void gl_window_load_journal (GlWindow *window, GlJournal *journal);
GtkWidget * gl_window_new (GtkApplication *application);
void gl_window_set_sort_order (GlWindow *window, GlSortOrder sort_order);
void disable_export (GlWindow *window);
void enable_export (GlWindow *window);

G_END_DECLS

#endif /* GL_WINDOW_H_ */
