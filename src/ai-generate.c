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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "ai-common.h"
#include "ai-database.h"

#include "egg-debug.h"

static const gchar *icon_sizes[] = { "22x22", "24x24", "32x32", "48x48", "scalable", NULL };

typedef struct {
	gchar	*key;
	gchar	*value;
	gchar	*locale;
} AppInstallData;

/**
 * ai_generate_desktop_data_free:
 **/
static void
ai_generate_desktop_data_free (AppInstallData *data)
{
	g_free (data->key);
	g_free (data->value);
	g_free (data->locale);
	g_free (data);
}

/**
 * ai_generate_create_icon_directories:
 **/
static gboolean
ai_generate_create_icon_directories (const gchar *directory)
{
	gboolean ret;
	GError *error = NULL;
	GFile *file;
	gchar *path;
	guint i;

	/* create main directory */
	ret = g_file_test (directory, G_FILE_TEST_IS_DIR);
	if (!ret) {
		file = g_file_new_for_path (directory);
		ret = g_file_make_directory (file, NULL, &error);
		g_object_unref (file);
		if (!ret) {
			egg_warning ("cannot create %s: %s", path, error->message);
			g_error_free (error);
			goto out;
		}
	}

	/* make sub directories */
	for (i=0; icon_sizes[i] != NULL; i++) {
		path = g_build_filename (directory, icon_sizes[i], NULL);
		ret = g_file_test (path, G_FILE_TEST_IS_DIR);
		if (!ret) {
			egg_debug ("creating %s", path);
			file = g_file_new_for_path (path);
			ret = g_file_make_directory (file, NULL, &error);
			if (!ret) {
				egg_warning ("cannot create %s: %s", path, error->message);
				g_clear_error (&error);
			}
			g_object_unref (file);
		}
		g_free (path);
	}
out:
	return ret;
}

/**
 * ai_generate_get_desktop_data:
 **/
