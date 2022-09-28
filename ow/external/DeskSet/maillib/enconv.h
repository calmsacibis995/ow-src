/*
 * Copyright (c) 1991, by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * @(#)enconv.h	3.1 92/04/03
 */


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
