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

#include "gl-eventviewrow.h"

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <stdlib.h>

#include "gl-enums.h"

enum
{
    PROP_0,
    PROP_CATEGORY,
    PROP_CLOCK_FORMAT,
    PROP_ENTRY,
    N_PROPERTIES
};

struct _GlEventViewRow
{
    /*< private >*/
    GtkListBoxRow parent_instance;
};

typedef struct
{
    GlEventViewRowCategory category;
    GlUtilClockFormat clock_format;
    GlRowEntry *entry;
    GtkWidget *category_label;
    GtkWidget *message_label;
    GtkWidget *time_label;
} GlEventViewRowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventViewRow, gl_event_view_row, GTK_TYPE_LIST_BOX_ROW)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

const gchar *
gl_event_view_row_get_command_line (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;
    GlRowEntry *row_entry;
    GlJournalEntry *entry;

    priv = gl_event_view_row_get_instance_private (row);

    row_entry = priv->entry;
    entry = gl_row_entry_get_journal_entry (row_entry);

    return gl_journal_entry_get_command_line (entry);
}

guint64
gl_event_view_row_get_timestamp (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;
    GlRowEntry *row_entry;
    GlJournalEntry *entry;

    priv = gl_event_view_row_get_instance_private (row);

    row_entry = priv->entry;
    entry = gl_row_entry_get_journal_entry (row_entry);

    return gl_journal_entry_get_timestamp (entry);
}

const gchar *
gl_event_view_row_get_message (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;
    GlRowEntry *row_entry;
    GlJournalEntry *entry;

    priv = gl_event_view_row_get_instance_private (row);

    row_entry = priv->entry;
    entry = gl_row_entry_get_journal_entry (row_entry);

    return gl_journal_entry_get_message (entry);
}

GtkWidget *
gl_event_view_row_get_category_label (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);

    return priv->category_label;
}

GtkWidget *
gl_event_view_row_get_message_label (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);

    return priv->message_label;
}

GtkWidget *
gl_event_view_row_get_time_label (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);

    return priv->time_label;
}

