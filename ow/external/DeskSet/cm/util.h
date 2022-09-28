/*
 * static  char sccsid[] = "@(#)util.h 3.9 94/08/17 Copyr 1991 Sun Microsystems, Inc.";
 */

/*	util.h		*/

#define MAXSTRING 80

#ifndef SVR4
#define MAXNAMELEN 255  /*for SVR4: defined in <sys/param.h> to be 256 */
#endif "SVR4"

#define CERROR	1
#define PWERROR	2
#define FILEERROR 3
#define SPACEERROR 4
#define DOTS(a)	(a[0] == '.' && \
	(a[1] == 0 || (a[1] == '.' && a[2] == 0)))

typedef enum {false, true} Boolean;
	
typedef struct lines {
	char *s;
	struct lines *next;
} Lines;

/* Take a glob of text and break it down into lines	*/
extern Lines	*text_to_lines(/* char *s; int n; */);

/* Destroy a linked list of lines created by 'to_lines' */
extern void	destroy_lines(/* Lines *l */);

/* Strip the carriages returns out of the string	*/
extern char	*cr_to_str(/* char *s */);

/* Copies of the string functions from the library with
bug fixes -- ie. NULL checking & bounds checking.	*/

/* Compute the length of a string			*/
extern int	cm_strlen(/* char *s */);

/* Return a copy of s2 in s1. No storage alloc.		*/
extern char	*cm_strcpy(/* char *s1, *s2 */);

/* Return a copy of s1. Storage alloc.			*/
extern char	*cm_strdup(/* char *s1 */);

/* Concatenate s2 onto s1. No storage alloc.		*/
extern char	*cm_strcat(/* char *s1, *s2 */);

/* Substring s, beginning a position m, for n places.
	   Storage alloc.					*/
extern char	*substr(/* char *s; int m, n */);

/* Transform string patters of \\ into \ and
	   \n into carriage returns. Storage alloc.		*/
extern char	*str_to_cr(/* char *s */);

/* Allocator package wrapped around malloc which gives
	diagnostic feedback upon failures.			*/
extern char	*ckalloc();

/* Optional diagnostic proc used with ckalloc		*/
extern void	syserr(/* char *msg; int a1, a2, a3 */);

/* definitions to satisfy saberC */
extern char *strncat();

/* calendar_name@host[.domain] -> calendar_name */
extern char *cm_target2name( /* char *target */);

/* calendar_name@host[.domain] -> host[.domain] */
extern char *cm_target2location( /* char *target */);

/* calendar_name@host[.domain] -> host */
extern char *cm_target2host( /* char *target */ );

/* calendar_name@host[.domain] -> domain */
extern char *cm_target2domain( /* char *target */ );

/* calendar_name -> calendar_name@host */
extern char *cm_pqtarget( /* char *target */ );

/*
 * Return a copy of the string up to the first occurrence of character sep.
 * If sep is not found in the string, a copy of the whole string is
 * returned.
 * E.g. ("ab.c.de", '.') -> "ab"
 */
extern char* get_head( /* char *str; char sep */ );

/*
 * Return a copy of the string after the first occurrence of character
 * sep.  If sep is not found in the string, NULL pointer is returned.
 * E.g. ("ab.c.de", 'b') -> ".c.de"
 *	("abcde", '.') -> NULL
 */
extern char *get_tail( /* char *str; char sep */ );

/*
 * returns local hostname
 * Result contained in static area and shouldn't be freed.
 */
extern char* cm_get_local_host();

/*
 * returns local domain
 * Result contained in static area and shouldn't be freed.
 */
extern char* cm_get_local_domain();

/*
 * returns user name
 * Result contained in static area and shouldn't be freed.
 */
extern char* cm_get_uname();

/*
 * Compare str2 against str1 which should be more fully qualified than str2
 * Correct format assumed, i.e. str = label1[.label2 ...]
 * **might need a more sophisticated routine which compares each component
 */
extern Boolean same_path(/* char *str1, *str2 */);

/*
 * compare user1 and user2
 * user1 = user@host[.domain]
 * user2 = any format in [user, user@host[.domain], user@domain]
 */
extern Boolean same_user(/* char *user1; char *user2 */);

/*
 * Get the user's access permission together with that associated with "world".
 * format of user: user@host[.domain]
 */
extern int user_permission(/* Access_Entry *l; char *user */);

/*
 * A blank line is one that consists of only \b, \t or \n.
 */
extern int blank_buf(/* char *buf */);
/*
 *  Does user have access ? 
 */
extern Boolean source_has_access(/* char *list, char *source */);
