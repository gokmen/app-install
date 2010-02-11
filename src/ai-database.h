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
 * GNU General Public License for more database.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __AI_DATABASE_H
#define __AI_DATABASE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define AI_TYPE_DATABASE		(ai_database_get_type ())
#define AI_DATABASE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), AI_TYPE_DATABASE, AiDatabase))
#define AI_DATABASE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), AI_TYPE_DATABASE, AiDatabaseClass))
#define AI_IS_DATABASE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), AI_TYPE_DATABASE))
#define AI_IS_DATABASE_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), AI_TYPE_DATABASE))
#define AI_DATABASE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), AI_TYPE_DATABASE, AiDatabaseClass))

typedef struct _AiDatabasePrivate	AiDatabasePrivate;
typedef struct _AiDatabase		AiDatabase;
typedef struct _AiDatabaseClass		AiDatabaseClass;

struct _AiDatabase
{
	 GObject		 parent;
	 AiDatabasePrivate	*priv;
};

struct _AiDatabaseClass
{
	GObjectClass		 parent_class;
};

GType		 ai_database_get_type		  	(void);
AiDatabase	*ai_database_new			(void);
gboolean	 ai_database_set_filename		(AiDatabase	*database,
							 const gchar	*filename,
							 GError		**error);
gboolean	 ai_database_set_icon_path		(AiDatabase	*database,
							 const gchar	*icon_path,
							 GError		**error);
gboolean	 ai_database_open			(AiDatabase	*database,
							 gboolean	 synchronous,
							 GError		**error);
gboolean	 ai_database_close			(AiDatabase	*database,
							 gboolean	 vaccuum,
							 GError		**error);
gboolean	 ai_database_create			(AiDatabase	*database,
							 GError		**error);
gboolean	 ai_database_remove_by_repo		(AiDatabase	*database,
							 const gchar	*repo,
							 GError		**error);
gboolean	 ai_database_remove_by_name		(AiDatabase	*database,
							 const gchar	*name,
							 GError		**error);
gboolean	 ai_database_query_by_repo		(AiDatabase	*database,
							 const gchar	*repo,
							 guint		*value,
							 GError		**error);
gboolean	 ai_database_query_by_name		(AiDatabase	*database,
							 const gchar	*name,
							 guint		*value,
							 GError		**error);
gboolean	 ai_database_import			(AiDatabase	*database,
							 const gchar	*filename,
							 guint		*value,
							 GError		**error);
gboolean	 ai_database_add_translation		(AiDatabase	*database,
							 const gchar	*application_id,
							 const gchar	*name,
							 const gchar	*summary,
							 const gchar	*locale,
							 GError		**error);
gboolean	 ai_database_add_application		(AiDatabase	*database,
							 const gchar	*application_id,
							 const gchar	*package,
							 const gchar	*categories,
							 const gchar	*repo,
							 const gchar	*icon,
							 const gchar	*name,
							 const gchar	*summary,
							 GError		**error);
gboolean	 ai_database_import_by_name		(AiDatabase	*database,
							 const gchar	*filename,
							 const gchar	*icondir,
							 const gchar	*name,
							 guint		*value,
							 GError		**error);
gboolean	 ai_database_import_by_repo		(AiDatabase	*database,
							 const gchar	*filename,
							 const gchar	*icondir,
							 const gchar	*repo,
							 guint		*value,
							 GError		**error);

G_END_DECLS

#endif /* __AI_DATABASE_H */

