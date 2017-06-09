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
    PROP_ENTRY,
    N_PROPERTIES
};

struct _GlEventViewDetail
{
    /*< private >*/
    GtkBin parent_instance;
};

typedef struct
{
    GlRowEntry *entry;
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
    GtkWidget *subject_field_label;
    GtkWidget *subject_label;
    GtkWidget *definedby_field_label;
    GtkWidget *definedby_label;
    GtkWidget *support_field_label;
    GtkWidget *support_label;
    GtkWidget *documentation_field_label;
    GtkWidget *documentation_label;
    GtkWidget *detailed_message_label;
} GlEventViewDetailPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewDetail, gl_event_view_detail, GTK_TYPE_BIN)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gl_event_view_detail_create_detail (GlEventViewDetail *detail)
{
    GlEventViewDetailPrivate *priv;
    GlJournalEntry *entry;
    GlRowEntry *row_entry;
    gchar *str;
    gchar *str_field;
    gchar *str_message;
    gchar *str_copy;
    GDateTime *now;

    priv = gl_event_view_detail_get_instance_private (detail);

    row_entry = priv->entry;
    entry = gl_row_entry_get_journal_entry (row_entry);

    /* Force LTR direction also for RTL languages */
    gtk_widget_set_direction (priv->grid, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->comm_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->message_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->audit_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->device_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->subject_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->definedby_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->support_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->documentation_label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_direction (priv->detailed_message_label, GTK_TEXT_DIR_LTR);

    if (gl_journal_entry_get_command_line (entry))
    {
        /* Command-line, look for a desktop file. */
        GDesktopAppInfo *desktop;

        /* TODO: Use g_desktop_app_info_search? */
        str = g_strconcat (gl_journal_entry_get_command_line (entry), ".desktop", NULL);
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
            gtk_label_set_text (GTK_LABEL (priv->comm_label), gl_journal_entry_get_command_line (entry));
        }
    }

    now = g_date_time_new_now_local ();
    str = gl_util_timestamp_to_display (gl_journal_entry_get_timestamp (entry), now,
                                        priv->clock_format, TRUE);
    g_date_time_unref (now);
    gtk_label_set_text (GTK_LABEL (priv->time_label), str);
    g_free (str);

    gtk_label_set_text (GTK_LABEL (priv->message_label), gl_journal_entry_get_message (entry));

    if (gl_journal_entry_get_audit_session (entry))
    {
        gtk_label_set_text (GTK_LABEL (priv->audit_label), gl_journal_entry_get_audit_session (entry));
        gtk_widget_show (priv->audit_field_label);
        gtk_widget_show (priv->audit_label);
    }

    if (gl_journal_entry_get_kernel_device (entry))
    {
        gtk_label_set_text (GTK_LABEL (priv->device_label), gl_journal_entry_get_kernel_device (entry));
        gtk_widget_show (priv->device_field_label);
        gtk_widget_show (priv->device_label);
    }

    /* TODO: Give a user-friendly representation of the priority. */
    str = g_strdup_printf ("%d", gl_journal_entry_get_priority (entry));
    gtk_label_set_text (GTK_LABEL (priv->priority_label), str);
    g_free (str);

    if (gl_journal_entry_get_catalog (entry) != NULL)
    {
        gint subject_count = 0;
        gint definedby_count = 0;
        gint support_count = 0;
        gint documentation_count = 0;

        str_copy = g_strdup (gl_journal_entry_get_catalog (entry));

        do
        {
            const gchar *label;

            if (subject_count == 0 && definedby_count == 0
                && support_count == 0 && documentation_count == 0)
            {
                str_field = strtok (str_copy, " ");
            }
            else
            {
                str_field = strtok (NULL, " ");
            }

            if (g_strcmp0 (str_field, "Subject:") == 0)
            {
                subject_count++;

                if (subject_count == 1)
                {
                    str_message = strtok (NULL, "\n");

                    if (str_message && *str_message)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->subject_label),
                                            str_message);
                        gtk_widget_show (priv->subject_field_label);
                        gtk_widget_show (priv->subject_label);
                    }
                }
                else
                {
                    str_field = strtok (NULL, "\n");
                    label = gtk_label_get_text (GTK_LABEL (priv->subject_label));
                    str = g_strconcat (label, "\n", str_field, NULL);

                    if (str && *str)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->subject_label),
                                            str);
                        gtk_widget_show (priv->subject_field_label);
                        gtk_widget_show (priv->subject_label);
                    }

                    g_free (str);
                }
            }
            else if (g_strcmp0 (str_field, "Defined-By:") == 0)
            {
                definedby_count++;

                if (subject_count == 1)
                {
                    str_message = strtok (NULL, "\n");

                    if (str_message && *str_message)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->definedby_label),
                                            str_message);
                        gtk_widget_show (priv->definedby_field_label);
                        gtk_widget_show (priv->definedby_label);
                    }
                }
                else
                {
                    str_field = strtok (NULL, "\n");
                    label = gtk_label_get_text (GTK_LABEL (priv->subject_label));
                    str = g_strconcat (label, "\n", str_field, NULL);

                    if (str && *str)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->definedby_label),
                                            str);
                        gtk_widget_show (priv->definedby_field_label);
                        gtk_widget_show (priv->definedby_label);
                    }

                    g_free (str);
                }
            }
            else if (g_strcmp0 (str_field, "Support:") == 0)
            {
                support_count++;

                if (support_count == 1)
                {
                    str_message = strtok (NULL, "\n");

                    if (str_message && *str_message)
                    {
                        gchar *str_link;

                        /* According to the spec, this should be a URI */
                        str_link = g_strdup_printf ("<a href=\"%s\">%s</a>",
                                                    str_message, str_message);

                        gtk_label_set_markup (GTK_LABEL (priv->support_label),
                                              str_link);
                        gtk_widget_show (priv->support_field_label);
                        gtk_widget_show (priv->support_label);

                        g_free (str_link);
                    }
                }
                else
                {
                    str_field = strtok (NULL, "\n");
                    label = gtk_label_get_text (GTK_LABEL (priv->subject_label));
                    str = g_strconcat (label, "\n", str_field, NULL);

                    if (str && *str)
                    {
                        gchar *str_link;

                        /* According to the spec, this should be a URI */
                        str_link = g_strdup_printf ("<a href=\"%s\">%s</a>",
                                                    str, str);

                        gtk_label_set_markup (GTK_LABEL (priv->support_label),
                                              str_link);
                        gtk_widget_show (priv->support_field_label);
                        gtk_widget_show (priv->support_label);

                        g_free (str_link);
                    }

                    g_free (str);
                }
            }
            else if (g_strcmp0 (str_field, "Documentation:") == 0)
            {
                documentation_count++;

                if (documentation_count == 1)
                {
                    str_message = strtok (NULL, "\n");

                    if (str_message && *str_message)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->documentation_label),
                                            str_message);
                        gtk_widget_show (priv->documentation_field_label);
                        gtk_widget_show (priv->documentation_label);
                    }
                }
                else
                {
                    str_field = strtok (NULL, "\n");
                    label = gtk_label_get_text (GTK_LABEL (priv->subject_label));
                    str = g_strconcat (label, "\n", str_field, NULL);

                    if (str && *str)
                    {
                        gtk_label_set_text (GTK_LABEL (priv->documentation_label),
                                            str);
                        gtk_widget_show (priv->documentation_field_label);
                        gtk_widget_show (priv->documentation_label);
                    }

                    g_free (str);
                }
            }
        } while (g_strcmp0 (str_field, "Subject:") == 0
                 || g_strcmp0 (str_field, "Defined-By:") == 0
                 || g_strcmp0 (str_field, "Support:") == 0
                 || g_strcmp0 (str_field, "Documentation:") == 0);

        str = strtok (NULL, "\0");
        str_field = g_strconcat (str_field, " ", str, NULL);

        if (str_field && *str_field)
        {
            gtk_label_set_text (GTK_LABEL (priv->detailed_message_label),
                                str_field);
            gtk_widget_show (priv->detailed_message_label);
        }

        g_free (str_field);
        g_free (str_copy);
    }
}

static void
gl_event_view_detail_finalize (GObject *object)
{
    GlEventViewDetail *detail = GL_EVENT_VIEW_DETAIL (object);
    GlEventViewDetailPrivate *priv = gl_event_view_detail_get_instance_private (detail);

    g_clear_object (&priv->entry);
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
        case PROP_ENTRY:
            g_value_set_object (value, priv->entry);
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
        case PROP_ENTRY:
            priv->entry = g_value_dup_object (value);
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

    obj_properties[PROP_ENTRY] = g_param_spec_object ("entry", "Entry",
                                                      "Row entry for this detailed view",
                                                      GL_TYPE_ROW_ENTRY,
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
                                                  subject_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  subject_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  definedby_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  definedby_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  support_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  support_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  documentation_field_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  documentation_label);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventViewDetail,
                                                  detailed_message_label);
}

static void
gl_event_view_detail_init (GlEventViewDetail *detail)
{
    gtk_widget_init_template (GTK_WIDGET (detail));
}

GtkWidget *
gl_event_view_detail_new (GlRowEntry *entry,
                          GlUtilClockFormat clock_format)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_DETAIL, "entry", entry,
                         "clock-format", clock_format, NULL);
}