static void
gl_event_view_row_finalize (GObject *object)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv = gl_event_view_row_get_instance_private (row);

    g_clear_object (&priv->entry);
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
        case PROP_CATEGORY:
            g_value_set_enum (value, priv->category);
            break;
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
gl_event_view_row_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv = gl_event_view_row_get_instance_private (row);

    switch (prop_id)
    {
        case PROP_CATEGORY:
            priv->category = g_value_get_enum (value);
            break;
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
gl_event_view_row_construct_category_label (GlEventViewRow *row,
                                            GlJournalEntry *entry)
{
    gint uid;
    GlEventViewRowPrivate *priv;
    const gchar *entry_uid_string;
    gint entry_uid;

    entry_uid_string = gl_journal_entry_get_uid (entry);
    entry_uid = entry_uid_string ? atoi (entry_uid_string) : -1;

    uid = gl_util_get_uid ();
    priv = gl_event_view_row_get_instance_private (row);

    /* The priority given to the categories should be determined by how
     * specific the checks are. The applications category is the most
     * specific, followed by the hardware category, then kernel, security
     * and finally the least-specific other category. So we check the category
     * in the order of applications, hardware, system, security and other. */
    if ((g_strcmp0 (gl_journal_entry_get_transport (entry), "kernel") == 0
         || g_strcmp0 (gl_journal_entry_get_transport (entry), "stdout") == 0
         || g_strcmp0 (gl_journal_entry_get_transport (entry), "syslog") == 0)
         && entry_uid == uid)
    {
        priv->category_label = gtk_label_new (_("Applications"));
    }
    else if (g_strcmp0 (gl_journal_entry_get_transport (entry), "kernel") == 0
             && gl_journal_entry_get_kernel_device (entry) != NULL)
    {
        priv->category_label = gtk_label_new (_("Hardware"));
    }
    else if (g_strcmp0 (gl_journal_entry_get_transport (entry), "kernel") == 0)
    {
        priv->category_label = gtk_label_new (_("System"));
    }
    else if (gl_journal_entry_get_audit_session (entry) != NULL)
    {
        priv->category_label = gtk_label_new (_("Security"));
    }
    else
    {
        priv->category_label = gtk_label_new (_("Other"));
    }
}

static gchar *
gl_event_view_row_replace_newline (const gchar *message)
{
    GString *newmessage;
    const gchar *newline_index;
    const gchar *prevpos;

    newmessage = g_string_sized_new (strlen (message));
    prevpos = message;

    newline_index = strchr (message, '\n');

    while (newline_index != NULL)
    {
        g_string_append_len (newmessage, prevpos, newline_index - prevpos);
        g_string_append_unichar (newmessage, 0x2424);

        prevpos = newline_index + 1;
        newline_index = strchr (prevpos, '\n');
    }

    g_string_append (newmessage, prevpos);

    return g_string_free (newmessage, FALSE);
}

static void
gl_event_view_row_constructed (GObject *object)
{
    GtkStyleContext *context;
    GtkWidget *grid;
    gchar *time;
    const gchar *message;
    gchar *newline_index;
    gboolean rtl;
    GlEventViewRowCategory category;
    GlUtilClockFormat tmp_clock_format;
    GlRowEntry *tmp_entry;
    GlJournalEntry *entry;
    GlRowEntry *row_entry;
    GDateTime *now;
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (object);
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);
    row_entry = priv->entry;
    entry = gl_row_entry_get_journal_entry (row_entry);

    rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

    context = gtk_widget_get_style_context (GTK_WIDGET (row));
    gtk_style_context_add_class (context, "event");
    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
    gtk_container_add (GTK_CONTAINER (row), grid);

    g_object_get (object,
                  "category", &category,
                  "clock-format", &tmp_clock_format,
                  "entry", &tmp_entry,
                  NULL);

    if (category == GL_EVENT_VIEW_ROW_CATEGORY_IMPORTANT)
    {
        gl_event_view_row_construct_category_label (row, entry);

        context = gtk_widget_get_style_context (GTK_WIDGET (priv->category_label));
        gtk_style_context_add_class (context, "dim-label");
        gtk_style_context_add_class (context, "event-monospace");
        gtk_label_set_xalign (GTK_LABEL (priv->category_label), 0);
        gtk_grid_attach (GTK_GRID (grid), priv->category_label,
                         rtl ? 2 : 0, 0, 1, 1);
    }

    message = gl_journal_entry_get_message (entry);

    newline_index = strchr (message, '\n');

    if (newline_index)
    {
        gchar *message_label;

        message_label = gl_event_view_row_replace_newline (message);

        priv->message_label = gtk_label_new (message_label);

        g_free (message_label);
    }
    else
    {
        priv->message_label = gtk_label_new (message);
    }

    gtk_widget_set_direction (priv->message_label, GTK_TEXT_DIR_LTR);
    context = gtk_widget_get_style_context (GTK_WIDGET (priv->message_label));
    gtk_style_context_add_class (context, "event-monospace");
    gtk_widget_set_halign (priv->message_label, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (priv->message_label),
                             PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign (GTK_LABEL (priv->message_label), 0);
    gtk_label_set_single_line_mode (GTK_LABEL (priv->message_label), TRUE);

    if (gl_row_entry_get_row_type (row_entry) == GL_ROW_ENTRY_TYPE_HEADER)
    {
        guint compressed_entries;
        GtkWidget *message_box;
        GtkWidget *compressed_entries_label;
        gchar *compressed_entries_string;

        message_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);

        gtk_box_pack_start (GTK_BOX (message_box),
                            priv->message_label,
                            TRUE, TRUE, 2);

        compressed_entries = gl_row_entry_get_compressed_entries (row_entry);
        compressed_entries_string = g_strdup_printf ("%d", compressed_entries);

        compressed_entries_label = gtk_label_new (compressed_entries_string);

        gtk_widget_set_direction (compressed_entries_label, GTK_TEXT_DIR_LTR);
        context = gtk_widget_get_style_context (GTK_WIDGET (compressed_entries_label));
        gtk_style_context_add_class (context, "compressed-entries-label");
        gtk_widget_set_halign (compressed_entries_label, GTK_ALIGN_START);
        gtk_label_set_xalign (GTK_LABEL (compressed_entries_label), 0);

        gtk_box_pack_start (GTK_BOX (message_box),
                            compressed_entries_label,
                            TRUE, TRUE, 0);

        gtk_grid_attach (GTK_GRID (grid), message_box,
                         1, 0, 1, 1);

        g_free (compressed_entries_string);
    }
    else
    {
        gtk_grid_attach (GTK_GRID (grid), priv->message_label,
                         1, 0, 1, 1);
    }

    now = g_date_time_new_now_local ();
    time = gl_util_timestamp_to_display (gl_journal_entry_get_timestamp (entry),
                                         now, priv->clock_format, FALSE);
    g_date_time_unref (now);
    priv->time_label = gtk_label_new (time);
    context = gtk_widget_get_style_context (GTK_WIDGET (priv->time_label));
    gtk_style_context_add_class (context, "dim-label");
    gtk_style_context_add_class (context, "event-monospace");
    gtk_style_context_add_class (context, "event-time");
    gtk_widget_set_halign (priv->time_label, GTK_ALIGN_END);
    gtk_widget_set_hexpand (priv->time_label, TRUE);
    gtk_label_set_xalign (GTK_LABEL (priv->time_label), 1);
    gtk_grid_attach (GTK_GRID (grid), priv->time_label, rtl ? 0 : 2, 0, 1, 1);

    g_free (time);
    g_object_unref (tmp_entry);

    gtk_widget_set_tooltip_text (GTK_WIDGET (row),
                                 gl_journal_entry_get_message (entry));

    gtk_widget_show_all (GTK_WIDGET (row));

    G_OBJECT_CLASS (gl_event_view_row_parent_class)->constructed (object);
}

