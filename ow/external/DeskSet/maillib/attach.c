#ident "@(#)attach.c	3.33 02/19/97 Copyright 1987-1997 Sun Microsystems, Inc."

/*
 *  Copyright (c) 1987-1997 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <memory.h>

#include "folder.h"
#include "msg.h"
#include "attach.h"
#include "misc.h"
#include "ck_strings.h"
#include "bool.h"

/* additional includes to support attachment fix*/
/*   1/10/97                     */
#include "charset.h"

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

#define	AT_RFC_FIELD(fno)	At_fields[fno].at_rfc
#define	AT_SUN_FIELD(fno)	At_fields[fno].at_sun
#define	AT_GEN_FIELD(fno)	At_fields[fno].at_generic
#define	NO_VALUE		""

/*********************************************
* additional includes to support attachment fix
*
* Use by at_copy_enumerate() 
* 
*/
/* Additional declarations for attachment fix */
/*  1/10/97                    */
extern struct __msg_methods msg_methods;

typedef
    int (*func_ptr_type)();
typedef
    unsigned long (*func_ptr_type_long)();


/*func defined in msg.c */
int count_bytes();
/*************************************************/


struct at_fields {
	char	*at_sun;		/* Beta Version header fields	*/
	char	*at_rfc;		/* RFC header fields		*/
	char	*at_generic;		/* Generic Mail header fields	*/
};

struct at_fields At_fields[] =
{
  { "X-Sun-Data-Type",	    "Content-Type",		"X-Data-Type"	},
  { "X-Sun-Data-Name",	    "Content-Description",	"X-Data-Name"	},
  { "X-Sun-Encoding-Info",  "Content-Transfer-Encoding","X-Encoding-Info" },
  { "X-Sun-Content-Length", "Content-Length", 		"X-Content-Length" },
  { "X-Sun-Content-Lines",  "X-Lines",			"X-Content-Lines" },
  { "X-Sun-Data-Description", NO_VALUE,			"X-Data-Description" },
  { "X-Sun-Data-File",	    NO_VALUE,			NO_VALUE	},
  { "X-Sun-Reference-File", NO_VALUE,			NO_VALUE	},
  { "X-Sun-Charset",	    NO_VALUE,			"X-Charset"	},
  { NULL,		    NULL,			NULL		},
};

#ifdef	RFC_MIME

char	*At_parameters[] =
{
	"charset",
	"boundary",
	"conversions",
	NULL,
};

char	DO_7_BIT[] = "7bit";	/* 7-bit - no encoding */
char	DO_8_BIT[] = "8bit";	/* 8-bit - no encoding */
char	DO_BINARY[] = "binary";	/* binary - no encoding */

#endif	RFC_MIME

char	At_marker[12]	= "----------\n";

#define	ATTACHMENT_DASH_LEN	(sizeof(At_marker)-1)

/* Macro to determine if a body part is in RFC-MIME multipart format
 * or in V3 attachment format.
 */
#define	at_is_mime(at)		mm_is_mime((at)->at_msg)
#define	at_is_other(at)		mm_is_other((at)->at_msg)
#define	at_is_v3(at)		mm_is_v3((at)->at_msg)

#define	NOT_BOUNDARY_MARKER	0
#define	BOUNDARY_MARKER		1
#define	END_MARKER		2

extern char *dgettext();

/* Use in at_ftype when attachment is being destroyed */
#define	DATA_TMP_FILE  	0	/* always unlink file */
#define	DATA_FILE      	1	/* never unlink file even if marked deleted */
#define	DATA_EXT_FILE	2	/* unlink file if marked deleted */
#define	DATA_REF_FILE  	3	/* never unlink file */

#define	S_CONTENT_LEN	0x1	/* save/submit with content-length field */
#define	S_LINE_COUNT	0x2	/* save/submit with line-count field */
#define	S_DONT_ADD_LF	0x4	/* not to add LF if not terminated by LF */
#define	S_NO_MARKER	0x8	/* not to write out the boundary marker */
#define	S_MSG_CONT_LEN	0x10	/* use "Content-Length:" */
#define	S_NO_HEADER	0x20	/* no attachment headers; change v3 text body
				 * part to msg
				 */

static char * at_init(struct attach *at, char *buf, char *bufend);
static char *at_parse(struct attach *at, char *bufstart, char *bufend);
static char * at_header(struct attach *at, char *name);
static struct attach *at_first(struct msg *);
static struct attach *at_next(struct attach *);
static int at_read(struct attach *at, char *tmpfile, Attach_attr filetype);
static int at_modified(struct attach *at);
static int at_copy(struct attach *at, int section, int (*func)(), void *param);
static int at_save(struct attach *at, int (*func)(), void *arg, int flag);
static int at_set(struct attach *at, Attach_attr attr, ...);
static void *at_get( struct attach *at, Attach_attr attr, ...);
static struct attach *at_create(void);
static struct attach *at_dup(struct attach *);
static int at_destroy(struct attach *);
static int at_read_attachment(char *path, struct attach *);
static void at_free_attachment(struct attach *);
static void at_destroy_file(struct attach *);
static int at_decode(struct attach *);
static int at_encode(struct attach *, void *argv[]);
static int at_set_headers(struct attach *);
static void at_del_headers(struct header_obj **);

static unsigned long check_encode(char *buf, int len, va_list);
static char * getheader(int field, char *buf, char *bufend);

extern char *findheader();
extern char *findchar();
extern char *findend_of_field();
extern char *skipwhite2();
extern struct header_obj *header_set_string();
extern struct header_obj *header_set();
extern struct header_obj *header_dup();
extern struct header_obj *header_delete();
extern struct header_obj *header_add();
extern void deleteheader();
extern char *header_name();
extern int uuencode();
extern int uudecode();
extern char DO_UUENCODE[];
#ifdef	RFC_MIME
extern int b64encode();
extern int b64decode();
extern int qpencode();
extern int qpdecode();
extern char DO_BASE64[];
extern char DO_QUOTED_PRINTABLE[];
#endif	RFC_MIME

struct __attach_methods attach_methods = {
	at_init,
	at_parse,
	at_read,
	at_first,
	at_next,
	at_copy,
	at_modified,
	at_save,
	at_create,
	at_dup,
	at_set,
	at_get,
	at_destroy,
	at_decode,
	at_encode,
	at_set_headers,
	at_del_headers
};

static struct attach *
at_create(void)
{
	struct attach *at;

	at = (struct attach *) ck_malloc (sizeof(struct attach));
	if (at == NULL)
		return (NULL);

	memset ((char *) at, '\0', sizeof (*at));
	at->at_format = AT_UNKNOWN;

	MP(("at_create()=%x\n", at));

	return (at);
}

static struct attach *
at_dup(
	struct attach *at
)
{
	int len;
	char *buf;
	struct attach *newat;

	/* disallow to duplicate a deleted attachment */
	if (at->at_deleted)
		return (NULL);

	newat = (struct attach *) ck_malloc (sizeof(struct attach));
	memset ((char *) newat, 0, sizeof(struct attach));
	MP(("at_dup: from at=%#x, to newat=%#x\n", at, newat));

	/* copy the headers, type, name, encoding information, and data */
	newat->at_hdr = header_dup (at->at_hdr);
	co_methods.co_copy (&newat->at_ctype, &at->at_ctype);
	newat->at_dtype = ck_strdup (at->at_dtype);
	newat->at_name = ck_strdup (at->at_name);
	newat->at_encode = ck_strdup (at->at_encode);
	newat->at_decode = ck_strdup (at->at_decode);
	newat->at_charset = ck_strdup (at->at_charset);
	newat->at_executable = at->at_executable;
	newat->at_format = at->at_format;

	MP(("at_dup: get from memory, encode_info='%s', decode_info='%s'\n",
		at->at_encode ? at->at_encode : "<null>", 
		at->at_decode ? at->at_decode : "<null>"));

	/* get the data from memory if data is not externalized and the file
	 * has not be changed.
	 */
	if ((!at_is_modified(at, &len) || (at->at_file == NULL)) && (len != 0))
	{
		MP(("at_dup: copy from at_buffer len=%d, file=%s\n",
			len, at->at_file ? at->at_file : "<null>"));

		/* the data from memory is usable, copy from memory */
		if ((buf = ck_zmalloc (len)) == NULL)
		{
			at_destroy (newat);
			return (NULL);
		}
		bm_copy (buf, at->at_buffer);
		newat->at_buffer = bm_alloc (buf, len);
		newat->at_mtype = AT_MMAP;
	}
	else if (len != 0)
	{
		MP(("at_dup: get from file %s\n", 
			at->at_file ? at->at_file : "<null>"));

		/* file has been modified: copy raw data from file */
		if (at_read_attachment (at->at_file, newat) != 0)
		{
			at_destroy (newat);
			return (NULL);
		}
	}

	return (newat);
}




