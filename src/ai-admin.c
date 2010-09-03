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
#include <locale.h>

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
	gboolean force = FALSE;
	gboolean refresh_installed = FALSE;
	gboolean upgrade = FALSE;
	gboolean create = FALSE;
	GOptionContext *context;
	gchar *database = NULL;
	gchar *local_application_root = NULL;
	gint retval = 0;
	AiDatabase *db = NULL;
	gboolean ret;
	GError *error = NULL;

	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
		  _("Show extra debugging information"), NULL },
		{ "force", 'f', 0, G_OPTION_ARG_NONE, &force,
		  _("Force the action, even if it would lead to a loss of data"), NULL },
		{ "refresh-installed", 'r', 0, G_OPTION_ARG_NONE, &refresh_installed,
		  _("Refresh the installed status of all the applications"), NULL },
		{ "create", 'c', 0, G_OPTION_ARG_NONE, &create,
		  _("Create a new empty database"), NULL },
		{ "upgrade", 'u', 0, G_OPTION_ARG_NONE, &upgrade,
		  _("Attempt to upgrade the database to the latest format"), NULL },
		{ "database", 'd', 0, G_OPTION_ARG_STRING, &database,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  _("Database file to use (if not specififed, default is used)"), NULL},
		{ "local-application-root", 'd', 0, G_OPTION_ARG_STRING, &local_application_root,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  _("Local application directory location (if not specififed, default is used)"), NULL},
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
	ret = g_option_context_parse (context, &argc, &argv, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to parse command line"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}
	g_option_context_free (context);

	g_type_init ();
	egg_debug_init (verbose);

	/* ensure the mode is sane */
	if ((!upgrade && !create && !refresh_installed) || (upgrade && create && refresh_installed)) {
		g_print ("%s\n", _("You have to specify either --create, --upgrade or --refresh-installed"));
		retval = 1;
		goto out;
	}

	/* open database */
	db = ai_database_new ();
	ai_database_set_filename (db, database, NULL);
	ret = ai_database_open (db, FALSE, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to open"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	/* create it */
	if (create) {
		ret = ai_database_create (db, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to create"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
	}

	/* upgrade it */
	if (upgrade) {
		ret = ai_database_upgrade (db, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to create"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
	}

	/* refresh it */
	if (refresh_installed) {
		GDir *dir;
		const gchar *filename;
		gchar *application_id;

		/* set all applications as uninstalled */
		ret = ai_database_set_installed_by_id (db, NULL, FALSE, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to create"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}

		/* open directory */
		if (local_application_root == NULL)
			local_application_root = g_build_filename (DATADIR, "applications", NULL);
		dir = g_dir_open (local_application_root, 0, &error);
		if (dir == NULL) {
			g_print ("%s: %s\n", _("Failed to list applications directory"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}

		/* walk the directory tree looking for applications */
		filename = g_dir_read_name (dir);
		while (filename != NULL) {
			guint len;
			len = strlen (filename);
			if (len > 8) {
				application_id = g_strndup (filename, len - 8);
				egg_debug ("filename=%s/%s, %s", local_application_root, filename, application_id);

				/* set all applications as uninstalled */
				ret = ai_database_set_installed_by_id (db, application_id, TRUE, NULL);
				if (!ret) {
					g_print ("Failed to set installed attribute on %s, possibly the entry does not exist: %s\n", application_id, error->message);
					g_clear_error (&error);
				}

				g_free (application_id);
			}
			filename = g_dir_read_name (dir);
		}
		g_dir_close (dir);
	}

out:
	if (db != NULL) {
		error = NULL;
		ret = ai_database_close (db, FALSE, &error);
		if (!ret) {
			g_print ("%s: %s\n", _("Failed to close"), error->message);
			g_error_free (error);
			retval = 1;
		}
		g_object_unref (db);
	}
	g_free (local_application_root);
	g_free (database);
	return retval;
}

