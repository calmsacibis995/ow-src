#ifndef lint
static  char sccsid[] = "@(#)util.c 3.15 94/09/13 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* util.c */

#include <stdio.h>
#include <pwd.h> 
#include <time.h> 
#include <string.h>
#ifdef SVR4
#include <netdb.h> 
#include <sys/systeminfo.h>
#else
#include <sys/dir.h>
#endif "SVR4"
#include <sys/param.h>
#include "rtable4.h"
#include "util.h"

/*--------------------------------------------------------------------------
 * THE FOLLOWING STRING FUNCTION redefinitions are a HACK !
 * 
 * The cm code should be changed so that
 *   a) the redefined functions use the same headers as in <string.h>
 *   b) no redefinition of these library function is necessary
 *
 * The cm definitions use different function headers than in <string.h>
 * Prefixing the functions will get rid of the resulting compiler error.
 * Now cm functions will use the cm_ string functions, but library functions, 
 * e.g. fprintf, will use strlen etc. which leads to core dumps. 
 * As part of the bootstrapping process, I am including the below redefinitions
 * of the system functions. This should be fixed later.
 * [vmh - 5/31/90]
 *--------------------------------------------------------------------------*/

extern char *
cm_strcpy(s1, s2)
        register char *s1, *s2;
{
	if (s1==NULL || s2==NULL) return(NULL);
	strcpy(s1, s2); 
        return (s1);
}

extern int 
cm_strlen(s)
        register char *s;
{
        register int n;
 
	if (s==NULL) return NULL;
	return (strlen(s));
}

extern char *
cm_strdup (s1)
        char *s1;
{
	char *s2;
	if (s1 == NULL) return NULL;
        s2 = strdup(s1);
	return (s2);
}

extern char *
cm_strcat(s1, s2)
        char *s1, *s2;
{
	if (s1==NULL || s2==NULL) return(s1);
	strcat(s1, s2);
	return s1;
}

/*      transform string patterns of \\ into \
        \n into carriage returns and
	\" into "	*/

extern char *
str_to_cr(s)
        char *s;
{
        int i, j, k;
        char *newstr;

        if (s==NULL) return(NULL);
        i = cm_strlen(s);

        newstr= (char *) ckalloc((unsigned)i + 1);
        k = 0;
        for (j=0; j<i; j++) {
                if (s[j]=='\\') {
                        if (s[j+1]=='n') {
                                newstr[k] = '\n';
                                j++;
                        }
                        else if (s[j+1]=='\\') {
                                newstr[k] = '\\';
                                j++;
                        }
			else if (s[j+1]=='\"') {
				newstr[k] = '\"';
				j++;
			}
                        else {
                                newstr[k] = s[j];
                        }
                }
                else {
                        newstr[k] = s[j];
                }
                k++;
        }
        newstr[k] = NULL;
        return(newstr);
}

/*      transform string patterns of \ into \\
        carriage returns into \n, and
	" into \"	*/

extern char *
cr_to_str(s)
	char *s;
{
	int i, j, k;
	char *newstr;

        if (s==NULL) return(NULL);
	i = cm_strlen(s);

	newstr = (char *) ckalloc((unsigned)((2 * i) + 1));
	k = 0;
	for (j=0; j<i; j++) {
		if (s[j]=='\n') {
			newstr[k] = '\\';
			newstr[k+1] = 'n';
			k+=2;
		}
		else if (s[j]=='\\') {
			newstr[k] = '\\';
			newstr[k+1] = '\\';
			k+=2;
		}
		else if (s[j]=='\"') {
			newstr[k] = '\\';
			newstr[k+1] = '\"';
			k+=2;
		}
		else {
			newstr[k] = s[j];
			k++;
		}
	}
	newstr[k] = NULL;
	return(newstr);
}

/* VARARGS1 */
extern void
syserr(msg, a1, a2, a3)
	char *msg;
{
	/* Taken from Unix World, July 1989, p. 66 */
	int saveerr;
	extern int errno;
	extern int sys_nerr;

	/* save the error number so fprintf doesn't step on it */
	saveerr = errno;

	(void) fprintf(stderr, "cm: ");
	/* print the actual message itself */
	(void) fprintf(stderr, msg, a1, a2, a3);

#if 0
	/* print the error, if any */
	if (saveerr != 0) {
		if (saveerr < 0 || saveerr > sys_nerr) 
			(void) fprintf(stderr, ":Unknown error %d", saveerr);
		else 
			(void) fprintf(stderr, ":%s", strerror(saveerr));
	}
#endif

	/* thow a newline on the end */
	(void) fprintf(stderr, "\n");

	/* exit with an error */
	if (saveerr==0)
		saveerr = -1;
	exit(saveerr);
}


