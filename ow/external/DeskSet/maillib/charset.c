
#ident  "@(#)charset.c	3.13 - 97/06/06"

/* Copyright (C) 1992, Sun Microsystems Inc.  All Rights Reserved */

/* charset.c -- translate a body part from one character set to
 * another
 */

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/param.h>

#include "charset.h"
#include "msg.h"
#include "attach.h"
#include "enconv.h"
#include "assert.h"
#include "global.h"


extern char *mt_value(char *);


static void *cs_translation_required(struct msg *m, char **intcode);
static int cs_copy(void *enconvinfo, int (*func)(), char *buf, int len,
	int param);
static int cs_copy2(void *enconvinfo, int (*func)(), char *buf, int len,
	int param);
static void cs_init(void);
static int cs_msg_label(struct msg *m, void **info);

struct __charset_methods cs_methods = {
	cs_init,
	cs_translation_required,
	cs_copy,
	cs_copy2,
	cs_msg_label,
};


static struct mcent *mclist;

extern char Msg_Charset[];
extern char Msg_Usascii[];
static char iso8859_1[] = "ISO-8859-1";


#define	CN_MAXLOCALELEN		 32
#define	CN_MAXINTCODELEN	 32
#define	CN_MAXEXTCODELEN	 32
#define	CN_MAXCOMMENTLEN	128

#define	CN_DELMITERS	" \t\n"

struct mcent {
	char *cn_locale;
	char *cn_intcode;
	char *cn_extcode;
	int   cn_flag;
	char *cn_comment;
};

static struct mcent *getmclist(char *locale);
static void freemclist(struct mcent* cn);

#define	CODETABLE "/usr/lib/locale/%s/LC_CTYPE/mailcodelist"

#define CNALLOC1(n) (calloc(n+1,1))
#define	CNFREE1(p) if (p != NULL) free(p)
#define	min(a, b) ((a)>(b) ? (b) : (a))
#define STREQ(a,b)	(!(strcmp((a),(b))))

static char codetable[MAXPATHLEN];
static struct mcent *get_mcent(char *internal_encoding,
	char *external_encoding);

#define ENCONV_ERROR ((enconv_t)-1)


#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"


static void
cs_init()
{
	static char *locale;

	locale = setlocale(LC_CTYPE, NULL);

	if (mclist) {
		free(mclist);
		mclist = NULL;
	}

	mclist = getmclist(locale);
}



/*
 * look at the headers of a message, and see if we need
 * to translate it.  Also return null if the translation does
 * not exist in database...
 */
static void *
cs_translation_required(struct msg *m, char **intcode)
{
	char *newlang;
	struct mcent *cn;
	char *locale;
	enconv_t enconvinfo;

	if (intcode) {
		*intcode = NULL;
	}

	/* if we have no mailcodetab table, don't bother */
	if (! mclist) {
		DP(("cs_translation_required: no mclist\n"));
		return (NULL);
	}

	locale = setlocale(LC_CTYPE, NULL);

	newlang = msg_methods.mm_get(m, MSG_HEADER, Msg_Charset);

	if (! newlang) {
		/* no charset header -- return the first entry
		 * for this locale
		 */
		DP(("cs_translation_required: no charset header\n"));
		for (cn = mclist; cn->cn_locale != NULL; cn++) {
			if (STREQ(locale, cn->cn_locale)) {
				goto lookup;
			}
		}

		/* no match */
		DP(("cs_translation_required: no match for locale\n"));
		return (NULL);
	}

	/* OK.  now look up this language and the locale in the mcent
	 * table, and see what we get...
	 */

	DP(("cs_translation_required: %s: %s\n", Msg_Charset, newlang));
	for (cn = mclist; cn->cn_locale != NULL; cn++) {
		if (STREQ(locale, cn->cn_locale) &&
			STREQ(newlang, cn->cn_extcode))
		{
			break;
		}
	}

	/* check for no match... */
	if (cn->cn_locale == NULL) return(NULL);

lookup:
	/* OK.  We have a match.  See if it needs translation */
	if (cn->cn_flag == 0) return(NULL);

	/* now get the enconv information */
	DP(("cs_translation_needed: intcode %s, extcode %s\n",
		cn->cn_intcode, cn->cn_extcode));
	enconvinfo = enconv_open(cn->cn_intcode, cn->cn_extcode);

	if (enconvinfo == ENCONV_ERROR) {
		DP(("cs_translation_needed: no translation available\n"));
		return (NULL);
	}

	if (intcode) {
		*intcode = cn->cn_intcode;
	}
	return (enconvinfo);
}


