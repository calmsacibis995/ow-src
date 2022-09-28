/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl.h 1.7	96/08/02 SMI"

#ifndef _LCL_H
#define _LCL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* lcl.h */
#include <sys/types.h>


/*** public part ***/

typedef enum {
    LctNUnused = 0,
    /* ? */
    LctNSourceType,
    LctNSourceForm,
    LctNKeepReference,
    /* for LctNSourceType */
    LctNMsgText,
    LctNPlainText,
    LctNTaggedText,
    LctNSeparatedTaggedText,
    LctNSourceUnknown,
    /* for LctNKeepReference */
    LctNKeepByValue,
    LctNKeepByReference,
    /* for LctNSourceForm */
    LctNDisplayForm,
    LctNInComingStreamForm,
    LctNOutGoingStreamForm,
    LctNOutGoingStreamForm_V3,
    LctNOutGoingStreamForm_V3_Attach,
    LctNFileForm,
    LctNAppForm,
    LctNPrintForm,
    LctNUnknownForm,
    /* attributes */
    LctNHeaderCharset,
    LctNHeaderCharsetList,
    LctNHeaderPossibleCharsetList,
    LctNHeaderSegment,
    LctNHeaderEncoding,
    LctNHeaderCharsetEncoding,
    LctNBodyCharset,
    LctNBodyCharsetList,
    LctNBodyPossibleCharsetList,
    LctNBodySegment,
    LctNBodyEncoding,
    LctNBodyCharsetEncoding,
    LctNTaggedTextCharset,
    LctNTaggedTextCharsetList,
    LctNTaggedTextPossibleCharsetList,
    LctNTaggedTextSegment,
    LctNTaggedTextEncoding,
    LctNPlainTextCharset,
    LctNPlainTextCharsetList,
    LctNPlainTextPossibleCharsetList,
    LctNPlainTextSegment,
    LctNPlainTextEncoding,
    /* additional attributes */
    LctNMailType,
    LctNAddHeader,
    LctNContentOfHeaderSegment,
    LctNBodyCharsetMailName,
    LctNContentOfTaggedTextSegment,
    LctNQuerySourceType,
    /* internal */
    LclNVaNextedList
} LctNEAttribute;



/* for LctNMailType */
#define LctNMailTypeMime	"MailTypeMime"
#define LctNMailTypeV3		"MailTypeV3"
#define LctNMailType822		"MailType822"
#define LctNMailTypeUnknown	"MailTypeUnknown"

/* for Null pointer */
#define LctNNone		1

/* ErrorCode */
#define LctErrorNone    0
#define LctErrorIconvCannotOpen	1
#define LctErrorIconvError	2
#define LctErrorNotEnoughResource	3
#define LctErrorDBCannotFindIconvDef	4
#define LctErrorDBCannotFindFromCharset	5
#define LctErrorDBCannotFindToCharset	6
#define LctErrorInvalidAttribute	7
#define LctErrorIconvNonIdenticalConversion	8
#define LctErrorIconvHalfDone	9

/* Sour Type */
#define LclSourceUnknownType	0
#define LclSourceTextType	1
#define LclSourceBinaryType	2

typedef enum {LclContentUnknown, LclContentText, LclContentBinary} LclContentType ;

/*** private part ***/

typedef enum {LclUnKnownType, LclMIMEType, LclV3Type, Lcl822Type} LclMailType ;
typedef enum {LclUnKnownEncoding, LclBase64Encoding, LclQPEncoding, Lcl822Encoding } LclMailEncoding ;
typedef enum {LclTypeDisplay, LclTypeInComingStream, LclTypeOutGoingStream, LclTypeFile, LclTypeApp, LclTypeOther} LclForm;
typedef enum {LclTypeOneway, LclTypeBothway} LclDirection;
typedef enum {LclUnKnownFormat, LclCsFormatASCII, LclCsFormat7bit, LclCsFormatISO9496, LclCsFormatISO94Ext, LclCsFormatMBString} LclFormat;


/* private structs */

