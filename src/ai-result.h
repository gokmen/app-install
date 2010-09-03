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
 * GNU General Public License for more result.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __AI_RESULT_H
#define __AI_RESULT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define AI_TYPE_RESULT		(ai_result_get_type ())
#define AI_RESULT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), AI_TYPE_RESULT, AiResult))
#define AI_RESULT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), AI_TYPE_RESULT, AiResultClass))
#define AI_IS_RESULT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), AI_TYPE_RESULT))
#define AI_IS_RESULT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), AI_TYPE_RESULT))
#define AI_RESULT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), AI_TYPE_RESULT, AiResultClass))

typedef struct _AiResultPrivate	AiResultPrivate;
typedef struct _AiResult		AiResult;
typedef struct _AiResultClass		AiResultClass;

struct _AiResult
{
	 GObject		 parent;
	 AiResultPrivate	*priv;
};

struct _AiResultClass
{
	GObjectClass		 parent_class;
};

GType		 ai_result_get_type		  	(void);
AiResult	*ai_result_new				(void);
const gchar	*ai_result_get_application_id		(AiResult	*result);
const gchar	*ai_result_get_application_name		(AiResult	*result);
const gchar	*ai_result_get_application_summary	(AiResult	*result);
const gchar	*ai_result_get_package_name		(AiResult	*result);
const gchar	*ai_result_get_categories		(AiResult	*result);
const gchar	*ai_result_get_repo_id			(AiResult	*result);
const gchar	*ai_result_get_icon_name		(AiResult	*result);
guint		 ai_result_get_rating			(AiResult	*result);
const gchar	*ai_result_get_screenshot_url		(AiResult	*result);
gboolean	 ai_result_get_installed		(AiResult	*result);

G_END_DECLS

#endif /* __AI_RESULT_H */

