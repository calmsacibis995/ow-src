#ifndef _XOL_FILENAV_H
#define _XOL_FILENAV_H

#pragma	ident	"@(#)filenav.h	1.9	93/02/11 include/Xol SMI"	/* OLIT	493	*/

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
 *      Interface of the file chooser file navigation module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <sys/types.h>		/* boolean_t, uid_t, gid_t */

#include <X11/Intrinsic.h>	/* String, XtCallbackProc */

#include <Xol/FileCh.h>		/* OlStat */
#include <Xol/SBTree.h>		/* _OlSBTree */


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *      Module Public Type Definitions
 *
 ************************************************************************/

typedef struct UUOlUserInfo {
	uid_t		euid;
	gid_t		egid;
	const gid_t*	groups;
	int		num_groups;
} *_OlUserInfo, _OlUserInfoRec;


/************************************************************************
 *
 *      External Module Interface Declarations
 *
 ************************************************************************/

extern boolean_t	_OlFileNavVisitFolder(
				const char*const	path,

				const uid_t		euid,
				const gid_t		egid,
				const gid_t**		groupsp,
				int*			num_groupsp,

				const OlDefine		operation,
				const boolean_t		hide_dot_files,
				const boolean_t		show_inactive,
				const boolean_t		show_glyphs,
				const String		filter_string,
				const XtCallbackProc	filter_proc,
				const OlComparisonFunc	comparison_func,

				_OlSBTree*		new_treep,
				OlStat*			stat_bufpp
			);

extern boolean_t	_OlFileNavCanVisitFolder(
				const char*const	path, 
				_OlUserInfo		u_info
			);

extern boolean_t	_OlFileNavCanReadDocument(
				const char*const	path, 
				_OlUserInfo		u_info
			);

extern boolean_t	_OlFileNavCanWriteDocument(
				const char*const	path, 
				_OlUserInfo		u_info
			);

extern boolean_t	_OlFileNavIsDotFolder(const char*const path);
extern boolean_t	_OlFileNavIsDotDotFolder(const char*const path);

extern boolean_t	_OlFileNavIsPath(const OlStr olstr, 
	const OlStrRep text_format);

extern boolean_t	_OlFileNavIsAbsolutePath(const char*const path);
extern boolean_t	_OlFileNavIsDotFile(const char*const path);
extern String		_OlFileNavParentFolder(const char*const path);
extern String		_OlFileNavExpandedPathName(const char*const path_exp);

#ifdef	DEBUG
extern void		_OlFileNavTest(const int argc, const char*const argv[]);
#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of filenav.h */
#endif	/* _XOL_FILENAV_H */
