#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)es_cp_file.c 20.33 93/07/08";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Routines to copy a file.  Stolen from cp.c, then modified.
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef OW_I18N
#include <euc.h>
#ifdef SVR4
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>
#else /* SVR4 */
#include <sys/dir.h>
#include <sys/file.h>
#endif /* SVR4 */
#endif /* OW_I18N */
#include <xview_private/portable.h>
#include <xview/pkg.h>
#include <xview/attrol.h>

#ifndef BSIZE 
#define	BSIZE	8192
#endif /* BSIZE */

#define FSTAT_FAILED	-1
#define WILL_OVERWRITE	 1
Pkg_private int
es_copy_status(to, fold, from_mode)
    char           *to;
    int             fold;
    int            *from_mode;
{
    struct stat     stfrom, stto;

    if (fstat(fold, &stfrom) < 0)
	return (FSTAT_FAILED);
    if (stat(to, &stto) >= 0) {
	if (stfrom.st_dev == stto.st_dev &&
	    stfrom.st_ino == stto.st_ino) {
	    return (WILL_OVERWRITE);
	}
    }
    *from_mode = (int) stfrom.st_mode;
    return (0);
}

#define	es_Perror(s)
Pkg_private int
es_copy_fd(from, to, fold)
    char           *from, *to;
    int             fold;
{
    int             fnew, fnew_mode, n;
    struct stat     stto;
    char           *last, destname[MAXNAMLEN], buf[BSIZE];

    if (stat(to, &stto) >= 0 &&
	(stto.st_mode & S_IFMT) == S_IFDIR) {
	last = (char *)XV_RINDEX(from, '/');
	if (last)
	    last++;
	else
	    last = from;
	if ((int)strlen(to) + (int)strlen(last) >= BSIZE - 1) {
	    return (1);
	}
	(void) sprintf(destname, "%s/%s", to, last);

	to = destname;
    }
    switch (es_copy_status(to, fold, &fnew_mode)) {
      case FSTAT_FAILED:
	es_Perror(from);
	return (1);
      case WILL_OVERWRITE:
	return (1);
      default:
	break;
    }
    fnew = creat(to, fnew_mode);
    if (fnew < 0) {
	es_Perror(to);
	return (1);
    }
    for (;;) {
	n = read(fold, buf, BSIZE);
	if (n == 0)
	    break;
	if (n < 0) {
	    es_Perror(from);
	    (void) close(fnew);
	    return (1);
	}
	if (write(fnew, buf, n) != n) {
	    es_Perror(to);
	    (void) close(fnew);
	    return (1);
	}
    }
    (void) close(fnew);
    return (0);
}


#ifdef OW_I18N
Pkg_private int
textsw_mbstowcs(wstr, str, n)
    wchar_t	     *wstr;
    register char    *str;
    int		     *n;
{
    if (!multibyte) {

        /* 
           We can use the fast conversion routine, with the number of
           converted bytes == to the number of wide chars
        */
           
        return (*n = _xv_mbstowcs(wstr,(unsigned char *)str,*n));

    } else {

        register int    bytes, chars = 0;
        char	   *str_org = str;
 
        while (chars < *n && *str) {
	/*
	 * Following ifdef is for performance up. Direct cast multibye to wide
	 * char is available against ascii characters with sun compiler.
	 */
#ifdef sun
	    if (isascii(*str)) {
	        *wstr++ = (wchar_t)*str++;
	        chars++;
	        continue;
	    }
#endif
	    if ((bytes = mbtowc(wstr++, str, MB_CUR_MAX)) == -1) {
	        *(--wstr) = 0;
	        *n = str - str_org;
	        return (chars);
	    }
	    str += bytes;
	    chars++;
        }
        if (chars < *n) /* Has enough room for the terminator. */
    	    *wstr = 0;
        return (chars);
    }
}

Pkg_private int
es_mb_to_wc_fd(from, to, fold, skipped)
    char            *from, *to;
    int             fold;
    int            *skipped;
{
    int             fnew_mode, n, temp, len_in_wc;
    char            *last, destname[MAXNAMLEN], buf[BSIZE + 1];
    wchar_t	    buf_ws[BSIZE + 10];
    int		    fnew;
    struct stat     stto;
    int		    old_pos, new_pos;

    if (stat(to, &stto) >= 0 &&
	(stto.st_mode & S_IFMT) == S_IFDIR) {
	last = (char *)XV_RINDEX(from, '/');
	if (last)
	    last++;
	else
	    last = from;
	if ((int)strlen(to) + (int)strlen(last) >= MAXNAMLEN - 1) {
	    return (1);
	}
	(void) sprintf(destname, "%s/%s", to, last);
	to = destname;
    }
    switch (es_copy_status(to, fold, &fnew_mode)) {
      case FSTAT_FAILED:
	es_Perror(from);
	return (1);
      case WILL_OVERWRITE:
	return (1);
      default:
	break;
    }
    fnew = creat(to, fnew_mode);
    if (fnew < 0) {
	es_Perror(to);
	return (1);
    }
    old_pos = new_pos = 0;
    for (;;) {
	n = read(fold, buf, BSIZE);
	if (n == 0)
	    break;
	if (n < 0) {
	    es_Perror(from);
	    (void) close(fnew);
	    (void) unlink(to);
	    return (1);
	}
	temp = n;
	buf[n] = NULL;
	
	len_in_wc = textsw_mbstowcs(buf_ws, buf, &temp);
	
	if (temp != n) {
	   /* re-read the incomplete mb character */
#ifdef SVR4
	    new_pos = lseek(fold, temp - n, SEEK_CUR);
#else
	    new_pos = lseek(fold, temp - n, L_INCR);
#endif /* SVR4 */
	    if (new_pos == old_pos) {
	       /* Invalid char, so advance to next byte */
#ifdef SVR4
	        old_pos = lseek(fold, 1L, SEEK_CUR);   
#else
	        old_pos = lseek(fold, 1L, L_INCR);   
#endif /* SVR4 */
		*skipped = 1;
	    } else
	        old_pos = new_pos;
	}
	if ((write(fnew, (char *)buf_ws, len_in_wc * sizeof(wchar_t))
					/ sizeof(wchar_t)) != len_in_wc) {
	    es_Perror(to);
	    (void) close(fnew);
	    (void) unlink(to);
	    return (2);
	}
    }
    close(fnew);
    return (0);
}
#endif /* OW_I18N */