static void *
at_get(
	struct attach *at,
	Attach_attr attr,
	...
)
{
	int	size;
	int	type;
	char	*buf;
	va_list ap;
	char	*name;

	va_start(ap, attr);

	switch (attr)
	{
	case ATTACH_DATA_TYPE:
		/* Get the V3 DeskSet type */
		if (at_is_v3(at))
		{
			/* Body part is in V3 format: dtype (from at_set) or
			 * type (from existing msg)
			 */
			if (at->at_dtype)
				return ((void *) at->at_dtype);
			else
				return ((void *) at->at_type);
		}
		/* Convert the RFC-MIME type/subtype to V3 DeskSet type */
		if (at->at_dtype == NULL)
		{
			if (maillib_methods.ml_rfc2ds == NULL)
				return (NULL);
			maillib_methods.ml_rfc2ds(at->at_type, at->at_subtype,
						&at->at_dtype);
		}
		return ((void *) at->at_dtype);

	case ATTACH_DATA_CLASS:
		/* RFC-MIME type */
#ifdef	RFC_MIME
		if (at->at_type == NULL)
		{
			/* Given the deskset type, get RFC-MIME type */
			if (at->at_dtype != NULL)
			{
				maillib_methods.ml_ds2rfc(at->at_dtype,
					&at->at_type, &at->at_subtype);
			}
		}
#endif	RFC_MIME
		return ((void *) at->at_type);

	case ATTACH_DATA_SUBCLASS:
		/* RFC-MIME subtype */
#ifdef	RFC_MIME
		if (at->at_type == NULL)
		{
			/* Given the deskset type, get RFC-MIME subtype */
			if (at->at_dtype != NULL)
			{
				maillib_methods.ml_ds2rfc(at->at_dtype,
					&at->at_type, &at->at_subtype);
			}
		}
#endif	RFC_MIME
		return ((void *) at->at_subtype);

	case ATTACH_DATA_DESC:
		return ((void *) at->at_comment);

	case ATTACH_DATA_NAME:
		return ((void *) at->at_name);

	case ATTACH_DATA_FILE:
		if (at->at_ftype == DATA_FILE)
			return ((void *) at->at_file);
		else
			return ((void *) NULL);

	case ATTACH_DATA_TMP_FILE:
		if (at->at_ftype == DATA_TMP_FILE)
			return ((void *) at->at_file);
		else
			return ((void *) NULL);

	case ATTACH_DATA_EXT_FILE:
		if (at->at_ftype == DATA_EXT_FILE)
			return ((void *) at->at_file);
		else
			return ((void *) NULL);

	case ATTACH_DATA_REF_FILE:
		if (at->at_ftype == DATA_REF_FILE)
			return ((void *) at->at_file);
		else
			return ((void *) NULL);

	case ATTACH_DECODE_INFO:
		return ((void *) at->at_decode);

	case ATTACH_ENCODE_INFO:
		return ((void *) at->at_encode);

	case ATTACH_CONTENT_LEN:
		/* external file has been modified, remmap the file */
		if (at->at_file && at->at_fromfile && at_is_modified(at, &size))
			mmap_at_file (at);
		return ((void *) bm_size(at->at_buffer));

	case ATTACH_BODY:
		if (at->at_buffer == NULL)
			return ((void *) NULL);
		/* external file has been modified, remmap the file */
		if (at->at_file && at->at_fromfile && at_is_modified(at, &size))
			mmap_at_file (at);
		if (at->at_buffer->bm_next != NULL)
		{
			/* put the buffers into a contiguous memory block */
			size = bm_size (at->at_buffer);
			buf = ck_zmalloc (size);
			bm_copy (buf, at->at_buffer);
			at_free_attachment (at);
			at->at_buffer = bm_alloc (buf, size);
			at->at_mtype = AT_MMAP;
			at->at_fromfile = 0;
		}
		return ((void *) at->at_buffer->bm_buffer);

	case ATTACH_HEADER:
		name = va_arg(ap, char *);
		return ((void *) at_header(at, name));

	case ATTACH_DELETED:
		return ((void *) at->at_deleted);

	case ATTACH_MODIFIED:
		return ((void *) at->at_modified);

	case ATTACH_CLIENT_DATA:
		return (at->at_client_data);

	case ATTACH_LINE_COUNT:
		return ((void *) at->at_lines);

	case ATTACH_MSG:
		return ((void *) at->at_msg);

	case ATTACH_FILE_TYPE:
		if (at->at_ftype == DATA_TMP_FILE)
			return ((void *) ATTACH_DATA_TMP_FILE);
		if (at->at_ftype == DATA_FILE)
			return ((void *) ATTACH_DATA_FILE);
		if (at->at_ftype == DATA_EXT_FILE)
			return ((void *) ATTACH_DATA_EXT_FILE);
		else
			return ((void *) ATTACH_DATA_REF_FILE);

	case ATTACH_EXECUTABLE:
		return ((void *) at->at_executable);

	case ATTACH_NUM_BYTES:
		/* A blank line between headers and contents is considered.
		 * ZZZ: this number is not very accurate.
		 */
		return ((void *) (header_length(at->at_hdr) + 1 +
			bm_size(at->at_buffer)));

	case ATTACH_IS_TEXT:
		if (at->at_dtype != NULL)
		{
			/* in V3 format, check deskset type */
			return ((void *) (strcasecmp(at->at_dtype,
						ATTACH_TEXT_TYPE) == 0));
		}
		else if (at->at_type == NULL) 
		{
			/* default to plain text */
			return ((void *) TRUE);
		}
		else
		{
			/* in RFC format, check type and subtype */
			if (strcasecmp(at->at_type, ATTACH_TEXT_TYPE) != 0)
				return ((void *) FALSE);

			/* default to text if subtype is missing */
			if (at->at_subtype == NULL)
				return ((void *) TRUE);

			return ((void *) (strcasecmp(at->at_subtype,
					ATTACH_PLAIN_SUBTYPE) == 0));
		}

	case ATTACH_IS_MULTIPART:
		type = msg_type(at->at_type ? at->at_type : at->at_dtype, NULL);
		return ((void *)((type == MSG_MULTIPART) ||
				 (type == MSG_ATTACHMENT)));

	case ATTACH_IS_MESSAGE:
		/* A message includes ordinary 822 message and
		 * Multipart/Attachment message.
		 */
		type = msg_type(at->at_type ? at->at_type : at->at_dtype);
		return ((void *)
			(type == MSG_MULTIPART || type == MSG_ATTACHMENT ||
			(at->at_type &&
			 strcasecmp(at->at_type, ATTACH_MESSAGE_TYPE) == 0) ||
			(at->at_dtype &&
			 strcasecmp(at->at_dtype, "sun-deskset-message")==0) ||
			(at->at_dtype &&
			 strcasecmp(at->at_dtype, "mail-message") == 0)));

	case ATTACH_CHARSET:
		return((void *)at->at_charset);

	default:
		return (NULL);
	}
}





static void
mark_msg_modified(
	struct attach *at
)
{
	struct msg *m;
	struct folder_obj *fo;

	if (m = at->at_msg)
	{
		m->mo_modified = 1;
		if (fo = m->mo_folder)
			fo->fo_changed = 1;
	}
}





static void
set_attach_buf(
	struct attach *at,
	int type,
	struct buffer_map *bm
)
{
	at_free_attachment (at);
	at->at_buffer = bm;
	at->at_mtype = type;
}





static int
at_set(
	struct attach *at,
	Attach_attr attr,
	...
)
{
	int	type;
	va_list ap;
	register char *name = NULL;
	char *string;
	char *value;

	va_start(ap, attr);

	switch (attr)
	{
	case ATTACH_DATA_TYPE:
		/* This is a deskset data type, not RFC type.  But it will be
		 * mapped to RFC type/subtype in at_get() or at_save() depend.
		 * on if it is V3 or RFC format.
		 */
		if (at->at_dtype) {
			ck_free (at->at_dtype);
			at->at_dtype = NULL;
		}
		at->at_dtype = ck_strdup(va_arg(ap, char *));
		if (at->at_type)
		{
			ck_free (at->at_type);
			at->at_type = NULL;
		}
		if (at->at_subtype)
		{
			ck_free (at->at_subtype);
			at->at_subtype = NULL;
		}
		break;

	case ATTACH_DATA_CLASS:
		/* Delay the header setting until at_save() because we
		 * have to support V3 and RFC formats and we are not sure
		 * what the proper header is.
		 */
		if (at->at_type) {
			ck_free (at->at_type);
		}
		at->at_type = ck_strdup(va_arg(ap, char *));
		break;

	case ATTACH_DATA_SUBCLASS:
		/* Delay the header setting until at_save() because we
		 * have to support V3 and RFC formats and we are not sure
		 * what the proper header is.
		 */
		if (at->at_subtype) {
			ck_free (at->at_subtype);
		}
		at->at_subtype = ck_strdup(va_arg(ap, char *));
		break;

	case ATTACH_DATA_PARAM:
		string = va_arg(ap, char *);
		co_methods.co_set(&at->at_ctype, string, va_arg(ap, char *));
		break;

	case ATTACH_DATA_DESC:
		/* Delay the header setting until at_save() because we
		 * have to support V3 and RFC formats and we are not sure
		 * what the proper header is.
		 */
		if (at->at_comment) {
			ck_free (at->at_comment);
		}
		at->at_comment = ck_strdup(va_arg(ap, char *));
		break;

	case ATTACH_DATA_NAME:
		/* Delay the header setting until at_save() because we
		 * have to support V3 and RFC formats and we are not sure
		 * what the proper header is.
		 */
		if (at->at_name) {
			ck_free(at->at_name);
		}
		at->at_name = ck_strdup(va_arg(ap, char *));
		break;

	case ATTACH_DATA_FILE:
		/* no attachment header is set; this is for submission,
		 * forwarding or add new attachment from file to existing msg
		 */
		type = DATA_FILE;
		goto Read_File;

	case ATTACH_DATA_TMP_FILE:
		/* no attachment header is set; this is for submission only */
		type = DATA_TMP_FILE;
Read_File:	ck_free (at->at_file);
		at->at_file = NULL;
		at->at_ftype = type;
		string = va_arg(ap, char *);
		if (at_read_attachment(string, at)) {
			return (-1);
		}

		/* if it is a tmp file, remove it */
		if (type == DATA_TMP_FILE) {
			unlink(string);
		}
		break;

	case ATTACH_DATA_EXT_FILE:
		/* attachment header is set; add ext. file to existing msg */
		name = header_name(at, AT_DATA_FILE);
		type = DATA_EXT_FILE;
		goto Set_File;

	case ATTACH_DATA_REF_FILE:
		/* attachment header is set; this is for submission only.
		 * Reference file does not need any data, just send the link
		 */
		name = header_name(at, AT_REF_FILE);
		type = DATA_REF_FILE;
Set_File:	ck_free (at->at_file);
		value = va_arg(ap, char *);
		at->at_file = ck_strdup(value);
		at->at_ftype = type;
		at_free_attachment(at);
		break;

	case ATTACH_ENCODE_INFO:
		string = va_arg(ap, char *);
		if (string == NULL)
		{
			/* special case to reset runtime encoding info */
			ck_free(at->at_decode);
			at->at_decode = NULL;
		} else if (at->at_decode == NULL) {
			at->at_decode = ck_strdup(string);
		} else {
			/* normal operation is to append encoding info */
			at->at_decode = realloc(at->at_decode,
				 strlen(at->at_decode) + strlen(string) + 3);

			if (at->at_decode) {
				strcat(at->at_decode, ", ");
				strcat(at->at_decode, string);
			}
		}
		break;

	case ATTACH_CONTENT_LEN:
		/* Note, make sure that attachment body must be set before
		 * setting content-length.  This assumes the data is in
		 * a single contiguous memory.
		 */
		if (at->at_buffer == NULL)
			return (-1);
		at->at_buffer->bm_size = va_arg(ap, int);
		break;

	/* Note, make sure that content-length must be set after setting the
	 * attachment body.  This assumes the data is in a single contiguous
	 * memory.  This attribute is provided for backward compatibility.
	 */
	case ATTACH_BODY:
		type = AT_USER;
		goto Set_Body;

	case ATTACH_MALLOC_BODY:
		type = AT_MALLOC;
		goto Set_Body;

	case ATTACH_MMAP_BODY:
		type = AT_MMAP;
Set_Body:	at_free_attachment (at);
		at->at_buffer = bm_alloc (va_arg(ap, void *), 0);
		at->at_mtype = type;
		ck_free(at->at_encode);
		at->at_encode = NULL;
		ck_free(at->at_charset);
		at->at_charset = NULL;
		at->at_fromfile = 0;
		/* obsolete the original data */
		bm_free (at->at_odata, NULL);
		at->at_odata = NULL;
		mark_msg_modified(at);
		break;

	case ATTACH_HEADER:
		name = va_arg(ap, char *);
		value = va_arg(ap, char *);
		break;

	case ATTACH_DELETED:
		/* this attribute should not be used in submission */
		mark_msg_modified(at);
		at->at_deleted = va_arg(ap, int);
		break;

	case ATTACH_MODIFIED:
		mark_msg_modified(at);
		at->at_modified = va_arg(ap, int);
		break;

	case ATTACH_CLIENT_DATA:
		at->at_client_data = va_arg(ap, void *);
		break;

	case ATTACH_LINE_COUNT:
		at->at_lines = va_arg(ap, int);
		break;

	case ATTACH_MSG:
		at->at_msg = va_arg(ap, struct msg *);
		break;

	/* Note, no need to set content-length after setting this block
	 * buffer body.  These attributes are for external use because they
	 * obsolete the original data.  set_attach_buf() is for internal use.
	 */
	case ATTACH_BODY_BUF:
		type = AT_USER;
		goto Set_Buffer;

	case ATTACH_MALLOC_BUF:
		type = AT_MALLOC;
		goto Set_Buffer;

	case ATTACH_MMAP_BUF:
		type = AT_MMAP;
Set_Buffer:	set_attach_buf(at, type, va_arg(ap, struct buffer_map *));
		at->at_fromfile = 0;
		/* obsolete the original data */
		bm_free(at->at_odata, NULL);
		at->at_odata = NULL;
		mark_msg_modified(at);
		break;

	case ATTACH_EXECUTABLE:
		at->at_executable = (va_arg(ap, int) ? 1 : 0);
		break;

	case ATTACH_DECODE_INFO:
		if (at->at_encode)
			ck_free(at->at_encode);
		at->at_encode = ck_strdup(va_arg(ap, char *));
		break;

	default:
		return (-1);
	}

	if (name != NULL)
	{
		if (header_set(&at->at_hdr, name, value) == NULL)
			return (-1);
		mark_msg_modified(at);
	}

	return (0);
}





