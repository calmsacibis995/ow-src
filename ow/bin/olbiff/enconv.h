#pragma ident	"@(#)libintl.h 1.2	92/03/15	SMI"
/*
 * Copyright (c) 1991, by Sun Microsystems, Inc.
 * All rights reserved.
 */


/* Libintl is a library of advanced internationalization functions. */

#ifndef	_LIBINTL_H
#define	_LIBINTL_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T
#define	_WCHAR_T
typedef long wchar_t;
#endif

#ifdef __STDC__
/* Uniforum messaging functions --- requires dynamic linking */
#if !defined(_LOCALE_H)	/* Temporary cure. <locale.h> shouldn't have these. */
extern char *dgettext(const char *, const char *);
extern char *gettext(const char *);
extern int textdomain(const char *);
extern char *bindtextdomain(const char *, const char *);
#endif

/* More functions related to the Uniforum messaging functions. */
/* Warning: these are subject to change. */
extern char * setdomainpath(const char *, const char *);
extern char * getdomainpath(const char *);
extern char * adddomainpath(const char *, const char *);

/* Word handling functions --- requires dynamic linking */
/* Warning: these are subject to change. */
extern int wdinit(void);
extern int wdchkind(wchar_t);
extern int wdbindf(wchar_t, wchar_t, int);
extern wddelim(wchar_t, wchar_t, int);
extern wchar_t mcfiller(void);
extern int mcwrap(void);

/* Mail code conversion functions --- requires dynamic linking */
/* Warning: these are subject to change. */
#ifndef _SIZE_T
#define	_SIZE_T
typedef unsigned int 	size_t;
#endif
enum euc_action {EUC_ENCODE, EUC_DECODE, EUC_RESET_STATE};
extern int mail_conv(const char *, char *, size_t,
	const char *, enum euc_action);
#else
/* Uniforum messaging functions --- requires dynamic linking */
#ifndef _LOCALE_H	/* Temporary cure. <locale.h> shouldn't have these. */
extern char *dgettext();
extern char *gettext();
extern int textdomain();
extern char *bindtextdomain();
#endif

/* More functions related to the Uniforum messaging functions. */
/* Warning: these are subject to change. */
extern char * setdomainpath();
extern char * getdomainpath();
extern char * adddomainpath();

/* Word handling functions --- requires dynamic linking */
/* Warning: these are subject to change. */
extern int wdinit();
extern int wdchkind();
extern int wdbindf();
extern wddelim();
extern wchar_t mcfiller();
extern int mcwrap();

/* Mail code conversion functions --- requires dynamic linking */
/* Warning: these are subject to change. */
enum euc_action {EUC_ENCODE, EUC_DECODE, EUC_RESET_STATE};
extern int mail_conv();
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _LIBINTL_H */

/* enconv interface */
#define _ICONV_PATH "/usr/lib/iconv/%s%%%s.so"

#define	MAXOUTSEQ 1

typedef struct {
	void	*ecv_handle;
	void   (*ecv_close)();
	size_t (*ecv_enconv)();
	void	*ecv_state;
} _enconv_info;

typedef _enconv_info *enconv_t;


extern enconv_t enconv_open(const char *tocode, const char *fromcode);
extern size_t enconv(enconv_t cd, char **inbuf, size_t *inbyteleft,
			char **outbuf, size_t *outbytesize);
extern void enconv_close(enconv_t cd);
extern int enconv_getinfo(int item, enconv_t cd);
