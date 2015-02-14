
#ifndef GL_JOURNAL_MODEL_H
#define GL_JOURNAL_MODEL_H

#include <gio/gio.h>

#define GL_TYPE_JOURNAL_MODEL gl_journal_model_get_type()
G_DECLARE_FINAL_TYPE (GlJournalModel, gl_journal_model, GL, JOURNAL_MODEL, GObject)

GlJournalModel *        gl_journal_model_new                            (void);

void                    gl_journal_model_set_matches                    (GlJournalModel      *model,
                                                                         const gchar * const *matches);

#endif
