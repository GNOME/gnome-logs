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
    GlQuerySearchType search_type;
} GlQueryItem;

struct _GlJournalModel
{
    GObject parent_instance;

    guint batch_size;

    GlJournal *journal;
    GPtrArray *entries;

    GlQuery *query;
    GPtrArray *token_array;

    guint n_entries_to_fetch;
    gboolean fetched_all;
    guint idle_source;
};

static void gl_journal_model_interface_init (GListModelInterface *iface);
static GPtrArray *tokenize_search_string (gchar *search_text);
static gboolean search_in_entry (GlJournalEntry *entry, GlJournalModel *model);
static gboolean gl_query_check_journal_end (GlQuery *query, GlJournalEntry *entry);

G_DEFINE_TYPE_WITH_CODE (GlJournalModel, gl_journal_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, gl_journal_model_interface_init))

enum
{
    PROP_0,
    PROP_LOADING,
    N_PROPERTIES
};

/* We define these two enum values as 2 and 3 to avoid the
 * conflict with TRUE and FALSE */
typedef enum
{
    LOGICAL_OR = 2,
    LOGICAL_AND = 3
} GlQueryLogic;

static GParamSpec *properties[N_PROPERTIES];

static gboolean
gl_journal_model_fetch_idle (gpointer user_data)
{
    GlJournalModel *model = user_data;
    GlJournalEntry *entry;
    guint last;

    g_assert (model->n_entries_to_fetch > 0);

    last = model->entries->len;
    if ((entry = gl_journal_previous (model->journal)) && gl_query_check_journal_end (model->query, entry))
    {
        if (search_in_entry (entry, model))
        {
            model->n_entries_to_fetch--;
            g_ptr_array_add (model->entries, entry);
            g_list_model_items_changed (G_LIST_MODEL (model), last, 0, 1);
        }
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
    if (model->token_array != NULL)
    {
        g_ptr_array_free (model->token_array, TRUE);
    }

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
    query->search_type = GL_QUERY_SEARCH_TYPE_SUBSTRING;
    query->start_timestamp = 0;
    query->end_timestamp = 0;

    return query;
}

static GlQueryItem *
gl_query_item_new (const gchar *field_name,
                   const gchar *field_value,
                   GlQuerySearchType search_type)
{
    GlQueryItem *queryitem;

    queryitem = g_slice_new (GlQueryItem);

    queryitem->field_name = g_strdup (field_name);
    queryitem->field_value = g_strdup (field_value);
    queryitem->search_type = search_type;

    return queryitem;
}

void
gl_query_set_search_type (GlQuery *query, GlQuerySearchType search_type)
{
    query->search_type = search_type;
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
populate_exact_matches (GlQueryItem *queryitem, GPtrArray *matches)
{
    gchar *match;

    if (queryitem->search_type == GL_QUERY_SEARCH_TYPE_EXACT)
    {
        match = gl_query_item_create_match_string (queryitem);

        g_ptr_array_add (matches, match);
    }
}

/* Get exact matches from the query object */
static GPtrArray *
gl_query_get_exact_matches (GlQuery *query)
{
    GPtrArray *matches;

    matches = g_ptr_array_new_with_free_func ((GDestroyNotify) g_free);

    g_ptr_array_foreach (query->queryitems, (GFunc) populate_exact_matches, matches);

    return matches;
}

static void
populate_substring_matches (GlQueryItem *queryitem, GPtrArray *matches)
{
    if (queryitem->search_type == GL_QUERY_SEARCH_TYPE_SUBSTRING)
    {
        g_ptr_array_add (matches, queryitem);
    }
}

/* Get exact matches from the query object */
static GPtrArray *
gl_query_get_substring_matches (GlQuery *query)
{
    GPtrArray *matches;

    matches = g_ptr_array_new ();

    g_ptr_array_foreach (query->queryitems, (GFunc) populate_substring_matches, matches);

    return matches;
}

/* Process the newly assigned query and repopulate the journal model */
static void
gl_journal_model_process_query (GlJournalModel *model)
{
    GPtrArray *category_matches;

    /* Set the exact matches first */
    category_matches = gl_query_get_exact_matches (model->query);

    /* Get the search string of the exact match field */
    if (model->query->search_type == GL_QUERY_SEARCH_TYPE_EXACT)
    {
        gchar *search_match;
        gchar *field_value_pos;

        /* Get the search match string */
        search_match = g_ptr_array_index (category_matches, category_matches->len - 1);

        field_value_pos = strchr (search_match, '=');

        /* If it has invalid string value remove it from the matches */
        if (!field_value_pos || !*(field_value_pos + 1))
            g_ptr_array_remove (category_matches, search_match);
    }

    gl_journal_set_matches (model->journal, category_matches);

    gl_journal_set_start_position (model->journal, model->query->start_timestamp);

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

    /* Tokenize the entered input only if search type is substring */
    if (query->search_type == GL_QUERY_SEARCH_TYPE_SUBSTRING)
    {
        GlQueryItem *search_match;
        GPtrArray *search_matches;

        search_matches = gl_query_get_substring_matches (model->query);

        /* Get search text from a search match */
        search_match = g_ptr_array_index (search_matches, 0);

        if (search_match->field_value != NULL)
        {
            model->token_array = tokenize_search_string (search_match->field_value);
        }

         g_ptr_array_free (search_matches, TRUE);

    }

    /* Start processing the new query */
    gl_journal_model_process_query (model);


}

/* Add a new queryitem to query */
void
gl_query_add_match (GlQuery *query,
                    const gchar *field_name,
                    const gchar *field_value,
                    GlQuerySearchType search_type)
{
    GlQueryItem *queryitem;

    queryitem = gl_query_item_new (field_name, field_value, search_type);

    g_ptr_array_add (query->queryitems, queryitem);
}

/* Check if current entry timestamp is less than the end timestamp */
static gboolean
gl_journal_entry_check_journal_end (GlJournalEntry *entry,
                                    guint64 end_timestamp)
{
    guint64 entry_timestamp = gl_journal_entry_get_timestamp (entry);

    if (end_timestamp)
    {
        /* Check if we have reached the end of given journal range */
        if (end_timestamp >= entry_timestamp)
            return FALSE;
    }

    return TRUE;
}

static gboolean
gl_query_check_journal_end (GlQuery *query, GlJournalEntry *entry)
{
    return gl_journal_entry_check_journal_end (entry, query->end_timestamp);
}

void
gl_query_set_journal_timestamp_range (GlQuery *query,
                                      guint64 start_timestamp,
                                      guint64 end_timestamp)
{
    query->start_timestamp = start_timestamp;
    query->end_timestamp = end_timestamp;
}

static gboolean
is_string_case_sensitive (const gchar *user_text)
{

    const gchar *search_text;

    for (search_text = user_text; search_text && *search_text;
         search_text = g_utf8_next_char (search_text))
    {
        gunichar c;

        c = g_utf8_get_char (search_text);

        if (g_unichar_isupper (c))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
utf8_strcasestr (const gchar *potential_hit,
                 const gchar *search_term)
{
  gchar *folded;
  gboolean matches;

  folded = g_utf8_casefold (potential_hit, -1);
  matches = strstr (folded, search_term) != NULL;

  g_free (folded);
  return matches;
}

static GPtrArray *
tokenize_search_string (gchar *search_text)
{
    gchar *field_name;
    gchar *field_value;
    GPtrArray *token_array;
    GScanner *scanner;

    token_array = g_ptr_array_new_with_free_func (g_free);
    scanner = g_scanner_new (NULL);
    scanner->config->cset_skip_characters = " =\t\n";

    /* All the characters used in the journal field values */
    scanner->config->cset_identifier_first = (
                                              G_CSET_a_2_z
                                              G_CSET_A_2_Z
                                              G_CSET_DIGITS
                                              "/_.-@:\\+"
                                              );

    scanner->config->cset_identifier_nth = (
                                            G_CSET_a_2_z
                                            G_CSET_A_2_Z
                                            G_CSET_DIGITS
                                            "/_.-@:\\"
                                            );

    g_scanner_input_text (scanner, search_text, strlen (search_text));

    do
    {
        g_scanner_get_next_token (scanner);
        if (scanner->value.v_identifier == NULL && scanner->token != '+')
        {
            break;
        }
        else if (scanner->token == '+')
        {
            g_ptr_array_add (token_array, g_strdup ("+"));

            g_scanner_get_next_token (scanner);
            if (scanner->value.v_identifier != NULL)
            {
                field_name = g_strdup (scanner->value.v_identifier);
                g_ptr_array_add (token_array, field_name);
            }
            else
            {
                field_name = NULL;
            }
        }
        else if (scanner->token == G_TOKEN_INT)
        {
            field_name = g_strdup_printf ("%lu", scanner->value.v_int);
            g_ptr_array_add (token_array, field_name);
        }
        else if (scanner->token == G_TOKEN_FLOAT)
        {
            field_name = g_strdup_printf ("%g", scanner->value.v_float);
            g_ptr_array_add (token_array, field_name);
        }
        else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
            if (token_array->len != 0)
            {
                g_ptr_array_add (token_array, g_strdup (" "));
            }

            field_name = g_strdup (scanner->value.v_identifier);
            g_ptr_array_add (token_array, field_name);
        }
        else
        {
            field_name = NULL;
        }

        g_scanner_get_next_token (scanner);
        if (scanner->token == G_TOKEN_INT)
        {
            field_value = g_strdup_printf ("%lu", scanner->value.v_int);
            g_ptr_array_add (token_array, field_value);
        }
        else if (scanner->token == G_TOKEN_FLOAT)
        {
            field_value = g_strdup_printf ("%g", scanner->value.v_float);
            g_ptr_array_add (token_array, field_value);
        }
        else if (scanner->token == G_TOKEN_IDENTIFIER)
        {
            field_value = g_strdup (scanner->value.v_identifier);
            g_ptr_array_add (token_array, field_value);
        }
        else
        {
            field_value = NULL;
        }
    } while (field_name != NULL && field_value != NULL);

    g_scanner_destroy (scanner);

    return token_array;
}

/* Functions for replacing the big if queries in calculate token match */
static const gchar *
gl_query_item_get_entry_parameter (GlQueryItem *search_match,
                                   GlJournalEntry *entry,
                                   gboolean case_sensitive)
{
    const gchar *comm;
    const gchar *message;
    const gchar *kernel_device;
    const gchar *audit_session;
    const gchar *pid;
    const gchar *uid;
    const gchar *gid;
    const gchar *systemd_unit;
    const gchar *executable_path;

    comm = gl_journal_entry_get_command_line (entry);
    message = gl_journal_entry_get_message (entry);
    kernel_device = gl_journal_entry_get_kernel_device (entry);
    audit_session = gl_journal_entry_get_audit_session (entry);
    systemd_unit = gl_journal_entry_get_systemd_unit (entry);
    pid = gl_journal_entry_get_pid (entry);
    uid = gl_journal_entry_get_uid (entry);
    gid = gl_journal_entry_get_gid (entry);
    executable_path = gl_journal_entry_get_executable_path (entry);

    if (case_sensitive)
    {
        if (strstr ("_MESSAGE", search_match->field_name))
        {
            return message;
        }
        else if (strstr ("_COMM", search_match->field_name))
        {
            return comm;
        }
        else if (strstr ("_KERNEL_DEVICE", search_match->field_name))
        {
            return kernel_device;
        }
        else if (strstr ("_AUDIT_SESSION", search_match->field_name))
        {
            return audit_session;
        }
        else if (strstr ("_SYSTEMD_UNIT", search_match->field_name))
        {
            return systemd_unit;
        }
        else if (strstr ("_PID", search_match->field_name))
        {
            return pid;
        }
        else if (strstr ("_UID", search_match->field_name))
        {
            return uid;
        }
        else if (strstr ("_GID", search_match->field_name))
        {
            return gid;
        }
        else if (strstr ("_EXE", search_match->field_name))
        {
            return executable_path;
        }
    }
    else
    {
        if (utf8_strcasestr ("_message", search_match->field_name))
        {
            return message;
        }
        else if (utf8_strcasestr ("_comm", search_match->field_name))
        {
            return comm;
        }
        else if (utf8_strcasestr ("_kernel_device", search_match->field_name))
        {
            return kernel_device;
        }
        else if (utf8_strcasestr ("_audit_session", search_match->field_name))
        {
            return audit_session;
        }
        else if (utf8_strcasestr ("_systemd_unit", search_match->field_name))
        {
            return systemd_unit;
        }
        else if (utf8_strcasestr ("_pid", search_match->field_name))
        {
            return pid;
        }
        else if (utf8_strcasestr ("_uid", search_match->field_name))
        {
            return uid;
        }
        else if (utf8_strcasestr ("_gid", search_match->field_name))
        {
            return gid;
        }
        else if (utf8_strcasestr ("_exe", search_match->field_name))
        {
            return executable_path;
        }
    }

    return NULL;
}

static gboolean
gl_query_item_get_match (GlQueryItem *search_match,
                         const gchar *field_value,
                         gboolean case_sensitive)
{
    if (field_value)
    {
        if (case_sensitive)
        {
            if (strstr (field_value, search_match->field_value))
            {
                return TRUE;
            }
        }
        else
        {
            if (utf8_strcasestr (field_value, search_match->field_value))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean
gl_query_items_check_parameters (GPtrArray *search_matches,
                                 GlJournalEntry *entry)
{
    GlQueryItem *search_match;
    const gchar *entry_parameter;
    gboolean field_value_case;
    gboolean field_name_case;
    gint i;

    for (i=0; i < search_matches->len ;i++)
    {
        search_match = g_ptr_array_index (search_matches, i);

        field_name_case = is_string_case_sensitive (search_match->field_name);
        field_value_case = is_string_case_sensitive (search_match->field_value);

        entry_parameter = gl_query_item_get_entry_parameter (search_match, entry, field_name_case);

        if (gl_query_item_get_match (search_match, entry_parameter, field_value_case))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
calculate_match (GlJournalEntry *entry,
                 GPtrArray *token_array,
                 GPtrArray *search_matches)
{
    GlQueryItem *token_match;
    gchar *field_name;
    gchar *field_value;
    gboolean field_name_case;
    gboolean field_value_case;
    const gchar *entry_parameter;
    gboolean matches;
    gint match_stack[10];
    guint match_count = 0;
    guint token_index = 0;
    gint i;

    /* No logical AND or OR used in search text */
    if (token_array->len == 1)
    {
        return gl_query_items_check_parameters (search_matches, entry);
    }

    /* If multiple tokens are present : execute the token mode */
    while (token_index < token_array->len)
    {
        field_name = g_ptr_array_index (token_array, token_index);
        token_index++;

        if (token_index == token_array->len)
        {
            break;
        }

        field_value = g_ptr_array_index (token_array, token_index);
        token_index++;


        /* check for matches */
        token_match = gl_query_item_new (field_name, field_value, GL_QUERY_SEARCH_TYPE_SUBSTRING);

        field_name_case = is_string_case_sensitive (field_name);
        field_value_case = is_string_case_sensitive (field_value);

        entry_parameter = gl_query_item_get_entry_parameter (token_match, entry, field_name_case);

        matches = gl_query_item_get_match (token_match, entry_parameter, field_value_case);

        gl_query_item_free (token_match);

        match_stack[match_count] = matches;
        match_count++;

        if (token_index == token_array->len)
        {
            break;
        }

        if (g_strcmp0 (g_ptr_array_index (token_array, token_index), " ") == 0)
        {
            match_stack[match_count] = LOGICAL_AND;
            match_count++;
            token_index++;
        }
        else if (g_strcmp0 (g_ptr_array_index (token_array, token_index),
                            "+") == 0)
        {
            match_stack[match_count] = LOGICAL_OR;
            match_count++;
            token_index++;
        }
    }

    /* match_count > 2 means there are still matches to be calculated in the
     * stack */
    if (match_count > 2)
    {
        /* calculate the expression with logical AND */
        for (i = 0; i < match_count; i++)
        {
            if (match_stack[i] == LOGICAL_AND)
            {
                int j;

                match_stack[i - 1] = match_stack[i - 1] && match_stack[i + 1];

                for (j = i; j < match_count - 2; j++)
                {
                    if (j == match_count - 3)
                    {
                        match_stack[j] = match_stack[j + 2];
                        /* We use -1 to represent the values that are not
                         * useful */
                        match_stack[j + 1] = -1;

                        break;
                    }

                    match_stack[j] = match_stack[j + 2];
                    match_stack[j + 2] = -1;
                }
            }
        }

        /* calculate the expression with logical OR */
        for (i = 0; i < match_count; i++)
        {
            /* We use -1 to represent the values that are not useful */
            if ((match_stack[i] == LOGICAL_OR) && (i != token_index - 1) &&
                (match_stack[i + 1] != -1))
            {
                int j;

                match_stack[i - 1] = match_stack[i - 1] || match_stack[i + 1];

                for (j = i; j < match_count - 2; j++)
                {
                    match_stack[j] = match_stack[j + 2];
                    match_stack[j + 2] = -1;
                }
            }
        }
    }

    matches = match_stack[0];

    return matches;
}

static gboolean
search_in_entry (GlJournalEntry *entry,
                 GlJournalModel *model)
{
    GlQueryItem *search_match;
    gboolean matches;
    GPtrArray *search_matches;

    search_matches = gl_query_get_substring_matches (model->query);

    /* Check if there is atleast one substring queryitem */
    if (search_matches->len)
    {
        /* Get search text from a search match */
        search_match = g_ptr_array_index (search_matches, 0);

        /* check for null and empty strings */
        if (!search_match->field_value || !*(search_match->field_value))
        {
            matches = TRUE;
        }
        else
        {
            /* calculate match depending on the number of tokens */
            matches = calculate_match (entry, model->token_array,
                                       search_matches);
        }
    }
    else
    {
        matches = TRUE;
    }

    g_ptr_array_free (search_matches, TRUE);

    return matches;
}

gchar *
gl_journal_model_get_boot_time (GlJournalModel *model,
                                const gchar *boot_match)
{
    return gl_journal_get_boot_time (model->journal, boot_match);
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
