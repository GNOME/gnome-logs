/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2015  Lars Uebernickel <lars@uebernic.de>
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

#include "gl-journal-model.h"
#include "gl-journal.h"

/* Details of match fields */
typedef struct GlQueryItem
{
    gchar *field_name;
    gchar *field_value;
} GlQueryItem;

struct _GlJournalModel
{
    GObject parent_instance;

    guint batch_size;

    GlJournal *journal;
    GPtrArray *entries;

    GlQuery *query;

    guint n_entries_to_fetch;
    gboolean fetched_all;
    guint idle_source;
};

static void gl_journal_model_interface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GlJournalModel, gl_journal_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, gl_journal_model_interface_init))

enum
{
    PROP_0,
    PROP_LOADING,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

static gboolean
gl_journal_model_fetch_idle (gpointer user_data)
{
    GlJournalModel *model = user_data;
    GlJournalEntry *entry;
    guint last;

    g_assert (model->n_entries_to_fetch > 0);

    last = model->entries->len;
    if ((entry = gl_journal_previous (model->journal)))
    {
        model->n_entries_to_fetch--;
        g_ptr_array_add (model->entries, entry);
        g_list_model_items_changed (G_LIST_MODEL (model), last, 0, 1);
    }
    else
    {
        model->fetched_all = TRUE;
        model->n_entries_to_fetch = 0;
    }

    if (model->n_entries_to_fetch > 0)
    {
        return G_SOURCE_CONTINUE;
    }
    else
    {
        model->idle_source = 0;
        g_object_notify_by_pspec (G_OBJECT (model), properties[PROP_LOADING]);
        return G_SOURCE_REMOVE;
    }
}

static void
gl_journal_model_init (GlJournalModel *model)
{
    model->batch_size = 50;
    model->journal = gl_journal_new ();
    model->entries = g_ptr_array_new_with_free_func (g_object_unref);

    gl_journal_model_fetch_more_entries (model, FALSE);
}

static void
gl_journal_model_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    GlJournalModel *model = GL_JOURNAL_MODEL (object);

    switch (property_id)
    {
    case PROP_LOADING:
        g_value_set_boolean (value, gl_journal_model_get_loading (model));
        break;

    default:
        g_assert_not_reached ();
    }
}



static void
gl_journal_model_stop_idle (GlJournalModel *model)
{
    if (model->idle_source)
    {
        g_source_remove (model->idle_source);
        model->idle_source = 0;
        g_object_notify_by_pspec (G_OBJECT (model), properties[PROP_LOADING]);
    }
}

static void
gl_journal_model_dispose (GObject *object)
{
    GlJournalModel *model = GL_JOURNAL_MODEL (object);

    gl_journal_model_stop_idle (model);

    if (model->entries)
    {
        g_ptr_array_free (model->entries, TRUE);
        model->entries = NULL;
    }

    g_clear_object (&model->journal);

    G_OBJECT_CLASS (gl_journal_model_parent_class)->dispose (object);
}

static GType
gl_journal_model_get_item_type (GListModel *list)
{
  return GL_TYPE_JOURNAL_ENTRY;
}

static guint
gl_journal_model_get_n_items (GListModel *list)
{
  GlJournalModel *model = GL_JOURNAL_MODEL (list);

  return model->entries->len;
}

static gpointer
gl_journal_model_get_item (GListModel *list,
                           guint       position)
{
    GlJournalModel *model = GL_JOURNAL_MODEL (list);

    if (position < model->entries->len)
        return g_object_ref (g_ptr_array_index (model->entries, position));

    return NULL;
}

