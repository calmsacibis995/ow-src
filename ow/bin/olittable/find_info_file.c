#pragma ident "@(#)find_info_file.c	1.3 91/07/17"

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *		   All rights reserved.
 *	 Notice of copyright on this source code 
 *	 product does not indicate publication. 
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 * 
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */


#ifndef SYSV
#include <sys/file.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/param.h>


extern	char    *getenv();
extern	char	*strrchr();
extern	char	*strdup();
#else	/* System V */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>
#include <sys/param.h>
#include <unistd.h>

static unsigned char	*search_for(unsigned char * filename , ...);

#endif	/* !SYSV */


unsigned char	*
find_config_file(unsigned char *file_name, char *env_var, char *arg0)
{
	char	*path;
	char	buff[MAXPATHLEN];
	char	fallback_path[MAXPATHLEN];
	char	*c;
	char	*localename;

#ifndef	SYSV
	char *search_for();
#endif	/* !SYSV */

	if((c = getenv("OPENWINHOME")) != NULL){
		if ((localename = setlocale(LC_ALL, NULL)) == NULL) {
			sprintf(fallback_path,
				"%s/lib/locale/C/help/olittable",c);
		} else
			sprintf(fallback_path,
				"%s/lib/locale/%s/help/olittable",c,localename);
        }

	/* if environment variable is set */
	if((path = (char *)search_for(file_name, env_var, 
			c != NULL ? fallback_path : 0, 0)) != NULL){
	}
	else if (arg0[0] == '/') /* if absolute path */
	{
		strcpy(buff, arg0);
		c = strrchr(buff, '/');
		c++;
		strcpy(c, (char *)file_name);
		path = buff;
	}
	else if(c = strrchr(arg0, '/')) /* if relative path */
	{
		if(!getcwd(buff, sizeof(buff)))
		{
			perror("pwd");
			return(NULL);
		}
		strcat(buff, "/");
		strcat(buff, arg0);
		c = strrchr(buff, '/');
		c++;
		strcpy(c, (char *)file_name);
		path = buff;
	}
	else /* look in the path env. var. (where this prg. was found) */
	{
		path = (char *)search_for(file_name, "$PATH", 0);
	}

	if(access(path, R_OK) == 0) /* file does exist */
	{
		return((unsigned char *)strdup(path));
	}
	else
	{
		return((unsigned char *)NULL); /* file not there either */
	}
}

/*
 * search for a directory in a path. 
 * 
 * search_for(name,path,path,path...,NULL)
 *	char	*name ;
 *	char	*path... ;
 * 
 * This function accepts one or more directory paths in the formats
 *	"path:path:path"
 *	"$NAME"
 * and returns the first directory that can be
 * found. In the second form (path starts with '$'), the variable is taken as
 * the name of an evironment variable (such as $PATH, $MANPATH, etc.) to be
 * checked.
 */

#ifndef	SYSV
char           *
search_for(va_alist)
va_dcl
#else	/* System V */
/*VARARGS*/
static unsigned char *
search_for(unsigned char * filename , ...)
#endif	/* !SYSV */
{
	char           *file, *cur_path;
	register char  *beg_ptr, *end_ptr;
	int             len;
	static char     path[MAXPATHLEN];
	va_list         args;

	va_start(args , filename);

	file = (char *)filename;

	cur_path = va_arg(args, char *);
	while (cur_path != NULL)
	{
		if (*cur_path == '$')
			cur_path = getenv(cur_path + 1);
		while (cur_path != NULL)
		{
			end_ptr = strchr(cur_path, ':');
			if(end_ptr)
			{
				strncpy(path, cur_path, end_ptr-cur_path);
				path[end_ptr-cur_path] = 0;
				end_ptr++;
			}
			else
			{
				strcpy(path, cur_path);
			}
			strcat(path, "/");
			strcat(path, file);

			if (access(path, R_OK) == 0)
				return((unsigned char *)path);
			cur_path = end_ptr;
		}
		cur_path = va_arg(args, char *);
	}

	va_end(args);
	return NULL;
}

