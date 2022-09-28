#ident "@(#)msg.c	3.35 07/02/97 Copyright 1987-1997 Sun Microsystems, Inc."

/* msg.c -- implement the message object */


/* additional header files 1/10/97*/
/* used by mm_enconv */
#include "lcl.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <xview/generic.h>
#include <string.h>
#include "folder.h"
#include "msg.h"
#include "misc.h"
#include "attach.h"
#include "ck_strings.h"
#include "bool.h"
#include "charset.h"
#include "headers.h"
#include "global.h"

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

static char *mm_init();
static void mm_delete();
static int mm_read();
static struct msg *mm_create();
static void mm_destroy();
static int mm_set( struct msg *m, Msg_attr attr, ...);
static void *mm_get(struct msg *m, Msg_attr attr, ...);
static void mm_dump();
static int  mm_copymsg();
static int  mm_copyheader();
static void mm_replace();
static char *mm_read_headers();
static int mm_write_msg();
static int mm_write_attach();
static int mm_write_other();
static int mm_write_bytes();
static int mm_read_bytes();
static void mm_free_headers();
static void mm_free_msg();
static char *mm_header();
static int mm_enumerate();
static struct msg *mm_first();
static struct msg *mm_next();
static struct msg *mm_prev();
static struct msg *mm_last();
static int mm_modified();
static void mm_add_attach();
static void mm_free_body();
static int mm_destroy_attach();
#ifdef	RFC_MIME
static char *mm_gen_boundary();
#endif	RFC_MIME

struct __msg_methods msg_methods = {
	mm_init,
	mm_read,
	mm_create,
	mm_destroy,
	mm_get,
	mm_set,
	mm_copymsg,
	mm_copyheader,
	mm_replace,
	mm_read_headers,
	mm_write_msg,
	mm_write_attach,
	mm_write_other,
	mm_write_bytes,
	mm_read_bytes,
	mm_free_headers,
	mm_free_msg,
	mm_enumerate,
	mm_first,
	mm_next,
	mm_prev,
	mm_last,
	mm_modified,
	mm_add_attach,
#ifdef DEBUG
	mm_dump,
#endif DEBUG
};

extern	char *dgettext();

extern char *findend_of_field();
extern char *findchar();
extern char *findheaderlist();
extern struct header_obj *header_create();
extern struct header_obj *header_split();
extern void header_destroy();
extern char *hackunixfrom();
extern char *skin_arpa();
extern char *get_recipients();
extern char *metoo_usermap();
extern char *b64senc();
extern int  msg_type();
static char *parse_msg();

char Msg_Status[]	= "Status";
char Msg_Content_Type[]	= "Content-Type";
char Msg_MIME_Version[] = "MIME-Version";
char Msg_Charset[]	= "X-Sun-Charset";
char Msg_Content_Len[]	= "Content-Length";
char Msg_Content_Desc[]	= "Content-Description";
char Msg_Reply_To[]	= "Reply-To";
char Msg_Header[]	= "From ";
char Msg_From[]		= "From";	/* From: as opposed to From<space> */
char Msg_Usascii[]	= "US-ASCII";
char Msg_LF[]		= "\n";
#ifdef	LINE_COUNT
char Msg_Lines[]	= "X-Lines";
#endif	LINE_COUNT

#define	MSG_HEADER_SIZE	5
#define	STREQ(a,b)	(!strcmp((a),(b)))
#define	STRNEQ(a,b,c)	(!strncmp((a),(b),(c)))

/*
 * Use by mm_copyheader_wo()
 */
#define	IGNORE_NONE	0
#define	IGNORE_LEN	0x1
#define	IGNORE_TYPE	0x2
#define	IGNORE_CHARSET	0x4
#define IGNORE_DESC     0x10
#define	IGNORE_MASK	0x7

static struct msg *
mm_create( attachment )
bool attachment;
{
	struct msg *m;

	m = (struct msg *) ck_malloc (sizeof(struct msg));
	memset ((char *) m, 0, sizeof(struct msg));

	if (attachment)
#ifdef	RFC_MIME
		m->mo_format = maillib_methods.ml_get(ML_SENDV3) ?
					MSG_ATTACHMENT : MSG_MULTIPART;
#else
		m->mo_format = MSG_ATTACHMENT;
#endif	RFC_MIME

	m->mo_mtype = MSG_NONE;

	return (m);
}

static void
mm_destroy( m )
struct msg *m;
{
	mm_free_body (m);

	msg_methods.mm_free_headers(m);

	ck_free(m);
}

char *mm_enconv(char *buf,int size) 
{
	char *new_buf;
	static LCLd lcld;
        LCTd lctd;
        LclError *ret;
        LclCharsetSegmentSet *segs;
	bool endofline=FALSE;	
	bool error_flag=FALSE;
	static int lcl_init= 0; 


	new_buf=malloc(size+1);
	new_buf[size]=(char)NULL;
	strncpy(new_buf,buf,size);

	/* stripping off \n otherwise lcl will lose them */ 

	if (new_buf[size-1]=='\n'){
		new_buf[size-1]=(char)NULL; 
		/*printf("Inside endofline IF\n"); */
		endofline=TRUE;
	}
	
		/* use lcl encoding convertion routines */	
		if (!lcl_init){
		   lcld = lcl_open(NULL);
		   lcl_init++;
		}
		if (lcld) {
    		      lctd = lct_create(lcld,
                             LctNSourceType, LctNMsgText, new_buf, LctNNone,
                             LctNSourceForm, LctNInComingStreamForm,
                             LctNKeepReference, LctNKeepByReference,
                             NULL);
    		      if (lctd) {
		         ret = lct_getvalues(lctd,
                               LctNDisplayForm,
                               LctNHeaderSegment, &segs,
                               NULL);
    		         if (ret) {
        		    lcl_destroy_error(ret);
    		         }
		         else { strcpy(new_buf,segs->seg[0].segment);	
		                lcl_destroy_segment_set(segs);
		              }
    		         lct_destroy(lctd);
		      }
	        }

		if (endofline) {
			strcat(new_buf,"\n");
			/*printf("Inside endofline IF\n");*/
		}

		return new_buf;
}




