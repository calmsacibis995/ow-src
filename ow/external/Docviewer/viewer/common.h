#ifndef	_DOCVIEWER_COMMON_H_
#define	_DOCVIEWER_COMMON_H_

#ident "@(#)common.h	1.64 96/11/13 Copyright 1989, 1992 Sun Microsystems, Inc."

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <doc/common.h>
#include <doc/console.h>
#include <doc/notify.h>
#include <sys/time.h>
#include "spothelp.h"

#include <locale.h>

// Forward class declarations
class	NOTIFY;
class	ABGROUP;

// Typedefs

typedef enum {
	FULL_PAGE = 0,
	PART_PAGE = 1
} ViewMode;

typedef enum {
	VE__NULL_EVENT = 0,
	VE__CLEAR,
	VE__CURRENT_DOC,
	VE__CUSTOM_MAG,
	VE__EXECUTE_LINK,
	VE__FIRST_PAGE,
	VE__FOLLOW_LINK,
	VE__FULL_PAGE,
	VE__GOBACK,
	VE__GOTO_PAGE,
	VE__HIDE_LINKS,
	VE__LARGER,
	VE__LAST_PAGE,
	VE__MAGNIFY,
	VE__NEXT_DOC,
	VE__NEXT_PAGE,
	VE__PAGE_INFO,
	VE__PART_PAGE,
	VE__PREV_DOC,
	VE__PREV_PAGE,
	VE__PRINT,
	VE__REDISPLAY,
	VE__RESIZE,
	VE__SELECT,
	VE__SHOW_LINKS,
	VE__SMALLER,
	VE__STD_SIZE
} ViewerEvent;

typedef enum {
	PE__PAINT = 0,
	PE__CLEAR,
	PE__CLEAR2,
	PE__IGNORE
} PaintEvent;

typedef	void (*ViewerEventProc)	(const ViewerEvent	event,
				 const void	       *ptrToEventData,
				 const void	       *cdata);

// UnCommon inline functions
inline int BoxHght(const BBox &box)
{
	return(box.ur_y - box.ll_y);
};

inline int BoxWdth(const BBox &box)
{
	return(box.ur_x - box.ll_x);
};

inline int	ToPercent(const float scale)
{
	return(rint(scale * 100.0));
}

#define	DocHght(x)	BoxHght((x))
#define	DocWdth(x)	BoxWdth((x))

// External variables
extern NOTIFY	*notify;
extern ABGROUP	*abgroup;
extern CONSOLE	console;

// External functions
#ifdef	DEBUG
extern	void		PrintRect(const char		*string,
				  const	Xv_opaque	xvobj);

#define	DbgRect(s, xv)	(debug >= 3 ? (PrintRect((s),(xv)),1) : 0)
#else
#define	DbgRect(s, xv)
#endif	/* DEBUG */

#endif	/* _DOCVIEWER_COMMON_H_ */
