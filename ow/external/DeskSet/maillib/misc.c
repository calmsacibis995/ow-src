#ident "@(#)misc.c	3.4 05/22/97 Copyright 1987-1992 Sun Microsystems, Inc."

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include "misc.h"
#include "ck_strings.h"
#include "attach.h"

static void ml_warn(void *channel, char *fmt, ...);
static void ml_error(void *channel, char *fmt, ...);
static int ml_confirm(void *channel, int optional, char *button1,
	char *button2, char *fmt, ...);
static void ml_perror(void *channel, int ecode, char *fmt, ...);

static void *ml_get(Ml_attr);
static void ml_set(Ml_attr, ...);

/* Mapping func to get encode w/o using CE */
static char *ml_encode(char *, void *);
/* Mapping func to get decode w/o using CE */
static char *ml_decode(char *, void *);

/* Map func V3 type to RFC type/subtype no CE */
static int ml_ds2rfc(char *, char**, char**);
/* Map func RFC type/subtype to V3 type no CE */
static int ml_rfc2ds(char *, char*, char**);

static int Maillib_errno;
static char *Maillib_errmsg;
static int Maillib_sendv3 = -1;

struct __maillib_methods maillib_methods = {
	ml_warn,	/* default warning function */
	ml_error,	/* default critical error function */
	ml_confirm,	/* ask a question of the user */
	ml_perror,	/* trivial replacement for perror */
	ml_get,
	ml_set,
	ml_encode,	/* retrieve encoding function */
	ml_decode,	/* retrieve decoding function */
#ifdef	RFC_MIME
	ml_ds2rfc,	/* mapping func V3 type to rfc type/subtype */
	ml_rfc2ds,	/* mapping func rfc type/subtype to V3 type */
#else
	NULL,
	NULL,
#endif	RFC_MIME
};


static void *
ml_get (Ml_attr attr)
{
	switch (attr)
	{
	case ML_ERRNO:
		return ((void *) Maillib_errno);
	case ML_ERRMSG:
		return ((void *) Maillib_errmsg);
	case ML_SENDV3:
		if (Maillib_sendv3 == -1)
#ifdef	RFC_MIME
			Maillib_sendv3 = !(int) mt_value("sendrfc");
#else
			Maillib_sendv3 = 1;
#endif	RFC_MIME
		return ((void *) Maillib_sendv3);
	default:
		return ((void *) NULL);
	}
}

static void
ml_set (Ml_attr attr, ...)
{
	va_list args;

	va_start(args, attr);

	switch (attr)
	{
	case ML_ERRNO:
		Maillib_errno = va_arg(args, int);
		break;
	case ML_ERRMSG:
		if (Maillib_errmsg)
			free (Maillib_errmsg);
		Maillib_errmsg = strdup(va_arg(args, char*));
		break;
	case ML_CLRERR:
		Maillib_errno = 0;
		if (Maillib_errmsg)
			free (Maillib_errmsg);
		Maillib_errmsg = NULL;
		break;
	case ML_ENCODE:
		maillib_methods.ml_encode =
			(char *(*)(char *, void *))
			va_arg(args, void *);
		break;
	case ML_DECODE:
		maillib_methods.ml_decode =
			(char *(*)(char *, void *))
			va_arg(args, void *);
		break;
	case ML_WARN:
		maillib_methods.ml_warn =
			(void(*)(void *channel, char *fmt, ...))
			va_arg(args, void *);
		break;
	case ML_ERROR:
		maillib_methods.ml_error =
			(void(*)(void *channel, char *fmt, ...))
			va_arg(args, void *);
		break;
	case ML_CONFIRM:
		maillib_methods.ml_confirm =
			(int(*)(void *, int, char*, char*, char*, ...))
			va_arg(args, void *);
		break;
	case ML_DS2RFC:
		maillib_methods.ml_ds2rfc =
			(int (*)(char *, char**, char**))
			va_arg(args, void *);
		break;
	case ML_RFC2DS:
		maillib_methods.ml_rfc2ds =
			(int (*)(char *, char*, char**))
			va_arg(args, void *);
		break;
	default:
		break;
	}
}

/* VARARGS2 */
static void
ml_warn(void *channel, char *fmt, ...)
{
	va_list args;


	channel = (void *) stderr;
	va_start(args, fmt);
	(void) vfprintf ((FILE *) channel, fmt, args);
	va_end (args);
}



/* VARARGS2 */
static void
ml_error(void *channel, char *fmt, ...)
{
	va_list args;

	channel = (void *) stderr;
	va_start(args, fmt);
	(void) vfprintf((FILE *) channel, fmt, args);
	va_end(args);

	exit(1);
}

/* VARARGS2 */
static int
ml_confirm(void *channel, int optional, char *button1, char *button2,
	char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	channel = (void *) stderr;
	(void) vfprintf((FILE *) channel, fmt, args);
	va_end(args);

	return(0);
}



/* VARARGS */
static void
ml_perror(void *channel, int ecode, char *fmt, ...)
{
	extern unsigned sys_nerr;
	va_list args;
	char buffer[1024];

	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);


	if (ecode > 0 && ecode <= sys_nerr) {
		maillib_methods.ml_warn(channel, "%s: %s\n",
			buffer, strerror(ecode));
	} else {
		maillib_methods.ml_warn(channel, "%s\n", buffer);
	}
}

