/* @(#)misc.h	3.2 02/09/94 */

/* defines external configuation for mail library, such as
 * error message handling, encoding/decoding retrieval functions
 */

#ifndef	__misc_h__
#define	__misc_h__

typedef	enum {
	ML_ERRNO,	/* int errno */
	ML_ERRMSG,	/* char *errmsg */
	ML_CLRERR,	/* void */
	ML_ENCODE,	/* char *(*encode)(char *code) */
	ML_DECODE,	/* char *(*decode)(char *code) */
	ML_WARN,	/* void (*warn)(void *channel, char *fmt, va_alist) */
	ML_ERROR,	/* void (*error)(void *channel, char *fmt, va_alist) */
	ML_CONFIRM,	/* void (*error)(void *channel, int optional,
				char *button1, char *button2, char *fmt, ...) */
	ML_SENDV3,	/* bool */
	ML_DS2RFC,	/* int (*ds2rfc)(char *, char **type, char **subtype) */
	ML_RFC2DS,	/* int (*rfc2ds)(char *type, char *subtype, char **) */
} Ml_attr;

struct __maillib_methods {
	void (*ml_warn)(void *channel, char *fmt, ...);
	void (*ml_error)(void *channel, char *fmt, ...);
	int (*ml_confirm)(void *channel, int optional,
			char *button1, char *button2, char *fmt, ...);
	void (*ml_perror)(void *channel, int ecode, char *fmt, ...);
	void *(*ml_get)(Ml_attr attr);
	void (*ml_set)(Ml_attr attr, ...);
	char *(*ml_encode)(char *code, void *param);
	char *(*ml_decode)(char *code, void *param);
	int  (*ml_ds2rfc)(char *dstype, char **type, char **subtype);
	int  (*ml_rfc2ds)(char *type, char *subtype, char **dstype);
};

extern struct __maillib_methods maillib_methods;

#endif	__misc_h__
