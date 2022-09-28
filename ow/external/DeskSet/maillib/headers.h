/* @(#)headers.h	3.4 05/04/94 */

/* defines externally msg objects to mail system */

#ifndef	__headers_h__
#define	__headers_h__

#define	HDR_NONE	0
#define	HDR_MALLOC	1
#define	HDR_MMAP	2

struct header_obj {
	char	*hdr_start;
	char	*hdr_end;
	struct header_obj *hdr_next;
	char	hdr_allocated;		/* allocate type */
	char	hdr_single_val;		/* TRUE if a header per node */
};

/*
 * RFC-MIME parameter: "attr=value"
 */
struct coparam {
	char	*co_attr;
	char	*co_value;
	struct coparam *co_next;
};

/*
 * RFC-MIME Content-Type: type "/" subtype *[";" parameter] (comment)
 */
struct cotype {
	char *co_type;
	char *co_subtype;
	struct coparam *co_parameter;
	char *co_comment;
	char co_tquoted;	/* is type a quoted-string? */
	char co_squoted;	/* is subtype a quoted-string? */
};

struct __co_methods {
	int (*co_read)( /* struct cotype *, char *buf, char *bufend */ );
	int (*co_write)( /* struct cotype *, int (*func)(), void arg */ );
	int (*co_copy)( /* struct cotype *c1, struct cotype *c2 */ );
	int (*co_concat)( /* char *src, int len, char *dst */ );
	int (*co_size)( /* char *src, int len, int *size */ );
	void (*co_destroy)( /* struct cotype * */ );
	char *(*co_get)( /* struct cotype *, char *attr */ );
	void (*co_set)( /* struct cotype *, char *attr, char *value */ );
	int (*co_enumerate)( /* struct cotype *, int (*func)(), void arg */ );
};

extern struct __co_methods co_methods;

/*
 * Index to the array of At_fields
 */
#define	AT_CONTENT_TYPE		0
#define	AT_LABEL		1
#define	AT_ENCODING		2
#define	AT_LEN			3
#define	AT_LINES		4
#define	AT_COMMENT		5
#define	AT_DATA_FILE		6
#define	AT_REF_FILE		7
#define	AT_CHARSET		8

void header_find(char *buf, char *end, ...);

#endif	!__headers_h__