static void *
mm_get(struct msg *m, Msg_attr attr, ...)
{
	va_list ap;
	char	*to;
	char	*all;
	char	*subj;
	char	*names;
	char	*cookie;
	int	count;
	char	*header;
	struct attach *at;

	va_start(ap, attr);

	if (m == NULL)
		return(NULL);

	switch (attr)
	{
	case MSG_DELETED:
		return ((void *) m->mo_deleted);

	case MSG_SELECTED:
		return ((void *) m->mo_selected);

	case MSG_CURRENT:
		return ((void *) m->mo_current);

	case MSG_HEADER:
		/* special case the "From:" if "From:" not found, use Unix
		 * "From".  Note, comment in "mo_from" has been stripped, so
		 * it cannot be used for general purpose (such as Find.)
		 */
		header = va_arg(ap, char *);
		names = (char *) mm_header(m, header);
		if (!names && strcasecmp(header, "from") == 0)
			return ((void *) ck_strdup(m->mo_from));
		else
			return ((void *) names);

	case MSG_TEXT_BODY:
		if (m->mo_format != MSG_TEXT)
			return ((void *) NULL);
		return ((void *) m->mo_body_ptr);

	case MSG_ATTACH_BODY:
		return ((void *) m->mo_first_at);

	case MSG_IS_ATTACHMENT:
		return ((void *) (mm_is_mime(m) || mm_is_v3(m)));

	case MSG_TYPE:
		return ((void *) m->mo_format);

	case MSG_REPLY_SUBJECT:
		if (m->mo_subject == NULL)
			return (NULL);
		subj = m->mo_subject;
		if (strncasecmp (subj, "re:", 3) == 0)
			return (ck_strdup (subj));

		/* go and make a new subject line */
		subj = ck_malloc(strlen(subj) + 5);
		sprintf(subj, "Re: %s", m->mo_subject);
		return (subj);

	case MSG_REPLY_TO_SENDER:
		to = get_recipients(m, Msg_Reply_To);
		if (!to && m->mo_from)
			to = ck_strdup(m->mo_from);
		return (to);

	case MSG_REPLY_TO_ALL:
		to = get_recipients(m, Msg_Reply_To);
		if (!to && m->mo_from)
			to = ck_strdup(m->mo_from);
		names = ck_strdup (m->mo_to);
		if (!names)
			return (to);
		all = ck_malloc (strlen(names) + strlen (to) + 3);
		sprintf (all, "%s, %s", names, to);
		ck_free (names);
		names = metoo_usermap (all);
		if (!names)
			return (to);
		ck_free (to);
		return (names);

	case MSG_REPLY_TO_CC:
		return (get_recipients(m, "cc"));

	case MSG_NUM_BYTES:
		/* add 2 because of LF between hdrs and body, and LF at eom */
		return ((void *) (header_length(m->mo_hdr) + 2 + m->mo_len));

	case MSG_CONTENT_LEN:
		return ((void *) m->mo_len);

	case MSG_NUM_LINES:
#ifdef	LINE_COUNT
		return ((void *) (header_lines(m->mo_hdr)+ 2 +m->mo_num_lines));
#else
		return ((void *) m->mo_num_lines);
#endif	LINE_COUNT

	case MSG_HEADERS:
	{
		int (*func)();
		int param;

		func = (int (*)()) va_arg(ap, void *);
		param = va_arg(ap, int);
		return ((void *) header_enumerate(m->mo_hdr, func, param));
	}

	case MSG_FROM:
		return ((void *) m->mo_unix_from);

	case MSG_DATE:
		return ((void *) m->mo_unix_date);

	case MSG_CONTENT_LINES:
		return ((void *) (m->mo_num_lines+1));

	case MSG_MODIFIED:
		return ((void *) m->mo_modified);

	case MSG_NUM_PARTS:
		count = 0;
		for (at = attach_methods.at_first(m); at != NULL;
		     at = attach_methods.at_next(at))
		{
			if (!(bool)attach_methods.at_get(at, ATTACH_DELETED))
				count++;
		}
		return ((void *) count);

#ifdef	RFC_MIME
	case MSG_BOUNDARY_MARKER:
		if (m->mo_format != MSG_MULTIPART)
			return ((void *) NULL);
		return ((void *)co_methods.co_get(&m->mo_ctype,
						AT_PARM_BOUNDARY));
	case MSG_SUBTYPE:
		return (m->mo_subtype);
#endif	RFC_MIME

	default:
		return ((void *) NULL);
	}
}

static int
mm_set( struct msg *m, Msg_attr attr, ...)
{
	va_list ap;

	va_start(ap, attr);

	switch (attr)
	{
	case MSG_DELETED:
		m->mo_deleted = va_arg(ap, int);
		m->mo_folder->fo_changed = 1;
		break;

	case MSG_SELECTED:
		m->mo_selected = va_arg(ap, int);
		break;

	case MSG_CURRENT:
		m->mo_current = va_arg(ap, int);
		break;

	/* 4 jan 94, Neil Katin
	 * adding a special case; when setting a header to value
	 * NULL (not ""), this really means delete the header.
	 * This is needed to clear out old fields that are no longer
	 * used...
	 */
	case MSG_HEADER:
	{
		char *name;
		char *value;

		name = va_arg(ap, char *);
		value = va_arg(ap, char *);

		if (value == NULL) {
			if (header_delete(&m->mo_hdr, name, strlen(name))
				== EOF)
			{
				return (-1);
			}
		} else if (header_set(&m->mo_hdr, name, value) == NULL) {
			return (-1);
		}
		break;
	}
	
	case MSG_TEXT_BODY:
	case MSG_MMAP_TEXT_BODY:
	case MSG_MALLOC_TEXT_BODY:
		m->mo_format = MSG_TEXT;
		mm_free_body (m);
		if (attr == MSG_MMAP_TEXT_BODY)
			m->mo_mtype = MSG_MMAP;
		else if (attr == MSG_MALLOC_TEXT_BODY)
			m->mo_mtype = MSG_MALLOC;
		else
			m->mo_mtype = MSG_NONE;
		m->mo_body_ptr = va_arg(ap, char *);
		break;

	case MSG_ATTACH_BODY:
	{
		struct attach *attach;
		unsigned pos;

#ifdef	RFC_MIME
		/*
		 * Mark the message to be RFC-MIME multipart if the old message
		 * is not in a V3 attachment format and the sending message
		 * (new) in not in V3 format, or the old message is MSG_OTHER.
		 */
		if (mm_is_other(m) || mm_is_mime(m) || (mm_is_text(m) &&
		    !(bool)maillib_methods.ml_get(ML_SENDV3)))
		{
			/* Mark it as the RFC-MIME message now. */
			m->mo_format = MSG_MULTIPART;
		}
		else
#endif	RFC_MIME
		{
			/* For V2 enclosure or V3 attachment */
			m->mo_format = MSG_ATTACHMENT;
		}
		attach = va_arg(ap, struct attach *);
		pos = va_arg(ap, unsigned);
		mm_add_attach(m, attach, pos);
		break;
	}

	case MSG_IS_ATTACHMENT:
		if (va_arg(ap, int) == MSG_TEXT) {
			m->mo_format = MSG_TEXT;
		}
		else
		{
			m->mo_format = (bool)maillib_methods.ml_get(ML_SENDV3) ?
					MSG_ATTACHMENT : MSG_MULTIPART;
		}
		break;

	case MSG_CONTENT_LEN:
		m->mo_len = va_arg(ap, int);
		break;

	case MSG_NUM_LINES:
		m->mo_num_lines = va_arg(ap, int);
		break;

	case MSG_DESTROY_ATTACH:
		return (mm_destroy_attach (m, va_arg(ap, unsigned)));

#ifdef	RFC_MIME
	case MSG_BOUNDARY_MARKER:
	{
		char	*cookie;
		char	*orig_cookie;

		cookie = orig_cookie = va_arg(ap, char *);
		if (cookie == NULL) {
			cookie = mm_gen_boundary();
		}
		co_methods.co_set(&m->mo_ctype, AT_PARM_BOUNDARY, cookie);
		if (orig_cookie == NULL) {
			ck_free(cookie);
		}
		return (-1);
	}
#endif	RFC_MIME

	default:
		return (-1);
	}

	return (0);
}

/*
 * handle the status field specially -- ignore its old value, and look at
 * the msg structure
 */
static int
copystatus(m, func, param)
struct msg *m;
int (*func)();
int param;
{
	char buffer[20];

	if (!m->mo_read && m->mo_new)
		return (0);

	sprintf( buffer, "%s: %s%s\n", Msg_Status,
		(m->mo_read) ? "R" : "",
		(m->mo_new) ? "" : "O" );

	return ((*func)(buffer, strlen(buffer), param));
}

static int
copylen(m, func, param)
struct msg *m;
int (*func)();
int param;
{
	char buffer[40];

	sprintf( buffer, "%s: %d\n", Msg_Content_Len, m->mo_len);
	return ((*func)(buffer, strlen(buffer), param));
}