static struct attach *
at_first(
	struct msg *msg
)
{
	if (msg == NULL)
		return (NULL);
	return (msg->mo_first_at);
}




static struct attach *
at_next(
	struct attach *at
)
{
	if (at == NULL)
		return (NULL);
	return (at->at_next);
}




static char *
at_header(
	struct attach *at,
	char *name
)
{
	char	*v;
	struct header_obj *p_hdr;

	p_hdr = at->at_hdr;
	while (p_hdr)
	{
		header_find(p_hdr->hdr_start, p_hdr->hdr_end, name, &v, 0);
		if (v) return (v);

		p_hdr = p_hdr->hdr_next;
	}

	return (NULL);
}






static int
get_counts(
	struct attach *at,
	char *buf,
	char *eoh,
	char *bufend
)
{
	char *length;
	char *lines;
	register int len;
	register char *eob;
	register int nlines;

	/* Get the content-length */
	length = getheader (AT_LEN, buf, eoh);
	if (length == NULL)
		len = -1;
	else
	{
		len = atoi (length);
		ck_free (length);

		eob = eoh + 1 + len;
#ifdef	RFC_MIME
		/* There is an extra LF before the RFC-MIME boundary marker */
		if (at_is_mime(at))
			++eob;
#endif	RFC_MIME

		/* check if content-length is good */
		if (((eob == bufend) && (*eob == '\n')) || ((eob < bufend) &&
		     (at_is_boundary(at, eob, bufend) != NOT_BOUNDARY_MARKER)))
			bufend = eob;
		else
			len = -1;
	}

#ifdef	RFC_MIME
	if (at_is_mime(at))
	{
		/* if byte count is absent or no good, use magic cookie */
		if (len != -1)
			return (len);
	}
	else
#endif	RFC_MIME
	{
		/* Get the line-count */
		lines = getheader (AT_LINES, buf, eoh);
		if (lines == NULL)
		{
			/* Line-count is missing here */

			/* Content-length is incorrect or missing too!  */
			if (len == -1)
				goto RECOVERY;

			/* use the content-length to calculate the line-count */
			at->at_lines = count_lines (eoh+1, len);
			mark_msg_modified (at);
		}
		else
		{
			/* Line-count exists */

			at->at_lines = nlines = atoi (lines);
			ck_free (lines);

			if (len == -1)
			{
				/* Content-length is missing or incorrect, use
				 * line-count to calculate it.  ZZZ: it
				 * doesn't work if the the content is binary.
				 */
				buf = eoh;
				while ((++buf < bufend) && (--nlines >= 0))
				{
					buf = findchar ('\n', buf, bufend);
				}

				if ((nlines <= 0) && ((*buf == '\n') ||
				     (at_is_boundary(at, buf, bufend) !=
				      NOT_BOUNDARY_MARKER)))
				{
					/* Line-count is correct */
					len = buf - eoh - 1;
				}
				else
				{
					/* Line-count is wrong! */
					goto RECOVERY;
				}
				mark_msg_modified (at);
			}

			/* ZZZ: if content-length is correct, we assume that the
			 * line-count is also correct.  We make this assumption
			 * to save time to validate the line-count.
			 */
		}
		return (len);
	}

RECOVERY:
	nlines = 0;
	buf = eoh;
	while (++buf < bufend)
	{
		if ((*buf == '-') &&
		    (at_is_boundary(at, buf, bufend) != NOT_BOUNDARY_MARKER))
			break;
		buf = findchar ( '\n', buf, bufend );
		nlines++;
	}
	if (buf > bufend)
		buf = bufend;

#ifdef	RFC_MIME
	/* Exclude the blank line before the boundary marker. */
	if (at_is_mime(at))
	{
		len = buf - eoh - 2;
		at->at_lines = --nlines;
	}
	else
#endif	RFC_MIME
	{
		len = buf - eoh - 1;
		at->at_lines = nlines;
	}
	mark_msg_modified (at);
	return (len);
}






static int
mmap_at_file(
	struct attach *at
)
{
	void	*buf;
	int	length;
	struct buffer_map *bm;

	buf = ck_mmap (at->at_file, &length);
	if (buf == NULL)
		return (-1);

	bm = bm_alloc (buf, length);
	set_attach_buf (at, AT_MMAP, bm);
	at->at_fromfile = 1;
	return (0);
}






static char *
at_init(
	struct attach *at,
	char *buf,
	char *bufend
)
{
	int	marker;
	char	*bufstart;

	MP(("at_init(at=%x)\n", at));

	/* Skip until the boundary marker */
	while (buf < bufend)
	{
		bufstart = findchar ( '\n', buf, bufend ) + 1;
		if ((*buf == '-') && (marker = at_is_boundary(at, buf, bufend)))
			break;
		buf = bufstart;
	}

	/* The boundary marker is not found; possibly corrupted. */
	if (buf >= bufend)
	{
		/* ZZZ: some data in this attachment was lost during the
		 * transmission; the next attachment may be affected (e.g.
		 * won't show up.)
		 */
		at->at_garbage = 1;
		return (buf);
	}
	/* The last optional boundary marker is not there: end of message */
	if ((bufstart >= bufend) || (marker == END_MARKER))
	{
		at->at_garbage = 1;
		return (bufend);
	}

	/* parse the headers excluding the boundary marker, and the body */
	return (at_parse (at, bufstart, bufend));
}






static char *
at_parse(
	struct attach *at,
	char *bufstart,
	char *bufend
)
{
	int	len;
	char	*end;
	char	*buf;
	char	*type;
	struct stat statbuf;

	/* determine the format of the body part */
#ifdef	RFC_MIME
	if (at_is_mime(at) || at_is_other(at))
		at->at_format = AT_RFC;
	else
		at->at_format = AT_SUN;
#else
	at->at_format = AT_SUN;
#endif	RFC_MIME

	/* find the end of the headers section; the first header excludes
	 * the boundary marker.  Put all headers into a linked list.  All
	 * header strings end at LF.  We need to take a special case to
	 * detect if the header is empty.
	 */
	end = buf = bufstart;
	while ((end < bufend) && (*end != '\n'))
	{
		bufstart = end;
		end = findend_of_field(end, bufend);

		header_set_string (&at->at_hdr, bufstart, end);
	}

	/* if type is missing, it is defaulted to "text". */
	if ((type = getheader (AT_CONTENT_TYPE, buf, end)) == NULL)
	{
#ifdef	RFC_MIME
		type = (char *) msg_methods.mm_get(at->at_msg, MSG_SUBTYPE);
		if (type && (strcasecmp(type, ATTACH_DIGEST_SUBTYPE) == 0))
			type = ck_strdup(ATTACH_MESSAGE_822_TYPE);
		else
			type = ck_strdup(ATTACH_TEXT_TYPE);
#else
			type = ck_strdup(ATTACH_TEXT_TYPE);
#endif	RFC_MIME
	}

	co_methods.co_read (&at->at_ctype, type, type+strlen(type));
	ck_free (type);

	if (at->at_comment == NULL)
		at->at_comment = getheader (AT_COMMENT, buf, end);
	at->at_name = getheader (AT_LABEL, buf, end);

	if (at->at_format == AT_SUN)
		at->at_encode = getheader (AT_ENCODING, buf, end);
#ifdef	RFC_MIME
	else
	{
		/* I'm not sure if i can get rid of the #ifdef RFC_MIME
		   directive all together but to be safe i'll just add
		   this here and comment out the conversion code as
		   conversions are no longer supported as stated by
		   the mime rfc spec */
		at->at_encode = getheader (AT_ENCODING, buf, end);

#if CONVERSIONS_NOT_SUPPORTED
		char	*enc;
		cmp = co_methods.co_get (&at->at_ctype, AT_PARM_CONVERSION);
		enc = getheader (AT_ENCODING, buf, end);
		if (cmp && enc)
		{
			/* Combine the conversion and transfer encoding code
			 * to a single field.
			 */
			at->at_encode = ck_malloc(strlen(cmp)+strlen(enc)+3);
			sprintf(at->at_encode, "%s, %s", cmp, enc);
			ck_free(cmp);
			ck_free(enc);
		}
		else if (cmp)
			at->at_encode = cmp;
		else
			at->at_encode = enc;
#endif CONVERSIONS_NOT_SUPPORTED
	}
#endif	RFC_MIME

	at->at_decode = NULL;

#ifdef	RFC_MIME
	at->at_charset = co_methods.co_get(&at->at_ctype, AT_PARM_CHARSET);
	if (at->at_charset == NULL)
#endif	RFC_MIME
		at->at_charset = getheader (AT_CHARSET, buf, end);

	if (at->at_file = findheader(AT_SUN_FIELD(AT_REF_FILE), buf, end))
	{
		/* reference data file is used; data is not stored in folder.
		 * ZZZ: it is not fully implemented.
		 */
		at->at_ftype = DATA_REF_FILE;
	}
	else if (at->at_file = findheader(AT_SUN_FIELD(AT_DATA_FILE), buf, end))
	{
		/* external data file is used; data is not stored in folder.
		 * file can be unlinked if the attachment is marked deleted
		 */
		at->at_ftype = DATA_EXT_FILE;
	}

	if ((at->at_file != NULL) && (stat(at->at_file, &statbuf) == 0))
	{
		/* contents were externally stored in file system */
		at->at_lines = 0;
		at->at_decode = at->at_encode;
		at->at_encode = NULL;
		mmap_at_file (at);
		at->at_mtype = AT_MMAP;
		at->at_odata = NULL;
		at->at_mod_time = statbuf.st_mtime;
		/* Bugid 1065651 */
		if ((len = get_counts (at, buf, end, bufend)) < 0)
			len = 0;
	}
	else
	{
		/* Get the content length and line count.  We also validate
		 * both values.  If they are both wrong or missing, it tries
		 * to do a simple error recovery.  The attachment body from
		 * folder starts after the newline.
		 */
		len = get_counts (at, buf, end, bufend);
		if (len > 0)
		{
			at->at_buffer = bm_alloc (end + 1, len);
			/* cache the original data for save after decoding */
			at->at_odata = bm_alloc (end + 1, len);
		}
		at->at_mtype = AT_NONE;
		at->at_fromfile = 0;
	}

	if (at->at_format == AT_SUN)
	{
		/* The check for type != NULL may be over-kill because
		 * once we reach this point we should have at least
		 * a type.  But if type is NULL then we'll choke on
		 * strlen() so...
		 */
		if (at->at_type != NULL && at->at_subtype != NULL)
		{
			/* Compose dtype from type and subtype:
			 * When calculating the amount of memory required
			 * we need to consider the slash "/" separator and
			 * the NULL terminator.  That's why we add two (2)
			 * to the malloc.  FYI the memory malloced here
			 * is freed in at_destroy().
			 */
			at->at_dtype = ck_malloc(strlen(at->at_type) +
				strlen(at->at_subtype) + 2);

			sprintf(at->at_dtype, "%s/%s", at->at_type,
				at->at_subtype);

			/* We are done with type and subtype. */
			ck_free(at->at_type);
			at->at_type = NULL;

			ck_free(at->at_subtype);
			at->at_subtype = NULL;
		}
		else {
			/* Set dtype to type (even if somehow it's NULL)
			 * and ignore subtype (it's probably NULL and
			 * and if not it's useless if type is NULL).
			 */
			at->at_dtype = at->at_type;
			at->at_type = NULL;

			/* This might be over-kill.  I think it is
			 * impossible for type to be NULL and subtype
			 * to be non-NULL.
			 */
			if (at->at_subtype != NULL)
			{
				ck_free(at->at_subtype);
				at->at_subtype = NULL;
			}
		}
	}

#ifdef	RFC_MIME
	/* advance to next attachment; add an extra byte to include the blank
	 * line before the boundary marker.
	 */
	if (at_is_mime(at))
		return (end + 2 + len);
#endif	RFC_MIME

	/* advance to next attachment */
	return (end + 1 + len);
}