/* Hide the rows to be compressed */
static void
on_parent_set (GtkWidget *widget,
               GtkWidget *old_parent,
               gpointer   user_data)
{
    GlEventViewRow *row = GL_EVENT_VIEW_ROW (user_data);
    GlEventViewRowPrivate *priv;

    priv = gl_event_view_row_get_instance_private (row);

    /* Execute only if the parent was not set earlier at all */
    if (old_parent == NULL &&
        gl_row_entry_get_row_type (priv->entry) == GL_ROW_ENTRY_TYPE_COMPRESSED)
    {
        gtk_widget_hide (widget);
    }
}

static void
gl_event_view_row_class_init (GlEventViewRowClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructed = gl_event_view_row_constructed;
    gobject_class->finalize = gl_event_view_row_finalize;
    gobject_class->get_property = gl_event_view_row_get_property;
    gobject_class->set_property = gl_event_view_row_set_property;

    obj_properties[PROP_CATEGORY] = g_param_spec_enum ("category", "Category",
                                                       "Filter rows from important category",
                                                       GL_TYPE_EVENT_VIEW_ROW_CATEGORY,
                                                       GL_EVENT_VIEW_ROW_CATEGORY_NONE,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_CLOCK_FORMAT] = g_param_spec_enum ("clock-format", "Clock format",
                                                           "Format of the clock in which to show timestamps",
                                                           GL_TYPE_UTIL_CLOCK_FORMAT,
                                                           GL_UTIL_CLOCK_FORMAT_24HR,
                                                           G_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT_ONLY |
                                                           G_PARAM_STATIC_STRINGS);

    obj_properties[PROP_ENTRY] = g_param_spec_object ("entry", "Entry",
                                                      "Row entry for this row",
                                                      GL_TYPE_ROW_ENTRY,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);
}

static void
gl_event_view_row_init (GlEventViewRow *row)
{
    /* The widgets are initialized in gl_event_view_row_constructed (), because
     * at _init() time the construct-only properties have not been set. */

    g_signal_connect (row, "parent-set",
                      G_CALLBACK (on_parent_set), row);
}

GlRowEntry *
gl_event_view_row_get_entry (GlEventViewRow *row)
{
    GlEventViewRowPrivate *priv;

    g_return_val_if_fail (GL_EVENT_VIEW_ROW (row), NULL);

    priv = gl_event_view_row_get_instance_private (row);

    return priv->entry;
}

GtkWidget *
gl_event_view_row_new (GlRowEntry *entry,
                       GlUtilClockFormat clock_format,
                       GlEventViewRowCategory category)
{
    return g_object_new (GL_TYPE_EVENT_VIEW_ROW, "entry", entry,
                         "clock-format", clock_format,
                         "category", category, NULL);
}
