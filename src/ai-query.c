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
#include "ai-result.h"

#include "egg-debug.h"

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gboolean verbose = FALSE;
	GOptionContext *context;
	gchar *database = NULL;
	gint retval = 0;
	AiDatabase *db = NULL;
	gboolean ret;
	GError *error = NULL;
	GPtrArray *array = NULL;
	guint i;
	AiResult *result;
	const gchar *locale;

	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
		  _("Show extra debugging information"), NULL },
		{ "database", 'd', 0, G_OPTION_ARG_STRING, &database,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  _("Database file to use (if not specififed, default is used)"), NULL},
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

	/* enough arguments */
	if (argc != 3) {
		g_print ("Arguments have to be app-install-query [id|name] search-term\n");
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

	/* get locale */
	locale = setlocale (LC_ALL, NULL);

	/* mode */
	if (g_strcmp0 (argv[1], "id") == 0) {
		array = ai_database_search_by_id_locale (db, argv[2], locale, &error);
		if (array == NULL) {
			g_print ("%s: %s\n", _("Failed to search"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
	} else if (g_strcmp0 (argv[1], "name") == 0) {
		array = ai_database_search_by_name_locale (db, argv[2], locale, &error);
		if (array == NULL) {
			g_print ("%s: %s\n", _("Failed to search"), error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}
	} else {
		g_print ("%s\n", ("Incorrect search term"));
		retval = 1;
		goto out;
	}

	/* no results */
	if (array->len == 0) {
		g_print ("No results found\n");
	} else {
		g_print ("Results for '%s':\n", argv[2]);
		for (i=0; i < array->len; i++) {
			result = g_ptr_array_index (array, i);
			g_print ("% 3i: %s\n", i,
				 ai_result_get_application_id (result));
			g_print ("      %s: %s\n", _("Application Name"), ai_result_get_application_name (result));
			g_print ("      %s: %s\n", _("Application Summary"), ai_result_get_application_summary (result));
			g_print ("      %s: %s\n", _("Package Name"), ai_result_get_package_name (result));
			g_print ("      %s: %s\n", _("Categories"), ai_result_get_categories (result));
			g_print ("      %s: %s\n", _("Repository ID"), ai_result_get_repo_id (result));
			g_print ("      %s: %s\n", _("Icon Name"), ai_result_get_icon_name (result));
			g_print ("      %s: %i\n", _("Rating"), ai_result_get_rating (result));
			g_print ("      %s: %s\n", _("Screenshot"), ai_result_get_screenshot_url (result));
			g_print ("      %s: %s\n", _("Installed"), ai_result_get_installed (result) ? "TRUE" : "FALSE");
		}
	}

	/* close it */
	ret = ai_database_close (db, FALSE, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to close"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}
out:
	if (array != NULL)
		g_ptr_array_unref (array);
	if (db != NULL)
		g_object_unref (db);
	g_free (database);
	return retval;
}