static char *
ml_encode(char *encode, void *at)
{
	if (strcmp (encode, "default-compress") == 0)
		return ("compress");
	if (strcmp (encode, "adpcm-encode") == 0)
		return ("adpcm_enc");
	return (NULL);
}

static char *
ml_decode(char *decode, void *at)
{
	if (strcmp (decode, "default-compress") == 0)
		return ("uncompress");
	if (strcmp (decode, "adpcm-encode") == 0)
		return ("adpcm_dec");
	return (NULL);
}

#ifdef	RFC_MIME
struct ds_rfc_tab
{
	char	*ds_type;	/* deskset type */
	int	ds_len;
	char	*rfc_type;
	char	*rfc_smask;	/* subtype printf fmt mask with ds_type */
};

#define X_SUN_LEN	6 /* value of strlen("x-sun-"); */

struct ds_rfc_tab Type_tab[] = 
{
	{ ATTACH_TEXT_TYPE, 0, ATTACH_TEXT_TYPE, ATTACH_PLAIN_SUBTYPE},
	{ "richtext",	0,     ATTACH_TEXT_TYPE, "richtext"	},
	{ "audio-file",	0,	"audio",	"basic"		},
	{ "default",	0,	"application",	"x-sun-%s" 	},
	{ "oda",	0,	"application",	"%s"		},
	{ "postscript-file",10,	"application",	"postscript"	},
	{ "sun-raster",	0,	"image",	"x-%s"		},
	{ "jpeg",	0,	"image",	"%s"		},
	{ "g3fax",	0,	"image",	"%s"		},
	{ "gif-file",	3,	"image",	"gif"		},
	{ "pbm",	0,	"image",	"%s"		},
	{ "pgm",	0,	"image",	"%s"		},
	{ "ppm",	0,	"image",	"%s"		},
	{ "xmp-file",	3,	"image",	"x-sun-%s"	},
	{ "tiff",	4,	"image",	"tiff-b-netfax" },
	{ "troff",	5,	ATTACH_TEXT_TYPE, "x-sun-%s"	},
	{ "nroff",	5,	ATTACH_TEXT_TYPE, "x-sun-%s"	},
	{ "h-file",	0,	ATTACH_TEXT_TYPE, "x-sun-%s"	},
	{ "c-file",	0,	ATTACH_TEXT_TYPE, "x-sun-%s"	},
	{ "makefile",	0,	ATTACH_TEXT_TYPE, "x-sun-%s"	},
	{ "mail-",	5,	"message",	"x-sun-%s"	},
	{ "message",	7,	"message",	"x-sun-%s"	},
	{ NULL,		0,	"application",	"x-sun-%s"	}, /* default */
};

static
ml_ds2rfc(char *dstype, char **type, char **subtype)
{
	char	buf[80];
	register struct ds_rfc_tab *p_tab;

	p_tab = Type_tab;
	while (p_tab->ds_type != NULL)
	{
		if (((p_tab->ds_len == 0) &&
		     (strcasecmp(p_tab->ds_type, dstype) == 0)) ||
		    ((p_tab->ds_len != 0) &&
		     (strncasecmp(p_tab->ds_type, dstype, p_tab->ds_len) == 0)))
		{
			break;
		}
		p_tab++;
	}

	*type = ck_strdup(p_tab->rfc_type);
	sprintf(buf, p_tab->rfc_smask, dstype);
	*subtype = ck_strdup(buf);
	return (0);
}

static
ml_rfc2ds(char *type, char *subtype, char **dstype)
{
	char	buf[80];
	register struct ds_rfc_tab *p_tab;

	if (type == NULL)
		return (-1);
	if (subtype == NULL)
	{
		*dstype = ck_strdup(type);
		return(0);
	}
	if ((strcasecmp (type, ATTACH_TEXT_TYPE) == 0) && (subtype == NULL))
		*dstype = ck_strdup (ATTACH_TEXT_TYPE);
	else if (strcasecmp (type, "binary") == 0)	/* SVR4 compatability */
		*dstype = ck_strdup (type);
	else if (strcasecmp (type, "audio") == 0)
		*dstype = ck_strdup ("audio-file");
	else if (strcasecmp (subtype, "postscript") == 0)
		*dstype = ck_strdup ("postscript-file");
	else if ((strcasecmp (type, "message") == 0) &&
		 (strcasecmp(subtype, "rfc822") == 0))
		*dstype = ck_strdup ("mail-message");
	else if (strcasecmp (type, "multipart") == 0)
		*dstype = ck_strdup (type);	/* not map to "mail-message" */
	else
	{
		p_tab = Type_tab;
		while (p_tab->ds_type != NULL)
		{
			if (strcasecmp(subtype, p_tab->rfc_smask) == 0)
				break;

			buf[0] = '\0';
			sscanf(subtype, p_tab->rfc_smask, buf);
			if (strcasecmp(buf, p_tab->ds_type) == 0)
				break;

			p_tab++;
		}
		if (p_tab->ds_type != NULL)
			*dstype = ck_strdup(p_tab->ds_type);
		else
		{
			buf[0] = '\0';
			sscanf(subtype, p_tab->rfc_smask, buf);
			if (buf[0] == '\0') {
				if (strncasecmp(subtype, p_tab->rfc_smask, X_SUN_LEN) == 0)
					*dstype = ck_strdup(subtype+X_SUN_LEN);
				else
					*dstype = ck_strdup(subtype);
			} else
				*dstype = ck_strdup(buf);
		}
	}

	return (0);
}

#endif	RFC_MIME
