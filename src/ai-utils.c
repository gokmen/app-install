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

#include <glib.h>
#include <glib/gstdio.h>

#include <archive.h>
#include <archive_entry.h>

#define BLOCK_SIZE	(1024 * 4 * 10) /* bytes */

#include "ai-utils.h"

/*
 * ai_utils_directory_remove:
 *
 * Removes the contents of a directory, but not the directory itself
 */
static gboolean
ai_utils_directory_remove_contents (const gchar *directory)
{
	gboolean ret = FALSE;
	GDir *dir;
	GError *error = NULL;
	const gchar *filename;
	gchar *src;
	gint retval;

	/* try to open */
	dir = g_dir_open (directory, 0, &error);
	if (dir == NULL) {
		g_warning ("cannot open directory: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* find each */
	while ((filename = g_dir_read_name (dir))) {
		src = g_build_filename (directory, filename, NULL);
		ret = g_file_test (src, G_FILE_TEST_IS_DIR);
		if (ret) {
			/* recurse */
			ai_utils_directory_remove_contents (src);
			retval = g_remove (src);
			if (retval != 0)
				g_warning ("failed to delete %s", src);
		} else {
			retval = g_unlink (src);
			if (retval != 0)
				g_warning ("failed to delete %s", src);
		}
		g_free (src);
	}
	g_dir_close (dir);
	ret = TRUE;
out:
	return ret;
}

/*
 * ai_utils_directory_remove:
 *
 * Removes a directory and all it's contents
 */
gboolean
ai_utils_directory_remove (const gchar *directory)
{
	gint retval;

	if (!g_file_test (directory, G_FILE_TEST_IS_DIR))
		return TRUE;

	ai_utils_directory_remove_contents (directory);
	retval = g_remove (directory);
	if (retval != 0)
		g_warning ("failed to delete %s", directory);
	return TRUE;
}

/*
 * ai_utils_extract_archive:
 *
 * Extracts an archive to a directory, fast.
 */
gboolean
ai_utils_extract_archive (const gchar *filename, const gchar *directory, GError **error)
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