/*	Wrapper around standard storage allocation, to localize errors.
	Taken from Unix World, July 1989, p. 66				*/
extern char *
ckalloc(size)
	unsigned int size;
{
	extern char *calloc();
	register char *p;

	/* try to get the memory */
	p = (char *)calloc(1, size);

	/* if it worked, return the memory directly */
	if (p != NULL) return(p);

	/* try allocation again */
	p = (char *)calloc(1, size);

	/* see if it worked the second time */
	if (p != NULL) return(p);

	/* no recovery available */
	syserr("ckalloc: cannot allocate %d bytes", size);
	return((char *)NULL);
}

	
extern void
print_tick(t)
        long t;
{
        char *a;
 
        a = ctime(&t);
        (void) fprintf (stderr, "%d %s\n", t, a);
}

int
min(i1, i2)
	int i1, i2;
{
	if (i1 > i2) return(i2);
	if (i1 < i2) return(i1);
	return(i1);
}

int
max(i1, i2)
	int i1, i2;
{
	if (i1 > i2) return(i1);
	if (i1 < i2) return(i2);
	return(i1);
}
	
extern Lines *
text_to_lines(s, n)
        char *s; int n;
{
	char *string, *line;
	Lines *prev_l = NULL, *l = NULL, *head= NULL;
	int i = 0;

	if (s == NULL || n <= 0) return NULL;

	string = cm_strdup(s);
	line = (char*)strtok(string, "\n");
	do {
		if (line == NULL) break;
		l = (Lines*)ckalloc(sizeof(Lines));
		if (head == NULL) head = l;
		if (prev_l != NULL) prev_l->next = l;
		l->s = cm_strdup(line);
		prev_l = l;
		i++;
		line = (char*)strtok(NULL, "\n");

	} while (i < n);
		
	free(string);
	return head;
}
 
extern void
destroy_lines(l)
        Lines *l;
{
        Lines *p;

        while (l != NULL) {
                free(l->s); l->s=NULL;
                p = l;
                l = l->next;
                free((char *)p); p=NULL;
        }
}
extern char *
get_head(str, sep)
char *str;
char sep;
{
        static char buf[BUFSIZ];
        char *ptr;

        if (str == NULL)
                return(NULL);

        ptr = buf;
        while (*str && *str != sep)
                *ptr++ = *str++;
        if (ptr == buf)
                return(NULL);
        else {
                *ptr = NULL;
                return(cm_strdup(buf));
        }
}

extern char *
get_tail(str, sep)
char *str;
char sep;
{
        char *ptr;
 
        if (str == NULL)
                return(NULL);
 
        while (*str && *str != sep)
                str++;
        if (*str)
                return(cm_strdup(++str));
        else
                return(NULL);
}

extern char *
cm_get_local_host()
{
	static char *local_host;

        if (local_host == NULL) {
#ifdef SVR4
                local_host = (char *)ckalloc(MAXHOSTNAMELEN);
                (void) sysinfo(SI_HOSTNAME, local_host, MAXHOSTNAMELEN);
#else
                local_host = (char *)ckalloc(MAXHOSTNAMELEN);
                (void) gethostname(local_host, MAXHOSTNAMELEN);
#endif "SVR4"
        }
        return local_host;
}


extern char *
cm_get_uname()
{
        static char *name;
        struct passwd *pw; 

        if (name == NULL) {
                if ((pw = (struct passwd *)getpwuid(geteuid())) == NULL)
                         name = strdup("nobody");
                else
                        name = strdup(pw->pw_name);
        }
        return name;
    
}

extern char *
cm_get_local_domain()
{
	static char *local_domain;

        if (local_domain == NULL) {
                local_domain = ckalloc(BUFSIZ);
#ifdef SVR4
                sysinfo(SI_SRPC_DOMAIN, local_domain, DOM_NM_LN);
#else
		(void) getdomainname(local_domain, BUFSIZ);
#endif "SVR4"
	}
        return(local_domain);
}

