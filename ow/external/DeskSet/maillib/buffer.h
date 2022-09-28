/* @(#)buffer.h       3.4 - 94/05/04 */

/* defines externally visible objects to mail system */

#ifndef __buffer_h__
#define __buffer_h__

#include <stdarg.h>

/*
 * a utility structure to keep track of the (possibly) multiple mmaps
 * that we have done...
 */
struct buffer_map {
	struct buffer_map *bm_next;
	char *bm_buffer;
	int bm_size;
};

struct dd_obj {
	int	dd_fd[2];
	int	dd_pid;
	u_int	dd_size;		/* default mmapped size */
	struct buffer_map dd_curr;	/* current read ptr */
	struct buffer_map *dd_head;	/* final output data */
	struct buffer_map *dd_tail;
};

/*
 * mechanism to calling data conversion program or function
 */
struct __dd_methods {
	struct dd_obj *(*dd_init)( /* char *command, struct buffer_map * */ );
	int (*dd_read)( /* struct dd_obj * */ );
	int (*dd_write)( /* struct dd_obj * */ );
	int (*dd_loop)( /* struct dd_obj * */ );
	struct buffer_map *(*dd_done)( /* struct dd_obj * */ );
	struct buffer_map *(*dd_put)( /* struct dd_obj *, char c */ );
	struct buffer_map *(*dd_puts)( /* struct dd_obj *, char *s, int n */ );
	int (*dd_getc)( /* struct dd_obj * */ );
	struct buffer_map *(*dd_conv)(
	   /* void *progufuc, bool isfunc, struct buffer_map *, char **argv*/ );
};


extern struct __dd_methods dd_methods;


extern struct buffer_map *bm_alloc();
extern struct buffer_map *bm_last();
extern void		bm_free();
extern void		bm_init();
extern int		bm_gets();
extern int		bm_getc();
extern int		bm_copy();
extern int		bm_write();
extern int		bm_size();
typedef unsigned long	(*BM_FUNC)(char *, int, va_list);
extern unsigned long	bm_enumerate(struct buffer_map *, BM_FUNC, ...);

/* Macro dd_getchar() is derived from dd_getc() and bm_getc() */
#define	THIS(dd)	(&((dd)->dd_curr))
#define	CURR(dd)	(THIS(dd)->bm_next)
#define	NEXT(dd)	(CURR(dd)->bm_next)

#define	dd_getchar(dd)	\
((int) ((THIS(dd)->bm_size < CURR(dd)->bm_size) ?\
	   ((u_char) THIS(dd)->bm_buffer[(THIS(dd)->bm_size)++]) :\
	   ((NEXT(dd) == NULL) ? EOF :\
		(THIS(dd)->bm_buffer = NEXT(dd)->bm_buffer,\
		THIS(dd)->bm_size = 0,\
		THIS(dd)->bm_next = NEXT(dd),\
		((u_char) THIS(dd)->bm_buffer[(THIS(dd)->bm_size)++])\
		)\
	   )\
	)\
)

#endif	!__buffer_h__
