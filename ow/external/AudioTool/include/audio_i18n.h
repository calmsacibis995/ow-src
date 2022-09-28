/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIO_I18N_H
#define	_AUDIO_I18N_H

#ident	"@(#)audio_i18n.h	1.12	93/02/04 SMI"

#ifndef SUNOS41
#include <locale.h>

#else /* SUNOS41 */
#ifdef __cplusplus
extern "C" {
#endif
/* XXX - define gettext() routines for 4.x systems */
extern char* gettext(char*);
extern char* dgettext(char*, char*);
extern char* bindtextdomain(const char*, const char*);
#ifdef __cplusplus
}
#endif
#endif /* SUNOS41 */

#ifdef __cplusplus
extern "C" {
#endif

extern char* ds_expand_pathname(const char*, char*);

#ifdef __cplusplus
}
#endif

#define DGET(labels, str)		(char *) dgettext(labels, str) 

#endif /* !_AUDIO_I18N_H */
