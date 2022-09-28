/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _ATOOL_I18N_H
#define	_ATOOL_I18N_H

#ident	"@(#)atool_i18n.h	1.13	93/02/04 SMI"

#include <audio_i18n.h>

#ifndef PRE_493
#define MGET(str)		(char *) gettext(str)

#else /* 4.x */
#define MGET(str)		str
#endif /* 4.x */

#endif /* !_ATOOL_I18N_H */
