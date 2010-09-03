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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib-object.h>

#include "egg-debug.h"

#include "ai-result.h"

static void     ai_result_finalize	(GObject     *object);

#define AI_RESULT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), AI_TYPE_RESULT, AiResultPrivate))

/*
 * AiResultPrivate:
 *
 * Private #AiResult data
 */
struct _AiResultPrivate
{
	gchar				*application_id;
	gchar				*application_name;
	gchar				*application_summary;
	gchar				*package_name;
	gchar				*categories;
	gchar				*repo_id;
	gchar				*icon_name;
};

enum {
	PROP_0,
	PROP_APPLICATION_ID,
	PROP_APPLICATION_NAME,
	PROP_APPLICATION_SUMMARY,
	PROP_PACKAGE_NAME,
	PROP_CATEGORIES,
	PROP_REPO_ID,
	PROP_ICON_NAME,
	PROP_LAST
};

G_DEFINE_TYPE (AiResult, ai_result, G_TYPE_OBJECT)

/*
 * ai_result_get_application_id:
 */
const gchar *
ai_result_get_application_id (AiResult *result)
{
	return result->priv->application_id;
}

/*
 * ai_result_get_application_name:
 */
const gchar *
ai_result_get_application_name (AiResult *result)
{
	return result->priv->application_name;
}

/*
 * ai_result_get_application_summary:
 */
const gchar *
ai_result_get_application_summary (AiResult *result)
{
	return result->priv->application_summary;
}

/*
 * ai_result_get_package_name:
 */
const gchar *
ai_result_get_package_name (AiResult *result)
{
	return result->priv->package_name;
}

/*
 * ai_result_get_categories:
 */
const gchar *
ai_result_get_categories (AiResult *result)
{
	return result->priv->categories;
}

/*
 * ai_result_get_repo_id:
 */
const gchar *
ai_result_get_repo_id (AiResult *result)
{
	return result->priv->repo_id;
}

/*
 * ai_result_get_icon_name:
 */
const gchar *
ai_result_get_icon_name (AiResult *result)
{
	return result->priv->icon_name;
}

/*
 * ai_result_get_property:
 */
static void
ai_result_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	AiResult *result = AI_RESULT (object);
	AiResultPrivate *priv = result->priv;

	switch (prop_id) {
	case PROP_APPLICATION_ID:
		g_value_set_string (value, priv->application_id);
		break;
	case PROP_APPLICATION_NAME:
		g_value_set_string (value, priv->application_name);
		break;
	case PROP_APPLICATION_SUMMARY:
		g_value_set_string (value, priv->application_summary);
		break;
	case PROP_PACKAGE_NAME:
		g_value_set_string (value, priv->package_name);
		break;
	case PROP_CATEGORIES:
		g_value_set_string (value, priv->categories);
		break;
	case PROP_REPO_ID:
		g_value_set_string (value, priv->repo_id);
		break;
	case PROP_ICON_NAME:
		g_value_set_string (value, priv->icon_name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*
 * ai_result_set_property:
 */
static void
ai_result_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	AiResult *result = AI_RESULT (object);
	AiResultPrivate *priv = result->priv;

	switch (prop_id) {
	case PROP_APPLICATION_ID:
		priv->application_id = g_value_dup_string (value);
		break;
	case PROP_APPLICATION_NAME:
		priv->application_name = g_value_dup_string (value);
		break;
	case PROP_APPLICATION_SUMMARY:
		priv->application_summary = g_value_dup_string (value);
		break;
	case PROP_PACKAGE_NAME:
		priv->package_name = g_value_dup_string (value);
		break;
	case PROP_CATEGORIES:
		priv->categories = g_value_dup_string (value);
		break;
	case PROP_REPO_ID:
		priv->repo_id = g_value_dup_string (value);
		break;
	case PROP_ICON_NAME:
		priv->icon_name = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*
 * ai_result_class_init:
 */
static void
ai_result_class_init (AiResultClass *klass)
{
	GParamSpec *pspec;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = ai_result_finalize;
	object_class->get_property = ai_result_get_property;
	object_class->set_property = ai_result_set_property;

	/*
	 * AiResult:application-id:
	 */
	pspec = g_param_spec_string ("application-id", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_APPLICATION_ID, pspec);

	/*
	 * AiResult:application-name:
	 */
	pspec = g_param_spec_string ("application-name", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_APPLICATION_NAME, pspec);

	/*
	 * AiResult:application-summary:
	 */
	pspec = g_param_spec_string ("application-summary", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_APPLICATION_SUMMARY, pspec);

	/*
	 * AiResult:package-name:
	 */
	pspec = g_param_spec_string ("package-name", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_PACKAGE_NAME, pspec);

	/*
	 * AiResult:categories:
	 */
	pspec = g_param_spec_string ("categories", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_CATEGORIES, pspec);

	/*
	 * AiResult:repo-id:
	 */
	pspec = g_param_spec_string ("repo-id", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_REPO_ID, pspec);

	/*
	 * AiResult:icon-name:
	 */
	pspec = g_param_spec_string ("icon-name", NULL, NULL,
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_ICON_NAME, pspec);

	g_type_class_add_private (klass, sizeof (AiResultPrivate));
}

/*
 * ai_result_init:
 */
static void
ai_result_init (AiResult *result)
{
	result->priv = AI_RESULT_GET_PRIVATE (result);
}

/*
 * ai_result_finalize:
 */
static void
ai_result_finalize (GObject *object)
{
	AiResult *result = AI_RESULT (object);
	AiResultPrivate *priv = result->priv;

	g_free (priv->application_id);
	g_free (priv->application_name);
	g_free (priv->application_summary);
	g_free (priv->package_name);
	g_free (priv->categories);
	g_free (priv->repo_id);
	g_free (priv->icon_name);

	G_OBJECT_CLASS (ai_result_parent_class)->finalize (object);
}

/*
 * ai_result_new:
 *
 * Return value: a new AiResult object.
 *
 * Since: 0.5.4
 */
AiResult *
ai_result_new (void)
{
	AiResult *result;
	result = g_object_new (AI_TYPE_RESULT, NULL);
	return AI_RESULT (result);
}