static void
at_destroy_file(
	struct attach *at
)
{
	char *tmpbuf;

	MP(("at_destroy_file(%x), at_next=%x\n", at, at->at_next));

	/* remove the file iff it is a temporary file, or it is externalized
	 * when the attachment is marked deleted or the msg is marked deleted
	 */
	if (at->at_file && ((at->at_ftype == DATA_TMP_FILE) ||
	    ((at->at_ftype == DATA_EXT_FILE) && (at->at_deleted ||
	     (at->at_msg && at->at_msg->mo_deleted)))))
	{
		unlink (at->at_file);
		/* bug 1179878; remove tmp dir created for tmp files when 
			invoking attachments. Get just the directory name 
			minus the filename. if just a filename then there 
			is no directory to remove */ 
		if ((tmpbuf = strrchr(at->at_file, '/')) != NULL) 
			if (tmpbuf != at->at_file) {
				*tmpbuf = '\0';
				if ((tmpbuf = getenv("TMPDIR")) == NULL)
					tmpbuf = "/tmp";
				/* if its just tmp directory then there is no other 
					directory to remove */
				if (strcmp(at->at_file, tmpbuf) != 0)
					rmdir (at->at_file);
			}
	}

	ck_free (at->at_file);	at->at_file = NULL;
}







static int
at_destroy(
	struct attach *at
)
{
	MP(("at_destroy(%x), at_next=%x\n", at, at->at_next));

	co_methods.co_destroy(&at->at_ctype);
	ck_free(at->at_dtype);		at->at_dtype = NULL;
	ck_free(at->at_name);		at->at_name = NULL;
	ck_free(at->at_encode);		at->at_encode = NULL;
	ck_free(at->at_decode);		at->at_decode = NULL;
	ck_free(at->at_charset);	at->at_charset = NULL;

	/* free up the attachment body part if it is mmapped or malloced */
	at_free_attachment (at);
	if (at->at_odata)
	{
		/* free up the cache */
		bm_free (at->at_odata, NULL);
		at->at_odata = NULL;
	}

	at_destroy_file(at);

	/* destroy the allocated headers only */
	MP(("at_destroy: header_destroy(%x)\n", at->at_hdr));
	header_destroy (at->at_hdr);
	at->at_hdr = NULL;

	ck_free (at);
	return (0);
}






static void *
map_code(
	char *code,
	bool encode,
	bool *isfunc,
	struct attach *at
)
{
	void *p;

	if (strcasecmp (code, DO_UUENCODE) == 0)
	{
		*isfunc = TRUE;
		return (encode ? (void *) uuencode : (void *) uudecode);
	}
#ifdef	RFC_MIME
	else if (strcasecmp (code, DO_BASE64) == 0)
	{
		*isfunc = TRUE;
		return (encode ? (void *) b64encode : (void *) b64decode);
	}
	else if (strcasecmp (code, DO_QUOTED_PRINTABLE) == 0)
	{
		*isfunc = TRUE;
		return (encode ? (void *) qpencode : (void *) qpdecode);
	}
	else if ((strcasecmp (code, DO_7_BIT) == 0) ||
		 (strcasecmp (code, DO_8_BIT) == 0) ||
		 (strcasecmp (code, DO_BINARY) == 0))
	{
		/* To perform no-op, "isfunc" must be set to FALSE. */
		*isfunc = FALSE;
		return ("");
	}
#endif	RFC_MIME
	else if (strcasecmp (code, "compress") == 0)
	{
		*isfunc = FALSE;
		return (encode ? (void *) "compress" : (void *) "uncompress");
	}
	else
	{
		*isfunc = FALSE;
		if (encode)
		{
			if (maillib_methods.ml_encode != NULL)
			{
				p = (void *)maillib_methods.ml_encode(code, at);
				if ((p != NULL) && (*(char *)p == '\0'))
					p = NULL;
				return (p);
			}
		}
		else
		{
			if (maillib_methods.ml_decode != NULL)
			{
				return((void *) maillib_methods.ml_decode(code,
									  at));
			}
		}
		return (NULL);
	}
}






static int
at_decode(
	struct attach *at
)
{
	void *func;
	int len;
	int ecode;
	int isfunc;
	char cmd[BUFSIZ];
	char encode[256];
	register char *ptr;
	register char *code;
	struct buffer_map *bm;

#ifdef	I18N
	if ((int) at_get(at, ATTACH_IS_TEXT))
	{
		/* if charset is ISO-2022-xxx-7, convert it into EUC.
		 * ZZZZ: somehow we have to remember that the conversion has
		 * been done, so we won't do it more than once.  Should we
		 * remove the "charset" parameter?
		 */
		code = at->at_charset;
		if ((code != NULL) && (strncasecmp(code, "iso-2022", 8) == 0))
		{
			bm = dd_methods.dd_conv( , TRUE, at->at_buffer, NULL);
			set_attach_buf (at, AT_MMAP, bm);
		}
	}
#endif	I18N

	/* if this is not encoded, then just return. */
	if (at->at_encode == NULL)
		return (0);

	ecode = 0;
	cmd[0] = '\0';
	strcpy (encode, at->at_encode);
	code = strchr (encode, '\0');
	for (;;)
	{
		/* get a token from right to left */
		if (cmd[0] == '\0')
			ptr = code;
		while ((--code >= encode) && (isspace(*(u_char *)code) ||
			(*code == ',')))
		{

			*code = '\0';
		}
		if (code < encode) {
			break;
		}

		do { 
			if (isspace(*(u_char *)code) || (*code == ',')) {
				break;
			}
		} while (--code >= encode);
		code++;

		func = map_code (code, FALSE, &isfunc, at);
		if (func == NULL)
		{
			code = ptr;
			ecode = -1;
			break;
		}

		if (!isfunc)
		{
			/* if decoding is done by a process, construct the
			 * pipe and delay the execution until a decoding
			 * performed by a function is needed.
			 */
			if (cmd[0] != '\0')
				strcat (cmd, " | ");
			strcat (cmd, (char *) func);
			continue;
		}
			
		if (cmd[0] != '\0')
		{
			/* perform a decoding done by processes */
			bm = dd_methods.dd_conv(cmd,FALSE,at->at_buffer,NULL);
			DP(("at_decode: %s %s/%s new length is %d\n", 
				cmd ? cmd : "<null>",
				at->at_name ? at->at_name : "<null>", 
				at_get(at, ATTACH_DATA_TYPE) ? 
					at_get(at, ATTACH_DATA_TYPE) : "<null>",
				bm_size(bm)));
			if (bm == NULL)
			{
				code = ptr;
				ecode = -1;
				break;
			}
			cmd[0] = '\0';

			/* now reassign the attachment */
			set_attach_buf (at, AT_MMAP, bm);
		}

		/* perform a decoding done by a function */
		bm = dd_methods.dd_conv (func, TRUE, at->at_buffer, NULL);
		if (bm == NULL)
		{
			code = ptr;
			ecode = -1;
			break;
		}

		/* now reassign the attachment */
		set_attach_buf (at, AT_MMAP, bm);

		DP(("at_decode: %s %s/%s new length is %d\n", 
			code ? code : "<null>",
			at->at_name ? at->at_name : "<null>", 
			at_get(at,ATTACH_DATA_TYPE) ? 
				at_get(at,ATTACH_DATA_TYPE) : "<null>", 
			bm_size(bm)));
	}

	if ((ecode == 0) && (cmd[0] != '\0'))
	{
		bm = dd_methods.dd_conv(cmd, FALSE, at->at_buffer, NULL);
		DP(("at_decode: %s %s/%s new length is %d\n", 
			cmd ? cmd : "<null>",
			at->at_name ? at->at_name : "<null>", 
			at_get(at,ATTACH_DATA_TYPE) ?
				at_get(at,ATTACH_DATA_TYPE) : "<null>", 
			bm_size(bm)));
		if (bm == NULL)
		{
			code = ptr;
			ecode = -1;
		}
		else
		{
			cmd[0] = '\0';
			/* now reassign the attachment */
			set_attach_buf (at, AT_MMAP, bm);
		}
	}

	/* remove the encode info string, save the decoded info string */
	if (code < encode)
		code = encode;
	len = strlen (&at->at_encode[code - encode]);
	if (ptr = at->at_decode)
		len += strlen (ptr);
	at->at_decode = ck_malloc (len + 1);
	strcpy (at->at_decode, &at->at_encode[code - encode]);
	if (ptr != NULL)
	{
		strcat (at->at_decode, ptr);
		ck_free (ptr);
	}
	at->at_encode[code - encode] = '\0';
	
	if (at->at_encode[0] == '\0')
	{
		ck_free(at->at_encode);
		at->at_encode = NULL;
	}

	DP(("at_decode: encode:%s, decode=%s\n", 
		at->at_encode ? at->at_encode : "<null>", 
		at->at_decode ? at->at_decode : "<null>"));

	/* don't mark the attachment as modified which is different from
	 * decoded.
	 */

	return (ecode);
}






