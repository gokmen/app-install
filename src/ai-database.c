/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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

#include "egg-debug.h"

#include "ai-database.h"
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
};

enum {
	PROP_0,
	PROP_LOCKED,
	PROP_LAST
};

G_DEFINE_TYPE (AiDatabase, ai_database, G_TYPE_OBJECT)

/*
 * ai_database_set_filename:
 */
void
ai_database_set_filename (AiDatabase *database, const gchar *filename)
{
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_if_fail (!priv->locked);

	g_free (priv->filename);

	/* use default */
	if (filename == NULL) {
		egg_debug ("database not specified, using %s", AI_DEFAULT_DATABASE);
		priv->filename = g_strdup (AI_DEFAULT_DATABASE);
	} else {
		priv->filename = g_strdup (filename);
	}
}

/*
 * ai_database_set_icon_path:
 */
void
ai_database_set_icon_path (AiDatabase *database, const gchar *icon_path)
{
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_if_fail (!priv->locked);

	g_free (priv->icon_path);

	/* use default */
//	if (icon_path == NULL) {
//		egg_debug ("database not specified, using %s", AI_DEFAULT_ICONDIR);
//		priv->icon_path = g_strdup (AI_DEFAULT_ICONDIR);
//	} else {
		priv->icon_path = g_strdup (icon_path);
//	}

	/* check it exists */
	if (priv->icon_path != NULL && !g_file_test (priv->icon_path, G_FILE_TEST_IS_DIR))
		egg_warning ("The icon directory '%s' could not be found", priv->icon_path);
}

/*
 * ai_database_open:
 */
gboolean
ai_database_open (AiDatabase *database, GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	const gchar *statement;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (!priv->locked, FALSE);

	/* open database */
	rc = sqlite3_open (priv->filename, &priv->db);
	if (rc) {
		g_set_error (error, 1, 0, "Can't open database: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	/* don't sync */
	statement = "PRAGMA synchronous=OFF";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't turn off sync: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}
out:
	return ret;
}

/*
 * ai_database_close:
 */
gboolean
ai_database_close (AiDatabase *database, GError **error)
{
	gboolean ret = TRUE;
	gint rc;
	const gchar *statement;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (!priv->locked, FALSE);

	/* reclaim memory */
	statement = "VACUUM";
	rc = sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	if (rc) {
		g_set_error (error, 1, 0, "Can't vanuum: %s\n", sqlite3_errmsg (priv->db));
		ret = FALSE;
		goto out;
	}

	sqlite3_close (priv->db);

//	/* if the database file was not installed (or was nuked) recreate it */
//	create_file = g_file_test (database, G_FILE_TEST_EXISTS);
//	if (create_file == TRUE) {
//		egg_warning ("already exists");
//		goto out;
//	}

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

	g_return_val_if_fail (!priv->locked, FALSE);

	/* create applications */
	statement = "CREATE TABLE applications ("
		    "application_id TEXT primary key,"
		    "package_name TEXT,"
		    "categories TEXT,"
		    "repo_id TEXT,"
		    "icon_name TEXT,"
		    "application_name TEXT,"
		    "application_summary TEXT);";
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
	gchar *statement;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (!priv->locked, FALSE);

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
	gchar *statement;
	gint rc;
	gchar *error_msg;
	AiDatabasePrivate *priv = AI_DATABASE (database)->priv;

	g_return_val_if_fail (!priv->locked, FALSE);

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

