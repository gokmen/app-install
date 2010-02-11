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
	gchar *database = NULL;
	gint retval = 0;
	AiDatabase *db = NULL;
	gboolean ret;
	GError *error = NULL;

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
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	g_type_init ();
	egg_debug_init (verbose);

	/* open database */
	db = ai_database_new ();
	ai_database_set_filename (db, database);
	ret = ai_database_open (db, FALSE, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to open"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
	}

	/* create it */
	ret = ai_database_create (db, &error);
	if (!ret) {
		g_print ("%s: %s\n", _("Failed to create"), error->message);
		g_error_free (error);
		retval = 1;
		goto out;
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
	if (db != NULL)
		g_object_unref (db);
	g_free (database);
	return retval;
}

