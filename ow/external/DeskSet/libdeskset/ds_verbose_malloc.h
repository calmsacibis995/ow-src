/*
 * "@(#)ds_verbose_malloc.h 1.1 92/09/18"
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

#ifndef	SVR4
extern	void *ds_verbose_malloc();
extern	void *ds_verbose_memalign();
extern	void *ds_verbose_calloc();
extern	void ds_verbose_free();
extern	void *ds_verbose_realloc();
extern	void *ds_verbose_valloc();
extern	char *ds_verbose_strdup();
#else
extern	void *ds_verbose_malloc(char *, int, size_t);
extern	void *ds_verbose_memalign(char *, int, size_t, size_t);
extern	void *ds_verbose_calloc(char *, int, size_t, size_t);
extern	void ds_verbose_free(char *, int, void *);
extern	void *ds_verbose_realloc(char *, int, void *, size_t);
extern	void *ds_verbose_valloc(char *, int, size_t);
extern	char *ds_verbose_strdup(char *, int, const char *);
#endif

#ifdef VERBOSE_MALLOC
#define	MALLOC(XX)	ds_verbose_malloc(__FILE__, __LINE__, (XX))
#define	CALLOC(XX, NN)	ds_verbose_calloc(__FILE__, __LINE__, (XX), (NN))
#define	FREE(XX)	ds_verbose_free(__FILE__, __LINE__, (XX))
#define	MEMALIGN(XX, NN)ds_verbose_memalign(__FILE__, __LINE__, (XX), (NN))
#define	REALLOC(XX, NN)	ds_verbose_realloc(__FILE__, __LINE__, (XX), (NN))
#define	VALLOC(XX)	ds_verbose_valloc(__FILE__, __LINE__, (XX))
#define	DS_STRDUP(XX)	ds_verbose_strdup(__FILE__, __LINE__, (XX))
#else
#define	MALLOC(XX)	malloc((XX))
#define	CALLOC(XX, NN)	calloc((XX), (NN))
#define	FREE(XX)	free((XX))
#define	MEMALIGN(XX, NN)memalign((XX), (NN))
#define	REALLOC(XX, NN)	realloc((XX), (NN))
#define	VALLOC(XX)	valloc((XX))
#define	DS_STRDUP(XX)	strdup((XX))
#endif