static int
copytype(m, func, param)
struct msg *m;
int (*func)();
int param;
{
	char	*buffer;
	int	size;
	int	error;

	size = 3 + strlen(Msg_Content_Type) + strlen(Msg_LF);
	error = co_methods.co_write(&m->mo_ctype, co_methods.co_size, &size);
	if (error != 0) return (error);

	buffer = malloc(size);
	if (buffer == NULL) return (1);

	sprintf( buffer, "%s: ", Msg_Content_Type);
	error = co_methods.co_write(&m->mo_ctype, co_methods.co_concat, buffer);
	if (error != 0) {
		free(buffer);
		return (error);
	}
	strcat(buffer, Msg_LF);
	error = (*func)(buffer, strlen(buffer), param);

	free(buffer);
	return (error);
}

#ifdef	LINE_COUNT
static int
copylines(m, func, param)
struct msg *m;
int (*func)();
int param;
{
	char buffer[40];

	sprintf (buffer, "%s: %d\n", Msg_Lines, m->mo_num_lines);
	return ((*func)(buffer, strlen(buffer), param));
}
#endif	LINE_COUNT

/*
 * write out headers, using a user provided write routine
 * if ignore_headers == MSG_FULL_HDRS, all headers will be ouptut.
 * if ignore_headers == MSG_ABBR_HDRS, output all headers except ignored.
 * if ignore_headers == MSG_SAVE_HDRS, output abbreviated and required headers
 */
static int
mm_copyheader(m, ignore_headers, func, param)
struct msg *m;
int ignore_headers;
int (*func)();
int param;
{
	return (mm_copyheader_wo (m, ignore_headers, func, param, IGNORE_NONE));
}

/*
 * write out headers except the specified, using a user provided write routine
 * with folder corruption checking (unless MSG_DONT_TST is specified).
 * if ignore_headers == MSG_FULL_HDRS, all headers will be ouptut.
 * if ignore_headers == MSG_ABBR_HDRS, output all headers except ignored.
 * if ignore_headers == MSG_SAVE_HDRS, output abbreviated and required headers
 */
int
mm_copyheader_wo(m, ignore_headers, func, param, ignore)
struct msg *m;
int ignore_headers;
int (*func)();
int param;
int ignore;
{
	register char *buf;
	register char *bufend;
	register char *end;
	int error = 0;
	int tcorrupt;
	bool needstatus = TRUE;
	bool needfrom = TRUE;	/* or so says RFC 822 and dtmail */
#ifdef	LINE_COUNT
	bool needlines = TRUE;
#endif	LINE_COUNT
	bool needcontentdesc = !(ignore & IGNORE_DESC);
	bool needcontentlen = !(ignore & IGNORE_LEN);
	bool needcontentype = !(ignore & IGNORE_TYPE);
	bool needcharset    = !(ignore & IGNORE_CHARSET);
	char name[BUFSIZ];
	register bool first = TRUE;
	register struct header_obj *p_hdr;
	char *new_buf;


	/* test optionally if the folder is corrupted */
	tcorrupt = ignore_headers & MSG_DONT_TST;
	if (!tcorrupt && folder_methods.fm_corrupt(m->mo_folder))
		return (-1);

	/* mask out the unexpected values */
	ignore_headers &= MSG_MASK;

	/* output the headers */
	p_hdr = m->mo_hdr;
	while (p_hdr) {

		buf = p_hdr->hdr_start;
		bufend = p_hdr->hdr_end;

		if (first && strncmp(buf, Msg_Header, MSG_HEADER_SIZE) == 0)
		{
			/* always output the "From " line */
			end = findend_of_field(buf, bufend);
			if (error = (*func)(buf, end - buf, param))
				return (error);
			buf = end;
		}
		first = 0;

		while (buf < bufend) {

			end = findend_of_field(buf, bufend);

			/* get the header name */
			getheadername(name, sizeof(name), buf, end);


			if ((ignore_headers == MSG_FULL_HDRS) ||
			    (ignore_headers == MSG_ABBR_HDRS &&
			     !ignore_field(name)) ||
			    (ignore_headers == MSG_SAVE_HDRS &&
			     (save_field(name) || !ignore_field(name))))
			{
				if (!strcasecmp(name, Msg_Status)) {
					/* handle status specially.  only
					 * output it once...
					 */
					if (needstatus) {
						error = copystatus(m, func,
								param);
						if (error) return (error);
						needstatus = 0;
					}
				} else if (!strcasecmp(name, Msg_Content_Len)) {
					if (needcontentlen) {
						/* write the correct content
						 * length now.  Otherwise, do
						 * it later if it is incorrect.
						 */
						error = copylen(m, func, param);
						if (error) return (error);
						needcontentlen = 0;
					}
				} else if (!strcasecmp(name, Msg_Content_Type)){
					if (needcontentype && m->mo_type) {
						error = copytype(m,func,param);
						if (error) return (error);
						needcontentype = 0;
					}
				} else if (!strcasecmp(name, Msg_Charset)){
					if (needcharset) {
						error = (*func)(buf, end - buf, param);
						if (error) return (error);
						needcharset = 0;
					}
#ifdef	LINE_COUNT
				} else if (!strcasecmp(name, Msg_Lines)) {
					if (needlines) {
						error = copylines(m,func,param);
						if (error) return (error);
						needlines = 0;
					}
#endif	LINE_COUNT
				} else if (!strcasecmp(name, Msg_From)) {
					if (needfrom) {
						error = (*func)(buf, end - buf, param);
						if (error) return (error);
						needfrom = 0;
					}
				} else if (!strcasecmp(name, Msg_Content_Desc)){
					if (needcontentdesc) {
						error = (*func)(buf, end - buf, param);
						if (error) return (error);
						needcontentdesc = 0;
					}
				} else {
					/* output the Subject field */
					if (!strcasecmp(name, "Subject")){
					   new_buf=mm_enconv(buf,end-buf);
			   		   (*func)(new_buf,strlen(new_buf), param); 
					   free(new_buf);
					}
					else {
					      error = (*func)(buf, end - buf, param);
					      if (error) return (error);
					     }
				}
			}
			buf = end;
		}

		p_hdr = p_hdr->hdr_next;
	}

	/* if we haven't output content length yet, then output it */
	if (needcontentlen && (ignore_headers != MSG_ABBR_HDRS ||
	    ((ignore_headers == MSG_ABBR_HDRS) &&
	     !ignore_field(Msg_Content_Len))))
	{
		error = copylen(m, func, param);
		if (error) return (error);
	}

	/* if we haven't output content type yet, then output it */
	if (needcontentype && m->mo_type && (ignore_headers != MSG_ABBR_HDRS ||
	    ((ignore_headers == MSG_ABBR_HDRS) &&
	     !ignore_field(Msg_Content_Type))))
	{
		error = copytype(m, func, param);
		if (error) return (error);
	}

#ifdef	LINE_COUNT
	/* if we havent't output the line count, yet, then output it */
	if (needlines && (ignore_headers != MSG_ABBR_HDRS ||
	    ((ignore_headers == MSG_ABBR_HDRS) && !ignore_field(Msg_Lines))))
	{
		error = copylines(m, func, param);
		if (error) return (error);
	}
#endif	LINE_COUNT

	/* if we haven't output a status yet, and we should, then output it */
	if (needstatus && (ignore_headers != MSG_ABBR_HDRS || 
	    ((ignore_headers == MSG_ABBR_HDRS) && !ignore_field(Msg_Status))))
	{
		error = copystatus(m, func, param);
		if (error) return (error);
	}

	return (error);
}



