#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)xv_util.c 1.5 93/06/06";
#endif
#endif

/*
 *	(c) Copyright 1990 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE 
 *	file for terms of the license.
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <xview/xview.h>

#ifdef hpux
#define USE_NAME
#endif
#ifdef SVR4
#define USE_NAME
#endif


#ifdef USE_UNAME
#include <sys/utsname.h>
#endif

/*
 * xv_get_hostname - emulates gethostname() on non-bsd systems.
 */

Xv_private int
xv_get_hostname (buf, maxlen)
    char *buf;
    int maxlen;
{
    int len;

#ifdef USE_UNAME
    struct utsname name;
 
    uname (&name);
    len = strlen (name.nodename);
    if (len >= maxlen) len = maxlen - 1;
    (void) strncpy (buf, name.nodename, len);
    buf[len] = '\0';
#else
    buf[0] = '\0';
    (void) gethostname (buf, maxlen);
    buf [maxlen - 1] = '\0';
    len = strlen(buf);
#endif
    return len;
}


#ifdef XV_NO_STRDUP
/*
 * For systems that do not have strdup()
 */
char *strdup(s)
     char *s;
{
  char *tmp;

  tmp = (char *)malloc(strlen(s) + 1);

  if(tmp == NULL)
    return(NULL);

  strcpy(tmp, s);

  return(tmp);
}
#endif /* XV_NO_STRDUP */


/*
 * XView defines it's own version of strtok here because we don't
 * want to affect any XView users who use strtok() directly.
 * 
 * NOTE: This function is destructive, it inserts null characters 
 * into 'token_string'.
 *
 * 'token_string' is a string of tokens separated by one or more
 * characters from the separator string 'sep_string'.
 *
 * The first call to xv_strtok() should specify both strings:
 *	tok = xv_strtok(string, "+");
 * This will return the pointer to the first token found.
 * Subsequent calls should look like:
 *	tok = xv_strtok(NULL, "+");
 * This will return pointers to the next token(s)
 *
 * The function keeps track of the current position in the token
 * string in subsequent calls (saved in 'save_pos').
 *
 * NULL is returned when no tokens remain.
 *
 * strpbrk() and strspn() are used to scan/skip the separator 
 * characters.
 *
 */

Xv_private char *
xv_strtok(token_string, sep_string)
char *token_string;
char *sep_string;
{
    char	*q, *r;
    static char	*save_pos;

    /*
     * If not first call, use saved pointer into string
     */
    if (token_string == NULL)  {
        token_string = save_pos;
    }

    /* 
     * If no more tokens left, return
     */
    if (token_string == 0)  {
        return(NULL);
    }

    /* 
     * skip leading separators 
     */
    q = token_string + strspn(token_string, sep_string);

    /* 
     * return if no tokens remaining 
     */
    if (*q == '\0')  {
        return(NULL);
    }

    /* 
     * move past token 
     */
    if((r = strpbrk(q, sep_string)) == NULL)  {
        /* 
	 * indicate this is last token 
	 */
        save_pos = 0;
    }
    else {
        *r = '\0';
        save_pos = r+1;
    }
    return(q);
}
