/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef	__DSTT_VTYPE_H__
#define __DSTT_VTYPE_H__

#ident	"@(#)dstt_vtype.h	1.5	93/02/22 SMI"

#include "dstt.h"

char	*dstt_get_sopt(Tt_message m, int n, char *ptype);

#define VTYPE_BOOL		"boolean"
#define VTYPE_BUFFERID		"bufferID"
#define VTYPE_CATEGORY		"string"
#define VTYPE_COMMAND		"string"
#define VTYPE_CONTENT		"content_type"
#define	VTYPE_COVERT		"boolean"
#define VTYPE_DEPTH		"integer"
#define VTYPE_DISPLAY		"string"
#define	VTYPE_DOMAIN		"domain"
#define VTYPE_ENV_VALUE		"string"
#define VTYPE_ENV_VAR		"string"
#define VTYPE_FILE_TYPE		"file_type"
#define VTYPE_FORCE		"boolean"
#define VTYPE_HEIGHT		"height"
#define	VTYPE_INQUISITIVE	"boolean"
#define VTYPE_LOCALE		"string"
#define VTYPE_MESSAGEID		"messageID"
#define VTYPE_RESOURCENAME	"string"
#define VTYPE_RESOURCEVAL	"string"
#define VTYPE_RESULTS		"string"
#define VTYPE_SIGNAL		"integer"
#define VTYPE_SILENT		"boolean"
#define VTYPE_SITUATION		"string"
#define	VTYPE_STATUS		"string"
#define VTYPE_TITLE		"title"
#define VTYPE_TOOLNAME		"string"
#define VTYPE_TOOLVERSION	"string"
#define	VTYPE_VENDOR		"string"
#define VTYPE_VISUAL		"string"
#define VTYPE_WIDTH		"width"
#define VTYPE_XOFFSET		"xOffset"
#define VTYPE_YOFFSET		"yOffset"
#define VTYPE_LOCATOR		"locator"
#define VTYPE_MAPPED		"boolean"
#define VTYPE_SHARELEVEL	"integer"
#define VTYPE_VECTOR		"vector"
#define VTYPE_VIEWID		"viewID"
#define VTYPE_X_SELECTION	"Sun_Deskset_X_Selection"

#endif __DSTT_VTYPE_H__