/* utility routine to only write header line if not ignored */
int
write_header(name, line, func, param, ignore_headers)
char *name;
char *line;
int (*func)();
int param;
int ignore_headers;
{
	DP(("write_header: line %s", line));

	/* mask out unused bits */
	ignore_headers &= MSG_MASK;
	if ((ignore_headers == MSG_FULL_HDRS) ||
		(ignore_headers == MSG_ABBR_HDRS && !ignore_field(name)) ||
		(ignore_headers == MSG_SAVE_HDRS && (save_field(name) ||
			!ignore_field(name))))
	{
		DP(("write_header: writing...\n"));
		return ((*func)(line, strlen(line), param));
	}

	return (0);
}


/* utility routine to count the number of bytes that will be output, and whether
 * they are 8 bit clean
 */
int
count_bytes(ptr, size, p)
unsigned char *ptr;
int size;
struct count_bytes_param *p;
{
	p->length += size;

	if (!p->not_seven_bit_clean) {
		while (size--) {
			if (*ptr++ & 0x80) {
				p->not_seven_bit_clean = 1;
				break;
			}
		}
	}
	return(0);
}

/*
 * output an entire message using a user provided write routine if optional
 * folder corruption checking (unless MSG_DONT_TST is specified).
 * if ignore_headers == MSG_FULL_HDRS, all headers will be ouptut.
 * if ignore_headers == MSG_ABBR_HDRS, output all headers except ignored.
 * if ignore_headers == MSG_SAVE_HDRS, output abbreviated and required headers
 */
static int
mm_copymsg(m, ignore_headers, func, param, screen)
struct msg *m;
int ignore_headers;
int (*func)();
int param;
int screen;
{
	int error;
	int tcorrupt;
	char *buf;
	char *bufend;
	char *iconvinfo;	/* destination language */
	char *intcode;
	int ignore_fields;
	struct count_bytes_param count;

	DP(("mm_copymsg: called.  ignore_headers 0x%x\n", ignore_headers));

	tcorrupt = ignore_headers & MSG_DONT_TST;
	if (!tcorrupt && folder_methods.fm_corrupt(m->mo_folder))
		return (-1);

	/* fix for bug # 1118315: switch to mm_copyattach() if this
	 * beast has attachments; mo_body_ptr may not be valid in this
	 * case...
	 */
	if ((attach_methods.at_first(m) != NULL)) {
		return(mm_copyattach(m, ignore_headers, 0, func, param, screen));
	}

	/* for bug # 1096353: later on this routine we may be translating
	 * a message to a different character set.  If we are, we need to
	 * replace the character set and content length header lines.
	 *
	 * we do that by first counting the size of the translated message;
	 * outputing the new size and character set, and the re-translating them
	 * message for the "real" output.
	 */
	buf = m->mo_body_ptr;
	bufend = &m->mo_body_ptr[m->mo_len];
	iconvinfo = NULL;
	ignore_fields = IGNORE_NONE;

	if (bufend != buf) {
		iconvinfo = cs_methods.cs_translation_required(m, &intcode);
		if (iconvinfo) {

			count.length = 0;
			count.not_seven_bit_clean = 0;
			error = cs_methods.cs_copy(iconvinfo, count_bytes, buf, m->mo_len,
				(int) &count);

			DP(("mm_copymsg: intcode %s. Old size %d, new size %d, 8 bit %d\n",
				intcode, m->mo_len, count.length, count.not_seven_bit_clean));

			if (count.not_seven_bit_clean == 0) {
				/* there are only 7 bit chars in here... */
				intcode = Msg_Usascii;
			}

			ignore_fields = IGNORE_LEN | IGNORE_CHARSET;
		}
	}

	error = mm_copyheader_wo(m, ignore_headers, func, param, ignore_fields);
	if (error) return (error);

	if (iconvinfo) {
		char buffer[128];

		/* output the new charset and the new len */
		sprintf(buffer, "%s: %s\n", Msg_Charset, intcode);
		if (write_header(Msg_Charset, buffer, func, param,
			ignore_headers))
		{
			return (error);
		}

		sprintf(buffer, "%s: %d\n", Msg_Content_Len, count.length);
		if (write_header(Msg_Content_Len, buffer, func, param,
			ignore_headers))
		{
			return(error);
		}

	}

	/* always output a blank line between the headers and the body */
	error = (*func)(Msg_LF, 1, param);
	if (error) return (error);

	/* output the body which does not start with the LF */

	if (bufend != buf) {
		if (iconvinfo) {
			error = cs_methods.cs_copy(iconvinfo, func, buf,
				m->mo_len, param);
		} else {
			error = (*func)(buf, m->mo_len, param);
		}
		if (error) return (error);
	}

	/* always output a blank line after the body */
	error = (*func)(Msg_LF, 1, param);

	return (error);
}

static int
mm_read (msg, buf, bufend)
struct msg *msg;
char *buf;
char *bufend;
{
	memset ((char *) msg, 0, sizeof (*msg));
	(void) parse_msg (msg, buf, bufend, 1);

	return (0);
}

static char *
mm_init (msg, buf, bufend)
struct msg *msg;
char *buf;
char *bufend;
{
	buf = parse_msg (msg, buf, bufend, 0);
	return (buf);
}

