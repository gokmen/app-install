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