static void
gl_journal_model_class_init (GlJournalModelClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GParamFlags default_flags = G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;

    object_class->dispose = gl_journal_model_dispose;
    object_class->get_property = gl_journal_model_get_property;

    properties[PROP_LOADING] = g_param_spec_boolean ("loading", "", "", TRUE,
                                                     G_PARAM_READABLE | default_flags);

    g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
gl_journal_model_interface_init (GListModelInterface *iface)
{
    iface->get_item_type = gl_journal_model_get_item_type;
    iface->get_n_items = gl_journal_model_get_n_items;
    iface->get_item = gl_journal_model_get_item;
}

GlJournalModel *
gl_journal_model_new (void)
{
    return g_object_new (GL_TYPE_JOURNAL_MODEL, NULL);
}

/* Free the given @queryitem */
static void
gl_query_item_free (GlQueryItem *queryitem)
{
    g_free (queryitem->field_name);
    g_free (queryitem->field_value);

    g_slice_free (GlQueryItem, queryitem);
}

/* Free the given @query */
static void
gl_query_free (GlQuery *query)
{
    g_ptr_array_free (query->queryitems, TRUE);

    g_slice_free (GlQuery, query);
}

GlQuery *
gl_query_new (void)
{
    GlQuery *query;

    query = g_slice_new (GlQuery);

    query->queryitems = g_ptr_array_new_with_free_func ((GDestroyNotify) gl_query_item_free);

    return query;
}

static GlQueryItem *
gl_query_item_new (const gchar *field_name,
                   const gchar *field_value)
{
    GlQueryItem *queryitem;

    queryitem = g_slice_new (GlQueryItem);

    queryitem->field_name = g_strdup (field_name);
    queryitem->field_value = g_strdup (field_value);

    return queryitem;
}

static gchar *
gl_query_item_create_match_string (GlQueryItem *queryitem)
{
    gchar *str;

    if (queryitem->field_value)
    {
        str = g_strdup_printf ("%s=%s", queryitem->field_name, queryitem->field_value);
    }
    else
    {
        str = g_strdup (queryitem->field_name);
    }

    return str;
}

static void
get_exact_match_string (GlQueryItem *queryitem, GPtrArray *matches)
{
    gchar *match;

    match = gl_query_item_create_match_string (queryitem);

    g_ptr_array_add (matches, match);
}

/* Get exact matches from the query object */
static GPtrArray *
gl_query_get_exact_matches (GlQuery *query)
{
    GPtrArray *matches;

    matches = g_ptr_array_new_with_free_func ((GDestroyNotify) g_free);

    g_ptr_array_foreach (query->queryitems, (GFunc) get_exact_match_string, matches);

    /* Add NULL terminator to determine end of pointer array */
    g_ptr_array_add (matches, NULL);

    return matches;
}

/* Process the newly assigned query and repopulate the journal model */
static void
gl_journal_model_process_query (GlJournalModel *model)
{
    GPtrArray *category_matches;

    /* Set the exact matches first */
    category_matches = gl_query_get_exact_matches (model->query);

    gl_journal_set_matches (model->journal, (const gchar * const *) category_matches->pdata);

    /* Start re-population of the journal */
    gl_journal_model_fetch_more_entries (model, FALSE);

    /* Free array */
    g_ptr_array_free (category_matches, TRUE);
}

/**
 * gl_journal_model_take_query:
 * @model: a #GlJournalModel
 * @query: (transfer full) : query object populated from view
 *
 * Takes the query object from the view.
 * Clears the previous entries in the journal model.
 * Sets query object on the journal model and processes the newly set query.
 */
void
gl_journal_model_take_query (GlJournalModel *model,
                             GlQuery *query)
{
    g_return_if_fail (GL_JOURNAL_MODEL (model));

    gl_journal_model_stop_idle (model);
    model->fetched_all = FALSE;

    if (model->entries->len > 0)
    {
        g_list_model_items_changed (G_LIST_MODEL (model), 0, model->entries->len, 0);

        g_ptr_array_free (model->entries, TRUE);

        model->entries = g_ptr_array_new_with_free_func (g_object_unref);
    }

    /* Clear the previous query */
    if (model->query)
    {
        gl_query_free (model->query);
    }

    /* Set new query */
    model->query = query;

    /* Start processing the new query */
    gl_journal_model_process_query (model);
}

/* Add a new queryitem to query */
void
gl_query_add_match (GlQuery *query,
                    const gchar *field_name,
                    const gchar *field_value)
{
    GlQueryItem *queryitem;

    queryitem = gl_query_item_new (field_name, field_value);

    g_ptr_array_add (query->queryitems, queryitem);
}

gchar *
gl_journal_model_get_current_boot_time (GlJournalModel *model,
                                        const gchar *boot_match)
{
    return gl_journal_get_current_boot_time (model->journal, boot_match);
}

GArray *
gl_journal_model_get_boot_ids (GlJournalModel *model)
{
    return gl_journal_get_boot_ids (model->journal);
}

/**
 * gl_journal_model_get_loading:
 * @model: a #GlJournalModel
 *
 * Returns %TRUE if @model is currently loading entries from the
 * journal. That means that @model will grow in the near future.
 *
 * Returns: %TRUE if the model is loading entries from the journal
 */
gboolean
gl_journal_model_get_loading (GlJournalModel *model)
{
    g_return_val_if_fail (GL_IS_JOURNAL_MODEL (model), FALSE);

    return model->idle_source > 0;
}

/**
 * gl_journal_model_fetch_more_entries:
 * @model: a #GlJournalModel
 * @all: whether to fetch all available entries
 *
 * @model doesn't loads all entries at once, but in batches. This
 * function triggers it to load the next batch, or all remaining entries
 * if @all is %TRUE.
 */
void
gl_journal_model_fetch_more_entries (GlJournalModel *model,
                                     gboolean        all)
{
    g_return_if_fail (GL_IS_JOURNAL_MODEL (model));

    if (model->fetched_all)
      return;

    if (all)
        model->n_entries_to_fetch = G_MAXUINT32;
    else
        model->n_entries_to_fetch = model->batch_size;

    if (model->idle_source == 0)
    {
        model->idle_source = g_idle_add_full (G_PRIORITY_LOW, gl_journal_model_fetch_idle, model, NULL);
        g_object_notify_by_pspec (G_OBJECT (model), properties[PROP_LOADING]);
    }
}