static char *
parse_msg (msg, buf, bufend, single_msg)
struct msg *msg;
char *buf;
char *bufend;
bool single_msg;
{
	char	*bufstart = buf;
	register int num_lines;
#ifdef	V2
	char	*skip_begin();
#endif	V2

	/* read the headers */
	buf = msg_methods.mm_read_headers( msg, buf, bufend );
	msg->mo_body_ptr = buf;

	/* validate the size field in header?
	 * Note, SVR4 mail and mailx may write out only 1 blank line between
	 * messages if msg has no contents, and not generate content-length
	 * header.  In this case, we don't validate content-length, but
	 * just scan for next "From ".
	 */
	if ((msg->mo_len > 0) && (validate_length(msg, bufend) >= 0))
	{
		if (msg->mo_num_lines < 0)
		{
			/* no line field */
#ifdef	LINE_COUNT
			msg->mo_num_lines = count_lines (buf, msg->mo_len);
#else
			msg->mo_num_lines += count_lines (buf, msg->mo_len);
#endif	LINE_COUNT
		}
		buf = msg->mo_body_ptr + msg->mo_len + 1;
	}
	else
	{
		/* no line field, no size field or the size field was wrong */

		/* scan for the start of the next message */
		num_lines = 0;
		while( buf < bufend ) {
			/* look for "From " only if it is not a single msg */
			if (!single_msg &&
			    STRNEQ( buf, Msg_Header, MSG_HEADER_SIZE ))
			{
				bufend = buf;
				break;
			}

			buf = findchar( '\n', buf, bufend );
			if (buf < bufend)
				buf++;
			num_lines++;
		}

		msg->mo_len = bufend - msg->mo_body_ptr;
		if (!single_msg) {
			/* len excludes the blank line before "From " or EOF */
			if ((msg->mo_len > 0) && (bufend[-1] == '\n') &&
			    (bufend[-2] == '\n')) {
				--num_lines;
				--msg->mo_len;
			}
		}

#ifdef	LINE_COUNT
		msg->mo_num_lines = num_lines;
#else
		msg->mo_num_lines += num_lines;
#endif	LINE_COUNT
	}

#ifdef	V2
	/* Compatible with V2 enclosure.  We convert it into an attachment */
	if (mm_is_text(msg) && msg->mo_subject &&
	    (strncmp(msg->mo_subject, "(E) ", 4) == 0))
	{
		long	mode;
		char	*ptr;
		char	name[128];
		struct attach *at;

		at = (struct attach *) ck_malloc (sizeof(struct attach));
		msg->mo_first_at = at;
		memset ((char *) at, 0, sizeof(struct attach));
		attach_methods.at_set (at, ATTACH_MSG, msg);
		attach_methods.at_parse (at, bufstart, buf);
		header_destroy (at->at_hdr);
		at->at_hdr = NULL;
		at->at_format = AT_SUN;

		mm_set(msg, MSG_HEADER, "Content-Type", ATTACHMENT_TYPE);
		msg->mo_type = ck_strdup(ATTACHMENT_TYPE);
		msg->mo_format = MSG_ATTACHMENT;

		/* V2 enclosure always compresses and uuencode its
		 * data, and the name always has the ".Z{}" suffix.
		 * Since the data type is not marked, we assume it
		 * is "binary" (SVR4 compatible) data type.
		 */
		attach_methods.at_set(at, ATTACH_DECODE_INFO,
					"compress, uuencode");
		attach_methods.at_set(at, ATTACH_DATA_TYPE, "binary");

		/* Find the name and check if it is executable */
		(void) skip_begin(msg->mo_body_ptr, buf-1, &mode, name);

		/* Strip off the suffix. */
		if ((ptr = strrchr(name, '{')) && (ptr[-1] == 'Z'))
			ptr[-2] = '\0';
		attach_methods.at_set(at, ATTACH_DATA_NAME, name);

#ifdef	RFC_MIME
		/* Add the V3 attachment headers */
		attach_methods.at_set_headers(at);
#endif	RFC_MIME

		if (mode & S_IXUSR)
			attach_methods.at_set(at, ATTACH_EXECUTABLE, TRUE);

		msg->mo_modified = 1; 
		if (msg->mo_folder)
			msg->mo_folder->fo_changed = 1;
		
	}
	else
#endif	V2

	/* set up the attachments if it is an attachment-type message */
	if (mm_is_v3(msg) || mm_is_mime(msg))
	{
		MP(("parse_msg: mo_body_ptr=%x\n", msg->mo_body_ptr));

		/* buf-1 points at the newline from "\nFrom " or \nEOF. */
		if (msg_add_attachments (msg, msg->mo_body_ptr, buf-1) == 0)
		{
			DP(("msg_no %d is a text\n", msg->mo_msg_number));

			/* Detect some inconsistency about the content-type */
			msg->mo_modified = 1;
			if (msg->mo_folder)
				msg->mo_folder->fo_changed = 1;
			

			msg->mo_format = MSG_TEXT;
			co_methods.co_destroy (&msg->mo_ctype);
		}
	}
	else if (mm_is_other(msg))
	{
		struct attach *at;

		/* ZZZ: this is a hack to temporarily convert the message
		 * into an attachment.  The real implementation is to treat
		 * everything as multipart; the text or binary message is
		 * just a degenerated case of multipart.
		 */
		at = (struct attach *) ck_malloc (sizeof(struct attach));
		msg->mo_first_at = at;
		memset ((char *) at, 0, sizeof(struct attach));
		attach_methods.at_set (at, ATTACH_MSG, msg);
		attach_methods.at_parse (at, bufstart, buf-1);
	}

	MP(( "%3d: %-16s %14s 0x%7x 0x%7x %3d/%d\n",
		msg->mo_msg_number, msg->mo_unix_from, msg->mo_unix_date,
		msg->mo_hdr->hdr_start, msg->mo_body_ptr,
		mm_get(msg, MSG_NUM_LINES), mm_get(msg, MSG_NUM_BYTES) ));
	MP(( "     '%s'\n", msg->mo_from ));
	MP(( "     '%s'\n", msg->mo_to ));
	MP(( "     '%s'\n", msg->mo_date ));
	MP(( "     '%s'\n", msg->mo_subject ));

	return (buf);
}

int
msg_type (type, subtype)
char *type;
char *subtype;
{
#ifdef	RFC_MIME
	if (strcasecmp(type, ATTACH_MULTIPART_TYPE) == 0)
		return (MSG_MULTIPART);
#endif	RFC_MIME

	if ((strcasecmp(type, "x-sun-attachment") == 0) ||
	    (strcasecmp(type, "x-multipart") == 0) ||
	    (strcasecmp(type, "attachment") == 0))
		return (MSG_ATTACHMENT);
	else if ((strcasecmp(type, ATTACH_TEXT_TYPE) == 0) && 
		 (subtype == NULL))
	    return (MSG_TEXT);
#ifdef	RFC_MIME
	/* Override the default if the content type variable is set to "text".
	 * For example: "set x-uuencode-apple-single=text" in .mailrc will
	 * show a Quick-Mail message with attachments as text.
	 */
	else if (type = (char *)mt_value(type))
		return ((strcasecmp(type, "text") == 0) ? MSG_TEXT : MSG_OTHER);
	return (MSG_OTHER);
#else
	return (MSG_TEXT);
#endif	RFC_MIME
}

/*
 * scan a message, picking out all the relevant headers
 */
static char *
mm_read_headers( msg, buf, bufend )
struct msg *msg;
char *buf;
char *bufend;
{
	char *to;
	char *end;
	char *length;
	char *lines;
	char *type;
	char *from;
	char *sender;
	char *status;
	char *start = buf;
	int num_lines;

	/* the first line is UNIX magic -- it looks like
	 * "From person day mon date hour:min:sec year"
	 */
	if (((*start == 'F') || (*start == 'f')) && (start[4] == ' '))
	{
		buf = hackunixfrom( start, bufend, &msg->mo_unix_from,
				&msg->mo_unix_date );
		num_lines = 1;
		buf++;
	}
	else
	{
		num_lines = 0;
		msg->mo_unix_from = NULL;
		msg->mo_unix_date = NULL;
	}

	/* find the end of the headers section */
	end = buf;
	while (end < bufend) {
		end = findchar('\n', end, bufend);
		num_lines++;

		/* check for end of buffer or a blank line */
		if (end >= bufend) break;
		if (++end >= bufend) break;

		if (*end == '\n') {
			/* we found a blank line! */
			num_lines++;
			break;
		}
	}

	/* create a single node to contain all headers which start with the
	 * "From xxx", "hdr_end" points at the '\n'
	 */
	msg->mo_hdr = header_create( start, end );

	/* now find the standard 822 header lines */
	header_find( buf, end,
		"sender",	&sender,
		"from",		&from,
		"to",		&to,
		"date",		&msg->mo_date,
		"subj",		&msg->mo_subject, /* compatibility hack */
		"subject",	&msg->mo_subject,
		"status",	&status,
		Msg_Content_Type, &type,
		Msg_Content_Len, &length,
#ifdef	LINE_COUNT
		Msg_Lines,	&lines,
#endif	LINE_COUNT
		0 );

	if (!from)
		from = sender;
	else if (sender)
		ck_free(sender);
		
	if (from) {
		msg->mo_from = skin_arpa(from);
		ck_free(from);
	} else {
		msg->mo_from = ck_strdup(msg->mo_unix_from);
	}

	if (to) {
		msg->mo_to = skin_arpa(to);
		ck_free(to);
	}

	msg->mo_read = 0;
	msg->mo_new = 1;
	if (status) {
		if (strchr(status, 'R')) msg->mo_read = 1;
		if (strchr(status, 'O')) msg->mo_new = 0;
		ck_free(status);
	}

	/* because the msg is new, it is marked modified so save changes
	 * will update the status flag.
	 */
	if (msg->mo_new) {
		msg->mo_modified = 1;
		if (msg->mo_folder)
			msg->mo_folder->fo_changed = 1;
	}
	 

	if (type != NULL)
	{
		co_methods.co_read(&msg->mo_ctype, type, type+strlen(type));
		ck_free(type);
	}

	if (msg->mo_type)
		msg->mo_format = msg_type(msg->mo_type, msg->mo_subtype);
	else
		msg->mo_format = MSG_TEXT;

	msg->mo_len = -1;
	if (length) {
		msg->mo_len = atoi(length);
		ck_free(length);
	}

#ifdef	LINE_COUNT
	msg->mo_num_lines = -1;
	if (lines)
	{
		msg->mo_num_lines = atoi(lines);
		ck_free(lines);
	}
#else
	msg->mo_num_lines = num_lines;
#endif	LINE_COUNT

	/* message body starts after the newline */
	if (end >= bufend)
		return (bufend);
	else
		return (end + 1);
}


