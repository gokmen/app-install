/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009-2010 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <glib-object.h>
#include <sqlite3.h>
#include <gio/gio.h>
#include <stdlib.h>

#include "egg-debug.h"

#include "ai-database.h"
#include "ai-result.h"
#include "ai-common.h"

static void     ai_database_finalize	(GObject     *object);

#define AI_DATABASE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), AI_TYPE_DATABASE, AiDatabasePrivate))

/*
 * AiDatabasePrivate:
 *
 * Private #AiDatabase data
 */
struct _AiDatabasePrivate
{
	sqlite3				*db;
	gchar				*filename;
	gchar				*icon_path;
	gboolean			 locked;
	guint				 dbversion;
};

enum {
	PROP_0,
	PROP_LOCKED,
	PROP_LAST
};

G_DEFINE_TYPE (AiDatabase, ai_database, G_TYPE_OBJECT)

/*
 * ai_database_set_filename:
 *
 * The source database filename.
 */
gboolean
ai_database_set_filename (AiDatabase *database, const gchar *filename, GError **error)
{
	gboolean ret = TRUE;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (priv->locked) {
		g_set_error (error, 1, 0, "database is already open");
		ret = FALSE;
		goto out;
	}

	g_free (priv->filename);

	/* use default */
	if (filename == NULL) {
		egg_debug ("database not specified, using %s", AI_DEFAULT_DATABASE);
		priv->filename = g_strdup (AI_DEFAULT_DATABASE);
	} else {
		priv->filename = g_strdup (filename);
	}
out:
	return ret;
}

/*
 * ai_database_set_icon_path:
 *
 * The icon path for the currently loaded database.
 */
gboolean
ai_database_set_icon_path (AiDatabase *database, const gchar *icon_path, GError **error)
{
	gboolean ret = TRUE;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (priv->locked) {
		g_set_error (error, 1, 0, "database is already open");
		ret = FALSE;
		goto out;
	}

	/* check it exists */
	if (icon_path != NULL && !g_file_test (icon_path, G_FILE_TEST_IS_DIR)) {
		g_set_error (error, 1, 0, "the icon directory '%s' could not be found", icon_path);
		ret = FALSE;
		goto out;
	}

	g_free (priv->icon_path);

	/* use default */
//	if (icon_path == NULL) {
//		egg_debug ("database not specified, using %s", AI_DEFAULT_ICONDIR);
//		priv->icon_path = g_strdup (AI_DEFAULT_ICONDIR);
//	} else {
		priv->icon_path = g_strdup (icon_path);
//	}

out:
	return ret;
}


/**
 * ai_database_get_dbversion_sqlite_cb:
 **/
static gint
ai_database_get_dbversion_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint *version = (guint *) data;
	/* we only expect one reply */
	if (argc != 1)
		return 1;

	/* parse version string */
	*version = atoi (argv[0]);
	return 0;
}

/*
 * ai_database_open:
 */
gboolean
ai_database_open (AiDatabase *database, gboolean synchronous, GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	const gchar *statement;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (priv->locked) {
		g_set_error (error, 1, 0, "database is already open");
		ret = FALSE;
		goto out;
	}

	/* open database */
	rc = sqlite3_open (priv->filename, &priv->db);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "Can't open database %s: %s\n", priv->filename, sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* don't sync */
	if (!synchronous) {
		statement = "PRAGMA synchronous=OFF";
		rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		if (rc != SQLITE_OK) {
			g_set_error (error, 1, 0, "Can't turn off sync from %s: %s\n", priv->filename, sqlite3_errmsg (priv->db));
			ret = FALSE;
			goto out;
		}
	}

	/* get version, failure is okay as v1 databases didn't have this table */
	statement = "SELECT value FROM config WHERE data = 'dbversion'";
	rc = sqlite3_exec (priv->db, statement, ai_database_get_dbversion_sqlite_cb, (void*) &priv->dbversion, NULL);
	if (rc != SQLITE_OK)
		priv->dbversion = 1;
	egg_debug ("operating on database version %i", priv->dbversion);

	/* okay for business */
	priv->locked = TRUE;
out:
	return ret;
}

/*
 * ai_database_get_version:
 */
guint
ai_database_get_version (AiDatabase *database)
{
	g_return_val_if_fail (AI_IS_DATABASE (database), 0);
	return database->priv->dbversion;
}

/*
 * ai_database_close:
 */
