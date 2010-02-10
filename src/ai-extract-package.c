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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <archive.h>
#include <archive_entry.h>

#define BLOCK_SIZE	(1024 * 4 * 10) /* bytes */

/*
 * ai_extract_archive:
 *
 * Extracts an archive to a directory, fast.
 */
static gboolean
ai_extract_archive (const gchar *filename, const gchar *directory, GError **error)
{
	gboolean ret = FALSE;
	struct archive *arch = NULL;
	struct archive_entry *entry;
	int r;
	int retval;
	gchar *retcwd;
	gchar buf[PATH_MAX];

	/* save the PWD as we chdir to extract */
	retcwd = getcwd (buf, PATH_MAX);
	if (retcwd == NULL) {
		g_set_error_literal (error, 1, 0, "failed to get cwd");
		goto out;
	}

	/* we can only read tar achives */
	arch = archive_read_new ();
	archive_read_support_format_all (arch);
	archive_read_support_compression_all (arch);

	/* open the tar file */
	r = archive_read_open_file (arch, filename, BLOCK_SIZE);
	if (r) {
		g_set_error (error, 1, 0, "cannot open: %s", archive_error_string (arch));
		goto out;
	}

	/* switch to our destination directory */
	retval = chdir (directory);
	if (retval != 0) {
		g_set_error (error, 1, 0, "failed chdir to %s", directory);
		goto out;
	}

	/* decompress each file */
	for (;;) {
		r = archive_read_next_header (arch, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			g_set_error (error, 1, 0, "cannot read header: %s", archive_error_string (arch));
			goto out;
		}
		r = archive_read_extract (arch, entry, 0);
		if (r != ARCHIVE_OK) {
			g_set_error (error, 1, 0, "cannot extract: %s", archive_error_string (arch));
			goto out;
		}
	}

	/* completed all okay */
	ret = TRUE;
out:
	/* close the archive */
	if (arch != NULL) {
		archive_read_close (arch);
		archive_read_finish (arch);
	}

	/* switch back to PWD */
	retval = chdir (buf);
	if (retval != 0)
		g_warning ("cannot chdir back!");

	return ret;
}

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
	ret = ai_extract_archive (package, directory, &error);
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