static int
validate_length( msg, bufend )
struct msg *msg;
char *bufend;
{
	char *end_of_msg;

	/* (mo_body_ptr+mo_len) lands on "\nFrom ", we add 1 to compensate it */
	end_of_msg = msg->mo_body_ptr + msg->mo_len + 1;

	if( end_of_msg == bufend ) {
		return (msg->mo_len);
	}

	if( end_of_msg < bufend ) {
		/* check if lands on "From " */
		if (STRNEQ( end_of_msg, Msg_Header, MSG_HEADER_SIZE )) {
			return (msg->mo_len);
		}
	}

	DP(("validate_length: failed msg: %.128\n\n\n", msg->mo_body_ptr));

	return (-1);
}


/*
 * replace the contents of a message with a new message.  We now "own"
 * the buffer that is passed in, and will free it when we are done with
 * the message.  Note, "buf" must come from malloc().
 */
static void
mm_replace( m, buf, len )
struct msg *m;
register char *buf;
register int len;
{
	char 		buflen[16];
	int		format;
	struct cotype	motype;
	register int	num_lines;
	register char	*bufend;
	register char	*body_ptr;

	/* there was a "content-type" field, but it was an "ignored" field */
	if (ignore_field (Msg_Content_Type) && (m->mo_type != NULL))
		co_methods.co_copy (&motype, &m->mo_ctype);
	else
		motype.co_type = NULL;

	format = m->mo_format;

	/* free up all the old header info */
	msg_methods.mm_free_headers(m);

	/* find out the length of the message context */
	body_ptr = buf;
	bufend = buf + len;
	len = 0;
	while (body_ptr < bufend) {
		if ((*body_ptr++ == '\n') && (body_ptr < bufend) &&
		    (*body_ptr++ == '\n')) {
			len = bufend - body_ptr;
			break;
		}
	}

	/* adjust for the content length and the context */
	if ((len < 1) || (bufend[-1] != '\n')) {
		/* if the message is not terminated by a LF, add one to it */
		buf = (char *) realloc (buf, bufend - buf + 1);
		bufend = &buf[bufend - buf];
		*bufend++ = '\n';
		len++;
	}
	else if ((len > 1) && (bufend[-1] == '\n') && (bufend[-2] == '\n')) {
		/* the last line in the msg is a blank line, then not to
		 * include it in the content length.  ZZZ: if the displayed
		 * message is modified binary, we may screw up the data!
		 */
		--len;
		--bufend;
	}

	body_ptr = msg_methods.mm_read_headers(m, buf, bufend);
	m->mo_body_ptr = body_ptr;

	/* if "Content-Length" exists but is incorrect, update it.  */
	if ((m->mo_len >= 0) && (m->mo_len != len)) {
		/* just free up the single node created by mm_read_headers,
		 * split the headers into individual nodes for easy replacement
		 */
		header_destroy (m->mo_hdr);
		m->mo_hdr = header_split (buf, body_ptr-1);
		sprintf (buflen, "%d", len);
		header_set (&m->mo_hdr, Msg_Content_Len, buflen);
	}

	/*
	 * IMPORTANT!!!  This flag makes the buffer to be owned by
	 * the message now.
	 */
	m->mo_hdr->hdr_allocated = HDR_MALLOC;
	m->mo_len = len;

	if (motype.co_type == NULL)
	{
		m->mo_format = format;
	}
	else
	{
		/* the content-type was "ignored", add it back */
		if (m->mo_type != NULL)
		{
			/* user adds the content-type manually */
			co_methods.co_destroy (&motype);
		}
		else
		{
			/* restore the content-type */
			memcpy ((char *)&m->mo_ctype, (char *)&motype,
				sizeof(struct cotype));
		}

		if (m->mo_type)
			m->mo_format = msg_type(m->mo_type, m->mo_subtype);
	}

	/* count the lines in the article and the blank line of "From "/EOB */
	num_lines = 0;
	while (body_ptr < bufend) {
		body_ptr = findchar ('\n', body_ptr, bufend);
		num_lines++;
		if (body_ptr >= bufend) break;
		body_ptr++;
	}
#ifdef	LINE_COUNT
	m->mo_num_lines = num_lines;
#else
	m->mo_num_lines += ++num_lines;
#endif	LINE_COUNT

	m->mo_modified = 1;
	if (m->mo_folder)
		m->mo_folder->fo_changed = 1;


	return;
}


#ifdef DEBUG
static void
mm_dump( msg )
struct msg *msg;
{
}
#endif DEBUG

/*
 * support routine for reading bytes from a file
 *
 * Returns 0 for EOF, or number of bytes read
 */
static int
mm_read_bytes(buf, len, f)
char *buf;
int len;
FILE *f;
{
	return (fread (buf, 1, len, f));
}

/*
 * support routine for writing bytes to a file
 *
 * Returns 0 for success, 1 for error, 2 for premature EOF
 */
static int
mm_write_bytes(buf, len, f)
char *buf;
int len;
FILE *f;
{
	int retval;

	while (len) {
		retval = fwrite(buf, 1, len, f);

		if (retval > 0) {
			buf += retval;
			len -= retval;
			if (len) {
				/* Incomplete Write - error? */
				if (ferror(f))
					return(1);
			}
		} else {
			/* either an error or an EOF.  Return 1 for error,
			 * 2 for EOF
			 */
			if (feof(f)) return(2);
			return(1);
		}
	}
	return (0);
}

/*
 * if suppress_hdrs == MSG_FULL_HDRS, all headers will be ouptut.
 * if suppress_hdrs == MSG_ABBR_HDRS, output all headers except ignored.
 * if suppress_hdrs == MSG_SAVE_HDRS, output abbreviated and required headers
 */
static int
mm_write_msg(m, suppress_hdrs, f)
struct msg *m;
int suppress_hdrs;
FILE *f;
{
	return(msg_methods.mm_copymsg(m, suppress_hdrs,
		mm_write_bytes, (int)f, 0));
}


static int
mm_write_attach(m, suppress_hdrs, f)
struct msg *m;
int suppress_hdrs;
FILE *f;
{
    	return(mm_copyattach(m, suppress_hdrs, 1, mm_write_bytes, (int)f), 0);
}


extern struct attach *
find_at2msg(
	struct msg *m
)
{
	struct attach *at;
	struct attach *first = NULL;

	/* check if only one attachment left in the message */
	for (at = attach_methods.at_first(m); at != NULL;
	     at = attach_methods.at_next(at))
	{
		if (!(bool)attach_methods.at_get(at, ATTACH_DELETED))
		{
			if (first == NULL)
				first = at;
			else
				return (NULL);	/* more than 1 attachment */
		}
	}

	/* convert only if the message contains 1 attachment and it is text. */
	if (first && (bool)attach_methods.at_get(first, ATTACH_IS_TEXT))
		return (first);
	else
		return (NULL);
}