gboolean
ai_database_close (AiDatabase *database, gboolean vaccuum, GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	const gchar *statement;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* reclaim memory */
	if (vaccuum) {
		statement = "VACUUM";
		rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		if (rc) {
			g_set_error (error, 1, 0, "Can't vaccuum: %s\n", sqlite3_errmsg (priv->db));
			ret = FALSE;
			goto out;
		}
	}

	sqlite3_close (priv->db);
	priv->locked = FALSE;
	priv->dbversion = 0;
out:
	return ret;
}

/*
 * ai_database_create:
 */
gboolean
ai_database_create (AiDatabase *database, GError **error)
{
	gboolean ret = TRUE;
	const gchar *statement;
	gint rc;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* create applications */
	statement = "CREATE TABLE applications ("
		    "application_id TEXT primary key,"
		    "package_name TEXT,"
		    "categories TEXT,"
		    "repo_id TEXT,"
		    "icon_name TEXT,"
		    "application_name TEXT,"
		    "application_summary TEXT,"
		    "rating INTEGER DEFAULT 0,"
		    "screenshot_url TEXT,"
		    "installed BOOLEAN DEFAULT FALSE);";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't create applications table: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* create translations */
	statement = "CREATE TABLE translations ("
		    "application_id TEXT,"
		    "application_name TEXT,"
		    "application_summary TEXT,"
		    "locale TEXT);";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't create translations table: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* create config */
	statement = "CREATE TABLE config ("
		    "data TEXT primary key,"
		    "value INTEGER);";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't create config table: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
	statement = "INSERT INTO config (data, value) VALUES ('dbversion', 2);";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't insert dbver: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
out:
	return ret;
}

/*
 * ai_database_upgrade:
 */
gboolean
ai_database_upgrade (AiDatabase *database, GError **error)
{
	gboolean ret = TRUE;
	const gchar *statement;
	gint rc;
	guint done_upgrade = FALSE;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* upgrade from version 1 */
	if (priv->dbversion == 1) {

		/* create config */
		statement = "CREATE TABLE config ("
			    "data TEXT primary key,"
			    "value INTEGER);";
		rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		if (rc) {
			g_set_error (error, 1, 0, "Can't create config table: %s\n", sqlite3_errmsg (priv->db));
			ret = FALSE;
			goto out;
		}

		/* add per-application new data */
		statement = "ALTER TABLE applications ADD COLUMN rating INTEGER DEFAULT 0;";
		sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		statement = "ALTER TABLE applications ADD COLUMN screenshot_url TEXT;";
		sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		statement = "ALTER TABLE applications ADD COLUMN installed BOOLEAN DEFAULT FALSE;";
		sqlite3_exec (priv->db, statement, NULL, NULL, NULL);

		priv->dbversion = 2;
		done_upgrade = TRUE;
	}

	/* set the new database version */
	if (done_upgrade) {
		statement = "INSERT INTO config (data, value) VALUES ('dbversion', 2);";
		rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
		if (rc) {
			g_set_error (error, 1, 0, "Can't change dbver: %s\n", sqlite3_errmsg (priv->db));
			ret = FALSE;
			goto out;
		}
	} else {
		egg_debug ("database already newest version, not performing any changes");
	}
out:
	return ret;

}

static const gchar *icon_sizes[] = { "22x22", "24x24", "32x32", "48x48", "scalable", NULL };

/**
 * ai_database_remove_icons_sqlite_cb:
 **/
static gint
ai_database_remove_icons_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint i;
	gchar *col;
	gchar *value;
	const gchar *application_id = NULL;
	const gchar *icon_name = NULL;
	gchar *path;
	const gchar *icondir = (const gchar *) data;
	GFile *file;
	gboolean ret;
	GError *error = NULL;

	for (i=0; i<(guint)argc; i++) {
		col = col_name[i];
		value = argv[i];
		if (g_strcmp0 (col, "application_id") == 0)
			application_id = value;
		else if (g_strcmp0 (col, "icon_name") == 0)
			icon_name = value;
	}
	if (application_id == NULL || icon_name == NULL)
		goto out;

	egg_debug ("removing icons for application: %s", application_id);

	/* delete all icon sizes */
	for (i=0; icon_sizes[i] != NULL; i++) {
		path = g_build_filename (icondir, icon_sizes[i], icon_name, NULL);
		ret = g_file_test (path, G_FILE_TEST_EXISTS);
		if (ret) {
			egg_debug ("removing file %s", path);
			file = g_file_new_for_path (path);
			ret = g_file_delete (file, NULL, &error);
			if (!ret) {
				egg_warning ("cannot delete %s: %s", path, error->message);
				g_clear_error (&error);
			}
			g_object_unref (file);
		}
		g_free (path);
	}
