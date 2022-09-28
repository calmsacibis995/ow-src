/*
 *
 * ds_verbose_malloc.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)ds_verbose_malloc.c 1.4 92/10/27 Copyr 1990 Sun Micro";
#endif

#include <stdio.h>
#include <stdlib.h>

void *
#ifndef __STDC__
ds_verbose_malloc(file, line, size)
char	*file;
int	line;
size_t	size;
#else
ds_verbose_malloc(char *file, int line, size_t size)
#endif
{
	void	*results = malloc(size);

	fprintf(stderr, "%s(%d)-%3d malloc bytes at 0x%X\n", file, line, size, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

void *
#ifndef __STDC__
ds_verbose_memalign(file, line, alignment, size)
char	*file;
int	line;
size_t	alignment;
size_t	size;
#else
ds_verbose_memalign(char *file, int line, size_t alignment, size_t size)
#endif
{
	void	*results = memalign(alignment, size);

	fprintf(stderr, "%s(%d)-%3d/%d memalign bytes at 0x%X\n", file, line, alignment, size, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

void *
#ifndef __STDC__
ds_verbose_calloc(file, line, nelem, elsize)
char	*file;
int	line;
size_t	nelem;
size_t	elsize;
#else
ds_verbose_calloc(char *file, int line, size_t nelem, size_t elsize)
#endif
{
	void	*results = calloc(nelem, elsize);

	fprintf(stderr, "%s(%d)-%d/%d calloc bytes at 0x%X\n", file, line, nelem, elsize, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

void
#ifndef __STDC__
ds_verbose_free(file, line, mem)
char	*file;
int	line;
void	*mem;
#else
ds_verbose_free(char *file, int line, void *mem)
#endif
{
	fprintf(stderr, "%s(%d)-freeing memory 0x%X\n", file, line, mem);
	free(mem);
}

void *
#ifndef __STDC__
ds_verbose_realloc(file, line, ptr, size)
char	*file;
int	line;
void	*ptr;
size_t	size;
#else
ds_verbose_realloc(char *file, int line, void *ptr, size_t size)
#endif
{
	void	*results = realloc(ptr, size);

	fprintf(stderr, "%s(%d)-%3d realloc bytes from 0x%X to 0x%X\n",
			file, line, size, ptr, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

void *
#ifndef __STDC__
ds_verbose_valloc(file, line, size)
char	*file;
int	line;
size_t	size;
#else
ds_verbose_valloc(char *file, int line, size_t size)
#endif
{
	void	*results = valloc(size);

	fprintf(stderr, "%s(%d)-%3d valloc bytes at 0x%X\n",file, line,  size, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

char *
#ifndef __STDC__
ds_verbose_strdup(file, line, ptr)
char	*file;
int	line;
char	*ptr;
#else
ds_verbose_strdup(char *file, int line, char *ptr)
#endif
{
	char	*results = (char *)strdup(ptr);

	fprintf(stderr, "%s(%d)-%3d(%15.15s) strdup bytes at 0x%X\n", file, line, strlen(ptr)+1, ptr, results);
	if(results)
	{
		return(results);
	}
	else
	{
		abort();
	}
}

