
#include "gl-journal-model.h"
#include "gl-journal.h"

struct _GlJournalModel
{
    GObject parent_instance;

    GlJournal *journal;
    GPtrArray *entries;

    guint idle_source;
};

static void gl_journal_model_interface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GlJournalModel, gl_journal_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, gl_journal_model_interface_init))

enum
{
    PROP_0,
    PROP_MATCHES,
    PROP_LOADING,
    N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

static gboolean
gl_journal_model_fetch_entries (gpointer user_data)
{
    GlJournalModel *model = user_data;
    guint last;
    gint i;

    last = model->entries->len;
    for (i = 0; i < 5; i++)
    {
        GlJournalEntry *entry;

        entry = gl_journal_previous (model->journal);
        if (entry)
        {
            g_ptr_array_add (model->entries, entry);
        }
        else
        {
            model->idle_source = 0;
            g_object_notify_by_pspec (G_OBJECT (model), properties[PROP_LOADING]);
            return G_SOURCE_REMOVE;
        }
    }

    g_list_model_items_changed (G_LIST_MODEL (model), last, 0, i);
    return G_SOURCE_CONTINUE;
}

static void
gl_journal_model_init (GlJournalModel *model)
{
    model->journal = gl_journal_new ();
    model->entries = g_ptr_array_new_with_free_func (g_object_unref);
    model->idle_source = g_idle_add_full (G_PRIORITY_LOW, gl_journal_model_fetch_entries, model, NULL);
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
gl_journal_model_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    GlJournalModel *model = GL_JOURNAL_MODEL (object);

    switch (property_id)
    {
    case PROP_MATCHES:
        gl_journal_model_set_matches (model, g_value_get_boxed (value));
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
    object_class->set_property = gl_journal_model_set_property;

    properties[PROP_MATCHES] = g_param_spec_boxed ("matches", "", "", G_TYPE_STRV,
                                                   G_PARAM_WRITABLE | default_flags);

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

/**
 * gl_journal_model_set_matches:
 * @model: a #GlJournalModel
 * @matches: new matches
 *
 * Changes @model's filter matches to @matches. This resets all items in
 * the model, as they have to be requeried from the journal.
 */
void
gl_journal_model_set_matches (GlJournalModel      *model,
                              const gchar * const *matches)
{
    g_return_if_fail (GL_IS_JOURNAL_MODEL (model));
    g_return_if_fail (matches != NULL);

    gl_journal_model_stop_idle (model);
    if (model->entries->len > 0)
    {
        g_list_model_items_changed (G_LIST_MODEL (model), 0, model->entries->len, 0);
        g_ptr_array_remove_range (model->entries, 0, model->entries->len);
    }

    gl_journal_set_matches (model->journal, matches);

    model->idle_source = g_idle_add_full (G_PRIORITY_LOW, gl_journal_model_fetch_entries, model, NULL);
    g_object_notify_by_pspec (G_OBJECT (model), properties[PROP_LOADING]);
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
