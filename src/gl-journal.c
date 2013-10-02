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

#include "gl-journal.h"

#include <glib-unix.h>

typedef struct
{
    sd_journal *journal;
    gint fd;
    guint source_id;
} GlJournalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GlJournal, gl_journal, G_TYPE_OBJECT)

static gboolean
on_journal_changed (gint fd,
                    GIOCondition condition,
                    GlJournal *self)
{
    gint ret;
    GlJournalPrivate *priv = gl_journal_get_instance_private (self);

    ret = sd_journal_process (priv->journal);

    switch (ret)
    {
        case SD_JOURNAL_NOP:
            g_debug ("Journal change was a no-op");
            break;
        case SD_JOURNAL_APPEND:
            g_debug ("New journal entries added");
            break;
        case SD_JOURNAL_INVALIDATE:
            g_debug ("Journal files added or removed");
            break;
        default:
            g_warning ("Error processing events from systemd journal: %s",
                       g_strerror (-ret));
            break;
    }

    return G_SOURCE_CONTINUE;
}

static void
gl_journal_finalize (GObject *object)
{
    GlJournal *journal = GL_JOURNAL (object);
    GlJournalPrivate *priv = gl_journal_get_instance_private (journal);

    g_source_remove (priv->source_id);
    g_clear_pointer (&priv->journal, sd_journal_close);
}

static void
gl_journal_class_init (GlJournalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = gl_journal_finalize;
}

static void
gl_journal_init (GlJournal *self)
{
    GlJournalPrivate *priv;
    sd_journal *journal;
    gint ret;

    priv = gl_journal_get_instance_private (self);

    ret = sd_journal_open (&journal, 0);
    priv->journal = journal;

    if (ret < 0)
    {
        g_critical ("Error opening systemd journal: %s", g_strerror (-ret));
    }

    ret = sd_journal_get_fd (journal);

    if (ret < 0)
    {
        g_warning ("Error getting polling fd from systemd journal: %s",
                   g_strerror (-ret));
    }

    priv->fd = ret;
    ret = sd_journal_get_events (journal);

    if (ret < 0)
    {
        g_warning ("Error getting poll events from systemd journal: %s",
                   g_strerror (-ret));
    }

    priv->source_id = g_unix_fd_add (priv->fd, ret,
                                     (GUnixFDSourceFunc) on_journal_changed,
                                     self);
    ret = sd_journal_reliable_fd (journal);

    if (ret < 0)
    {
        g_warning ("Error checking reliability of systemd journal poll fd: %s",
                   g_strerror (-ret));
    }
    else if (ret == 0)
    {
        g_debug ("Latency expected while polling for systemd journal activity");
    }
    else
    {
        g_debug ("Immediate wakeups expected for systemd journal activity");
    }

}

sd_journal *
gl_journal_get_journal (GlJournal *self)
{
    GlJournalPrivate *priv;

    g_return_val_if_fail (GL_JOURNAL (self), NULL);

    priv = gl_journal_get_instance_private (self);

    return priv->journal;
}

GlJournal *
gl_journal_new (void)
{
    return g_object_new (GL_TYPE_JOURNAL, NULL);
}
