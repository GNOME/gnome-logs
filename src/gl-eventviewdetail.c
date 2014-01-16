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
} GlEventViewDetailPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewDetail, gl_event_view_detail, GTK_TYPE_BIN)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static GtkWidget *
create_widget_for_comm (const gchar *comm)
{
    if (comm && *comm)
    {
        /* Command-line, look for a desktop file. */
        gchar *str;
        GDesktopAppInfo *desktop;

        /* TODO: Use g_desktop_app_info_search? */
        str = g_strconcat (comm, ".desktop", NULL);
        desktop = g_desktop_app_info_new (str);
        g_free (str);

        if (desktop)
        {
            GtkWidget *box;
            GIcon *icon;
            GtkWidget *image;
            GtkWidget *label;
            GtkStyleContext *context;

            box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

            icon = g_app_info_get_icon (G_APP_INFO (desktop));
            image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
            gtk_box_pack_end (GTK_BOX (box), image, TRUE, TRUE, 0);

            label = gtk_label_new (g_app_info_get_name (G_APP_INFO (desktop)));
            gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
            gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
            context = gtk_widget_get_style_context (label);
            gtk_style_context_add_class (context, "detail-comm");
            gtk_box_pack_end (GTK_BOX (box), label, TRUE, TRUE, 0);

            g_object_unref (desktop);

            return box;
        }
        else
        {
            GtkWidget *label;
            GtkStyleContext *context;

            label = gtk_label_new (comm);
            gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
            gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
            context = gtk_widget_get_style_context (label);
            gtk_style_context_add_class (context, "detail-comm");

            return label;
        }
    }
    else
    {
        /* No command-line. */
        return gtk_label_new (NULL);
    }
}

static void
gl_event_view_detail_create_detail (GlEventViewDetail *detail)
{
    GlEventViewDetailPrivate *priv;
    GlJournalResult *result;
    gchar *str;
    gboolean rtl;
    GtkWidget *box;
    GtkWidget *description;
    GtkWidget *grid;
    GtkWidget *label;
    GtkStyleContext *context;

    priv = gl_event_view_detail_get_instance_private (detail);

    result = priv->result;

    rtl = gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL;

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    description = create_widget_for_comm (result->comm);
    gtk_box_pack_start (GTK_BOX (box), description, TRUE, TRUE, 0);

    str = gl_util_timestamp_to_display (result->timestamp, priv->clock_format);
    label = gtk_label_new (str);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-time");
    gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
    g_free (str);

    grid = gtk_grid_new ();
    gtk_grid_attach (GTK_GRID (grid), box, 0, 0, 2, 1);

    label = gtk_label_new (_("Message"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-field-label");
    gtk_style_context_add_class (context, "dim-label");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 1, 1, 1);

    label = gtk_label_new (result->message);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    /* The message label expands, so that the field label column is as narrow
     * as possible. */
    gtk_widget_set_hexpand (label, TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-message");
    gtk_style_context_add_class (context, "event-monospace");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 1, 1, 1);

    /* TODO: Give a user-friendly representation of the priority. */
    label = gtk_label_new (_("Priority"));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-field-label");
    gtk_style_context_add_class (context, "dim-label");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 2, 1, 1);

    str = g_strdup_printf ("%d", result->priority);
    label = gtk_label_new (str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-priority");
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 2, 1, 1);
    g_free (str);

    label = gtk_label_new (result->catalog);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_selectable (GTK_LABEL (label), TRUE);
    context = gtk_widget_get_style_context (label);
    gtk_style_context_add_class (context, "detail-catalog");
    gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 2, 1);

    if (result->kernel_device && *result->kernel_device)
    {
        gtk_grid_insert_row (GTK_GRID (grid), 2);

        label = gtk_label_new (_("Kernel Device"));
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        context = gtk_widget_get_style_context (label);
        gtk_style_context_add_class (context, "detail-field-label");
        gtk_style_context_add_class (context, "dim-label");
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 2, 1, 1);

        label = gtk_label_new (result->kernel_device);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_selectable (GTK_LABEL (label), TRUE);
        context = gtk_widget_get_style_context (label);
        gtk_style_context_add_class (context, "detail-kernel_device");
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 2, 1, 1);
    }

    if (result->audit_session && *result->audit_session)
    {
        gtk_grid_insert_row (GTK_GRID (grid), 2);

        label = gtk_label_new (_("Audit Session"));
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        context = gtk_widget_get_style_context (label);
        gtk_style_context_add_class (context, "detail-field-label");
        gtk_style_context_add_class (context, "dim-label");
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 2, 1, 1);

        label = gtk_label_new (result->audit_session);
        gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_selectable (GTK_LABEL (label), TRUE);
        context = gtk_widget_get_style_context (label);
        gtk_style_context_add_class (context, "detail-kernel_device");
        gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 2, 1, 1);
    }

    gtk_widget_show_all (grid);

    gtk_container_add (GTK_CONTAINER (detail), grid);
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
}

static void
gl_event_view_detail_init (GlEventViewDetail *detail)
{
    /* See gl_event_view_detail_constructed (). */
}

GtkWidget *
gl_event_view_detail_new (GlJournalResult *result,
                          GlUtilClockFormat clock_format)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_DETAIL, "result", result,
                         "clock-format", clock_format, NULL);
}