/*
 * Read an attachment from a message, then put it into a file.  There are 3
 * favors of file (filetype):
 *	ATTACH_DATA_FILE -	copy it into a file; user is responsible for it
 *	ATTACH_DATA_TMP_FILE -	copy it into a temporary file (default)
 *	ATTACH_DATA_EXT_FILE -	copy it into a file; the attachment is no longer
 *				lives in the message
 */
static int
at_read(
	struct attach *at,
	char *tmpfile,
	Attach_attr filetype
)
{
	struct stat	statbuf;
	register int	ecode;
	register FILE	*fp;
	int		fd;
	int		mode;


	if (at->at_garbage)
	{
		maillib_methods.ml_warn (NULL, dgettext("SUNW_DESKSET_MAILLIB",
			"This attachment is corrupted!\nPlease delete it."));
		return (-1);
	}

	if (at->at_msg && folder_methods.fm_corrupt(at->at_msg->mo_folder))
	{
		maillib_methods.ml_warn (NULL, dgettext("SUNW_DESKSET_MAILLIB",
"Mail Tool is confused about the state of your mail file\n%s\n\
and cannot determine how to incorporate the changes to\n\
this mail file. Continue discards all changes to the mail\n\
file, restoring the mail file to its original state.\n\
Cancel will cancel this request to save your changes\n\
to the mail file."),
			at->at_msg->mo_folder->fo_file_name);
		return (-1);
	}

	/* attachment resides in the external file; pass back the file path */
	if ((filetype != ATTACH_DATA_FILE) && (at->at_file != NULL))
	{
		/* make sure the file exist */
		strcpy(tmpfile, at->at_file);
		if (stat(tmpfile, &statbuf) == 0)
			return (0);

		/* file does not exist, write the data to file again */
	}

	/* attachment has not been externalized; write it to external file */
	mode = at->at_executable ? 0700 : 0600;
	fd = open(tmpfile, O_WRONLY|O_CREAT|O_TRUNC, mode);
	if (fd < 0) {
		maillib_methods.ml_perror(NULL, errno, 
			"could not open %s", tmpfile);
		return (errno);
	}
	if ((fp = fdopen(fd, "w")) == NULL) {
		maillib_methods.ml_perror(NULL, errno, 
			"could not open %s", tmpfile);
		return (errno);
	}

	if (bm_size(at->at_buffer) > 0)
	{
		ecode = at_decode(at);

		if (ecode) {
			maillib_methods.ml_warn(NULL, dgettext("SUNW_DESKSET_MAILLIB",
		"Could not decode attachment %s\nIgnoring that attachment"),
				at->at_name ?
					(int) at->at_name :
					(int) at_get(at,ATTACH_DATA_TYPE));
			fclose (fp);
			unlink (tmpfile);
			return (ecode);
		}

		mode = AT_BODY;

#ifdef	RFC_MIME
		/* Normally, we only write out the contents.  But if it is
		 * RFC-MIME's multipart or V3 attachment, we need to copy out
		 * the header because it looks like a message and content-type
		 * is very important.
		 */
		if ((bool) at_get(at, ATTACH_IS_MULTIPART))
			mode |= AT_HEADER;
#endif	RFC_MIME

		ecode = at_copy (at, mode, msg_methods.mm_write_bytes, fp);

		if (ecode) {
			maillib_methods.ml_perror(NULL, ecode == 1 ? errno : 0,
		dgettext("SUNW_DESKSET_MAILLIB", "Could not save attachment %s to %s"),
				at->at_name ? 
					(int) at->at_name :
					(int) at_get(at,ATTACH_DATA_TYPE),
			tmpfile);
			fclose (fp);
			unlink (tmpfile);
			return (ecode);
		}

	}

	if (fclose (fp) != 0)
	{
		maillib_methods.ml_perror(NULL, errno,
			dgettext("SUNW_DESKSET_MAILLIB", "error while closing %s"), tmpfile);
		unlink (tmpfile);
		return (-1);
	}

	if (stat (tmpfile, &statbuf) != 0)
	{
		unlink (tmpfile);
		return (-1);
	}

	/* export the attachment to user's filesystem and the user owns it */
	if (filetype == ATTACH_DATA_FILE)
		return (0);

	/* copy out the attachment to either a temp file or externalize
	 * the attachment data for a viewer.  always save the last mod time.
	 */
	at->at_file = strdup (tmpfile);
	at->at_mod_time = statbuf.st_mtime;
	if (filetype == ATTACH_DATA_TMP_FILE)
	{
		/* attachment was written to a tmp file */
		at->at_ftype = DATA_TMP_FILE;
	}
	else
	{
		/* wants to retain the tmp file as external file, mark
		 * the attachment, message and folder been changed.
		 */
		at->at_ftype = DATA_EXT_FILE;
		mark_msg_modified(at);
	}

	/* discard the memory data, get data by mmapping the file.
	 * Note, at_odata may still contain the original data.
	 */
	return (mmap_at_file (at));
}



/*  1/10/97 altered section */ 

static unsigned long
at_copy_enumerate(char *buffer, int size, va_list ap)
{
	typedef unsigned long (*at_copy_enumerate_func)(char *, int, void *,struct msg *);
	unsigned long (*func)(); 
	struct msg *m;
	int param;
        int tcorrupt;
        char *buf;
        char *bufend;
        char *iconvinfo;        /* destination language */
        char *intcode;
	unsigned long error;

	func = va_arg(ap, at_copy_enumerate_func);
	param = (int)va_arg(ap, void *);
	m=va_arg(ap, struct msg *);

	/* Enable encoding convertion  */

	buf=buffer;
	bufend=&buffer[size];

        iconvinfo = NULL;


        /* output the body which does not start with the LF */
        if (bufend != buf) {
		/* if encoding convertion required and not saving the mail call cs_copy */
                iconvinfo = cs_methods.cs_translation_required(m, &intcode); 
                if ((iconvinfo) && (func!=(func_ptr_type_long)msg_methods.mm_write_bytes)){
                   error = cs_methods.cs_copy(iconvinfo, (func_ptr_type) func,buf,size,param);
                } else {
                        error = (*func)(buf, size, param);
                }
        }


	return error;
}



/*
 * Copy the in-memory attachment using specified function.
 * Note, at_buffer may be obsolete if the contents have been written to a file
 * and modified.  In such case, call mmap_at_file() to re-mmap the file.
 */
static int
at_copy(
	struct attach *at,
	int section,
	int (*func)(),
	void *param
)
{
	int	len;
	int	error = 0;
	char	*cookie;
	char	*fmt;
	struct msg *m;
	struct header_obj *p_hdr;
	char	marker[80];

	if (section & (AT_MARKER|AT_END_MARKER))
	{
#ifdef	RFC_MIME
		m = at->at_msg;
		if (cookie = (char *)msg_methods.mm_get(m, MSG_BOUNDARY_MARKER))
		{
			/* AT_MARKER and AT_END_MARKER are mutually exclusive */
			if (section & AT_MARKER)
				fmt = "\n--%s\n";
			else
				fmt = "\n--%s--\n";
			sprintf(marker, fmt, cookie);
			len = strlen (marker);
			ck_free (cookie);
		}
		else
#endif	RFC_MIME
		{
			/* V3 mail format: it includes LF already. */
			strcpy (marker, At_marker);
			len = ATTACHMENT_DASH_LEN;
		}

		if (error = (*func)(marker, len, param))
			return (error);
	}

	if (section & AT_HEADER)
	{
		p_hdr = at->at_hdr;
		while (p_hdr)
		{
			error = (*func) (p_hdr->hdr_start, (p_hdr->hdr_end -
					 p_hdr->hdr_start), param);
			if (error)
				return (error);
			p_hdr = p_hdr->hdr_next;
		}

		/* write out the LF between attachment header and body */
		if (error = (*func) ("\n", 1, param))
			return (error);
	}

	if (section & AT_BODY)
	{
		m = at->at_msg;
		/* loop through all data buffers and write them out */
		error = bm_enumerate(at->at_buffer, at_copy_enumerate,func,param,m);
	}

	return (error);
}






static int
at_is_modified(
	struct attach *at,
	int *p_size
)
{
	register int modified;
	struct stat statbuf;

	*p_size = bm_size (at->at_buffer);
	modified = (at->at_modified || at->at_deleted);

	/* data is not externalized, but it may be changed in memory */
	if (at->at_file == NULL)
		return (modified);

	/* assuming no change if attachment ext file is mysteriously removed */
	if (stat (at->at_file, &statbuf))
		return (modified);

	/* attachment has been marked modified or deleted, save modified time */
	if (modified)
	{
		at->at_mod_time = statbuf.st_mtime;
		*p_size = statbuf.st_size;
		return (1);
	}

	/* attachment tmp file has not been changed. */
	if (at->at_mod_time == statbuf.st_mtime)
		return (0);
	else
	{
		at->at_mod_time = statbuf.st_mtime;
		at->at_modified = 1;
		*p_size = statbuf.st_size;
		return (1);
	}
}






static int
at_modified(
	struct attach *at
)
{
	int	len;

	if (at_is_modified (at, &len))
		return (1);
	return (0);
}






static int
at_count_lines(
	struct attach *at
)
{
	register char *buf;
	register int len;
	register int lines;
	register struct buffer_map *bm;

	lines = 0;
	bm = at->at_buffer;
	while (bm != NULL)
	{
		buf = bm->bm_buffer;
		len = bm->bm_size;
		lines += count_lines (buf, len);

		/* is it a last buffer? */
		if ((bm = bm->bm_next) != NULL)
		{
			/* it is O.K. to have an incomplete line */
			if (buf[len-1] != '\n')
				--lines;
		}
	}
	return (lines);
}