out:
	return 0;
}

/*
 * ai_database_remove_by_repo:
 */
gboolean
ai_database_remove_by_repo (AiDatabase *database, const gchar *repo, GError **error)
{
	gboolean ret = TRUE;
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* remove icons */
	if (priv->icon_path != NULL) {
		statement = g_strdup_printf ("SELECT application_id, icon_name FROM applications WHERE repo_id = '%s'", repo);
		rc = sqlite3_exec (priv->db, statement, ai_database_remove_icons_sqlite_cb, (void*) priv->icon_path, &error_msg);
		g_free (statement);
		if (rc != SQLITE_OK) {
			g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
			sqlite3_free (error_msg);
			ret = FALSE;
			goto out;
		}
	}

	/* delete from translations (translations has no repo_id, so key off applications) */
	statement = g_strdup_printf ("DELETE FROM translations WHERE EXISTS ( "
				     "SELECT applications.application_id FROM applications WHERE "
				     "applications.application_id = applications.application_id AND applications.repo_id = '%s')", repo);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	g_free (statement);
	if (rc) {
		g_set_error (error, 1, 0, "Can't remove rows: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
	egg_debug ("%i removals from translations", sqlite3_changes (priv->db));

	/* delete from applications */
	statement = g_strdup_printf ("DELETE FROM applications WHERE repo_id = '%s'", repo);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	g_free (statement);
	if (rc) {
		g_set_error (error, 1, 0, "Can't remove rows: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
	egg_debug ("%i removals from applications", sqlite3_changes (priv->db));
out:
	return ret;
}

/*
 * ai_database_remove_by_name:
 */
gboolean
ai_database_remove_by_name (AiDatabase *database, const gchar *name, GError **error)
{
	gboolean ret = TRUE;
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* remove icons */
	if (priv->icon_path != NULL) {
		statement = g_strdup_printf ("SELECT application_id, icon_name FROM applications WHERE package_name = '%s'", name);
		rc = sqlite3_exec (priv->db, statement, ai_database_remove_icons_sqlite_cb, (void*) priv->icon_path, &error_msg);
		g_free (statement);
		if (rc != SQLITE_OK) {
			g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
			sqlite3_free (error_msg);
			ret = FALSE;
			goto out;
		}
	}

	/* delete from translations (translations has no repo_id, so key off applications) */
	statement = g_strdup_printf ("DELETE FROM translations WHERE EXISTS ( "
				      "SELECT applications.application_id FROM applications WHERE "
				      "applications.application_id = applications.application_id AND applications.package_name = '%s')", name);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	g_free (statement);
	if (rc) {
		g_set_error (error, 1, 0, "Can't remove rows: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
	egg_debug ("%i removals from translations", sqlite3_changes (priv->db));

	/* delete from applications */
	statement = g_strdup_printf ("DELETE FROM applications WHERE package_name = '%s'", name);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	g_free (statement);
	if (rc) {
		g_set_error (error, 1, 0, "Can't remove rows: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
	egg_debug ("%i removals from applications", sqlite3_changes (priv->db));
out:
	return ret;
}

/**
 * ai_database_get_number_sqlite_cb:
 **/
static gint
ai_database_get_number_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint *number = (guint *) data;
	(*number)++;
	return 0;
}

/*
 * ai_database_query_number_by_repo:
 */
gboolean
ai_database_query_number_by_repo (AiDatabase *database, const gchar *repo, guint *value, GError **error)
{
	gboolean ret = TRUE;
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* set to initial state */
	*value = 0;

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT application_id FROM applications WHERE repo_id = '%s'", repo);
	rc = sqlite3_exec (priv->db, statement, ai_database_get_number_sqlite_cb, (void*) value, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		sqlite3_free (error_msg);
		goto out;
	}
out:
	g_free (statement);
	return ret;
}

/*
 * ai_database_query_number_by_name:
 */
gboolean
ai_database_query_number_by_name (AiDatabase *database, const gchar *name, guint *value, GError **error)
{
	gboolean ret = TRUE;
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* set to initial state */
	*value = 0;

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT application_id FROM applications WHERE package_name = '%s'", name);
	rc = sqlite3_exec (priv->db, statement, ai_database_get_number_sqlite_cb, (void*) value, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		sqlite3_free (error_msg);
		goto out;
	}
out:
	g_free (statement);
	return ret;
}

/**
 * ai_database_search_sqlite_cb:
 **/
static gint
ai_database_search_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	GPtrArray *array = (GPtrArray *) data;
	guint i;
	const gchar *application_id = NULL;
	const gchar *package_name = NULL;
	const gchar *categories = NULL;
	const gchar *repo_id = NULL;
	const gchar *icon_name = NULL;
	const gchar *application_name = NULL;
	const gchar *application_summary = NULL;
	guint rating = 0;
	const gchar *screenshot_url = NULL;
	gboolean installed = TRUE;
	AiResult *result;

	for (i=0; i<(guint)argc; i++) {
		if (g_strcmp0 (col_name[i], "application_id") == 0)
			application_id = argv[i];
		else if (g_strcmp0 (col_name[i], "package_name") == 0)
			package_name = argv[i];
		else if (g_strcmp0 (col_name[i], "categories") == 0)
			categories = argv[i];
		else if (g_strcmp0 (col_name[i], "repo_id") == 0)
			repo_id = argv[i];
		else if (g_strcmp0 (col_name[i], "icon_name") == 0)
			icon_name = argv[i];
		else if (g_strstr_len (col_name[i], -1, "application_name") != NULL)
			application_name = argv[i];
		else if (g_strstr_len (col_name[i], -1, "application_summary") != NULL)
			application_summary = argv[i];
		else if (g_strstr_len (col_name[i], -1, "rating") != NULL)
			rating = atoi (argv[i]);
		else if (g_strstr_len (col_name[i], -1, "screenshot_url") != NULL)
			screenshot_url = argv[i];
		else if (g_strstr_len (col_name[i], -1, "installed") != NULL)
			installed = atoi (argv[i]);
		else
			egg_warning ("unhandled column: %s", col_name[i]);
	}

	/* create new object and add to the array */
	egg_debug ("application_id=%s, package_name=%s, application_name=%s", application_id, package_name, application_name);
	result = g_object_new (AI_TYPE_RESULT,
			       "application-id", application_id,
			       "package-name", package_name,
			       "categories", categories,
			       "repo-id", repo_id,
			       "icon-name", icon_name,
			       "application-name", application_name,
			       "application-summary", application_summary,
			       "rating", rating,
			       "screenshot-url", screenshot_url,
			       "installed", installed,
			       NULL);
	g_ptr_array_add (array, result);
	return 0;
}

/*
 * ai_database_search_by_id:
 */
GPtrArray *
ai_database_search_by_id (AiDatabase *database, const gchar *value, GError **error)
{
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	GPtrArray *array = NULL;
	GPtrArray *array_tmp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		goto out;
	}

	/* create array */
	array_tmp = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT application_id, package_name, categories, "
				     "repo_id, icon_name, application_name, application_summary, "
				     "rating, screenshot_url, installed "
				     "FROM applications WHERE application_id = '%s'", value);
	rc = sqlite3_exec (priv->db, statement, ai_database_search_sqlite_cb, (void*) array_tmp, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		sqlite3_free (error_msg);
		goto out;
	}

	/* success */
	array = g_ptr_array_ref (array_tmp);
out:
	g_ptr_array_unref (array_tmp);
	g_free (statement);
	return array;
}

/*
 * ai_database_search_by_name:
 */
GPtrArray *
ai_database_search_by_name (AiDatabase *database, const gchar *value, GError **error)
{
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	GPtrArray *array = NULL;
	GPtrArray *array_tmp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		goto out;
	}

	/* create array */
	array_tmp = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT application_id, package_name, categories, "
				     "repo_id, icon_name, application_name, application_summary, "
				     "rating, screenshot_url, installed "
				     "FROM applications WHERE application_name LIKE '%%%s%%'", value);
	rc = sqlite3_exec (priv->db, statement, ai_database_search_sqlite_cb, (void*) array_tmp, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		sqlite3_free (error_msg);
		goto out;
	}

	/* success */
	array = g_ptr_array_ref (array_tmp);
out:
	g_ptr_array_unref (array_tmp);
	g_free (statement);
	return array;
}


/*
 * ai_database_search_by_id_locale:
 */
GPtrArray *
ai_database_search_by_id_locale (AiDatabase *database, const gchar *value, const gchar *locale, GError **error)
{
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	GPtrArray *array = NULL;
	GPtrArray *array_tmp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		goto out;
	}

	/* create array */
	array_tmp = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT a.application_id, a.package_name, a.categories, "
				     "a.repo_id, a.icon_name, "
				     "a.rating, a.screenshot_url, a.installed, "
				     "COALESCE(t.application_name, a.application_name), "
				     "COALESCE(t.application_summary, a.application_summary) "
				     "FROM applications a LEFT JOIN translations t ON a.application_id = t.application_id AND t.locale = 'pt_BR' "
				     "WHERE a.application_id = '%s'", value);
	rc = sqlite3_exec (priv->db, statement, ai_database_search_sqlite_cb, (void*) array_tmp, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		sqlite3_free (error_msg);
		goto out;
	}

	/* success */
	array = g_ptr_array_ref (array_tmp);
out:
	g_ptr_array_unref (array_tmp);
	g_free (statement);
	return array;
}

