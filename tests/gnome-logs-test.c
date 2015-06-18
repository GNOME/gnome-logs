/*
 *  GNOME Logs - View and search logs
 *  Copyright (C) 2015 Rashi Aswani
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

#include "gl-journal-mock.h"

static void
check_log_message (void)
{
   
   GlMockJournal *journal = gl_mock_journal_new();
   GlMockJournalEntry *entry = gl_mock_journal_previous(journal);
   const gchar *mystring = gl_mock_journal_entry_get_message(entry);
   g_assert_cmpstr(mystring, ==, "This is a test");
}

int
main (int argc, char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/util/check_log_message", check_log_message);

    return g_test_run ();
}