static int
output_as_msg(
	struct attach	*at,
	struct msg	*m,
	int		suppress_hdrs,
	int		(*func)(),
	int		f,
	int 		screen
)
{
	int retval, error;
	char buffer[4096];
	char *outptr;
        size_t outlen;
	char    *intcode;

	/* write out headers except content length and content type */
	retval = mm_copyheader_wo (m, suppress_hdrs, func, f,
			 IGNORE_LEN|IGNORE_TYPE|IGNORE_DESC);

	if (retval) return (retval);

	header_destroy(at->at_hdr);
	at->at_hdr = NULL;

	/* write out the contents */
	if (screen) {
		glob.g_iconvinfo = cs_methods.cs_translation_required(m, &intcode); 
		retval = attach_methods.at_save(at, func, 
			(void *) f, AT_SAVE_BODY|AT_SAVE_SUBMIT|AT_SAVE_NOMARKER);
	}
	else {
		glob.g_iconvinfo = 0;
		retval = attach_methods.at_save(at, func, (void *) f, 
			AT_SAVE_CONT_LEN|AT_SAVE_FILE|AT_SAVE_NOMARKER);
	}

	if (glob.g_iconvinfo && screen) {
		/* force out a reset sequence */
		outptr = buffer;
        	outlen = sizeof buffer;
        	enconv(glob.g_iconvinfo, NULL, 0, &outptr, &outlen);
        	if (outptr != buffer) {
                	DP(("cs_copy: writing %d bytes (reset sequence)\n",
                        	outptr - buffer));
                	error = (*func)(buffer, outptr - buffer, f);
                	if (error) return (error);
        	}
	}

	/* always output a blank line after the body */
	if (retval == 0)
		retval = (*func)(Msg_LF, 1, f);
	return (retval);
}

#ifdef	RFC_MIME
static int
mm_msg2mp(m)
struct msg *m;
{
	struct header_obj *hdr;

	hdr = header_split(m->mo_hdr->hdr_start, m->mo_hdr->hdr_end);
	ck_free(m->mo_hdr);
	m->mo_hdr = hdr;

	/* Remove the RFC-MIME multipart header from the message header */
	attach_methods.at_del_headers(&m->mo_hdr);
	co_methods.co_destroy(&m->mo_ctype);

	if (m->mo_format == MSG_ATTACHMENT)
		m->mo_type = ck_strdup(ATTACHMENT_TYPE);
	else
	{
		/* for MSG_MULTIPART */
		m->mo_type = ck_strdup(ATTACH_MULTIPART_TYPE);
		m->mo_subtype = ck_strdup(ATTACH_MIXED_SUBTYPE);
	}
	return (0);
}
#endif	RFC_MIME



/*
 * utility routine that looks like "func" in mm_copyattach() to
 * sum up the size of an "attachment"
 */
static int
mm_countlength(buf, size, totallength)
char *buf;
int size;
int *totallength;
{
	*totallength += size;
	return (0);
}



extern int
mm_copyattach(m, suppress_hdrs, arg_is_fp, func, arg, screen)
struct msg *m;
int suppress_hdrs;
int arg_is_fp;
int (*func)();
int arg;
int screen;
{
	int	type;
	int	retval;
	struct attach *at;


	/* check if only one text attachment left in the message.  If so,
	 * write it out as a normal text message.
	 */
	if ((at = find_at2msg(m)) != NULL)
	{
		return (output_as_msg(at, m, suppress_hdrs, func, arg, screen));
	}

	/* Now write out either the attachment or multipart format */

#ifdef	RFC_MIME
	/* This code is very ugly; hate it!  Here is the idea:
	 * if the original message is not v3 attachment or multipart
	 * format, we will convert it into v3 attachment or multipart.
	 * We replace the original content-type, encoding header, and
	 * label header by the v3 attachment or multipart header.
	 */
	if (m->mo_type)
	{
		type = msg_type(m->mo_type, m->mo_subtype);
		if ((type != MSG_ATTACHMENT) && (type != MSG_MULTIPART))
			mm_msg2mp(m);
	}

	/* If it is multipart format, generate the boundary marker if needed */
	if (m->mo_format == MSG_MULTIPART)
	{
		char	*cookie;
		if ((cookie = mm_get(m, MSG_BOUNDARY_MARKER)) == NULL)
			mm_set(m, MSG_BOUNDARY_MARKER, 0);
		else
			ck_free(cookie);
	}
#endif	RFC_MIME

	/* write out the headers except the content length */
	retval = mm_copyheader_wo(m, suppress_hdrs, func, arg,
				  IGNORE_LEN);
	if (retval) return (retval);


	/* this routine might be writing to a file (where seek
	 * works) or not.  We split the code up at this
	 * point so we can do things faster if fseek() and
	 * ftell() work, but still get a correct answer
	 * if they do not.
	 */

	if (arg_is_fp) {
		return (mm_copyattach_2(m, arg_is_fp, func, arg, 0));
	} else {
		int length = 0;

		/* we need to get the size first, then
		 * write the attachment....
		 */
		retval = mm_copyattach_2(m, arg_is_fp,
			mm_countlength, &length, 0);

		if (retval) return (retval);

		/* now run through the same code again, this
		 * time writing out the proper length.
		 *
		 * HACK ALERT: we have the magic # adjustment
		 * for the length built in there.  This is a
		 * crock.
		 *
		 * the 16 comes from:
		 *  ": %11.11\n\n" (== 15) plus the extra nl at the end.
		 */

		length -= strlen(Msg_Content_Len) + 16;
		return (mm_copyattach_2(m, arg_is_fp, func, arg, length));
	}
}



static int
mm_copyattach_2(m, arg_is_fp, func, arg, length)
struct msg *m;
int arg_is_fp;
int (*func)();
int arg;
int length;
{
	FILE *fp = (FILE *) arg;
	char	buf[64];
	int	retval;
	long	offset;

	/* write out an empty content length header */
#ifdef	RFC_MIME

	/* Multipart boundary marker has a leading blank line, so there is
	 * no need to add a blank line after the message header.
	 */
	sprintf(buf, "%s: %-11d\n%s", Msg_Content_Len, length,
		((m->mo_format == MSG_MULTIPART) ? "" : Msg_LF));
#else
	sprintf(buf, "%s: %-11d\n\n", Msg_Content_Len, length);
#endif	RFC_MIME

	retval = (*func)(buf, strlen(buf), arg);
	if (retval) return (retval);

	if (arg_is_fp) {
		offset = ftell(fp);
	}

	retval = msg_methods.mm_enumerate(m, attach_methods.at_save,
		(int) func, arg, AT_SAVE_FILE, 0);

	if (retval) return (retval);

#ifdef	RFC_MIME
	if (m->mo_format == MSG_MULTIPART)
	{
		/* since the blank line after the message header is
		 * written by the multipart marker, add 1 byte offset.
		 */
		offset++;

		/* write the optional ending boundary marker */
		retval = attach_methods.at_copy(
				attach_methods.at_first(m),
				AT_END_MARKER, func, (void *) arg);
		if (retval) return (retval);
	}
#endif	RFC_MIME

	/* write out the actual content length */
	if (arg_is_fp) {
		m->mo_len = ftell(fp) - offset;
		sprintf (buf, "%u", m->mo_len);
		fseek(fp, offset - 13, 0);
		retval = (*func)(buf, strlen(buf), arg);
		if (retval) return (retval);

		fseek(fp, 0L, 2);
	}

	/* write out a newline as a message separator */
	return ((*func)(Msg_LF, 1, arg));
}