/*
 * ai_database_search_by_name_locale:
 */
GPtrArray *
ai_database_search_by_name_locale (AiDatabase *database, const gchar *value, const gchar *locale, GError **error)
{
	gchar *statement = NULL;
	gint rc;
	gchar *error_msg;
	GPtrArray *array = NULL;
	GPtrArray *array_tmp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		goto out;
	}

	/* create array */
	array_tmp = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	/* check that there are no existing entries from this repo */
	statement = g_strdup_printf ("SELECT a.application_id, a.package_name, a.categories, "
				     "a.repo_id, a.icon_name, "
				     "a.rating, a.screenshot_url, a.installed, "
				     "COALESCE(t.application_name, a.application_name), "
				     "COALESCE(t.application_summary, a.application_summary) "
				     "FROM applications a LEFT JOIN translations t ON a.application_id = t.application_id AND t.locale = '%s' "
				     "WHERE a.application_name LIKE '%%%s%%'", locale, value);
	rc = sqlite3_exec (priv->db, statement, ai_database_search_sqlite_cb, (void*) array_tmp, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", sqlite3_errmsg (priv->db));
		sqlite3_free (error_msg);
		goto out;
	}

	/* success */
	array = g_ptr_array_ref (array_tmp);
out:
	g_ptr_array_unref (array_tmp);
	g_free (statement);
	return array;
}