/* partially qualified target */
extern char*
cm_pqtarget(name)
        char *name;
{
        char *host, *target=NULL;
 
        host = (char*)strchr(name, '@');
        if (host == NULL) {
                host = (char*)cm_get_local_host();
                target = (char *)ckalloc(cm_strlen(name) +
                                cm_strlen(host) + 2);
                sprintf(target, "%s@%s", name, host);
        }
        else
                target = strdup(name);
 
        return target;
}
/*
 * calendar_name@host[.domain] -> calendar_name
 */
extern char *
cm_target2name(target)
char *target;
{
        return(get_head(target, '@'));
}
 
/*
 * calendar_name@host[.domain] -> host[.domain]
 */
extern char *
cm_target2location(target)
char *target;
{
        return(get_tail(target, '@'));
}
 
/*
 * calendar_name@host[.domain] -> host
 */
extern char *
cm_target2host(target)
char *target;
{
        char *location, *host;
 
        location = get_tail(target, '@');
        if (location != NULL) {
                host = get_head(location, '.');
                free(location);
                return(host);
        } else
                return(NULL);
}
/*
 * calendar_name@host[.domain] -> domain
 */
extern char *
cm_target2domain(target)
char *target;
{
        char *location, *domain;
 
        location = get_tail(target, '@');
        if (location != NULL) {
                domain = get_tail(location, '.');
                free(location);
                return(domain);
        } else
                return(NULL);
}

/*
 * str consists of components separated by token
 * get and copy the first component into comp and
 * strip it out of str, so str would point to the first
 * token or the null terminator.
 */
extern void
get_component(str, comp, token)
char **str;
char *comp;
char token;
{
	char *ptr;

	*comp = 0;

	if (str == NULL)
		return;
	else
		ptr = *str;

	while (ptr && *ptr != 0 && *ptr != token)
		*comp++ = *ptr++;

	*str = ptr;

	*comp = 0;
}

/*
 * head and tail points to the first and last character
 * of a string which consists of components separated by token.
 * get and copy the last component into comp and
 * strip it out of the string, so tail would point to the last
 * token or the head of the string.
 */
extern void
get_last_component(head, tail, comp, token)
char *head;
char **tail;
char *comp;
char token;
{
	char *ptr, *cptr;

	*comp = 0;

	if (tail == NULL)
		return;
	/*
	else if (head == *tail)
		return;
	*/
	else
		cptr = *tail;

	while (cptr != head && *cptr != token)
		cptr--;

	if (*cptr == token)
		ptr = cptr + 1;
	else
		ptr = cptr;

	while (ptr != (*tail + 1))
		*comp++ = *ptr++;

	*tail = cptr;

	*comp = 0;
}

extern Boolean
match_forward(str1, str2)
char *str1, *str2;
{
	char com1[BUFSIZ], com2[BUFSIZ];

	if (str1 == NULL || str2 == NULL)
		return (false);

	while (true) {
		get_component(&str1, com1, '.');
		get_component(&str2, com2, '.');

		if (*com1) {
			if (*com2 == NULL)
				return (true);
		} else {
			if (*com2 == NULL)
				return (true);
			else
				return (false);
		}

		if (strcasecmp(com1, com2) != 0)
			return (false);

		/* take care of case: a.b a. */
		if (strcmp(str2, ".") == 0
		    && (strcmp(str1, ".") != 0 || *str1 != NULL))
			return (false);

		/* skip "." */
		if (*str1 == '.') {
			if (*str2 == NULL)
				return (true);
			else {
				str1++;
				str2++;
			}
		} else if (strcmp(str2, ".") == 0 || *str2 == NULL)
			return (true);
		else
			return (false);
	}
}

extern Boolean
match_backward(str1, str2)
char *str1, *str2;
{
	int len1, len2;
	char *ptr1, *ptr2;
	char com1[BUFSIZ], com2[BUFSIZ];

	if (str1 == NULL || str2 == NULL)
		return (false);

	len1 = strlen(str1);
	len2 = strlen(str2);
	if (len2 > len1)
		return (false);
	else if (len2 == 0)
		return (true);

	ptr1 = (len1 ? (str1 + len1 - 1) : str1);
	ptr2 = (len2 ? (str2 + len2 - 1) : str2);

	if (*ptr1 == '.' && ptr1 != str1)
		ptr1--;

	if (*ptr2 == '.' && ptr2 != str2)
		ptr2--;

	while (true) {
		get_last_component(str1, &ptr1, com1, '.');
		get_last_component(str2, &ptr2, com2, '.');

		if (*com1) {
			if (*com2 == NULL)
				return (true);
		} else {
			if (*com2 == NULL)
				return (true);
			else
				return (false);
		}

		if (strcasecmp(com1, com2) != 0)
			return (false);

		/* skip "." */
		if (*ptr1 == '.') {
			if (ptr1 != str1)
				ptr1--;
			else
				return (false); /* bad format */
		} else
			return (true); /* done */

		if (*ptr2 == '.') {
			if (ptr2 != str2)
				ptr2--;
			else
				return (false); /* bad format */
		} else
			return (true); /* done */
	}
}

