#ifndef	_NEWS_ENCODING_H
#define _NEWS_ENCODING_H


#ident "@(#)encoding.h	1.2 06/11/93 NEWS SMI"



/*
 * Definitions for the compressed PostScript encoding.
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#define enc_mask		0364
#define enc_int			0200/* 16 */
#define	enc_short_string	0220/* 16 */
#define	enc_string		0240/* 4 */
#define enc_IEEEfloat		0244/* 1 */
#define enc_IEEEdouble		0245/* 1 */
#define enc_syscommon2		0246/* 1 */
#define enc_lusercommon		0247/* 4 */
#define enc_eusercommon		0253/* 1 */
#define enc_boolean		0254/* 1 */
#define enc_image		0255/* 1 */
#define enc_larray		0256/* 1 */
#define enc_earray		0257/* 1 */
#define enc_syscommon		0260/* 32 */
#define enc_usercommon		0320/* 32 */
#define enc_nonobject		0360/* 1 */	/* server->client */
#define enc_new_tag		0361/* 1 */	/* server->client */
#define enc_binary		0362/* 1 */	/* hook */
#define enc_extended		0363/* 1 */	/* hook */
/* 12 free */


/*
 * Necessary to allow connection with older servers
 * (used only by pscanf)
 */
#define enc_old_tag enc_syscommon
   

/*
 * As of OpenWindows V3 libwire will accept either the new or the old
 * tag encodings, but we need to support a small number of third-party
 * applications statically linked against V2 libraries, so we can't
 * just change the definition yet.
 *
 * The good news is that the binary compatability mode in SVr4 requires
 * the use of dynamic linking, so this is only a problem for SunOS 4.x.
 */
#if  !defined(SUNOS41)
#   define enc_tag	enc_new_tag
#else
    /*
     * Temporary - change to enc_new_tag once psio changes have been tested.
     */
#   define enc_tag	enc_old_tag
#endif

/*
 * Encodings of image format types. (Used in the enc_image parameter byte.)
 * These numbers are restricted to be in the range 0..63
 */
#define enc_image_sun_raster	0

#endif /* _NEWS_ENCODING_H */
