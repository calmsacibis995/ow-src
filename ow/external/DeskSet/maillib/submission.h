/* @(#)submission.h       3.4 - 94/04/13 */

/* defines externally visible objects to mail system */

/* folder.h, msg.h and attach.h must be included */

#ifndef	__submission_h__
#define	__submission_h__

typedef enum SUB_ERRORS {
	SUB_OK = 0,		/* it all worked OK */
	SUB_NO_SWAP,		/* insufficient memory */
	SUB_FORK,		/* fork failed */
	SUB_NO_TMP_SPACE,	/* a write to tmp failed */
	SUB_CRYPT,		/* a problem with PEM */
	SUB_INTERNAL,		/* something else... */
	SUB_CHARSET,		/* a problem with translating the charset */
	SUB_LOG_ERR,		/* error with logging message */
	SUB_ERROR_SIZE		/* the number of errors */
} Sub_error;

typedef enum {
	SUB_RECORD	=	0,	/* char * record_file	*/
	SUB_TMPFILE	=	1,	/* char * tmp_file	*/
	SUB_WAIT	=	2,	/* bool			*/
	SUB_DEADLETTER	=	3,	/* char * dead_letter	*/
	SUB_PID		=	4,	/* get only: int child_pid */
#ifdef PEM
	SUB_PEM_NONE	=	5,	/* no PEM */
	SUB_PEM_INT	=	6,	/* PEM with integrity */
	SUB_PEM_CONF	=	7,	/* PEM with confidentiality */
	SUB_PEM_PRIV	=	8,	/* PEM with privacy */
#endif PEM
	SUB_ERROR	= 	9,	/* get only: int error status */
} Sub_attr;

struct __submission_methods {
	struct submission *(*sub_create)(char *errfile);
	int (*sub_set)(struct submission *, Sub_attr, ...);
	void *(*sub_get)(struct submission *, Sub_attr, ...);

	Sub_error (*sub_done)(struct submission *, struct msg *);
	void (*sub_destroy)(struct submission *);
};

extern struct __submission_methods submission_methods;


struct link_obj {
	caddr_t		l_value;
	struct link_obj *l_next;
};

struct submission {
	char	*sub_tmpfile;		/* temporary file for submission */
	char	*sub_errfile;		/* error file for submission */
	char	*sub_deadletter;	/* dead letter for submission */
	struct link_obj	*sub_to;	/* to: */
	struct link_obj	*sub_cc;	/* cc: */
	struct link_obj	*sub_bcc;	/* bcc: */
	struct link_obj	*sub_rec;	/* +file, /path */
	struct link_obj	*sub_prog;	/* | program */
	int	sub_wait;		/* wait for completion */
	int	sub_pid;		/* child pid for submission,
					 * it is 0 if sub_wait is set 
					 */
	int	sub_error;		/* there was an error before sendmail */
#ifdef PEM
	int	sub_pem_flags;		/* flags for PEM */
#endif PEM
};


#endif	!__submission_h__