static GPtrArray *
ai_generate_get_desktop_data (const gchar *filename)
{
	gboolean ret;
	GError *error = NULL;
	GPtrArray *data = NULL;
	gchar *contents = NULL;
	gchar **lines;
	gchar **parts;
	guint i, len;
	AppInstallData *obj;

	/* get all the contents */
	ret = g_file_get_contents (filename, &contents, NULL, &error);
	if (!ret) {
		egg_warning ("cannot read source file: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* split lines and extract data */
	data = g_ptr_array_new ();
	lines = g_strsplit (contents, "\n", -1);
	for (i=0; lines[i] != NULL; i++) {
		parts = g_strsplit_set (lines[i], "=[]", -1);
		len = g_strv_length (parts);
		if (len == 2) {
			obj = g_new0 (AppInstallData, 1);
			obj->key = g_strdup (parts[0]);
			obj->value = g_strdup (parts[1]);
			g_ptr_array_add (data, obj);
		} else if (len == 4) {
			obj = g_new0 (AppInstallData, 1);
			obj->key = g_strdup (parts[0]);
			obj->locale = g_strdup (parts[1]);
			obj->value = g_strdup (parts[3]);
			g_ptr_array_add (data, obj);
		}
		g_strfreev (parts);
	}
	g_strfreev (lines);
	g_free (contents);
out:
	return data;
}

/**
 * ai_generate_get_value_for_locale:
 **/
static gchar *
ai_generate_get_value_for_locale (GPtrArray *data, const gchar *key, const gchar *locale)
{
	guint i;
	gchar *value = NULL;
	const AppInstallData *obj;

	/* find data matching key name and locale */
	for (i=0; i<data->len; i++) {
		obj = g_ptr_array_index (data, i);
		if (g_strcmp0 (key, obj->key) == 0 && g_strcmp0 (locale, obj->locale) == 0) {
			value = g_strdup (obj->value);
			break;
		}
	}
	return value;
}

/**
 * ai_generate_get_locales:
 **/
static GPtrArray *
ai_generate_get_locales (GPtrArray *data)
{
	guint i, j;
	GPtrArray *locales;
	const AppInstallData *obj;

	/* find data matching key name and locale */
	locales = g_ptr_array_new ();
	for (i=0; i<data->len; i++) {
		obj = g_ptr_array_index (data, i);

		/* no point */
		if (obj->locale == NULL)
			continue;

		/* is already in locale list */
		for (j=0; j<locales->len; j++) {
			if (g_strcmp0 (obj->locale, g_ptr_array_index (locales, j)) == 0)
				break;
		}
		/* not already there */
		if (j == locales->len)
			g_ptr_array_add (locales, g_strdup (obj->locale));
	}
	return locales;
}

/**
 * ai_generate_get_application_id:
 **/
static gchar *
ai_generate_get_application_id (const gchar *filename)
{
	gchar *find;
	gchar *application_id;

	find = g_strrstr (filename, "/");
	application_id = g_strdup (find+1);
	find = g_strrstr (application_id, ".");
	*find = '\0';
	return application_id;
}

/**
 * ai_generate_applications_sql:
 **/
static gboolean
ai_generate_applications_sql (AiDatabase *db, GPtrArray *data, const gchar *repo, const gchar *package, const gchar *application_id, GError **error)
{
	gchar *name = NULL;
	gchar *comment = NULL;
	gchar *icon_name = NULL;
	gchar *categories = NULL;
	gboolean ret;

	name = ai_generate_get_value_for_locale (data, "Name", NULL);
	icon_name = ai_generate_get_value_for_locale (data, "Icon", NULL);
	comment = ai_generate_get_value_for_locale (data, "Comment", NULL);
	categories = ai_generate_get_value_for_locale (data, "Categories", NULL);

	/* remove invalid icons */
	if (icon_name != NULL &&
	    (g_str_has_prefix (icon_name, "/") ||
	     g_str_has_suffix (icon_name, ".png"))) {
		g_free (icon_name);
		icon_name = NULL;
	}

	egg_debug ("application_id=%s, name=%s, comment=%s, icon=%s, categories=%s", application_id, name, comment, icon_name, categories);
	ret = ai_database_add_application (db, application_id, package, categories, repo, icon_name, name, comment, error);
	if (!ret)
		goto out;
out:
	g_free (name);
	g_free (comment);
	g_free (icon_name);
	g_free (categories);
	return ret;
}

/**
 * ai_generate_translations_sql:
 **/
static gboolean
ai_generate_translations_sql (AiDatabase *db, GPtrArray *data, GPtrArray *locales, const gchar *application_id, GError **error)
{
	gchar *name = NULL;
	gchar *comment = NULL;
	const gchar *locale;
	guint i;
	gboolean ret = TRUE;

	for (i=0; i<locales->len; i++) {
		locale = g_ptr_array_index (locales, i);
		name = ai_generate_get_value_for_locale (data, "Name", locale);
		comment = ai_generate_get_value_for_locale (data, "Comment", locale);

		/* append the application data to the sql string if either not null */
		if (name != NULL || comment != NULL) {
			ret = ai_database_add_translation (db, application_id, name, comment, locale, error);
			if (!ret)
				goto out;
		}
		g_free (name);
		g_free (comment);
	}
out:
	return ret;
}

/**
 * ai_generate_copy_icons:
 **/
static gboolean
ai_generate_copy_icons (const gchar *root, const gchar *directory, const gchar *icon_name)
{
	gboolean ret;
	GError *error = NULL;
	GFile *file;
	GFile *remote;
	gchar *dest;
	gchar *iconpath;
	gchar *icon_name_full;
	guint i;

	/* copy all icon sizes if they exist */
	for (i=0; icon_sizes[i] != NULL; i++) {

		/* get the icon name */
		if (g_strcmp0 (icon_sizes[i], "scalable") == 0)
			icon_name_full = g_strdup_printf ("%s.svg", icon_name);
		else
			icon_name_full = g_strdup_printf ("%s.png", icon_name);

		/* build the icon path */
		iconpath = g_build_filename (root, "/usr/share/icons/hicolor", icon_sizes[i], "apps", icon_name_full, NULL);
		ret = g_file_test (iconpath, G_FILE_TEST_EXISTS);
		if (ret) {
			dest = g_build_filename (directory, icon_sizes[i], icon_name_full, NULL);
			egg_debug ("copying file %s to %s", iconpath, dest);
			file = g_file_new_for_path (iconpath);
			remote = g_file_new_for_path (dest);
			ret = g_file_copy (file, remote, G_FILE_COPY_TARGET_DEFAULT_PERMS | G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);
			if (!ret) {
				egg_warning ("cannot copy %s: %s", dest, error->message);
				g_clear_error (&error);
			}
			g_object_unref (file);
			g_object_unref (remote);
			g_free (dest);
		} else {
			egg_debug ("does not exist: %s, so not copying", iconpath);
		}
		g_free (iconpath);
		g_free (icon_name_full);
	}
	return TRUE;
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gboolean verbose = FALSE;
	GOptionContext *context;
	gint retval = 0;
	gchar *repo = NULL;
	gchar *root = NULL;
	gchar *desktopfile = NULL;
	gchar *icondir = NULL;
	gchar *package = NULL;
	gboolean ret;
	GError *error = NULL;
	GPtrArray *data = NULL;
	gchar *application_id = NULL;
	gchar *icon_name = NULL;
	GPtrArray *locales = NULL;
	gchar *filename = NULL;
	AiDatabase *db = NULL;
	gchar *database = NULL;

	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
		  _("Show extra debugging information"), NULL },
		{ "database", 'd', 0, G_OPTION_ARG_STRING, &database,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  _("Database file to use (if not specififed, default is used)"), NULL},
		{ "root", 'r', 0, G_OPTION_ARG_STRING, &root,
		  /* TRANSLATORS: the root database, typically used for adding */
		  _("The root directory of the package data"), NULL},
		{ "desktopfile", 'f', 0, G_OPTION_ARG_STRING, &desktopfile,
		  /* TRANSLATORS: the icon directory */
		  _("Desktop file in the root"), NULL},
		{ "package", 'p', 0, G_OPTION_ARG_STRING, &package,
		  /* TRANSLATORS: the repo of the software root, e.g. fedora */
		  _("Name of the package"), NULL},
		{ "icondir", 'i', 0, G_OPTION_ARG_STRING, &icondir,
		  /* TRANSLATORS: the icon directory */
		  _("Icon directory"), NULL},
		{ "repo", 'n', 0, G_OPTION_ARG_STRING, &repo,
		  /* TRANSLATORS: the repo of the software root, e.g. fedora */
		  _("Name of the remote repo"), NULL},
		{ NULL}
	};

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (NULL);
	/* TRANSLATORS: tool that gets called when the command is not found */
	g_option_context_set_summary (context, _("Application Database Installer"));
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	g_type_init ();
	egg_debug_init (verbose);

	/* things we require */
	if (repo == NULL) {
		g_print ("A repo name is required\n");
		retval = 1;
		goto out;
	}
	if (icondir == NULL) {
		g_print ("A icon directory is required\n");
		retval = 1;
		goto out;
	}
	if (desktopfile == NULL) {
		g_print ("A desktop file is required\n");
		retval = 1;
		goto out;
	}
	if (package == NULL) {
		g_print ("A package name is required\n");
		retval = 1;
		goto out;
	}

	/* use defaults */
	if (root == NULL) {
		egg_debug ("root not specified, using /");
		root = g_strdup ("/");
	}

	/* check directories exist */
	if (!g_file_test (root, G_FILE_TEST_IS_DIR)) {
		g_print ("The root filename '%s' could not be found\n", root);
		retval = 1;
		goto out;
	}
	if (!g_file_test (icondir, G_FILE_TEST_IS_DIR)) {
		g_print ("The icon output directory '%s' could not be found\n", icondir);
		retval = 1;
		goto out;
	}

	/* open database */
	db = ai_database_new ();
	ai_database_set_filename (db, database, NULL);
	ret = ai_database_open (db, TRUE, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to open"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	/* generate the sub directories in the icondir if they dont exist */
	ai_generate_create_icon_directories (icondir);

	if (desktopfile[0] != '/') {
		filename = g_build_filename (root, "/usr/share/applications", desktopfile, NULL);
	} else {
		filename = g_build_filename (root, desktopfile, NULL);
	}
	egg_debug ("filename: %s", filename);

	/* get app-id */
	application_id = ai_generate_get_application_id (filename);

	/* extract data */
	data = ai_generate_get_desktop_data (filename);
	if (data == NULL) {
		g_print ("Could not get desktop data from %s\n", filename);
		retval = 1;
		goto out;
	}

	/* form application SQL */
	ret = ai_generate_applications_sql (db, data, repo, package, application_id, &error);
	if (!ret) {
		g_print ("Could not generate application data for %s: %s\n", package, error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	/* get list of locales in this file */
	locales = ai_generate_get_locales (data);
	if (locales == NULL) {
		g_print ("Could not get locale data from %s\n", filename);
		retval = 1;
		goto out;
	}

	/* form translations SQL */
	ret = ai_generate_translations_sql (db, data, locales, application_id, &error);
	if (!ret) {
		g_print ("Could not generate translation data for %s: %s\n", package, error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	/* copy icons */
	icon_name = ai_generate_get_value_for_locale (data, "Icon", NULL);
	if (icon_name != NULL && !g_str_has_suffix (icon_name, ".png"))
		ai_generate_copy_icons (root, icondir, icon_name);

out:
	/* close it */
	ret = ai_database_close (db, FALSE, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to close"), error->message);
		g_error_free (error);
		retval = 1;
	}
	if (db != NULL)
		g_object_unref (db);
	if (locales != NULL) {
		g_ptr_array_foreach (locales, (GFunc) g_free, NULL);
		g_ptr_array_free (locales, TRUE);
	}
	if (data != NULL) {
		g_ptr_array_foreach (data, (GFunc) ai_generate_desktop_data_free, NULL);
		g_ptr_array_free (data, TRUE);
	}
	g_free (icondir);
	g_free (database);
	g_free (icon_name);
	g_free (package);
	g_free (filename);
	g_free (application_id);
	g_free (repo);
	g_free (root);
	g_free (desktopfile);
	return retval;
}