static int
write_attachment(
	struct attach *at,
	int	(*func)(),
	void	*arg,
	int	stype
)
{
	int	len;
	int	ecode;
	int	sections;
	int	add_newline;
	char	buf[128];
	char	*field;
	struct buffer_map *bm;

	/* Copy data from memory.  Assume that data is ready be sent (i.e.
	 * encoded and compression were done) and the attachment structure is
	 * accurate.  Here we may adjust the byte-count and line-count if
	 * the attachment does not a trailing LF.
	 */

	/* If the attachment body is not terminated by a newline and not
	 * sending binary data, add one
	 */
	if (!(stype & S_DONT_ADD_LF) && (bm = bm_last(at->at_buffer)) &&
	    (bm->bm_size > 0) && (bm->bm_buffer[bm->bm_size-1] != '\n'))
		add_newline = 1;
	else
		add_newline = 0;

	len = bm_size (at->at_buffer);
	DP(("write_attachment: total bytes=%d, total lines=%d\n",
		len + add_newline, at->at_lines));

	if (stype & S_CONTENT_LEN)
	{
		/* Save or submit the byte-count header field.  */
		sprintf(buf, "%d", len + add_newline);
		if (stype & S_MSG_CONT_LEN)
			header_set(&at->at_hdr, AT_RFC_FIELD(AT_LEN), buf);
		else
			header_set(&at->at_hdr, AT_SUN_FIELD(AT_LEN), buf);
	}

	if (stype & S_LINE_COUNT)
	{
		/* Save or submit the line-count header field */
		sprintf(buf, "%d", at->at_lines);
		header_set(&at->at_hdr, AT_SUN_FIELD(AT_LINES), buf);
	}

	/* No header is used when converting v3 text body part into message */
	if (stype & S_NO_HEADER)
	{
		sections = AT_BODY;
		if (stype & S_MSG_CONT_LEN)
		{
			sprintf(buf, "%s: %d\n", AT_RFC_FIELD(AT_LEN),
				 len + add_newline);
			ecode = (*func)(buf, strlen(buf), arg);
			if (ecode) return (ecode);
		}
		ecode = (*func)("\n", 1, arg);
		if (ecode) return (ecode);
	}
	else
	{
		if (stype & S_NO_MARKER) {
			sections = AT_HEADER|AT_BODY;
		} else {
			sections = AT_MARKER|AT_HEADER|AT_BODY;
		}
	}

	ecode = at_copy(at, sections, func, arg);
	if (ecode == 0) {
		at->at_modified = 0;
	}

	if (add_newline) {
		int newecode;
		newecode = (*func)("\n", 1, arg);

		if (! ecode) ecode = newecode;
	}

	return (ecode);
}




/*
 * This function is to save an attachment to a file for either submission or
 * saving to a folder with an option not to write the boundary marker.
 *
 * There are 5 possible cases:
 * 1.	to externalize an attachment for Save, or save/submit refereneced
 *	attachment
 * 2.	attachment is never read from folder, attachment is read/decoded but
 *	never modified
 * 3.	D&D to add a raw attachment but never re-modified
 * 4.	modified read attachment from folder, or modified read attachament
 *	from D&D, or externalized attachment for Submission
 * 5.	D&D modifies a raw attachment which is externally stored.
 *
 * Action:
 * 1.	just write out the headers with updated information
 * 2.	get data from at_odata directly
 * 3.	get data from at_buffer, but may need encoding
 * 4.	remmap at_file to at_buffer which may need encoding
 * 5.	get data from at_buffer, write out to the external file
 */

static int
at_save(
	struct attach *at,
	int	(*func)(),
	void	*arg,
	int	flag
)
{
	int	stype;
	int	ecode;
	bool	modified;
	void	*argv[2];
	bool	istext;
	bool	submit;
	bool	msghdr;
	bool	nomarker;
	char	*charset = NULL;
	char	*encode = NULL;
	bool	suppressencode = FALSE;		/* for submission only */
	register bool v3fmt = TRUE;
	struct buffer_map *bm;

	DP(("at_save: at %x, deleted %d, mtype %d\n",
		at, at->at_deleted, at->at_mtype));

	/* Don't submit or save any deleted attachment */
	if (at->at_deleted)
		return (0);

	submit = flag & AT_SAVE_SUBMIT;
	msghdr = flag & AT_SAVE_CONT_LEN;

	/* Determine if we want to write out the boundary marker.  It is
	 * used when we want to save a modified message with content-type
	 * not a multipart or attachment.  Since we implement such message
	 * as a body part, we just leverage the codes.  ZZZ: yes, it is a hack!
	 */
	nomarker = flag & AT_SAVE_NOMARKER;

#ifdef	RFC_MIME
	/* Since this function can be used when msg is not attachment or
	 * multipart, but MSG_OTHER (it should only happen to RFC-MIME msg).
	 * So, if v3fmt is FALSE, the body part written out will be in
	 * RFC-MIME format.
	 */

	/* Headers may be saved in RFC-MIME, but values depend on the msg */
	v3fmt = at_is_v3(at);
#endif	RFC_MIME

	istext = (int) at_get(at, ATTACH_IS_TEXT);

	at->at_format = v3fmt ? AT_SUN : AT_RFC;

	/* Remove the V3 or RFC-MIME dependent headers; we will add back
	 * the appropriated headers back later.
	 */
	at_del_headers(&at->at_hdr);

	/*************************************************************
	 * Case 1: to save the externalized file, or save/submit
	 * a referenced attachment.  Save the attachment headers only.
	 *************************************************************/
	if (at->at_file && ((at->at_ftype == DATA_REF_FILE) ||
			   ((at->at_ftype == DATA_EXT_FILE) && !submit)))
	{
		DP(("at_save: submit=%d external/reference file %s\n",
			submit, at->at_file ? at->at_file : "<null>"));

		/* Temporary switch the encoded codes before set the headers */
		encode = at->at_encode;
		if (at->at_decode)
			at->at_encode = at->at_decode;
		ecode = at_set_headers(at);
		if (ecode) return (ecode);
		at->at_encode = encode;

		/* Always save the externalized filename in case it
		 * has been changed.
		 */
		if (at->at_ftype == DATA_EXT_FILE)
		{
			header_set(&at->at_hdr, AT_SUN_FIELD(AT_DATA_FILE),
				at->at_file);
		}
		
		if (v3fmt)
			header_set(&at->at_hdr, AT_SUN_FIELD(AT_LINES), "0");
		at->at_lines = 0;

		/* Set the length in attachment header to 0 bytes. */
		header_set(&at->at_hdr, AT_SUN_FIELD(AT_LEN), "0");

		if (nomarker)
			stype = AT_HEADER;
		else
			stype = AT_HEADER|AT_MARKER;
		ecode = at_copy(at, stype, func, arg);
		if (ecode == 0)
		{
			/*****************************************************
			 * Case 5: data was stored externally, but it had been
			 * modified (at_buffer is not mmapped from at_file.)
			 *****************************************************/
			if (!at->at_fromfile)
			{
				FILE *ofp;
				if ((ofp = fopen (at->at_file, "w")) != NULL)
				{
					ecode = at_copy(at, AT_BODY,
						msg_methods.mm_write_bytes,
						ofp);
					fclose(ofp);
				}
			}
		}

		/* Free up the content */
		at_free_attachment (at);

		return (ecode);
	}

	/* Caching this information */
	modified = at_modified (at);

	/* Determine which header field to be saved or submitted */
	if (v3fmt)
	{
		/* Don't save any attachment header when converting a v3 text
		 * body part to a message.  But save the Content-Length. Also,
		 * no more encoding/compression because no headers are saved.
		 */
		if (nomarker)
		{
			stype = S_NO_HEADER;
			if (msghdr)
				stype |= S_MSG_CONT_LEN;
			if (at->at_decode)
			{
				ck_free(at->at_decode);
				at->at_decode = NULL;
			}
		}
		else
		{
			stype = submit ? S_LINE_COUNT :
					 S_LINE_COUNT|S_CONTENT_LEN;
		}
	}
	else
	{
		/* If not to write out boundary marker, LF padding is needed. */
		stype = nomarker ? S_NO_MARKER : S_DONT_ADD_LF;
		if (!submit)
			stype |= S_CONTENT_LEN;
		if (msghdr)
			stype |= S_MSG_CONT_LEN;
		if (flag & AT_SAVE_BODY)
                        stype |= S_NO_HEADER; 
	}

	/*************************************************************
	 * Case 2: there is no change to the attachment, use the
	 * original data from folder.
	 *************************************************************/
	if (!modified && (at->at_odata != NULL))
	{
		DP(("at_save: no change; at %x, at_odata=%x, at_buffer=%x\n",
			at, at->at_odata, at->at_buffer));

		encode = at->at_encode;
		if (at->at_decode)
			at->at_encode = at->at_decode;
		ecode = at_set_headers(at);
		if (ecode) return (ecode);
		at->at_encode = encode;
		
		/* Temporary switch to the original data */
		bm = at->at_buffer;
		at->at_buffer = at->at_odata;
		/* Don't add any LF to make it same as in folder. */
		ecode = write_attachment(at, func, arg, (stype|S_DONT_ADD_LF));
		at->at_buffer = bm;
		return (ecode);
	}

	DP(("at_save: Case 3/4: at_file=%s, modified=%d, submit=%d, mtype=%d\n",
		at->at_file ? at->at_file : "<null>", 
		modified, 
		submit, 
		at->at_mtype));

	if (modified && at->at_file)
	{
		/*************************************************************
		 * Case 4: Data in the file has been modified.  Remmap the
		 * file to get the modified raw data and it may need encoding.
		 *************************************************************/
		if (mmap_at_file (at) == 0)
		{
			bm_free (at->at_odata, NULL);
			at->at_odata = NULL;
			ck_free (at->at_encode);
			at->at_encode = NULL;
			ck_free(at->at_charset);
			at->at_charset = NULL;

			DP(("at_save: Case 4: mmap file=%s\n", 
				at->at_file ? at->at_file : "<null>"));
		}
	}

	/*************************************************************
	 * Case 3 or 4: Encoding the attachment data from mapped memory.
	 *************************************************************/
	/* suppress encoding for X.400 submission */
	suppressencode = submit && get_bool("suppressencode");

	/* preform any Content-Encoding such as compression, (and possibly
	 * transfer-encoding from previously decoding).
	 */
	if (!suppressencode)
	{
		argv[0] = (void *) 0600;
		if ((argv[1] = (void *) at->at_name) == NULL)
			argv[1] = (void *) at_get(at, ATTACH_DATA_TYPE);

		if (at_encode(at, argv) != 0)
			goto post_encode;
	}

	/*
	 * if the content was never encoded with Transfer-Encoding,
	 * check if it should be encoded.
	 */
#ifdef	RFC_MIME
	if (at->at_encode && ((strstr(at->at_encode, DO_BASE64) != NULL) ||
			(strstr(at->at_encode, DO_QUOTED_PRINTABLE) != NULL) ||
			(strstr(at->at_encode, DO_UUENCODE) != NULL)))
		goto post_encode;
#else
	if (at->at_encode && strstr(at->at_encode, DO_UUENCODE) != NULL)
		goto post_encode;
#endif	RFC_MIME

	/* construct the transfer encoding information if not 7-bit ASCII,
	 * or need encoding if it is not text and "From " starts at BOL.
	 */
	if ((encode = (char *)bm_enumerate(at->at_buffer, check_encode,
				!istext, v3fmt, &charset)) && !suppressencode)
	{
		char buf[256];

		DP(("at_save: encode is needed\n"));

		/* Default uuencode permission is 600 */
		argv[0] = (void *) 0600;
		if ((argv[1] = (void *) at->at_name) == NULL)
			argv[1] = (void *) at_get(at, ATTACH_DATA_TYPE);

		if (at->at_decode != NULL)
		{
			sprintf (buf, "%s, %s", at->at_decode, encode);
			ck_free (at->at_decode);
			encode = buf;
		}

		at->at_decode = ck_strdup (encode);
		if (at_encode(at, argv) != 0)
			goto post_encode;

		DP(("at_save: buffer is %d bytes long\n",
			bm_size(at->at_buffer)));
	}

post_encode:

#ifdef	RFC_MIME
	deleteheader(&at->at_hdr, AT_CHARSET);
	if (charset)
	{
		if (v3fmt)
		{
			header_set(&at->at_hdr, AT_SUN_FIELD(AT_CHARSET),
					charset);
		}
		else
		{
			co_methods.co_set(&at->at_ctype, AT_PARM_CHARSET, 
					charset);
		}
	}
#endif	RFC_MIME

	ecode = at_set_headers(at);
	if (ecode) return (ecode);

	/*
	 * For X.400 submission only, it requests to send the content-length
	 * header field instead of the line-count field.
	 */
	if (suppressencode)
	{
		stype = S_CONTENT_LEN;

		/* If unencoded binary data is being sent and its last line is
		 * not terminated by LF, don't pad the line with any LF.
		 */
		if (encode)
			stype |= S_DONT_ADD_LF;
	}

	if (v3fmt && (stype & S_LINE_COUNT))
	{
		/* get the line-count. */
		at->at_lines = at_count_lines (at);
	}

	ecode = write_attachment(at, func, arg, stype);
	return (ecode);
}