/*
 * ai_database_import:
 */
gboolean
ai_database_import (AiDatabase *database, const gchar *filename, guint *value, GError **error)
{
	gboolean ret = TRUE;
	gchar *contents = NULL;
	gint rc;
	gchar *error_msg;
	gchar **lines = NULL;
	guint i;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* get all the sql from the source file */
	ret = g_file_get_contents (filename, &contents, NULL, error);
	if (!ret)
		goto out;

	/* split into lines, so we can do the query in smaller lumps */
	lines = g_strsplit (contents, "\n", -1);
	for (i=0; lines[i] != NULL; i++) {

		/* copy all the applications and translations into remote db */
		rc = sqlite3_exec (priv->db, lines[i], NULL, NULL, &error_msg);
		if (rc == SQLITE_OK) {
			/* success */
			if (value != NULL)
				(*value)++;
		} else {
			g_set_error (error, 1, 0, "SQL error: %s, '%s'\n", error_msg, lines[i]);
			sqlite3_free (error_msg);
			ret = FALSE;
			goto out;
		}
	}
out:
	g_free (contents);
	g_strfreev (lines);
	return ret;
}

/*
 * ai_database_add_translation:
 */
gboolean
ai_database_add_translation (AiDatabase *database,
			     const gchar *application_id,
			     const gchar *name,
			     const gchar *summary,
			     const gchar *locale,
			     GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	gchar *statement = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* generate SQL */
	statement = sqlite3_mprintf ("INSERT INTO translations (application_id, application_name, application_summary, locale) "
				     "VALUES (%Q, %Q, %Q, %Q);", application_id, name, summary, locale);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't add translation: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
out:
	sqlite3_free (statement);
	return ret;
}

/*
 * ai_database_add_application:
 */
