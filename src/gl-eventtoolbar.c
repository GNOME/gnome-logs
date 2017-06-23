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

#include "config.h"
#include "gl-eventtoolbar.h"

#include <glib/gi18n.h>

#include "gl-enums.h"
#include "gl-eventviewlist.h"
#include "gl-util.h"

struct _GlEventToolbar
{
    /*< private >*/
    GtkHeaderBar parent_instance;
};

typedef struct
{
    GtkWidget *current_boot;
    GtkWidget *output_button;
    GtkWidget *menu_button;
    GtkWidget *search_button;
    GlEventToolbarMode mode;
} GlEventToolbarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventToolbar, gl_event_toolbar, GTK_TYPE_HEADER_BAR)

void
gl_event_toolbar_change_current_boot (GlEventToolbar *toolbar,
                                      const gchar *current_boot)
{
    GlEventToolbarPrivate *priv;

    priv = gl_event_toolbar_get_instance_private (toolbar);

    /* set text to priv->current_boot */
    gtk_label_set_text (GTK_LABEL (priv->current_boot), current_boot);
}

void
gl_event_toolbar_add_boots (GlEventToolbar *toolbar,
                           GArray *boot_ids)
{
    GtkWidget *grid;
    GtkWidget *title_label;
    GtkWidget *arrow;
    GMenu *boot_menu;
    GMenu *section;
    GlEventToolbarPrivate *priv;
    GtkStyleContext *context;
    gint i;
    gchar *current_boot = NULL;

    priv = gl_event_toolbar_get_instance_private (toolbar);

    boot_menu = g_menu_new ();
    section = g_menu_new ();

    for (i = MAX (boot_ids->len, 5) - 5; i < boot_ids->len; i++)
    {
        gchar *boot_match;
        gchar *time_display;
        GlJournalBootID *boot_id;
        GMenuItem *item;
        GVariant *variant;

        boot_id = &g_array_index (boot_ids, GlJournalBootID, i);
        boot_match = boot_id->boot_match;

        time_display = gl_util_boot_time_to_display (boot_id->realtime_first,
                                                     boot_id->realtime_last);
        if (i == boot_ids->len - 1)
        {
            current_boot = g_strdup (time_display);
        }

        item = g_menu_item_new (time_display, NULL);
        variant = g_variant_new_string (boot_match);
        g_menu_item_set_action_and_target_value (item, "win.view-boot",
                                                 variant);
        g_menu_prepend_item (section, item);

        g_free (time_display);
        g_object_unref (item);
    }

    /* Translators: Boot refers to a single run (or bootup) of the system */
    g_menu_prepend_section (boot_menu, _("Boot"), G_MENU_MODEL (section));

    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (priv->menu_button),
                                    G_MENU_MODEL (boot_menu));

    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
    gtk_container_add (GTK_CONTAINER (priv->menu_button), grid);

    title_label = gtk_label_new (_("Logs"));
    context = gtk_widget_get_style_context (GTK_WIDGET (title_label));
    gtk_style_context_add_class (context, "title");
    gtk_grid_attach (GTK_GRID (grid), title_label, 0, 0, 1, 1);

    gtk_label_set_text (GTK_LABEL (priv->current_boot), current_boot);
    context = gtk_widget_get_style_context (GTK_WIDGET (priv->current_boot));
    gtk_style_context_add_class (context, "subtitle");
    gtk_grid_attach (GTK_GRID (grid), priv->current_boot, 0, 1, 1, 1);

    arrow = gtk_image_new_from_icon_name ("pan-down-symbolic",
                                          GTK_ICON_SIZE_BUTTON);
    gtk_grid_attach (GTK_GRID (grid), arrow, 1, 0, 1, 2);
    gtk_widget_show_all (grid);

    gtk_header_bar_set_custom_title (GTK_HEADER_BAR (toolbar),
                                     priv->menu_button);

    g_free (current_boot);
}

static void
gl_event_toolbar_class_init (GlEventToolbarClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-eventtoolbar.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  output_button);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  menu_button);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  search_button);
}

static void
gl_event_toolbar_init (GlEventToolbar *toolbar)
{
    GlEventToolbarPrivate *priv;

    priv = gl_event_toolbar_get_instance_private (toolbar);

    gtk_widget_init_template (GTK_WIDGET (toolbar));

    priv->current_boot = gtk_label_new (NULL);
}

GtkWidget *
gl_event_toolbar_new (void)
{
    return g_object_new (GL_TYPE_EVENT_TOOLBAR, NULL);
}
