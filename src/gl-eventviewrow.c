/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2013, 2014  Red Hat, Inc.
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

#include "gl-eventviewrow.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>

#include "gl-enums.h"

enum
{
    PROP_0,
    PROP_CLOCK_FORMAT,
    PROP_RESULT,
    PROP_STYLE,
    N_PROPERTIES
};

typedef struct
{
    GlUtilClockFormat clock_format;
    GlJournalResult *result;
    GlEventViewRowStyle style;
} GlEventViewRowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewRow, gl_event_view_row, GTK_TYPE_LIST_BOX_ROW)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gl_event_view_row_finalize (GObject *object)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv = gl_event_view_row_get_instance_private (row);

    g_clear_pointer (&priv->result, gl_journal_result_unref);
}

static void
gl_event_view_row_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv = gl_event_view_row_get_instance_private (row);

    switch (prop_id)
    {
        case PROP_CLOCK_FORMAT:
            g_value_set_enum (value, priv->clock_format);
            break;
        case PROP_RESULT:
            g_value_set_boxed (value, priv->result);
            break;
        case PROP_STYLE:
            g_value_set_enum (value, priv->style);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_row_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv = gl_event_view_row_get_instance_private (row);

    switch (prop_id)
    {
        case PROP_CLOCK_FORMAT:
            priv->clock_format = g_value_get_enum (value);
            break;
        case PROP_RESULT:
            priv->result = g_value_dup_boxed (value);
            break;
        case PROP_STYLE:
            priv->style = g_value_get_enum (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gl_event_view_row_create_cmdline (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;
    GtkStyleContext *context;
    GtkWidget *grid;
    gchar *markup;
    GtkWidget *label;
    gchar *time;
    gboolean rtl;
    GtkWidget *image;
    GlJournalResult *result;
    GDateTime *now;

    priv = gl_event_view_row_get_instance_private (row);
    result = priv->result;

    rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

    context = gtk_widget_get_style_context (GTK_WIDGET (row));
    gtk_style_context_add_class (context, "event");
    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
    gtk_container_add (GTK_CONTAINER (row), grid);

    markup = g_markup_printf_escaped ("<b>%s</b>",
                                      result->comm ? result->comm : "");
    label = gtk_label_new (NULL);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    gtk_widget_set_hexpand (label, TRUE);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_label_set_markup (GTK_LABEL (label), markup);
    g_free (markup);
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

    label = gtk_label_new (result->message);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    context = gtk_widget_get_style_context (GTK_WIDGET (label));
    gtk_style_context_add_class (context, "event-monospace");
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);

    now = g_date_time_new_now_local ();
    time = gl_util_timestamp_to_display (result->timestamp, now,
                                         priv->clock_format);
    g_date_time_unref (now);
    label = gtk_label_new (time);
    context = gtk_widget_get_style_context (GTK_WIDGET (label));
    gtk_style_context_add_class (context, "dim-label");
    gtk_style_context_add_class (context, "event-time");
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 0 : 1, 0, 1, 1);

    image = gtk_image_new_from_icon_name ("go-next-symbolic",
                                          GTK_ICON_SIZE_MENU);
    gtk_grid_attach (GTK_GRID (grid), image, 2, 0, 1, 2);

    g_free (time);
}

static void
gl_event_view_row_create_simple (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;
    GtkStyleContext *context;
    GtkWidget *grid;
    GtkWidget *label;
    gchar *time;
    gboolean rtl;
    GtkWidget *image;
    GlJournalResult *result;
    GDateTime *now;

    priv = gl_event_view_row_get_instance_private (row);
    result = priv->result;

    rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

    context = gtk_widget_get_style_context (GTK_WIDGET (row));
    gtk_style_context_add_class (context, "event");
    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
    gtk_container_add (GTK_CONTAINER (row), grid);

    label = gtk_label_new (result->message);
    gtk_widget_set_direction (label, GTK_TEXT_DIR_LTR);
    context = gtk_widget_get_style_context (GTK_WIDGET (label));
    gtk_style_context_add_class (context, "event-monospace");
    gtk_widget_set_hexpand (label, TRUE);
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

    now = g_date_time_new_now_local ();
    time = gl_util_timestamp_to_display (result->timestamp, now,
                                         priv->clock_format);
    g_date_time_unref (now);
    label = gtk_label_new (time);
    context = gtk_widget_get_style_context (GTK_WIDGET (label));
    gtk_style_context_add_class (context, "dim-label");
    gtk_style_context_add_class (context, "event-time");
    gtk_widget_set_halign (label, GTK_ALIGN_END);
    gtk_grid_attach (GTK_GRID (grid), label, rtl ? 1 : 0, 0, 1, 1);

    image = gtk_image_new_from_icon_name ("go-next-symbolic",
                                          GTK_ICON_SIZE_MENU);
    gtk_grid_attach (GTK_GRID (grid), image, 1, 0, 1, 2);

    g_free (time);
}

static void
gl_event_view_row_constructed (GObject *object)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);

    /* contruct-only properties have already been set. */
    switch (priv->style)
    {
        case GL_EVENT_VIEW_ROW_STYLE_CMDLINE:
            gl_event_view_row_create_cmdline (row);
            break;
        case GL_EVENT_VIEW_ROW_STYLE_SIMPLE:
            gl_event_view_row_create_simple (row);
            break;
        default:
            g_assert_not_reached ();
    }

    G_OBJECT_CLASS (gl_event_view_row_parent_class)->constructed (object);
}

static void
gl_event_view_row_class_init (GlEventViewRowClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructed = gl_event_view_row_constructed;
    gobject_class->finalize = gl_event_view_row_finalize;
    gobject_class->get_property = gl_event_view_row_get_property;
    gobject_class->set_property = gl_event_view_row_set_property;

    obj_properties[PROP_CLOCK_FORMAT] = g_param_spec_enum ("clock-format", "Clock format",
                                                           "Format of the clock in which to show timestamps",
                                                           GL_TYPE_UTIL_CLOCK_FORMAT,
                                                           GL_UTIL_CLOCK_FORMAT_24HR,
                                                           G_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT_ONLY |
                                                           G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_RESULT] = g_param_spec_boxed ("result", "Result",
                                                      "Journal query result for this row",
                                                      GL_TYPE_JOURNAL_RESULT,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_STYLE] = g_param_spec_enum ("style", "Style",
                                                    "Row display style",
                                                    GL_TYPE_EVENT_VIEW_ROW_STYLE,
                                                    GL_EVENT_VIEW_ROW_STYLE_CMDLINE,
                                                    G_PARAM_READWRITE |
                                                    G_PARAM_CONSTRUCT_ONLY |
                                                    G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);
}

static void
gl_event_view_row_init (GlEventViewRow *row)
{
    /* See gl_event_view_row_constructed (). */
}

GlJournalResult *
gl_event_view_row_get_result (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;

    g_return_val_if_fail (GL_EVENT_VIEW_ROW (row), NULL);

    priv = gl_event_view_row_get_instance_private (row);

    return priv->result;
}

GtkWidget *
gl_event_view_row_new (GlJournalResult *result,
                       GlEventViewRowStyle style,
                       GlUtilClockFormat clock_format)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_ROW, "style", style, "result",
                         result, "clock-format", clock_format, NULL);
}