static int
cs_copy2(void *enconvinfo, int (*func)(), char *buf, int length, int param)
{
	char buffer[4096];
	char *inptr;
	size_t inlen;
	char *outptr;
	size_t outlen;
	int len;
	int error;

	DP(("cs_copy2: called\n"));

	inlen = length;
	inptr = buf;

	while (inlen > 0) {
		outptr = buffer;
		outlen = sizeof buffer;
		len = enconv(enconvinfo, &inptr, &inlen, &outptr, &outlen);

		if (outptr != buffer) {
			DP(("cs_copy2: writing %d bytes\n",
				outptr - buffer));
			error = (*func)(buffer, outptr - buffer, param);

			if (error) return (error);
		}
	}
	return (0);
}


static int
cs_copy(void *enconvinfo, int (*func)(), char *buf, int length, int param)
{
	char buffer[4096];
	char *inptr;
	size_t inlen;
	char *outptr;
	size_t outlen;
	int len;
	int error;

	DP(("cs_copy: called\n"));

	inlen = length;
	inptr = buf;

	while (inlen > 0) {
		outptr = buffer;
		outlen = sizeof buffer;
		len = enconv(enconvinfo, &inptr, &inlen, &outptr, &outlen);
		glob.g_iconvinfo = NULL;

		if (outptr != buffer) {
			DP(("cs_copy: writing %d bytes\n",
				outptr - buffer));
			error = (*func)(buffer, outptr - buffer, param);

			if (error) return (error);
		}
	}

	/* force out a reset sequence */
	outptr = buffer;
	outlen = sizeof buffer;
	enconv(enconvinfo, NULL, 0, &outptr, &outlen);
	if (outptr != buffer) {
		DP(("cs_copy: writing %d bytes (reset sequence)\n",
			outptr - buffer));
		error = (*func)(buffer, outptr - buffer, param);
		if (error) return (error);
	}

	return(0);
}

static void
cs_decode_msg(msg)
void *msg;
{
	return;
}

static void
cs_decode_at(at)
void *at;
{
	char *body;
	int len;
	char *newbody;
	int newlen;
	char *charset;
	char *newcharset;

	charset = attach_methods.at_get(at, ATTACH_CHARSET);

	if (! charset) return;

	body = attach_methods.at_get(at, ATTACH_BODY);
	len = (int) attach_methods.at_get(at, ATTACH_CONTENT_LEN);

	if (newcharset != 0) {
		attach_methods.at_set(at, ATTACH_MALLOC_BODY, newbody);
		attach_methods.at_set(at, ATTACH_CONTENT_LEN, newlen);
		attach_methods.at_set(at, ATTACH_CHARSET, newcharset);
	}

}