static char *
charset_name(
	char *locale
)
{
#ifdef	XXX_OW_I18N
	static char *charset = NULL;		/* only allocated once */
	register struct mcent **entries, **p;
	extern struct mcent **getmclist();
	extern void freemclist();

	if (charset == NULL)
	{
		if (locale == NULL)
			*charset = "iso-8859-1";
		else
		{
			entries = getmclist();
			for (p = entries; *p != NULL; p++)
			{
				if (strcasecmp(locale, p->cn_locale) == 0)
				{
					charset = ck_strdup(p->cn_extcode);
					break;
				}
			}
			freemclist(entries);
		}
	}
	return (charset);
#else
	return ("iso-8859-1");
#endif	OW_I18N
}






/*
 * check to see if the attachment can be transmitted by sendmail,
 * or if it needs to be uuencoded...  "enc_from" is to encode the "From "
 * when the data is not text.  "charset" is pointed to a static buffer.
 */
static char *
check_encode1(
	char *buf,
	int len,
	va_list ap
)
{
	register int count = 0;
	register u_char c;
	int maxcount = 900;
	char *from = "From ";
	int fromlen = strlen(from);
	register bool is8bit = FALSE;
	bool enc_from;
	bool v3fmt;
	char **charset;
#ifdef	RFC_MIME
	register bool needqp = FALSE;
#endif	RFC_MIME

#ifdef DEBUG
	int largest = 0;
#endif

	enc_from = va_arg(ap, bool);
	v3fmt = va_arg(ap, bool);
	charset = va_arg(ap, char **);

	*charset = NULL;

	/* the buffer starts with "From ".  its needs encoding... */
	if (enc_from && len > fromlen && !strncmp(buf, from, fromlen)) {
		DP(("check_encode: first line is From\n"));

#ifdef	RFC_MIME
		needqp = TRUE;
		if (v3fmt)
#endif	RFC_MIME
			return (DO_UUENCODE);
	}

	while (--len >= 0) {
		switch (c = *buf++) {
		case '\n':
#ifdef DEBUG
			if (count > largest) largest = count;
#endif
			count = 0;

			/* look to see if the next line begins with "From ".
			 * if it does, then we need to encode the attachment.
			 */
			if (enc_from && (len >= fromlen) &&
			    !strncmp(buf, from, fromlen)) {
				DP(("check_encode: has a \\nFrom\n"));

#ifdef	RFC_MIME
				needqp = TRUE;
				if (v3fmt)
#endif	RFC_MIME
					return (DO_UUENCODE);
			}
			break;

		case '\0':
			/* nulls mean it must be encoded */
#ifdef	RFC_MIME
			if (!v3fmt)
			{
				DP(("check_encode: NULLs seen.  BASE64\n"));
				return (DO_BASE64);
			}
			else
#endif	RFC_MIME
			{
				DP(("check_encode: NULLs seen.  UUENCODE\n"));
				return (DO_UUENCODE);
			}

		default:
			if (++count >= maxcount)
			{
#ifdef	RFC_MIME
				if (!v3fmt)
				{
					needqp = TRUE;
					break;
				}
				else
#endif	RFC_MIME
				{
					DP(("check_encode: line too long. UUENCODE\n"));
					return (DO_UUENCODE);
				}
			}
#ifdef	RFC_MIME
			if (!v3fmt)
			{
				if ((isascii(c) && !isspace(c) && !isprint(c))
				     || (c >= 0177 && c <= 0240))
					return (DO_BASE64);
			}
#endif	RFC_MIME
			if (c >= 0241 && c <= 0377)
			{
				is8bit = TRUE;
#ifdef	RFC_MIME
				needqp = TRUE;
#endif	RFC_MIME
			}
		}
	}

	if (is8bit)
		*charset = charset_name(getenv("LOCALE"));
	else
		*charset = "us-ascii";

#ifdef	RFC_MIME
	if (!v3fmt)
	{
		if (needqp)
		{
			DP(("check_encode: returning QUOTED_PRINTABLE\n"));
			return (DO_QUOTED_PRINTABLE);
		}
		else
		{
			DP(("check_encode: returning DO_7_BIT\n"));
			return (DO_7_BIT);
		}
	}
#endif	RFC_MIME

	DP(("check_encode: returning NULL.  largest line is %d\n", count));
	return (NULL);
}



static unsigned long
check_encode(
	char *buf,
	int len,
	va_list ap
)
{
	return ((unsigned long) check_encode1(buf, len, ap));
}





/*
 * Get the boolean value from either envirnoment variable whose value starts
 * with (t)rue, (y)es, 1, or from in .mailrc whose key may by preceeded
 * with "no" to negate the value.
 */
static int
get_bool(
	char *key
)
{
	char	*not_to;

	not_to = (char *) mt_value (key);
	return (not_to && strpbrk (not_to, "YyTt1"));
}







/* read a copy of the attachment into mapped memory */
static int
at_read_attachment(
	char *path,
	struct attach *at
)
{
	int fd;
	int ecode;
	char *buf;
	struct stat statbuf;

	DP(("at_read_attachment: reading in %s\n", path ? path : "<null>"));

	at_free_attachment (at);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		DP(("at_read_attachment: can't open %s: %d\n",
			at->at_file ? at->at_file : "<null>", errno));
		return (-1);
	}

	if (fstat(fd, &statbuf) < 0) return (-1);

	if ((buf = ck_zmalloc (statbuf.st_size)) == NULL)
	{
		DP(("at_read_attachment: can't ck_zmalloc(%d)\n",
			statbuf.st_size));
		ecode = -1;
	}
	else
	{
		at->at_buffer = bm_alloc (buf, statbuf.st_size);
		at->at_buffer->bm_size = read (fd, buf, statbuf.st_size);
		at->at_mtype = AT_MMAP;
		ecode = 0;
	}

	close(fd);
	return (ecode);
}






/* free the copy of attachment which is either memory mapped or malloc-ed */
static void
at_free_attachment(
	struct attach *at
)
{
	if (at->at_mtype == AT_MMAP)
		bm_free (at->at_buffer, (void (*)()) ck_zfree);
	else if (at->at_mtype == AT_MALLOC)
		bm_free (at->at_buffer, (void (*)()) ck_free);
	else
		bm_free (at->at_buffer, NULL);

	at->at_buffer = NULL;
	at->at_mtype = AT_NONE;
}






char *
header_name(
	struct attach *at,
	int field
)
{
#ifdef	RFC_MIME
	char *value;

	if (at != NULL && (at_is_mime(at) || at_is_other(at)))
	{
		if (((value = AT_RFC_FIELD(field)) != NULL) && (*value != '\0'))
			return (value);
	}
#endif	RFC_MIME

	return (AT_SUN_FIELD(field));
}






static char *
getheader(
	int field,
	char *buf,
	char *bufend
)
{
	char	*sunv3;
	char	*gen;
	char	*rfc;
	char	*svalue = NULL;
	char	*gvalue = NULL;
	char	*rvalue = NULL;

	sunv3 = AT_SUN_FIELD(field);
	gen = AT_GEN_FIELD(field);
#ifdef	RFC_MIME
	rfc = AT_RFC_FIELD(field);
#endif	RFC_MIME

	/* ZZZ: the superset of headers must be first. */
	header_find(buf, bufend,
		sunv3, &svalue,
		gen, &gvalue,
#ifdef	RFC_MIME
		rfc, &rvalue,
#endif	RFC_MIME
		0);

	if (svalue != NULL)
	{
		ck_free (gvalue);
		ck_free (rvalue);
		return (svalue);
	}
	if (gvalue != NULL)
	{
		ck_free (svalue);
		ck_free (rvalue);
		return (gvalue);
	}
	if (rvalue != NULL)
	{
		ck_free (svalue);
		ck_free (gvalue);
		return (rvalue);
	}
	return (NULL);
}






void
deleteheader(
	struct header_obj **hdr,
	int	field
)
{
	char	*buf;

	buf = AT_SUN_FIELD(field);
	if (header_delete (hdr, buf, strlen(buf)) != (struct header_obj *) EOF)
		return;
	buf = AT_GEN_FIELD(field);
	if (header_delete (hdr, buf, strlen(buf)) != (struct header_obj *) EOF)
		return;
#ifdef	RFC_MIME
	buf = AT_RFC_FIELD(field);
	header_delete (hdr, buf, strlen(buf));
#endif	RFC_MIME
}






