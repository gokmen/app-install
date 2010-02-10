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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>

#include "ai-common.h"
#include "ai-database.h"

#include "egg-debug.h"

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gboolean verbose = FALSE;
	GOptionContext *context;
	gint retval = 0;
	gchar *database = NULL;
	gchar *repo = NULL;
	gchar *package = NULL;
	gchar *icondir = NULL;
	gchar *source_database = NULL;
	gchar *source_icondir = NULL;
	guint number = 0;
	gboolean ret;
	GError *error = NULL;
	AiDatabase *db = NULL;

	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
		  _("Show extra debugging information"), NULL },
		{ "database", 'd', 0, G_OPTION_ARG_STRING, &database,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  _("Database file to use (if not specififed, default is used)"), NULL},
		{ "source-database", '\0', 0, G_OPTION_ARG_STRING, &source_database,
		  /* TRANSLATORS: the source database, typically used for adding */
		  _("Source database file to add to the main database"), NULL},
		{ "icondir", 'i', 0, G_OPTION_ARG_STRING, &icondir,
		  /* TRANSLATORS: the icon directory */
		  _("Icon directory"), NULL},
		{ "source-icondir", '\0', 0, G_OPTION_ARG_STRING, &source_icondir,
		  /* TRANSLATORS: the icon directory */
		  _("Icon directory"), NULL},
		{ "repo", 'r', 0, G_OPTION_ARG_STRING, &repo,
		  /* TRANSLATORS: the repo of the software source, e.g. fedora */
		  _("Name of the remote repo"), NULL},
		{ "package", 'p', 0, G_OPTION_ARG_STRING, &package,
		  /* TRANSLATORS: the package name, e.g. kernel */
		  _("Name of the package"), NULL},
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

	egg_debug ("database=%s, source_database=%s, repo=%s, icondir=%s", database, source_database, repo, icondir);

	if (repo == NULL && package == NULL) {
		g_print ("%s\n", _("Please specify --repo or --package"));
		retval = 1;
		goto out;
	}
	if (source_database == NULL) {
		g_print ("%s\n", _("A source database filename is required"));
		retval = 1;
		goto out;
	}

	/* open database */
	db = ai_database_new ();
	ai_database_set_filename (db, database);
	ai_database_set_icon_path (db, icondir);
	ret = ai_database_open (db, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to open"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	if (repo != NULL) {
		/* create it */
		ret = ai_database_query_by_repo (db, repo, &number, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to query by repo"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}

		/* already have data for this repo */
		if (number > 0) {
			egg_warning ("There are already %i entries for repo %s", number, repo);
			goto out;
		}

		/* import it */
		ret = ai_database_import_by_repo (db, source_database, source_icondir, repo, &number, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to create"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
		egg_debug ("%i additions to the database", number);
	}

	if (package != NULL) {
		/* create it */
		ret = ai_database_query_by_name (db, package, &number, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to query by name"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}

		/* already have data for this name */
		if (number > 0) {
			egg_warning ("There are already %i entries for name %s", number, package);
			goto out;
		}

		/* import it */
		ret = ai_database_import_by_name (db, source_database, source_icondir, package, &number, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to create"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
		egg_debug ("%i additions to the database", number);
	}

	/* close it */
	ret = ai_database_close (db, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to close"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}
out:
	if (db != NULL)
		g_object_unref (db);
	g_free (package);
	g_free (repo);
	g_free (database);
	g_free (icondir);
	g_free (source_database);
	g_free (source_icondir);
	return 0;
}

