/* @(#)attach.h	3.9 08/19/94 */

/* defines externally visible objects to mail system */

/* folder.h and msg.h must be included */

#ifndef	__attach_h__
#define	__attach_h__

#include "headers.h"
#include "buffer.h"

extern struct __attach_methods attach_methods;

/* Use in at_set() */
typedef enum {
	ATTACH_DATA_TYPE	=	0,	/* char * (deskset type)     */
	ATTACH_DATA_CLASS	=	1,	/* char * (RFC-MIME type)    */
	ATTACH_DATA_SUBCLASS	=	2,	/* char * (RFC-MIME subtype) */
	ATTACH_DATA_PARAM	=	3,	/* char *, char *, set only  */
	ATTACH_DATA_DESC	=	4,	/* char *		*/
	ATTACH_DATA_NAME	=	5,	/* char *		*/
	ATTACH_DATA_FILE	=	6,	/* char *		*/
	ATTACH_DATA_TMP_FILE	=	7,	/* char *		*/
	ATTACH_DATA_EXT_FILE	=	8,	/* char *		*/
	ATTACH_DATA_REF_FILE	=	9,	/* char *		*/
	ATTACH_ENCODE_INFO	=	10,	/* char *		*/
	ATTACH_CONTENT_LEN	=	11,	/* int			*/
	ATTACH_BODY		=	12,	/* char *		*/
	ATTACH_MALLOC_BODY	=	13,	/* char *, set only	*/
	ATTACH_MMAP_BODY	=	14,	/* char *, set only	*/
	ATTACH_HEADER		=	15,	/* char * name, char * value */
	ATTACH_DELETED		=	16,	/* bool			*/
	ATTACH_MODIFIED		=	17,	/* bool			*/
	ATTACH_CLIENT_DATA	=	18,	/* void *		*/
	ATTACH_LINE_COUNT	=	19,	/* int			*/
	ATTACH_MSG		=	20,	/* struct msg *		*/
	ATTACH_FILE_TYPE	=	21,	/* Attach_attr, get only */
	ATTACH_BODY_BUF		=	22,	/* struct buffer_map *, set */
	ATTACH_MALLOC_BUF	=	23,	/* struct buffer_map *, set */
	ATTACH_MMAP_BUF		=	24,	/* struct buffer_map *, set */
	ATTACH_EXECUTABLE	=	25,	/* bool			*/
	ATTACH_NUM_BYTES	=	26,	/* int, get only	*/
	ATTACH_IS_TEXT		=	27,	/* bool, get only	*/
	ATTACH_IS_MULTIPART	=	28,	/* bool, get only	*/
	ATTACH_IS_MESSAGE	=	29,	/* bool, get only	*/
	ATTACH_DECODE_INFO	=	30,	/* char *, set only	*/
	ATTACH_CHARSET		=	31,	/* char *		*/
	ATTACH_CONVERSION	=	32,	/* char *		*/
} Attach_attr;

/*
 * Bit mask for "section" in at_copy
 */
#define	AT_HEADER	0x1
#define	AT_BODY		0x2
#define	AT_MARKER	0x4		/* AT_MARKER and AT_END_MARKER are */
#define	AT_END_MARKER	0x8		/* mutually exclusive		   */

/*
 * Various allocation methods for attachment body: at_mtype
 */
#define	AT_NONE		0		/* none of the below (from folder) */
#define	AT_MALLOC	1		/* attachment body is malloced */
#define	AT_MMAP		2		/* attachment body is mmapped */
#define	AT_USER		3		/* attachment body is user-allocated */

/*
 * Normally, "at_file" and "at_buffer" are set mutual exclusively.  Except
 * when the attachment from a folder is read, "at_buffer" points to the
 * original folder data, "at_file" points to the file name whose context could
 * have be modified.  Also, "at_encode" is set when the "at_buffer"
 * contains the encoded data.  Therefore, "at_encode" can be used to
 * distinguish if "at_buffer" is from folder or from Drag_&_Drop.
 */