static int
at_is_boundary(
	struct attach *at,
	char *buf,
	char *bufend
)
{
	int	result;
	char	*marker;
	struct msg *msg;

	/* All boundary marker must start with 2 dashes. */
	if ((*buf++ != '-') || (*buf++ != '-'))
		return (0);

#ifdef	RFC_MIME
	msg = at->at_msg;

	/*  Return no boundary if attachment is a text type. */
	if ((msg != NULL) && !strcasecmp(msg->mo_type, ATTACH_TEXT_TYPE))
	{
		return (NOT_BOUNDARY_MARKER);
	}

	if ((msg != NULL) && (marker = co_methods.co_get(&msg->mo_ctype,
							AT_PARM_BOUNDARY)))
	{
		int	len;

		len = strlen(marker);
		result = strncmp(buf, marker, len);
		ck_free(marker);
		if (result == 0)
		{
			/* Fix for 4009812 - error parsing multi part messages */
			/* Cookie must be terminated with \n */

			/* magic cookie is "--cookie\n" or "--cookie--\n" */
			if (((buf + len + 3) <= bufend) &&
			    ((buf[len] == '-') && (buf[len+1] == '-')
				&& (buf[len+2] == '\n')))
				return (END_MARKER);
			else if (buf[len] == '\n')
				return (BOUNDARY_MARKER);
		}
	}
	else
#endif	RFC_MIME
	{
		result = strncmp(buf, At_marker+2, ATTACHMENT_DASH_LEN-2);
		if (result == 0)
		{
			if ((buf + ATTACHMENT_DASH_LEN - 2) >= bufend)
				return (END_MARKER);
			else
				return (BOUNDARY_MARKER);
		}
	}

	return (NOT_BOUNDARY_MARKER);
}






static int
at_encode(
	struct attach *at,
	void *argv[]
)
{
	int len;
	int isfunc;
	void *func;
	char buf[256];
	char cmd[BUFSIZ];
	char encode[256];
	register u_char *ptr;
	register char *code;
	struct buffer_map *bm;


#ifdef	I18N
	if ((int) at_get(at, ATTACH_IS_TEXT))
	{
		/* if charset is EUC, convert it into ISO-2022-xxx-7.
		 * ZZZZ: somehow we have to remember that the conversion has
		 * been done, so we won't do it more than once.
		 */
		code = at->at_charset;
		if ((code != NULL) && (strncasecmp(code, "iso-2022", 8) == 0))
		{
			bm = dd_methods.dd_conv( , TRUE, at->at_buffer, NULL);
			set_attach_buf (at, AT_MMAP, bm);
		}
	}
#endif	I18N

	/* if there is nothing to encode, just return */
	if ((ptr = (u_char *) at->at_decode) == NULL)
		return (0);

	cmd[0] = '\0';
	buf[0] = '\0';
	encode[0] = '\0';

	while (isspace(*ptr) || (*ptr == ','))
		ptr++;
	while ((code = (char *) ptr) && (*code != '\0'))
	{
		if ((ptr = (u_char *) strpbrk(code, " ,\t\n")) != NULL)
		{
			*ptr++ = '\0';
			while (isspace(*ptr) || (*ptr == ','))
				ptr++;
		}

		func = map_code (code, TRUE, &isfunc, at);
		if (func == NULL)
		{
			/* ZZZ: ignore the code if can't map to a program */
			continue;
		}

		if (!isfunc)
		{
			/* if decoding is done by a process, construct the
			 * pipe and delay the execution until a decoding
			 * performed by a function is needed.
			 */
			if (cmd[0] != '\0')
				strcat (cmd, " | ");
			strcat (cmd, (char *) func);
			if (buf[0] != '\0')
				strcat (buf, ", ");
			strcat (buf, code);
			continue;
		}
			
		if (cmd[0] != '\0')
		{
			/* perform an encoding by processes */
			bm = dd_methods.dd_conv(cmd,FALSE,at->at_buffer,argv);
			DP(("at_encode: %s %s/%s new length is %d\n", 
				cmd ? cmd : "<null>",
				at->at_name ? at->at_name : "<null>", 
				at_get(at,ATTACH_DATA_TYPE) ?
					at_get(at,ATTACH_DATA_TYPE) : "<null>", 
				bm_size(bm)));
			if (bm != NULL)
			{
				if (encode[0] != '\0')
					strcat (encode, ", ");
				strcat (encode, buf);

				/* now reassign the attachment */
				set_attach_buf (at, AT_MMAP, bm);
			}

			cmd[0] = '\0';
			buf[0] = '\0';
		}

		/* perform a decoding done by a function */
		bm = dd_methods.dd_conv (func, TRUE, at->at_buffer, argv);
		if (bm != NULL)
		{
			/* save the encoded information */
			if (encode[0] != '\0')
				strcat (encode, ", ");
			strcat (encode, code);

			/* now reassign the attachment */
			set_attach_buf (at, AT_MMAP, bm);
		}

		DP(("at_encode: %s %s/%s new length is %d\n", 
			code ? code : "<null>",
			at->at_name ? at->at_name : "<null>", 
			at_get(at,ATTACH_DATA_TYPE) ?
				at_get(at,ATTACH_DATA_TYPE) : "<null>", 
			bm_size(bm)));
	}

	if (cmd[0] != '\0')
	{
		/* perform the final encoding by processes */
		bm = dd_methods.dd_conv(cmd, FALSE, at->at_buffer, argv);
		DP(("at_encode: %s %s/%s new length is %d\n", 
			cmd ? cmd : "<null>",
			at->at_name ? at->at_name : "<null>", 
			at_get(at,ATTACH_DATA_TYPE) ?
				at_get(at,ATTACH_DATA_TYPE) : "<null>", 
			bm_size(bm)));
		if (bm != NULL)
		{
			if (encode[0] != '\0')
				strcat (encode, ", ");
			strcat (encode, buf);

			/* now reassign the attachment */
			set_attach_buf (at, AT_MMAP, bm);
		}

		cmd[0] = '\0';
		buf[0] = '\0';
	}

	/* update the encoded info string and remove the decode info string */
	code = "";
	len = strlen (encode);
	if ((ptr = (u_char *) at->at_encode) && (*ptr != '\0'))
	{
		len += strlen((char *) ptr);
		/* if the string is not terminated by comma, add one */
		if ((code = strrchr((char *) ptr, ',')) == NULL) {
			code = ", ";
		}
		else
		{
			code++;
			while (*code == ' ') {
				code++;
			}
			if (*code != '\0') {
				code = ", ";
			}
		}
	}
	at->at_encode = (char *) ck_malloc (len + strlen(code) + 1);
	if (ptr == NULL) {
		at->at_encode[0] = '\0';
	}
	else
	{
		strcpy(at->at_encode, (char *) ptr);
		strcat (at->at_encode, code);
		ck_free (ptr);
	}
	strcat (at->at_encode, encode);
	ck_free (at->at_decode);
	at->at_decode = NULL;

	DP(("at_encode: encode=%s, decode=%s\n", 
		at->at_encode ? at->at_encode : "<null>", 
		at->at_decode ? at->at_decode : "<null>"));

	return (0);
}





/*
 * Set the V3 or RFC-MIME dependent headers.
 */
static int
at_set_headers(
	struct attach	*at
)
{
	char	*type;
	int	size;
	int	error;

	if (at->at_format == AT_SUN)
	{
		header_add(&at->at_hdr, NULL, AT_SUN_FIELD(AT_ENCODING),
				at->at_encode);
		header_add(&at->at_hdr, NULL, AT_SUN_FIELD(AT_LABEL),
				at->at_name);
		header_add(&at->at_hdr, NULL, AT_SUN_FIELD(AT_COMMENT),
				at->at_comment);
		header_add(&at->at_hdr, NULL, AT_SUN_FIELD(AT_CONTENT_TYPE),
				at->at_dtype);
	}
#ifdef	RFC_MIME
	else
	{
		/* AT_RFC */
	    /* probably, uuencode will never happen here, but it
	       doesn't hurt to compare with it last */
		if (at->at_encode)
			if (((strncasecmp(at->at_encode, DO_QUOTED_PRINTABLE,
					  strlen(DO_QUOTED_PRINTABLE)) ==0) ||
			     (strncasecmp(at->at_encode, DO_BASE64,
					  strlen(DO_BASE64)) == 0) ||
			     (strncasecmp(at->at_encode, DO_UUENCODE, 
					  strlen(DO_UUENCODE)) == 0))
			    && !at_is_other(at)) {
			    header_add(&at->at_hdr, NULL,
				       AT_RFC_FIELD(AT_ENCODING), 
				       at->at_encode);
			}
		header_add(&at->at_hdr, NULL, AT_RFC_FIELD(AT_LABEL),
			at->at_name);

		/* sync V3 type with RFC-MIME type/subtype */
		(void) at_get(at, ATTACH_DATA_CLASS);
		size = 1;

		error = co_methods.co_write(&at->at_ctype, co_methods.co_size,
			&size);

		if (error != 0) {
			/* we got an error */
			return (error);
		}

		type = malloc(size);
		if (type == NULL) return;
                *type = '\0';

		error = co_methods.co_write(&at->at_ctype, co_methods.co_concat,
			type);

		if (error != 0) {
			/* we got an error */
			free(type);
			return (error);
		}

		/* Add the content type at the beginning of the header */
		header_add(&at->at_hdr, NULL, AT_RFC_FIELD(AT_CONTENT_TYPE),
			type);

		free(type);
	}
#endif	RFC_MIME

	return (0);
}




/*
 * Remove the V3 and RFC-MIME dependent headers.
 */
static void
at_del_headers(
	struct header_obj **hdr
)
{
	deleteheader(hdr, AT_CONTENT_TYPE);
	deleteheader(hdr, AT_ENCODING);
	deleteheader(hdr, AT_LABEL);
	deleteheader(hdr, AT_COMMENT);

	/*
	deleteheader(hdr, AT_CHARSET);
	deleteheader(hdr, AT_DATA_FILE);
	deleteheader(hdr, AT_REF_FILE);
	*/
}

int 
at_has_coding(
      struct attach *at
)
{
    char *info;
      if(at) {
	  info = at_get(at, ATTACH_ENCODE_INFO);
	  if (!info || (strlen(info)==0)) {
	      info = at_get(at, ATTACH_DECODE_INFO);
	  }
	  /* return false if 7-bit, 8-bit or binary content 
	     transfer encoding */
	  if (info && (strlen(info)!=0)) {
	      if ((strncasecmp(info,DO_BINARY,strlen(DO_BINARY))==0)||
		  (strncasecmp(info,DO_UUENCODE,strlen(DO_UUENCODE))==0)||
		  (strncasecmp(info,DO_7_BIT,strlen(DO_7_BIT))==0)||
		  (strncasecmp(info,DO_8_BIT,strlen(DO_8_BIT))==0)) {
		  return FALSE;
	      } else {
		  return TRUE;
	      }
	  } else {
	      return FALSE;
	  }
      } else
	  return FALSE;
}
