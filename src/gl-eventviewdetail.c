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

#include "gl-eventviewdetail.h"

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>

#include "gl-enums.h"

enum
{
    PROP_0,
    PROP_CLOCK_FORMAT,
    PROP_RESULT,
    N_PROPERTIES
};

typedef struct
{
    GlJournalResult *result;
    GlUtilClockFormat clock_format;
    GtkWidget *grid;
    GtkWidget *comm_image;
    GtkWidget *comm_label;
    GtkWidget *time_label;
    GtkWidget *message_label;
    GtkWidget *audit_field_label;
    GtkWidget *audit_label;
    GtkWidget *device_field_label;
    GtkWidget *device_label;
    GtkWidget *priority_label;
    GtkWidget *catalog_label;
} GlEventViewDetailPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewDetail, gl_event_view_detail, GTK_TYPE_BIN)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gl_event_view_detail_create_detail (GlEventViewDetail *detail)
{
    GlEventViewDetailPrivate *priv;
    GlJournalResult *result;
    gchar *str;

    priv = gl_event_view_detail_get_instance_private (detail);

    result = priv->result;

    /* Force LTR direction also for RTL languages */
    gtk_widget_set_direction (priv->grid, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->comm_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->message_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->audit_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->device_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->catalog_label, GTK_TEXT_DIR_LTR);

    if (result->comm && *result->comm)
    {
        /* Command-line, look for a desktop file. */
        GDesktopAppInfo *desktop;

        /* TODO: Use g_desktop_app_info_search? */
        str = g_strconcat (result->comm, ".desktop", NULL);
        desktop = g_desktop_app_info_new (str);
        g_free (str);

        if (desktop)
        {
            GIcon *icon;

            icon = g_app_info_get_icon (G_APP_INFO (desktop));
            gtk_image_set_from_gicon (GTK_IMAGE (priv->comm_image),
                                      icon, GTK_ICON_SIZE_DIALOG);
            gtk_widget_show (priv->comm_image);
            gtk_label_set_text (GTK_LABEL (priv->comm_label),
                                g_app_info_get_name (G_APP_INFO (desktop)));

            g_object_unref (desktop);
        }
        else
        {
            gtk_label_set_text (GTK_LABEL (priv->comm_label), result->comm);
        }
    }

    str = gl_util_timestamp_to_display (result->timestamp, priv->clock_format);
    gtk_label_set_text (GTK_LABEL (priv->time_label), str);
    g_free (str);

    gtk_label_set_text (GTK_LABEL (priv->message_label), result->message);

    if (result->audit_session && *result->audit_session)
    {
        gtk_label_set_text (GTK_LABEL (priv->audit_label), result->audit_session);
        gtk_widget_show (priv->audit_field_label);
        gtk_widget_show (priv->audit_label);
    }

    if (result->kernel_device && *result->kernel_device)
    {
        gtk_label_set_text (GTK_LABEL (priv->device_label), result->kernel_device);
        gtk_widget_show (priv->device_field_label);
        gtk_widget_show (priv->device_label);
    }

    /* TODO: Give a user-friendly representation of the priority. */
    str = g_strdup_printf ("%d", result->priority);
    gtk_label_set_text (GTK_LABEL (priv->priority_label), str);
    g_free (str);

    gtk_label_set_text (GTK_LABEL (priv->catalog_label), result->catalog);
}

static void
gl_event_view_detail_finalize (GObject *object)
{
    GlEventViewDetail *detail = GL_EVENT_VIEW_DETAIL (object);
    GlEventViewDetailPrivate *priv = gl_event_view_detail_get_instance_private (detail);

    g_clear_pointer (&priv->result, gl_journal_result_unref);
}

static void
gl_event_view_detail_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
    GlEventViewDetail *detail = GL_EVENT_VIEW_DETAIL (object);
    GlEventViewDetailPrivate *priv = gl_event_view_detail_get_instance_private (detail);

    switch (prop_id)
    {
        case PROP_CLOCK_FORMAT:
            g_value_set_enum (value, priv->clock_format);
            break;
        case PROP_RESULT:
            g_value_set_boxed (value, priv->result);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_detail_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    GlEventViewDetail *detail = GL_EVENT_VIEW_DETAIL (object);
    GlEventViewDetailPrivate *priv = gl_event_view_detail_get_instance_private (detail);

    switch (prop_id)
    {
        case PROP_CLOCK_FORMAT:
            priv->clock_format = g_value_get_enum (value);
            break;
        case PROP_RESULT:
            priv->result = g_value_dup_boxed (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_detail_constructed (GObject *object)
{
    GlEventViewDetail *detail = GL_EVENT_VIEW_DETAIL (object);

    /* contruct-only properties have already been set. */
    gl_event_view_detail_create_detail (detail);

    G_OBJECT_CLASS (gl_event_view_detail_parent_class)->constructed (object);
}

static void
gl_event_view_detail_class_init (GlEventViewDetailClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->constructed = gl_event_view_detail_constructed;
    gobject_class->finalize = gl_event_view_detail_finalize;
    gobject_class->get_property = gl_event_view_detail_get_property;
    gobject_class->set_property = gl_event_view_detail_set_property;

    obj_properties[PROP_CLOCK_FORMAT] = g_param_spec_enum ("clock-format", "Clock format",
                                                           "Format of the clock in which to show timestamps",
                                                           GL_TYPE_UTIL_CLOCK_FORMAT,
                                                           GL_UTIL_CLOCK_FORMAT_24HR,
                                                           G_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT_ONLY |
                                                           G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_RESULT] = g_param_spec_boxed ("result", "Result",
                                                      "Journal query result for this detailed view",
                                                      GL_TYPE_JOURNAL_RESULT,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-eventviewdetail.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  grid);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  comm_image);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  comm_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  time_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  message_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  audit_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  audit_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  device_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  device_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  priority_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  catalog_label);
}

static void
gl_event_view_detail_init (GlEventViewDetail *detail)
{
    gtk_widget_init_template (GTK_WIDGET (detail));
}

GtkWidget *
gl_event_view_detail_new (GlJournalResult *result,
                          GlUtilClockFormat clock_format)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_DETAIL, "result", result,
                         "clock-format", clock_format, NULL);
}
