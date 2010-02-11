/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Richard Hughes <richard@hughsie.com>
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

//#include <string.h>
#include <glib.h>
//#include <glib/gstdio.h>

#include "ai-utils.h"

/*
 * main:
 */
int
main (int argc, char *argv[])
{
	gboolean ret;
	gint retval = 0;
	GOptionContext *context;
	gchar *package = NULL;
	gchar *directory = NULL;
	GError *error = NULL;

	const GOptionEntry options[] = {
		{ "package", 'p', 0, G_OPTION_ARG_STRING, &package,
		  /* TRANSLATORS: if we are specifing a out-of-tree database */
		  "Package to decompress", NULL},
		{ "directory", 'd', 0, G_OPTION_ARG_STRING, &directory,
		  /* TRANSLATORS: the icon directory */
		  "Directory to decompress to", NULL},
		{ NULL}
	};

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, "Extract an rpm, fast");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	/* check */
	if (package == NULL || directory == NULL) {
		g_print ("%s\n", "missing --package or --directory");
		goto out;
	}

	/* extract it */
	ret = ai_utils_extract_archive (package, directory, &error);
	if (!ret) {
		g_print ("%s: %s\n", "failed to decompress", error->message);
		retval = 1;
		goto out;
	}
out:
	g_free (package);
	g_free (directory);
	return retval;
}