struct attach {
	struct header_obj *at_hdr;
	struct msg *at_msg;		/* back ptr to msg */
	time_t	at_mod_time;		/* last modified time */
	int	at_lines;		/* lines in attachment context */
	char	*at_dtype;		/* data type in DeskSet namespace */
	struct cotype at_ctype;		/* data type in RFC-MIME format */
	char	*at_name;		/* data name */
	char	*at_file;		/* external file name */
	char	*at_decode;		/* decoded info about at_buffer */
	char	*at_encode;		/* encoding info about at_buffer */
	char	*at_charset;		/* character set for the attachment */
	struct attach *at_next;		/* next body part */
	struct buffer_map *at_buffer;	/* ptr to the buffers */
	struct buffer_map *at_odata;	/* original data from folder */
	void	*at_client_data;	/* client data */

	int	at_deleted : 1;		/* deleted? */
	int	at_modified : 1;	/* is at_file modified? */
	int	at_ftype : 2;		/* external file type */
	int	at_mtype : 2;		/* how attachment body is allocated? */
	int	at_garbage : 1;		/* indicate if damaged */
	int	at_fromfile : 1;	/* is at_buffer from at_file? */
	int	at_executable : 1;	/* is the attachment really a program */
	int	at_format : 2;		/* AT_RFC, AT_SUN, AT_UNKNOWN */
};

/* Values for at_style */
#define	AT_UNKNOWN	0		/* Undetermined yet		*/
#define	AT_SUN		1		/* OWV3 attachment format	*/
#define	AT_RFC		2		/* RFC-MIME format		*/

/* Values for at_save */
#define	AT_SAVE_FILE		0x0	/* AT_SAVE_FILE and AT_SAVE_SUBMIT */
#define	AT_SAVE_SUBMIT		0x1	/* are mutually exclusive	   */
#define	AT_SAVE_NOMARKER	0x2
#define	AT_SAVE_CONT_LEN	0x4	/* save "Content-Length:"	*/
#define	AT_SAVE_BODY		0x8	/* just save the message body	*/

#define	ATTACH_TEXT_TYPE		"text"
#define	ATTACH_TEXT_PLAIN_TYPE		"text/plain"
#define	ATTACH_PLAIN_SUBTYPE		"plain"

#define	ATTACH_VERSION			"1.0"
#define	ATTACH_MULTIPART_TYPE		"multipart"
#define	ATTACH_MIXED_SUBTYPE		"mixed"
#define	ATTACH_DIGEST_SUBTYPE		"digest"

#define	ATTACH_MESSAGE_TYPE		"message"
#define	ATTACH_MESSAGE_822_TYPE		"message/rfc822"
#define	ATTACH_EXTERNAL_SUBTYPE		"external-body"
#define	ATTACH_PARTIAL_SUBTYPE		"partial"

#define	ATTACHMENT_TYPE			"X-sun-attachment"

#define	at_type		at_ctype.co_type
#define at_subtype	at_ctype.co_subtype
#define	at_parameter	at_ctype.co_parameter
#define	at_comment	at_ctype.co_comment

extern	char			*At_parameters[];
#define AT_PARM_CHARSET		At_parameters[0]
#define AT_PARM_BOUNDARY	At_parameters[1]

struct __attach_methods {
	char *(*at_init)(struct attach *, char *buf, char *bufend);
	char *(*at_parse)(struct attach *, char *buf, char *bufend);
	int (*at_read)(struct attach *, char *path, Attach_attr ftype);

	struct attach *(*at_first)(struct msg *);
	struct attach *(*at_next)(struct attach *);

	int (*at_copy)(
		struct attach *, int section, int (*func)(), void *param);
	int (*at_modified)(struct attach *);
	int (*at_save)(
		struct attach *, int (*func)(), void *param, int submit);

	struct attach *(*at_create)(void);
	struct attach *(*at_dup)(struct attach *);
	int (*at_set)(struct attach *, Attach_attr attr, ...);
	void *(*at_get)(struct attach *, Attach_attr attr, ...);
	int (*at_destroy)(struct attach *);
	int (*at_decode)(struct attach *);
	int (*at_encode)(struct attach *, void *argv[]);
	int (*at_set_headers)(struct attach *);
	void (*at_del_headers)(struct header_obj **);
};

#endif	!__attach_h__
