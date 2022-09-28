#ifndef	_XOL_FILECH_H
#define	_XOL_FILECH_H

#pragma	ident	"@(#)FileCh.h	1.8	93/02/25 include/Xol SMI"	/* OLIT	493 */

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


/************************************************************************
 *
 *      Interface of the OLIT file chooser panel widget
 *
 ************************************************************************/

/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <sys/stat.h>		/* struct stat */
#include <sys/types.h>		/* boolean_t */

#include <Xol/OpenLook.h>	/* OlStr, OL_*, OlGlyph */
#include <Xol/ScrollingL.h>	/* OlSlistItemPtr */


#ifdef	__cplusplus
extern "C" {
#endif


/* New types */

typedef struct stat	*OlStat, OlStatRec;

typedef struct _OlFNavNode {
	String		name;
	boolean_t	is_folder;
	boolean_t	operational;
	boolean_t	filtered;
	boolean_t	active;
				/*! collapse into bit field !*/
	OlGlyph		glyph;
	OlStat		sbufp;
} *OlFNavNode, OlFNavNodeRec;


#ifdef	__STDC__ 

typedef	int	(*OlStrComparisonFunc)(
	const OlStr	left_string,
	const OlStr	right_string
);

typedef	int	(*OlComparisonFunc)(
	const XtPointer	left_key,
	const XtPointer	right_key
);

extern int	OlSortStrCaseAscending(OlStr lstr, OlStr rstr);
extern int	OlSortStrCaseDescending(OlStr lstr, OlStr rstr);
extern int	OlSortStrNoCaseAscending(OlStr lstr, OlStr rstr);
extern int	OlSortStrNoCaseDescending(OlStr lstr, OlStr rstr);

#else   /* __STDC__ */
 
typedef	int	(*OlStrComparisonFunc)();
typedef	int	(*OlComparisonFunc)();

extern int	OlSortStrCaseAscending();
extern int	OlSortStrCaseDescending();
extern int	OlSortStrNoCaseAscending();
extern int	OlSortStrNoCaseDescending();

#endif  /* __STDC__ */

typedef String*	OlFolderList;

#define	OlSortStrCaseAscending		(OlComparisonFunc)strcoll

typedef struct _FileChooserClassRec*	FileChooserWidgetClass;
typedef struct _FileChooserRec*		FileChooserWidget;

extern WidgetClass fileChooserWidgetClass;


/* Callback structure definitions */
typedef struct _OlFileChGenericCallbackStruct {
	/* OLIT standard fields */
	int			reason;	/* OL_REASON_FOLDER_OPENED, ... */

	/* FileChooser standard fields */
	XtPointer		extension;	/* reserved */
	OlDefine		operation;	/* OL_OPEN, ... */
	String			current_folder;	/* absolute path name */
} OlFileChGenericCallbackStruct;

typedef struct _OlFileChFolderCallbackStruct {
	/* OLIT standard fields */
	int			reason;		/* OL_REASON_OPEN_FOLDER */

	/* FileChooser standard fields */
	XtPointer		extension;	/* reserved */
	OlDefine		operation;	/* OL_OPEN, ... */
	String			current_folder;	
						/* absolute path name */
	
	/* Callback-dependent fields */
	String			request_folder; /* may be set to NULL */
	OlFNavNode		request_folder_node;
} OlFileChFolderCallbackStruct;

typedef struct _OlFileChDocumentCallbackStruct {
	/* OLIT standard fields */
	int			reason;		/* OL_REASON_OPEN_DOCUMENT */
						/* OL_REASON_SAVE_DOCUMENT */

	/* FileChooser standard fields */
	XtPointer		extension;	/* reserved */
	OlDefine		operation;	/* OL_OPEN, ... */
	String			current_folder;	/* absolute path name */
	
	/* Callback-dependent fields */
	String			request_document_folder; 
						/* absolute path name */
	String			request_document; /* base name */
	OlFNavNode		request_document_node;
} OlFileChDocumentCallbackStruct;

typedef struct _OlFileChListChoiceCallbackStruct {
	/* OLIT standard fields */
	int			reason;		/* OL_REASON_LIST_CHOICE */

	/* FileChooser standard fields */
	XtPointer		extension;	/* reserved */
	OlDefine		operation;	/* OL_OPEN, ... */
	String			current_folder;	/* absolute path name */
	
	/* Callback-dependent fields */
	OlSlistItemPtr		chosen_item;
	int 			chosern_item_pos;  
	OlFNavNode		chosen_item_node;

} OlFileChListChoiceCallbackStruct;


#ifdef	__cplusplus
}
#endif


/* end of %M */
#endif	/* _XOL_FILECH_H */
