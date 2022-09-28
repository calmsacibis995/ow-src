
#include <X11/Xos.h>
#include <dirent.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
 
#define CHUNKSIZE 1024
 


/* A convenient shorthand. */
typedef struct dirent    ENTRY;

/* Initial guess at directory size. */
#define INITIAL_SIZE    50

static int StrCmp(char **a, char **b)
{
    return strcmp(*a, *b);
}


int
ScanDir(char *Name, char ***List, int (*Selector)())
{
    char	 **names;
    ENTRY	  *E;
    DIR	  *Dp;
    int	   i;
    int	   size;

    /* Get initial list space and open directory. */
    size = INITIAL_SIZE;
    if (!(names = (char **)malloc(size * sizeof(char *))) ||
	!(Dp = opendir(Name)))
	return(-1);

    /* Read entries in the directory. */
    for (i = 0; E = readdir(Dp); )
	if (!Selector || (*Selector)(E)) {
	    /* User wants them all, or he wants this one. */
	    if (++i >= size) {
		size <<= 1;
		names = (char**)realloc((char *)names, size * sizeof(char*));
		if (!names) {
		    closedir(Dp);
		    return(-1);
		}
	    }

	    /* Copy the entry. */
	    if (!(names[i - 1] = (char *)malloc(strlen(E->d_name) + 1))) {
		closedir(Dp);
		return(-1);
	    }
	    (void)strcpy(names[i - 1], E->d_name);
	}

    /* Close things off. */
    names[i] = (char *)0;
    *List = names;
    closedir(Dp);

    /* Sort? */
    if (i)
	qsort((char *)names, i, sizeof(char *), StrCmp);

    return(i);
}
