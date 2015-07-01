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

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gl-mock-journal.h"
#include "../src/gl-application.h"
#include "../src/gl-eventviewdetail.h"
#include "../src/gl-journal-model.h"
#include "../src/gl-categorylist.h"
#include "../src/gl-eventview.h"
#include "../src/gl-enums.h"
#include "../src/gl-eventtoolbar.h"
#include "../src/gl-eventviewlist.h"
#include "../src/gl-eventviewrow.h"
#include "../src/gl-util.h"
#include "../src/gl-window.h"


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
    GtkApplication *application;
    int status;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (PACKAGE_TARNAME, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_prgname (PACKAGE_TARNAME);
    application = gl_application_new ();
    status = g_application_run (G_APPLICATION (application), argc, argv);
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/util/check_log_message", check_log_message);
    g_object_unref (application);
 
   return g_test_run ();
}