static int
mm_write_other(m, suppress_hdrs, f)
struct msg *m;
int suppress_hdrs;
int f;
{
	struct attach *at = m->mo_first_at;

	if (!(bool)attach_methods.at_get(at, ATTACH_MODIFIED))
	{
		/* there is no change in the contents, write out as is */
		return (mm_write_msg (m, suppress_hdrs, f));
	}
	else
	{
		/* remove all body part headers from the attachment and
		 * message; they will be rebuilt from the cache.
		 */
		header_destroy(at->at_hdr);
		at->at_hdr = NULL;
		if (!m->mo_hdr->hdr_single_val)
		{
			struct header_obj *hdr;

			hdr = header_split(m->mo_hdr->hdr_start,
					   m->mo_hdr->hdr_end);
			ck_free(m->mo_hdr);
			m->mo_hdr = hdr;
		}
		attach_methods.at_del_headers(&m->mo_hdr);
		return (output_as_msg(at, m, suppress_hdrs,
			msg_methods.mm_write_bytes, f, 0));
	}
}

static int
mm_modified(m)
struct msg *m;
{
	struct attach *at;

	if (m->mo_modified)
		return (1);

	for (at = m->mo_first_at; at != NULL; at = at->at_next)
	{
		if (attach_methods.at_modified (at))
		{
			m->mo_modified = 1; 
			return (1);
			
		}
	}
	return (0);
}

/*
 * free up all the headers in a message, without touching the
 * message itself
 */
static void
mm_free_headers(m)
struct msg *m;
{
	header_destroy(m->mo_hdr);	m->mo_hdr = 0;

	ck_free(m->mo_unix_from);	m->mo_unix_from = 0;
	ck_free(m->mo_unix_date);	m->mo_unix_date = 0;
	ck_free(m->mo_from);		m->mo_from = 0;
	ck_free(m->mo_to);		m->mo_to = 0;
	ck_free(m->mo_date);		m->mo_date = 0;
	ck_free(m->mo_subject);		m->mo_subject = 0;

	co_methods.co_destroy(&m->mo_ctype);

	/* although maillib didn't allocate this, someone has to do it */
	ck_free(m->m_header);		m->m_header = 0;
}

/*
 * free up a messsage and any attachments; it's assumed to be already
 * unlinked from the folder
 */
static void
mm_free_msg(m)
struct msg *m;
{
	if (m->mo_first_at)
	{
		mm_enumerate (m, attach_methods.at_destroy);
		m->mo_first_at = NULL;
	}

	mm_destroy (m);
}

/* free the copy of message body which is either memory mapped or malloced */
static void
mm_free_body( m )
struct msg *m;
{
	if (m->mo_mtype == MSG_MMAP)
		ck_zfree (m->mo_body_ptr);
	else if (m->mo_mtype == MSG_MALLOC)
		ck_free (m->mo_body_ptr);
	else
		return;
	
	m->mo_body_ptr = NULL;
	m->mo_mtype = MSG_NONE;
}

static char *
mm_header(m, field_name)
struct msg *m;
char *field_name;
{
	return (findheaderlist(field_name, m->mo_hdr));
}

static int
mm_enumerate (msg, call, a, b, c, d)
struct msg *msg;
int (*call)();
int a, b, c, d;
{
	struct attach *at, *next;
	int retval = 0;

	if (msg == NULL)
		return (0);
	
	for (at = msg->mo_first_at; !retval && at; at = next)
	{
		next = at->at_next;
		retval = (*call) (at, a, b, c, d);
	}
	return (retval);
}

static struct msg *
mm_first (folder)
struct folder_obj *folder;
{
	return (folder ? folder->fo_first_msg : NULL);
}

static struct msg *
mm_next (m)
struct msg *m;
{
	return (m ? m->mo_next : NULL);
}

static struct msg *
mm_prev (m)
struct msg *m;
{
	return (m ? m->mo_prev : NULL);
}

static struct msg *
mm_last (folder)
struct folder_obj *folder;
{
	return (folder ? folder->fo_last_msg : NULL);
}

static int
msg_add_attachments (msg, buf, bufend)
struct msg *msg;
char *buf;
char *bufend;
{
	struct attach *at;
	struct attach **p_at;
	int n_attach = 0;

	if (msg == NULL)
		return (0);

	p_at = &msg->mo_first_at;
	while (buf < bufend)
	{
		at = (struct attach *) ck_malloc (sizeof (struct attach));
		memset ((char *) at, 0, sizeof (*at));

		MP(("at_init(buf=%x, bufend=%x)\n", buf, bufend));

		attach_methods.at_set (at, ATTACH_MSG, msg);
		buf = attach_methods.at_init (at, buf, bufend);

		MP(("at_init() = %x\n", buf));

		if (at->at_garbage)
		{
			attach_methods.at_destroy (at);
			continue;
		}
		n_attach++;

		/* link the attachment into the message */
		*p_at = at;
		p_at = &(at->at_next);
	}

	return (n_attach);
}

/*
 * Add an attachment to an existing attachment-type message where the position
 * "pos" is from 1 to MSG_LAST_ATTACH.
 */
static void
mm_add_attach(msg, at, pos)
struct msg *msg;
struct attach *at;
unsigned pos;
{
	struct attach *p_curr;
	struct attach *p_prev;

	if ((msg == NULL) || (at == NULL))
		return;

	p_prev = NULL;
	p_curr = msg->mo_first_at;
	while (p_curr != NULL)
	{
		if (--pos == 0)
			break;
		p_prev = p_curr;
		p_curr = p_curr->at_next;
	}

	if (p_prev == NULL)
		msg->mo_first_at = at;
	else
		p_prev->at_next = at;
	at->at_next = p_curr;

	attach_methods.at_set (at, ATTACH_MSG, msg);
	if (!msg->mo_modified)
	{
		
		msg->mo_modified = 1;
		if (msg->mo_folder)
			msg->mo_folder->fo_changed = 1;
		
	}

	/* Update the message size.  It is not very accurate. */
	msg->mo_len += (int) attach_methods.at_get(at, ATTACH_NUM_BYTES);
}

static int
mm_destroy_attach (msg, pos)
struct msg *msg;
unsigned pos;
{
	register int lastone;
	register struct attach *curr;
	register struct attach *prev;

	if (pos == 0)
		return (-1);

	lastone = (pos == MSG_LAST_ATTACH);

	prev = NULL;
	curr = msg->mo_first_at;
	while (--pos > 0)
	{
		if (curr == NULL)
			return (-1);

		if (lastone && (curr->at_next == NULL))
			break;

		prev = curr;
		curr = curr->at_next;
	}

	if (prev == NULL)
		msg->mo_first_at = curr->at_next;
	else
		prev->at_next = curr->at_next;

	attach_methods.at_destroy (curr);
	return (0);
}

#ifdef	RFC_MIME

/*
 * Generate the random boundary marker for multipart.  The first two tokens
 * are random numbers.  The last token is time-of-day.  All tokens are encoded
 * by using base64.  The magic cookie has the leading "=_" which is excluded
 * from base64 and quoted-printable encodings.
 */
static char *
mm_gen_boundary()
{
	long	tod;
	int	count;
	char	buf[80];
	static long seed;
	register char *end;

	tod = time((time_t *) NULL);
	if (seed == 0)
	{
#ifdef	SVR4
		srand(tod);
#else
		srandom(tod);
#endif	SVR4
	}

	end = buf;
	count = 2;
	while (--count >= 0)
	{
#ifdef	SVR4
		seed = rand();
#else
		seed = random();
#endif	SVR4
		memcpy(end, (char *)&seed, sizeof(seed));
		end += sizeof(seed);
	}
	memcpy(end, (char *)&tod, sizeof(tod));
	end += sizeof(tod);

	end = b64senc(buf, end-buf);
	sprintf (buf, "=_%s", end);
	ck_free(end);

	return (ck_strdup(buf));
}

#endif	RFC_MIME