/*
 * Correct format assumed, i.e. str = label1[.label2 ...]
 * Compare str2 against str1 which should be more fully qualified than str2
 */
extern Boolean
same_path(str1, str2)
char *str1, *str2;
{
	char *ptr1,*ptr2;
	char *user;
	int res, n;

	if (str1 == NULL || str2 == NULL)
		return(false);

	/* check format */
	if (*str1 == '.' || *str2 == '.')
		return (false); /* bad format */

	if (match_forward(str1, str2) == true)
		return (true);
	else
		return (match_backward(str1, str2));
}

/*
 * compare user1 and user2
 * user1 = user@host[.domain]
 * user2 = any format in (user, user@host[.domain], user@domain)
 */
extern Boolean
same_user(user1, user2)
	char *user1; char *user2;
{
	char *str1, *str2;
	char *host, *domain;
	char buf[BUFSIZ];
	Boolean res;

	if (user1 == NULL || user2 == NULL)
		return false;

	/* compare user name */
	str1 = get_head(user1, '@');
	str2 = get_head(user2, '@');

	if (str1 == NULL || str2 == NULL)
		return(false);

	if (strcmp(str1, str2)) {
		free(str1);
		free(str2);
		return(false);
	}
	free(str1);
	free(str2);

	/* if only user name is specified, don't need to check domain */
	str2 = strchr(user2, '@');
	if (str2 == NULL)
		return(true);

	/* first assume user2=user@domain */
	str1 = strchr(user1, '.');
	if (str1 == NULL) {
		if (same_path(cm_get_local_domain(), ++str2))
			return(true);
	} else {
		if (same_path(++str1, ++str2))
			return(true);
	}

	/* assume user2=user@host[.domain] */
	if (str1 == NULL) {
		str1 = strchr(user1, '@');
		sprintf(buf, "%s.%s", ++str1, cm_get_local_domain());
		str1 = buf;
	} else {
		str1 = strchr(user1, '@');
		str1++;
	}

	if (same_path(str1, str2))
		return(true);
	else
		return(false);
}

/*
 * Get the user's access permission together with that associated with "world".
 * format of user: user@host[.domain]
 */
extern int
user_permission(l, user)
	Access_Entry *l; char *user;
{
	int p_world = access_none;

	if (l==NULL || user==NULL) return(access_none);

	while(l != NULL) {
		if (strcmp (l->who, WORLD) == 0)
			p_world = l->access_type;
		if (same_user(user, l->who))
			break;
		l = l->next;
	}
	if (l == NULL)
		return(p_world);
	else
		return(l->access_type | p_world);
}

/*
 * A blank line is one that consists of only \b, \t or \n.
 */
extern int
blank_buf(buf)
	char *buf;
{
	char *ptr = buf;

	if (ptr == NULL) return TRUE;
	while (ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n'))
		ptr++;
	if (*ptr == '\0')
		return TRUE;
	else
		return FALSE;
}

extern Boolean
source_has_access(list, src)
        Access_Entry *list; char *src;
{
        int src_in_list = false;
	char *shead, *stail, *lhead, *ltail;

        if (list==NULL || src==NULL) 
		return true;

	shead = get_head(src, '@');
	stail = get_tail(src, '@');
        while(list != NULL) {
		if (strcmp(src, list->who) != 0) {
			lhead = get_head(list->who, '@');
			/* names are the same */
			if (strcmp(shead, lhead) == 0) {
				/* names match and no host (tail) 
				   specified in list implies all access */
				ltail = get_tail(list->who, '@');
				if (ltail == NULL) {
					free(shead); free(stail); free(lhead); 
					return true;
				}
				src_in_list = true;
				free(ltail);
			}
			free(lhead);
		}
		else { /* names are the same; give access */
			free(shead); free(stail);
			return true;
		}
                list = list->next;
        }
	free(shead); free(stail);
	if (src_in_list)
		/* if we made it here then, the username is in the list but
		   the host did not match so block access */
		return false;
	/* if we made it here then, the user is not in list so all access
	   is open */
        return true;
}
