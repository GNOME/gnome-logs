/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2014, 2015  Red Hat, Inc.
 *  Copyright (C) 2014  Jonathan Kang
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

#include "gl-eventview.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>

#include "gl-categorylist.h"
#include "gl-enums.h"
#include "gl-eventtoolbar.h"
#include "gl-eventviewdetail.h"
#include "gl-eventviewlist.h"
#include "gl-journal.h"
#include "gl-util.h"

enum
{
    PROP_0,
    PROP_MODE,
    N_PROPERTIES
};

struct _GlEventView
{
    /*< private >*/
    GtkStack parent_instance;
};

typedef struct
{
    GtkWidget *events;
    GlRowEntry *entry;
    GlUtilClockFormat clock_format;
    GlEventViewMode mode;
} GlEventViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventView, gl_event_view, GTK_TYPE_STACK)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static const gchar DESKTOP_SCHEMA[] = "org.gnome.desktop.interface";
static const gchar CLOCK_FORMAT[] = "clock-format";

gchar *
gl_event_view_get_output_logs (GlEventView *view)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    return gl_event_view_list_get_output_logs (events);
}

gchar *
gl_event_view_get_boot_time (GlEventView *view,
                             const gchar *boot_match)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    return gl_event_view_list_get_boot_time (events, boot_match);
}

GArray *
gl_event_view_get_boot_ids (GlEventView *view)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    return gl_event_view_list_get_boot_ids (events);
}

void
gl_event_view_view_boot (GlEventView *view, const gchar *match)
{
    GlEventViewList *events;
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    gl_event_view_list_view_boot (events, match);
}

void
gl_event_view_show_detail (GlEventView *view)
{
    GlEventViewList *events;
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);
    priv->entry = gl_event_view_list_get_detail_entry (events);
}

gboolean
gl_event_view_handle_search_event (GlEventView *view,
                                   GAction *action,
                                   GdkEvent *event)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    if (gl_event_view_list_handle_search_event (events, action,
                                                event) == GDK_EVENT_STOP)
    {
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

void
gl_event_view_set_search_mode (GlEventView *view,
                               gboolean state)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    gl_event_view_list_set_search_mode (events, state);
}

void
gl_event_view_set_sort_order (GlEventView *view,
                              GlSortOrder sort_order)
{
    GlEventViewPrivate *priv;
    GlEventViewList *events;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);
    events = GL_EVENT_VIEW_LIST (priv->events);

    gl_event_view_list_set_sort_order (events, sort_order);
}

static void
gl_event_view_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
    GlEventView *view = GL_EVENT_VIEW (object);
    GlEventViewPrivate *priv = gl_event_view_get_instance_private (view);

    switch (prop_id)
    {
        case PROP_MODE:
            g_value_set_enum (value, priv->mode);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
    GlEventView *view = GL_EVENT_VIEW (object);
    GlEventViewPrivate *priv = gl_event_view_get_instance_private (view);

    switch (prop_id)
    {
        case PROP_MODE:
            priv->mode = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_class_init (GlEventViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->get_property = gl_event_view_get_property;
    gobject_class->set_property = gl_event_view_set_property;

    obj_properties[PROP_MODE] = g_param_spec_enum ("mode", "Mode",
                                                   "Event display mode",
                                                   GL_TYPE_EVENT_VIEW_MODE,
                                                   GL_EVENT_VIEW_MODE_LIST,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-eventview.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlEventView,
                                                  events);
}

static void
gl_event_view_init (GlEventView *view)
{
    GlEventViewPrivate *priv;
    GSettings *settings;

    gtk_widget_init_template (GTK_WIDGET (view));

    priv = gl_event_view_get_instance_private (view);

    /* TODO: Monitor and propagate any GSettings changes. */
    settings = g_settings_new (DESKTOP_SCHEMA);
    priv->clock_format = g_settings_get_enum (settings, CLOCK_FORMAT);

    g_object_unref (settings);
}

void
gl_event_view_set_mode (GlEventView *view,
                        GlEventViewMode mode)
{
    GlEventViewPrivate *priv;

    g_return_if_fail (GL_EVENT_VIEW (view));

    priv = gl_event_view_get_instance_private (view);

    if (priv->mode != mode)
    {
        priv->mode = mode;
        g_object_notify_by_pspec (G_OBJECT (view),
                                  obj_properties[PROP_MODE]);
    }
}

GtkWidget *
gl_event_view_new (void)
{
    return g_object_new (GL_TYPE_EVENT_VIEW, NULL);
}
