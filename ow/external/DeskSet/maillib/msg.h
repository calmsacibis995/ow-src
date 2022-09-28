/* @(#)msg.h	3.7 01/14/97 */

/* defines externally msg objects to mail system */

/* folder.h must be included first */

#ifndef	__msg_h__
#define	__msg_h__

#include "headers.h"
#include "folder.h"
#include "attach.h"

/* Use in msg_set(), msg_get() */
typedef enum {
	MSG_DELETED		= 0,	/* bool				*/
	MSG_SELECTED		= 1,	/* bool				*/
	MSG_CURRENT		= 2,	/* bool				*/
	MSG_HEADER		= 3,	/* char *name, char *value	*/
	MSG_TEXT_BODY		= 4,	/* char *			*/
	MSG_MMAP_TEXT_BODY	= 5,	/* char *, get only		*/
	MSG_MALLOC_TEXT_BODY	= 6,	/* char *, get only		*/
	MSG_ATTACH_BODY		= 7,	/* struct attach *, unsigned pos */
	MSG_IS_ATTACHMENT	= 8,	/* bool				*/
	MSG_TYPE		= 9,	/* int, get only		*/
	MSG_REPLY_SUBJECT	= 10,	/* char *, get only		*/
	MSG_REPLY_TO_SENDER	= 11,	/* char *, get only		*/
	MSG_REPLY_TO_ALL	= 12,	/* char *, get only		*/
	MSG_REPLY_TO_CC		= 13,	/* char *, get only		*/
	MSG_NUM_BYTES		= 14,	/* int, get only		*/
	MSG_CONTENT_LEN		= 15,	/* int, 			*/
	MSG_NUM_LINES		= 16,	/* int, get only		*/
	MSG_HEADERS		= 17,	/* (int (*)()) func, caddr_t arg, get only */
	MSG_DESTROY_ATTACH	= 18,	/* unsigned pos, set only	*/
	MSG_FROM		= 19,	/* char *, get only		*/
	MSG_DATE		= 20,	/* char *, get only		*/
	MSG_CONTENT_LINES	= 21,	/* int, get only		*/
	MSG_MODIFIED		= 22,	/* bool, get only		*/
	MSG_NUM_PARTS		= 23,	/* int, get only		*/
	MSG_BOUNDARY_MARKER	= 24,	/* char *; if 0, set random cookie */
	MSG_SUBTYPE		= 25,	/* char *, get only, no malloc	*/
} Msg_attr;

/* 10/1/97 */
/* make count_bytes_param visable to the outside world */

struct count_bytes_param {
        int length;
        int not_seven_bit_clean;
};

/*
 * position for MSG_ATTACH_BODY which ranges from 1 to MSG_LAST_ATTACH
 */
#define	MSG_LAST_ATTACH	((unsigned) 0xffffffff)

/*
 * A message is the fundamental unit of the mail system.  In general,
 * they are collected together in objects called "folders".
 */

struct __msg_methods {
	char *(*mm_init)(struct msg *, char *buf, char *bufend);
	int (*mm_read)(struct msg *, char *buf, char *bufend);
	struct msg *(*mm_create)(int attachment);
	void (*mm_destroy)(struct msg *);
	void *(*mm_get)(struct msg *, Msg_attr attr, ...);
	int (*mm_set)(struct msg *, Msg_attr, ...);

	int (*mm_copymsg)(
		struct msg *, int ignore, int (*func)(), int param, int screen);

	int (*mm_copyheader)(
		struct msg *, int ignore, int (*func)(), int param);

	void (*mm_replace)(struct msg *, char *buffer, int length);

	char *(*mm_read_headers)(struct msg *, char *buf, char *bufend);

	int (*mm_write_msg)(struct msg *, int supress_hdrs, FILE *f);
	int (*mm_write_attach)(struct msg *, int supress_hdrs, FILE *f);
	int (*mm_write_other)(struct msg *, int supress_hdrs, FILE *f);
	int (*mm_write_bytes)(char *buf, int len, FILE *fdes);
	int (*mm_read_bytes)(char *buf, int len, FILE *fdes);

