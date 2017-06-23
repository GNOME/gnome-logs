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

#ifndef GL_EVENT_TOOLBAR_H_
#define GL_EVENT_TOOLBAR_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * GlEventToolbarMode:
 * @GL_EVENT_TOOLBAR_MODE_LIST:
 * @GL_EVENT_TOOLBAR_MODE_DETAIL:
 *
 * The mode, selected in #GlEventView, to show in the toolbar.
 */
typedef enum
{
    GL_EVENT_TOOLBAR_MODE_LIST,
    GL_EVENT_TOOLBAR_MODE_DETAIL
} GlEventToolbarMode;

#define GL_TYPE_EVENT_TOOLBAR (gl_event_toolbar_get_type ())
G_DECLARE_FINAL_TYPE (GlEventToolbar, gl_event_toolbar, GL, EVENT_TOOLBAR, GtkHeaderBar)

GtkWidget * gl_event_toolbar_new (void);
void gl_event_toolbar_add_boots (GlEventToolbar *toolbar, GArray *boot_ids);
void gl_event_toolbar_change_current_boot (GlEventToolbar *toolbar,
                                           const gchar *current_boot);

G_END_DECLS

#endif /* GL_EVENT_TOOLBAR_H_ */