typedef struct _LclCharsetLinkedList LclCharsetLinkedList;
struct _LclCharsetLinkedList{
	char	*name;
	LclCharsetLinkedList	*next;
};
	
typedef struct _LclFormInfo	LclFormInfo;
struct _LclFormInfo{
	LclForm	type;
	LctNEAttribute name;
	LclCharsetLinkedList	*msg_header_charset;
	LclCharsetLinkedList	*msg_body_charset;
	LclCharsetLinkedList	*plaintext_body_charset;
	LclCharsetLinkedList	*taggedtext_header_charset;
	LclCharsetLinkedList	*taggedtext_body_charset;
	LclMailEncoding		msg_header_encoding;
	LclMailEncoding		msg_body_encoding;
	LclMailEncoding		plaintext_body_encoding;
	LclMailEncoding		taggedtext_header_encoding;
	LclMailEncoding		taggedtext_body_encoding;
	LclMailType	mail_type;
	LclFormInfo	*next;
};

typedef struct{
	char	*from;
	char	*to;
} LclIconvAtom;

typedef struct _LclIconvInfo	LclIconvInfo;
struct _LclIconvInfo{
	char	*from_encoding;
	char	*to_encoding;
	LclDirection	direction;
	int	atom_num;
	LclIconvAtom	*atom;
	LclIconvInfo	*next;
};

typedef struct _LclCharsetInfo	LclCharsetInfo;
struct _LclCharsetInfo{
	char	*name;
	char	*mime_name;
	char	*v3_name;
	char	*iconv_name;
	LclMailEncoding	mail_header_encoding;
	LclMailEncoding	mail_body_encoding;
	LclFormat format;
	char	*ascii_end;
	char	*ascii_superset;
	LclCharsetInfo	*next;
};

typedef struct _LCLd{
	char	*locale;
	void	*db;
	LclFormInfo	*form_info;
	LclIconvInfo	*iconv_info;
	LclCharsetInfo	*cs_info;
} *LCLd;

typedef struct{
	boolean_t	copy;
	LclMailType	mail_type;
	LctNEAttribute	type;
	LctNEAttribute	form;
	char	*header;
	char	*body;
	char	*plaintext;
	char	*taggedtext;
	char	*buf;
	size_t	buf_len;
} LclContents;

typedef struct _LclFormAttrInfo	LclFormAttrInfo;
struct _LclFormAttrInfo{
	LclForm	type;
        LctNEAttribute name;
	LclCharsetLinkedList	*header_charset;
	LclCharsetLinkedList	*body_charset;
	LclMailEncoding		header_encoding;
	LclMailEncoding		body_encoding;
	LclMailType	mail_type;
	LclFormAttrInfo	*next;
};

typedef struct _LCTd{
	LCLd	lcld;
	LclFormAttrInfo	*attr_info;
	LclContents	*contents;
} *LCTd;


/* public structs */

typedef struct {
	char* segment ;
	char* charset;
	size_t size;
} LclCharsetSegment;

typedef struct {
	int num;
	LclCharsetSegment *seg;
} LclCharsetSegmentSet;

typedef struct {
	int num;
	char	**charset;
} LclCharsetList;

typedef struct {
	LclContentType	type;
	LclCharsetList	*charset_list;
} LclContentInfo;

typedef struct {
	LctNEAttribute	attribute;
	unsigned long	error_code;
	void	*detail;
} LclError;


/* public functions */

LCLd	lcl_open(char *locale);
void	lcl_close(LCLd lcld);
LCTd	lct_create(LCLd lcld, ...);
void	lct_destroy(LCTd lctd);
LclError	*lct_setvalues(LCTd lctd, LctNEAttribute target, ...);
LclError	*lct_getvalues(LCTd lctd, LctNEAttribute target, ...);
void	lcl_destroy_charset_list(LclCharsetList *);
void	lcl_destroy_segment_set(LclCharsetSegmentSet *);
void	lcl_destroy_content_info(LclContentInfo *);
void	lcl_destroy_error(LclError *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _LCL_H */
