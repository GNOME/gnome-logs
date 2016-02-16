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

enum
{
    PROP_0,
    PROP_MODE,
    N_PROPERTIES
};

struct _GlEventToolbar
{
    /*< private >*/
    GtkHeaderBar parent_instance;
};

typedef struct
{
    GtkWidget *current_boot;
    GtkWidget *back_button;
    GtkWidget *output_button;
    GtkWidget *menu_button;
    GtkWidget *search_button;
    GlEventToolbarMode mode;
} GlEventToolbarPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlEventToolbar, gl_event_toolbar, GTK_TYPE_HEADER_BAR)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

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
on_gl_event_toolbar_back_button_clicked (GlEventToolbar *toolbar,
                                         GtkButton *button)
{
    gl_event_toolbar_set_mode (toolbar, GL_EVENT_TOOLBAR_MODE_LIST);
}

static void
on_notify_mode (GlEventToolbar *toolbar,
                GParamSpec *pspec,
                gpointer user_data)
{
    GlEventToolbarPrivate *priv;
    GtkWidget *toplevel;

    priv = gl_event_toolbar_get_instance_private (toolbar);

    switch (priv->mode)
    {
        case GL_EVENT_TOOLBAR_MODE_LIST:
            gtk_widget_hide (priv->back_button);
            gtk_widget_show (priv->output_button);
            gtk_widget_show (priv->menu_button);
            gtk_widget_show (priv->search_button);
            break;
        case GL_EVENT_TOOLBAR_MODE_DETAIL:
            gtk_widget_show (priv->back_button);
            gtk_widget_hide (priv->output_button);
            gtk_widget_hide (priv->menu_button);
            gtk_widget_hide (priv->search_button);
            break;
        default:
            g_assert_not_reached ();
            break;
    }

    /* Propagate change to GlWindow. */
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (toolbar));

    if (gtk_widget_is_toplevel (toplevel))
    {
        GAction *mode;
        GEnumClass *eclass;
        GEnumValue *evalue;

        mode = g_action_map_lookup_action (G_ACTION_MAP (toplevel),
                                           "toolbar-mode");
        eclass = g_type_class_ref (GL_TYPE_EVENT_TOOLBAR_MODE);
        evalue = g_enum_get_value (eclass, priv->mode);

        g_action_activate (mode, g_variant_new_string (evalue->value_nick));

        g_type_class_unref (eclass);
    }
    else
    {
        g_error ("Widget not in toplevel window, not switching toolbar mode");
    }
}

static void
gl_event_toolbar_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    GlEventToolbar *toolbar = GL_EVENT_TOOLBAR (object);
    GlEventToolbarPrivate *priv = gl_event_toolbar_get_instance_private (toolbar);

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
gl_event_toolbar_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    GlEventToolbar *view = GL_EVENT_TOOLBAR (object);
    GlEventToolbarPrivate *priv = gl_event_toolbar_get_instance_private (view);

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
gl_event_toolbar_class_init (GlEventToolbarClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gobject_class->get_property = gl_event_toolbar_get_property;
    gobject_class->set_property = gl_event_toolbar_set_property;

    obj_properties[PROP_MODE] = g_param_spec_enum ("mode", "Mode",
                                                   "Mode to determine which buttons to show",
                                                   GL_TYPE_EVENT_TOOLBAR_MODE,
                                                   GL_EVENT_TOOLBAR_MODE_LIST,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (gobject_class, N_PROPERTIES,
                                       obj_properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/Logs/gl-eventtoolbar.ui");
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  output_button);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  back_button);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  menu_button);
    gtk_widget_class_bind_template_child_private (widget_class, GlEventToolbar,
                                                  search_button);

    gtk_widget_class_bind_template_callback (widget_class,
                                             on_gl_event_toolbar_back_button_clicked);
}

static void
gl_event_toolbar_init (GlEventToolbar *toolbar)
{
    GlEventToolbarPrivate *priv;

    priv = gl_event_toolbar_get_instance_private (toolbar);

    gtk_widget_init_template (GTK_WIDGET (toolbar));

    priv->current_boot = gtk_label_new (NULL);

    g_signal_connect (toolbar, "notify::mode", G_CALLBACK (on_notify_mode),
                      NULL);
}

gboolean
gl_event_toolbar_handle_back_button_event (GlEventToolbar *toolbar,
                                           GdkEventKey *event)
{
    GlEventToolbarPrivate *priv;
    GdkModifierType state;
    GdkKeymap *keymap;
    gboolean is_rtl;

    g_return_val_if_fail (toolbar != NULL && event != NULL,
                          GDK_EVENT_PROPAGATE);

    priv = gl_event_toolbar_get_instance_private (toolbar);

    state = event->state;
    keymap = gdk_keymap_get_default ();
    gdk_keymap_add_virtual_modifiers (keymap, &state);
    state = state & gtk_accelerator_get_default_mod_mask ();
    is_rtl = gtk_widget_get_direction (priv->back_button) == GTK_TEXT_DIR_RTL;

    if ((!is_rtl && state == GDK_MOD1_MASK && event->keyval == GDK_KEY_Left)
        || (is_rtl && state == GDK_MOD1_MASK && event->keyval == GDK_KEY_Right)
        || event->keyval == GDK_KEY_Back)
    {
        return GDK_EVENT_STOP;
    }
    else
    {
        return GDK_EVENT_PROPAGATE;
    }
}

void
gl_event_toolbar_set_mode (GlEventToolbar *toolbar, GlEventToolbarMode mode)
{
    GlEventToolbarPrivate *priv;

    g_return_if_fail (GL_EVENT_TOOLBAR (toolbar));

    priv = gl_event_toolbar_get_instance_private (toolbar);

    if (priv->mode != mode)
    {
        priv->mode = mode;
        g_object_notify_by_pspec (G_OBJECT (toolbar),
                                  obj_properties[PROP_MODE]);
    }
}

GtkWidget *
gl_event_toolbar_new (void)
{
    return g_object_new (GL_TYPE_EVENT_TOOLBAR, NULL);
}