gboolean
ai_database_add_application (AiDatabase *database,
			     const gchar *application_id,
			     const gchar *package,
			     const gchar *categories,
			     const gchar *repo,
			     const gchar *icon,
			     const gchar *name,
			     const gchar *summary,
			     GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	gchar *statement = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (application_id != NULL, FALSE);
	g_return_val_if_fail (package != NULL, FALSE);
	g_return_val_if_fail (repo != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* generate SQL */
	statement = sqlite3_mprintf ("INSERT INTO applications (application_id, package_name, categories, "
				     "repo_id, icon_name, application_name, application_summary) "
				     "VALUES (%Q, %Q, %Q, %Q, %Q, %Q, %Q);",
				     application_id, package, categories, repo, icon, name, summary);
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't add application: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
out:
	sqlite3_free (statement);
	return ret;
}

typedef struct {
	const gchar	*icondir;
	AiDatabase	*database;
} AiDatabaseTemp;

/**
 * ai_database_add_applications_sqlite_cb:
 **/
static gint
ai_database_add_applications_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint i;
	gchar *col;
	gchar *value;
	const gchar *application_id = NULL;
	const gchar *package = NULL;
	const gchar *categories = NULL;
	const gchar *repo = NULL;
	const gchar *icon = NULL;
	const gchar *name = NULL;
	const gchar *summary = NULL;
	AiDatabaseTemp *temp = (AiDatabaseTemp *) data;
	gboolean ret;
	gint retval = 0;
	GError *error = NULL;

	for (i=0; i<(guint)argc; i++) {
		col = col_name[i];
		value = argv[i];
		if (g_strcmp0 (col, "application_id") == 0)
			application_id = value;
		else if (g_strcmp0 (col, "package_name") == 0)
			package = value;
		else if (g_strcmp0 (col, "categories") == 0)
			categories = value;
		else if (g_strcmp0 (col, "repo_id") == 0)
			repo = value;
		else if (g_strcmp0 (col, "icon_name") == 0)
			icon = value;
		else if (g_strcmp0 (col, "application_name") == 0)
			name = value;
		else if (g_strcmp0 (col, "application_summary") == 0)
			summary = value;
	}

	/* add to the target database */
	egg_debug ("adding %s", application_id);
	ret = ai_database_add_application (temp->database, application_id, package, categories, repo, icon, name, summary, &error);
	if (!ret) {
		egg_warning ("failed to add: %s", error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}
out:
	return retval;
}

/**
 * ai_database_add_translations_sqlite_cb:
 **/
static gint
ai_database_add_translations_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint i;
	gchar *col;
	gchar *value;
	const gchar *application_id = NULL;
	const gchar *name = NULL;
	const gchar *summary = NULL;
	const gchar *locale = NULL;
	AiDatabaseTemp *temp = (AiDatabaseTemp *) data;
	gboolean ret;
	gint retval = 0;
	GError *error = NULL;

	for (i=0; i<(guint)argc; i++) {
		col = col_name[i];
		value = argv[i];
		if (g_strcmp0 (col, "application_id") == 0)
			application_id = value;
		else if (g_strcmp0 (col, "locale") == 0)
			locale = value;
		else if (g_strcmp0 (col, "application_name") == 0)
			name = value;
		else if (g_strcmp0 (col, "application_summary") == 0)
			summary = value;
	}

	/* add to the target database */
	ret = ai_database_add_translation (temp->database, application_id, name, summary, locale, &error);
	if (!ret) {
		egg_warning ("failed to add: %s", error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}
out:
	return retval;
}

/**
 * ai_database_copy_icons_sqlite_cb:
 **/
static gint
ai_database_copy_icons_sqlite_cb (void *data, gint argc, gchar **argv, gchar **col_name)
{
	guint i;
	gchar *col;
	gchar *value;
	const gchar *application_id = NULL;
	const gchar *icon_name = NULL;
	gchar *path;
	gchar *dest;
	GFile *file;
	GFile *remote;
	AiDatabaseTemp *temp = (AiDatabaseTemp *) data;
	gboolean ret;
	gchar *icon_name_full;
	GError *error = NULL;

	for (i=0; i<(guint)argc; i++) {
		col = col_name[i];
		value = argv[i];
		if (g_strcmp0 (col, "application_id") == 0)
			application_id = value;
		else if (g_strcmp0 (col, "icon_name") == 0)
			icon_name = value;
	}
	if (application_id == NULL || icon_name == NULL)
		goto out;

	egg_debug ("copying icon %s for application: %s", icon_name, application_id);
	icon_name_full = g_strdup_printf ("%s.png", icon_name);

	/* copy all icon sizes if they exist */
	for (i=0; icon_sizes[i] != NULL; i++) {
		path = g_build_filename (temp->icondir, icon_sizes[i], icon_name_full, NULL);
		ret = g_file_test (path, G_FILE_TEST_EXISTS);
		if (ret) {
			dest = g_build_filename (temp->database->priv->icon_path, icon_sizes[i], icon_name_full, NULL);
			egg_debug ("copying file %s to %s", path, dest);
			file = g_file_new_for_path (path);
			remote = g_file_new_for_path (dest);
			ret = g_file_copy (file, remote, G_FILE_COPY_TARGET_DEFAULT_PERMS, NULL, NULL, NULL, &error);
			if (!ret) {
				egg_warning ("cannot copy %s: %s", path, error->message);
				g_clear_error (&error);
			}
			g_object_unref (file);
			g_object_unref (remote);
			g_free (dest);
		} else {
			egg_debug ("failed to find icon %s", path);
		}
		g_free (path);
	}
	g_free (icon_name_full);
out:
	return 0;
}

/*
 * ai_database_import_by_name:
 */
gboolean
ai_database_import_by_name (AiDatabase *database,
			    const gchar *filename,
			    const gchar *icondir,
			    const gchar *name,
			    guint *value,
			    GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	gchar *statement = NULL;
	gchar *error_msg;
	sqlite3	*foreign_db = NULL;
	AiDatabaseTemp *temp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* open database */
	rc = sqlite3_open (filename, &foreign_db);
	if (rc) {
		g_set_error (error, 1, 0, "Can't open database: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* use a temp object as we can only pass one pointer */
	temp = g_new0 (AiDatabaseTemp, 1);
	temp->icondir = icondir;
	temp->database = database;

	/* select all the application data for these packages */
	statement = g_strdup_printf ("SELECT application_id, package_name, categories, repo_id, icon_name, application_name, application_summary "
				     "FROM applications WHERE package_name = '%s'", name);
	rc = sqlite3_exec (foreign_db, statement, ai_database_add_applications_sqlite_cb, (void*) temp, &error_msg);
	g_free (statement);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
		sqlite3_free (error_msg);
		ret = FALSE;
		goto out;
	}

	/* select all the translation data for these packages */
	statement = g_strdup_printf ("SELECT application_id, application_name, application_summary, locale FROM translations WHERE EXISTS ( "
				     "SELECT applications.application_id FROM applications WHERE "
				     "applications.application_id = applications.application_id AND applications.package_name = '%s')", name);
	rc = sqlite3_exec (foreign_db, statement, ai_database_add_translations_sqlite_cb, (void*) temp, &error_msg);
	g_free (statement);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
		sqlite3_free (error_msg);
		ret = FALSE;
		goto out;
	}

	/* copy all the icons */
	if (icondir != NULL) {
		statement = g_strdup_printf ("SELECT application_id, icon_name FROM applications WHERE package_name = '%s'", name);
		rc = sqlite3_exec (foreign_db, statement, ai_database_copy_icons_sqlite_cb, (void*) temp, &error_msg);
		g_free (statement);
		if (rc != SQLITE_OK) {
			g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
			sqlite3_free (error_msg);
			ret = FALSE;
			goto out;
		}
	}

	/* get additions */
	if (value != NULL)
		*value = 1;
out:
	g_free (temp);
	if (foreign_db != NULL)
		sqlite3_close (foreign_db);
	return ret;
}

/*
 * ai_database_import_by_repo:
 */
gboolean
ai_database_import_by_repo (AiDatabase *database,
			    const gchar *filename,
			    const gchar *icondir,
			    const gchar *repo,
			    guint *value,
			    GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	gchar *statement = NULL;
	gchar *error_msg;
	sqlite3	*foreign_db = NULL;
	AiDatabaseTemp *temp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (repo != NULL, FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* database does not exist */
	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		g_set_error (error, 1, 0, "The source filename '%s' could not be found", filename);
		ret = FALSE;
		goto out;
	}

	/* icondir do not exist */
	if (icondir != NULL && !g_file_test (icondir, G_FILE_TEST_IS_DIR)) {
		g_set_error (error, 1, 0, "The icon directory '%s' could not be found", icondir);
		ret = FALSE;
		goto out;
	}

	/* open database */
	rc = sqlite3_open (filename, &foreign_db);
	if (rc) {
		g_set_error (error, 1, 0, "Can't open database: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* use a temp object as we can only pass one pointer */
	temp = g_new0 (AiDatabaseTemp, 1);
	temp->icondir = icondir;
	temp->database = database;

	/* select all the application data for these packages */
	statement = g_strdup_printf ("SELECT application_id, package_name, categories, repo_id, icon_name, application_name, application_summary "
				     "FROM applications WHERE repo_id = '%s'", repo);
	rc = sqlite3_exec (foreign_db, statement, ai_database_add_applications_sqlite_cb, (void*) temp, &error_msg);
	g_free (statement);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
		sqlite3_free (error_msg);
		ret = FALSE;
		goto out;
	}

	/* select all the translation data for these packages */
	statement = g_strdup_printf ("SELECT application_id, application_name, application_summary, locale FROM translations WHERE EXISTS ( "
				     "SELECT applications.application_id FROM applications WHERE "
				     "applications.application_id = applications.application_id AND applications.repo_id = '%s')", repo);
	rc = sqlite3_exec (foreign_db, statement, ai_database_add_translations_sqlite_cb, (void*) temp, &error_msg);
	g_free (statement);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
		sqlite3_free (error_msg);
		ret = FALSE;
		goto out;
	}

	/* copy all the icons */
	if (icondir != NULL) {
		statement = g_strdup_printf ("SELECT application_id, icon_name FROM applications WHERE repo_id = '%s'", repo);
		rc = sqlite3_exec (foreign_db, statement, ai_database_copy_icons_sqlite_cb, (void*) temp, &error_msg);
		g_free (statement);
		if (rc != SQLITE_OK) {
			g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
			sqlite3_free (error_msg);
			ret = FALSE;
			goto out;
		}
	}

	/* get additions */
	if (value != NULL)
		*value = 1;
out:
	g_free (temp);
	if (foreign_db != NULL)
		sqlite3_close (foreign_db);
	return ret;
}

/*
 * ai_database_set_installed_by_id:
 */
gboolean
ai_database_set_installed_by_id (AiDatabase *database,
				 const gchar *application_id, gboolean value,
				 GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	gchar *statement = NULL;
	gchar *error_msg;
	AiDatabaseTemp *temp = NULL;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (AI_IS_DATABASE (database), FALSE);

	/* check database is in correct state */
	if (!priv->locked) {
		g_set_error (error, 1, 0, "database is not open");
		ret = FALSE;
		goto out;
	}

	/* check database is in correct state */
	if (priv->dbversion < 2) {
		g_set_error (error, 1, 0, "database is new enough to support this feature");
		ret = FALSE;
		goto out;
	}

	/* NULL means all */
	if (application_id == NULL)
		application_id = "*";

	/* copy all the icons */
	statement = g_strdup_printf ("UPDATE applications SET installed = '%i' WHERE application_id = '%s'", value, application_id);
	rc = sqlite3_exec (priv->db, statement, ai_database_copy_icons_sqlite_cb, (void*) temp, &error_msg);
	if (rc != SQLITE_OK) {
		g_set_error (error, 1, 0, "SQL error: %s\n", error_msg);
		sqlite3_free (error_msg);
		ret = FALSE;
		goto out;
	}
out:
	g_free (statement);
	return ret;
}

/*
 * ai_database_get_property:
 */
static void
ai_database_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	AiDatabase *database = AI_DATABASE (object);
	AiDatabasePrivate *priv = database->priv;

	switch (prop_id) {
	case PROP_LOCKED:
		g_value_set_boolean (value, priv->locked);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*
 * ai_database_set_property:
 */
static void
ai_database_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
//	AiDatabase *database = AI_DATABASE (object);
//	AiDatabasePrivate *priv = database->priv;

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*
 * ai_database_class_init:
 */
static void
ai_database_class_init (AiDatabaseClass *klass)
{
	GParamSpec *pspec;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = ai_database_finalize;
	object_class->get_property = ai_database_get_property;
	object_class->set_property = ai_database_set_property;

	/*
	 * AiDatabase:locked:
	 */
	pspec = g_param_spec_boolean ("locked", NULL, NULL,
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_LOCKED, pspec);

	g_type_class_add_private (klass, sizeof (AiDatabasePrivate));
}

/*
 * ai_database_init:
 */
static void
ai_database_init (AiDatabase *database)
{
	database->priv = AI_DATABASE_GET_PRIVATE (database);
	database->priv->filename = NULL;
	database->priv->icon_path = NULL;
}

/*
 * ai_database_finalize:
 */
static void
ai_database_finalize (GObject *object)
{
	AiDatabase *database = AI_DATABASE (object);
	AiDatabasePrivate *priv = database->priv;

	g_free (priv->filename);
	g_free (priv->icon_path);
	if (priv->locked) {
		egg_warning ("YOU HAVE TO MANUALLY CALL ai_database_close()!!!");
		sqlite3_close (priv->db);
	}

	G_OBJECT_CLASS (ai_database_parent_class)->finalize (object);
}

/*
 * ai_database_new:
 *
 * Return value: a new AiDatabase object.
 *
 * Since: 0.5.4
 */
AiDatabase *
ai_database_new (void)
{
	AiDatabase *database;
	database = g_object_new (AI_TYPE_DATABASE, NULL);
	return AI_DATABASE (database);
}