	void (*mm_free_headers)(struct msg *);
	void (*mm_free_msg)(struct msg *);

	int (*mm_enumerate)(struct msg *, int (*func)(),
		int a, int b, int c, int d);

	struct msg *(*mm_first)(struct folder_obj *);
	struct msg *(*mm_next)(struct msg *);
	struct msg *(*mm_prev)(struct msg *);
	struct msg *(*mm_last)(struct folder_obj *);

	int (*mm_modified)(struct msg *);
	void (*mm_add_attach)(struct msg *, struct attach *);

#ifdef DEBUG
	void (*mm_dump)(struct msg *);
#endif DEBUG
};

extern struct __msg_methods msg_methods;

/*
 * Various "ignore" type for mm_copymsg/mm_write_msg/mm_copyheader
 */
#define	MSG_DONT_TST	0x100		/* Internal: mm_copymsg/mm_write_msg */
#define	MSG_MASK	0x3		/* Internal: Mask for ignore type */
#define	MSG_FULL_HDRS	0		/* Copy all headers */
#define	MSG_ABBR_HDRS	1		/* Copy abbr. (not ignored) headers */
#define	MSG_SAVE_HDRS	2		/* Copy abbr. and required headers */

/*
 * Various allocation methods for message body: mo_mtype
 */
#define	MSG_NONE	0		/* none of the below */
#define	MSG_MALLOC	1		/* message body is malloced */
#define	MSG_MMAP	2		/* message body is mmapped */

struct msg {
	char *mo_unix_from;
	char *mo_unix_date;
	char *mo_from;			/* arpa stuff compressed out */
	char *mo_to;			/* arpa stuff compressed out */
	char *mo_date;
	char *mo_subject;
	struct cotype mo_ctype;		/* RFC-MIME Content-Type */
	long mo_num_lines;		/* lines in msg text */
	long mo_len;			/* content length */
	struct header_obj *mo_hdr;	/* msg headers */
	char *mo_body_ptr;
	struct attach *mo_first_at;	/* point to 1st attachment */
	int mo_msg_number;		/* ordinal number of msg in folder */
	struct folder_obj *mo_folder;
	struct msg *mo_next;
	struct msg *mo_prev;

	/* for mailtool */
	int m_lineno;
	char *m_header;
	struct msg *m_next;		/* deleted list */

	int mo_selected : 1;
	int mo_boxed : 1;		/* is the sel box drawn on screen? */
	int mo_current : 1;
	int mo_read : 1;
	int mo_new : 1;
	int mo_deleted : 1;
	int mo_mtype : 2;
	int mo_modified : 1;
	int mo_format : 2;		/* MSG_TEXT, MSG_ATTACHMENT,
					 * MSG_MULTIPART, MSG_OTHER
					 */
};

#define	MSG_TEXT	0		/* Text type message		*/
#define	MSG_ATTACHMENT	1		/* Sun's attachment format	*/
#define	MSG_MULTIPART	2		/* RFC-MIME multipart format	*/
#define	MSG_OTHER	3		/* Non text/multipart format	*/

#define	mo_type		mo_ctype.co_type
#define	mo_subtype	mo_ctype.co_subtype
#define	mo_parameter	mo_ctype.co_parameter
#define	mo_comment	mo_ctype.co_comment

#define	mm_is_text(m)		((m) && ((m)->mo_format == MSG_TEXT))
#define	mm_is_mime(m)		((m) && ((m)->mo_format == MSG_MULTIPART))
#define	mm_is_v3(m)		((m) && ((m)->mo_format == MSG_ATTACHMENT))
#define	mm_is_other(m)		((m) && ((m)->mo_format == MSG_OTHER))

#endif	!__msg_h__
