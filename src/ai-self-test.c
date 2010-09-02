/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2010 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <glib.h>
#include <glib-object.h>

#include "egg-debug.h"
#include "ai-database.h"

static void
ai_test_database_func (void)
{
	gboolean ret;
	GError *error = NULL;
	AiDatabase *db;
	AiDatabase *db2;
	guint value;

	/* nuke test file */
	g_unlink ("test.db");
	g_unlink ("test2.db");

	/* get an instance */
	db = ai_database_new ();
	g_assert (db != NULL);

	/* set filename (once) */
	ret = ai_database_set_filename (db, "test-do-not-use.db", NULL);
	g_assert (ret);

	/* set filename (again) */
	ret = ai_database_set_filename (db, "test.db", NULL);
	g_assert (ret);

	/* create database without open */
	ret = ai_database_create (db, NULL);
	g_assert (!ret);

	/* open database */
	ret = ai_database_open (db, TRUE, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* create database after open */
	ret = ai_database_create (db, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* check correct number by repo */
	ret = ai_database_query_number_by_repo (db, "fedora", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 0);

	/* check correct number by name */
	ret = ai_database_query_number_by_name (db, "gnome-packagekit", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 0);

	/* add translation */
	ret = ai_database_add_translation (db,
					   "gpk-application",
					   "GNOME PackageKit",
					   "Package Installer",
					   "en_GB",
					   &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* add application */
	ret = ai_database_add_application (db,
					   "gpk-application",
					   "gnome-packagekit",
					   "GNOME;games;",
					   "fedora",
					   "gpk-app.png",
					   "GNOME PackageKit",
					   "Package Installer",
					   &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* add application (another one) */
	ret = ai_database_add_application (db,
					   "gpk-prefs",
					   "gnome-packagekit",
					   "GNOME;games;",
					   "rpmfusion",
					   "gpk-app.png",
					   "GNOME PackageKit Preferences",
					   "Package Installer Preferences",
					   &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* check correct number by repo */
	ret = ai_database_query_number_by_repo (db, "fedora", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 1);

	/* check correct number by name */
	ret = ai_database_query_number_by_name (db, "gnome-packagekit", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 2);

	/* remove by repo */
	ret = ai_database_remove_by_repo (db, "fedora", &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* check correct number by repo */
	ret = ai_database_query_number_by_repo (db, "fedora", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 0);

	/* remove by name */
	ret = ai_database_remove_by_name (db, "gnome-packagekit", &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* check correct number by name */
	ret = ai_database_query_number_by_name (db, "gnome-packagekit", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 0);

	/* close database */
	ret = ai_database_close (db, TRUE, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* close database (again) */
	ret = ai_database_close (db, FALSE, NULL);
	g_assert (!ret);

	/* try to import by name */
	ret = ai_database_open (db, FALSE, NULL);
	g_assert (ret);
	ret = ai_database_add_application (db,
					   "gpk-application",
					   "gnome-packagekit",
					   "GNOME;games;",
					   "fedora",
					   "gpk-app.png",
					   "GNOME PackageKit",
					   "Package Installer",
					   NULL);
	g_assert (ret);
	ret = ai_database_add_translation (db,
					   "gpk-application",
					   "GNOME PackageKit",
					   "Package Installer",
					   "en_GB",
					   NULL);
	g_assert (ret);
	ret = ai_database_close (db, TRUE, NULL);
	g_assert (ret);

	ret = ai_database_set_filename (db, "test2.db", NULL);
	g_assert (ret);
	ret = ai_database_open (db, TRUE, NULL);
	g_assert (ret);
	ret = ai_database_create (db, NULL);
	g_assert (ret);
	ret = ai_database_import_by_name (db, "test.db", NULL, "gnome-packagekit", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 1);

	/* check correct number by name */
	ret = ai_database_query_number_by_name (db, "gnome-packagekit", &value, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (value, ==, 1);

	ai_database_close (db, FALSE, NULL);

	g_object_unref (db);

	/* nuke test file */
	g_unlink ("test.db");
	g_unlink ("test2.db");
}

int
main (int argc, char **argv)
{
	if (! g_thread_supported ())
		g_thread_init (NULL);
	g_type_init ();
	egg_debug_init (FALSE);
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	/* components */
	g_test_add_func ("/app-install/database", ai_test_database_func);

	return g_test_run ();
}