static struct mcent *
getmclist(loc)
char *loc;
{
	FILE *fp, *fopen();
	char linebuf[BUFSIZ];
	char *p, *q, *delp, *ep, *fgets();
	struct mcent *chead, *cp;
	int i, n;

	p = codetable;
#ifdef DEBUG
	if (ep = getenv("ICONVROOT")) {
		strcpy(p, ep);
		p += strlen(ep);
	}
#endif /* DEBUG */
	sprintf(p, CODETABLE, loc);

	if ((fp = fopen(codetable, "r")) == NULL)
		return (NULL);

	n = 0;
	while (fgets(linebuf, BUFSIZ, fp) != NULL)
		n++;

	if ((chead = (void *)calloc(n + 1, sizeof (struct mcent))) == NULL) {
		fclose(fp);
		return (NULL);
	}

	rewind(fp);

	delp = CN_DELMITERS;
	i = 0;
	while (fgets(linebuf, BUFSIZ, fp) != NULL) {
		p = strtok(linebuf, delp);
		cp = &chead[i];
		if (p == NULL ||
		(cp->cn_locale = CNALLOC1(CN_MAXLOCALELEN)) == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		strncpy(cp->cn_locale, p, CN_MAXLOCALELEN);

		p = strtok(NULL, delp);
		if (p == NULL ||
		(cp->cn_intcode = CNALLOC1(CN_MAXINTCODELEN)) == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		strncpy(cp->cn_intcode, p, CN_MAXINTCODELEN);

		p = strtok(NULL, delp);
		if (p == NULL ||
		(cp->cn_extcode = CNALLOC1(CN_MAXEXTCODELEN)) == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		strncpy(cp->cn_extcode, p, CN_MAXEXTCODELEN);

		p = strtok(NULL, delp);
		if (p == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		cp->cn_flag = atoi(p);

		q = p + strlen(p) + 1;
		p = q + strspn(q, delp);
		if ((q = strpbrk(p, "\n")) != NULL)
			*q = '\0';
		if ((cp->cn_comment = CNALLOC1(CN_MAXCOMMENTLEN)) == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		strncpy(cp->cn_comment, p, CN_MAXCOMMENTLEN);

		if (++i > n)
			break;
	}

	chead[n].cn_locale = NULL;

	return (chead);
}



static void
freemclist(struct mcent* chead)
{
	struct mcent *cp;
	int i;

	i = 0;
	while (chead[i].cn_locale != NULL) {
		cp = &chead[i];
		CNFREE1(cp->cn_locale);
		CNFREE1(cp->cn_intcode);
		CNFREE1(cp->cn_extcode);
		CNFREE1(cp->cn_comment);
		i++;
	}

	free(chead);
}



static int
cs_msg_label(struct msg *m, void **info)
{

	char *s;
	char *end;
	int  eigth_bit_set;
	int err;
	enconv_t enconvinfo;
	int num_parts;

	/* search the message to see if it contains any eigth bit set
	 * stuff.  If not, label with us-ascii.  Otherwise label with
	 * the local default stuff
	 */

	DP(("cs_msg_label: called\n"));

	/* this routine is only callable if the msg does
	 * not have attachments.  This test should never
	 * succeed, but who can tell?
	 */

	num_parts = (int) msg_methods.mm_get(m, MSG_NUM_PARTS);
	ASSERT(num_parts == 0);
	if (num_parts != 0) return(-1);

	eigth_bit_set = 0;
	*info = NULL;
	end = &m->mo_body_ptr[m->mo_len];
	for (s = m->mo_body_ptr; s < end; s++) {
		if (*s & 0x80) {
			eigth_bit_set = 1;
			break;
		}
	}

	if (eigth_bit_set) {
		struct mcent *cn;

		DP(("cs_msg_label: 8th bit set\n"));
		/* we need to look to see what charset we are in.  If we
		 * don't have a table, take a guess that it is iso-8859-1
		 */
		
		cn = get_mcent(NULL, mt_value("encoding"));

		if (!cn) {
			/* no entry for this locale */

			DP(("cs_msg_label: no entry for locale, using %s\n",	
				iso8859_1));

			err = msg_methods.mm_set(m, MSG_HEADER,
				Msg_Charset, iso8859_1);

			return (err);
		}

		/* OK. we have an entry.  See if we can get a lookup
		 * for it.
		 */
		DP(("cs_msg_label: trying %s to %s\n",
			cn->cn_intcode, cn->cn_extcode));

		if (cn->cn_flag) {
			/* we need to translate */
			enconvinfo = enconv_open(cn->cn_extcode,
				cn->cn_intcode);

			if (enconvinfo == ENCONV_ERROR) {
				DP(("cs_msg_label: can't get conversion routine for %s to %s\n",
					cn->cn_intcode, cn->cn_extcode));

				return (-1);
			}

			*info = enconvinfo;
		}

		err = msg_methods.mm_set(m, MSG_HEADER,
			Msg_Charset, cn->cn_extcode);


	} else {
		DP(("cs_msg_label: 8th bit clear\n"));
		err = msg_methods.mm_set(m, MSG_HEADER, Msg_Charset, Msg_Usascii);
	}

	DP(("cs_msg_label: returning %d\n", err));
	return (err);
}



/*
 * look through the mail code entries for one that matches the
 * current locale
 */
static struct mcent *
get_mcent(char *internal, char *external)
{
	struct mcent *cn;
	char *locale;

	if (!mclist) return (NULL);

	/* bookkeeping to handle null strings */
	if (internal != NULL && *internal == '\0') {
		internal = NULL;
	}

	if (external != NULL && *external == '\0') {
		external = NULL;
	}

	locale = setlocale(LC_CTYPE, NULL);

	for (cn = mclist; cn->cn_locale != NULL; cn++) {
		if (!STREQ(locale, cn->cn_locale)) {
			continue;
		}

		if (internal != NULL && !STREQ(internal, cn->cn_intcode)) {
			continue;
		}

		if (external != NULL && !STREQ(external, cn->cn_extcode)) {
			continue;
		}

		return (cn);
		/* NOTREACHED*/
	}

	return (NULL);
}



