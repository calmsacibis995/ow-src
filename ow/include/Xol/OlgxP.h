#ifndef	_XOL_OLGXP_H
#define	_XOL_OLGXP_H

#pragma	ident	"@(#)OlgxP.h	1.10	93/04/23 include/Xol SMI"	/* olg:Olg.h 302.5	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */


#include <Xol/OpenLookP.h>
#include <Xol/Slider.h>
#define OW_I18N
#include <olgx/olgx.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Until we're able to implement OLGX with Resolution-independence,
 * we'll use this instead of OlScreenPointToPixel (OpenLook.h) so
 * that points = pixels.  Once we OLGX supports RI, we'll revert back
 */
#define OlgxScreenPointToPixel(direction, value, screen) value

typedef struct _OlgxDevice {
    struct _OlgxDevice*	next;	/* ptr to next device struct */
    unsigned int	refCnt;

    Screen*	scr;		/* display structure ptr */
    Boolean	threed;		/* current state  */

    Pixmap	inactiveStipple;/* Pixmap for inactive stippling */
    Pixmap	dropTarget;     /* Default "Full" pixmap for droptarget */

    char	scale;		/* rendering size */
    Colormap	cmap;		/* needed for caching */
    Cardinal	depth;
} _OlgxDevice;

typedef union {
	Pixel			pixel;
	Pixmap			pixmap;
} OlgxBG;

typedef struct _OlgxAttrs {
	int			refCnt;		/* reference count */
	Widget*			refWidgets;	/* list of referents */
	struct _OlgxAttrs*	next;		/* pointer to next struct in list */

	Pixel			fg;		/* foreground pixel */
	OlgxBG			bg;		/* background pixel or Pixmap */

	Graphics_info		*ginfo; 	/* pointer to OLGX rendering structure */
	_OlgxDevice*		pDev;	        /* pointer to device specific data */
	unsigned char		flags;
} OlgxAttrs;

typedef struct {
	OlStr			label;
	OlFont			font;
	OlStrRep		text_format;
	unsigned char		*qualifier;
	Boolean			meta;
	unsigned char		*accelerator;
	unsigned char		mnemonic;
	unsigned char		justification;
	unsigned char		flags;
} OlgxTextLbl;

typedef union _OlgxPixmapLabel {
	Pixmap			pixmap;
	XImage*			image;
} _OlgxPixmapLabel;

typedef struct {
	_OlgxPixmapLabel	label;
	unsigned char		justification;
	unsigned char		type;
	unsigned char		flags;
} OlgxPixmapLbl;

typedef void (*OlSizeProc)(Screen* scr, OlgxAttrs* pInfo, XtPointer labeldata,
			   Dimension* pWidth, Dimension* pHeight);
typedef void (*OlDrawProc)(Screen* scr, Drawable win, OlgxAttrs *pInfo,
			   Position x, Position y,
			   Dimension width, Dimension height,
			   XtPointer labeldata, OlDefine buttonType,
			   int flags);

extern	unsigned int	_olgIs3d;

/* attributes defines */
#define OLGX_BGPIXMAP	1
#define OLGX_ALLOCBG2	2
#define OLGX_ALLOCBG3	4
#define OLGX_ALLOCWHITE	8

/* Slider defines */
#define SL_POSITION	1
#define SL_DRAG		2
#define SL_BEGIN_ANCHOR	4
#define SL_END_ANCHOR	8

/* Label defines */
#define TL_LEFT_JUSTIFY		0
#define TL_CENTER_JUSTIFY	1
#define TL_RIGHT_JUSTIFY 	2
#define TL_POPUP		1
#define TL_LAYOUT_WITH_META	2

/* Image label defines */
#define PL_IMAGE	0
#define PL_PIXMAP	1
#define PL_BITMAP	2
#define PL_TILED	1

/* Drop Target defines */
#define DT_INSENSITIVE	8

/* Miscellaneous Macros */

#define OlgxGetInactiveStipple(p)	\
		((p) ? (p)->pDev->inactiveStipple : (Pixmap) 0)

#define OlgxGetScreen(p)	((p) ? (p)->pDev->scr : (Screen *) 0)

#define OlgxGetValidScale(s)	(s <= 11? 10 : \
				 s <= 13? 12 : \
				 s <= 15? 14 : \
				 s <= 17? 16 : \
				 s <= 19? 19 : \
				 s <= 22? 20 : 24)


/*
 * External functions
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* OlgxAttr */

extern OlgxAttrs*	OlgxCreateAttrs(Widget w, Pixel fg, OlgxBG*  bg,
			 	unsigned bgIsPixmap, unsigned scale, 
				OlStrRep textFormat, OlFont textFont);

extern void		OlgxDestroyAttrs(Widget w, OlgxAttrs*  pInfo);

extern void		OlgSetStyle3D(Screen*  scr, unsigned draw3d);


/* OlgxInit */

extern _OlgxDevice*	_OlgxGetDeviceData(Widget w, int scale);

extern void		_OlgxFreeDeviceData(Screen*  screen);

extern void		_OlgxFreeDeviceDataRef(_OlgxDevice*  dev);


/* OlgxButton */

extern void	OlgxDrawTextButton(Screen *scr, Drawable win, OlgxAttrs *pInfo,
	Position x, Position y, Dimension width, Dimension height,
	XtPointer lbldata, OlDefine buttonType, int flags);

extern void	OlgxSizeTextLabel(Screen *scr, OlgxAttrs *pInfo,
	XtPointer lbldata, Dimension *pWidth, Dimension *pHeight);

extern void	OlgxDrawImageButton(Screen *scr, Drawable win, OlgxAttrs *pInfo,
	Position x, Position y, Dimension width, Dimension height,
	XtPointer lbldata, OlDefine buttonType, int flags);

extern void	OlgxSizePixmapLabel(Screen *scr, OlgxAttrs *pInfo, 
	XtPointer lbldata, Dimension *pWidth, Dimension *pHeight);


/* OlgxSize */

extern void	OlgxSizeOblongButton(Screen*  scr, OlgxAttrs*  pInfo,
				 XtPointer label, OlSizeProc sizeProc,
				 unsigned flags,
				 Dimension* pWidth, Dimension* pHeight);

extern void	OlgxSizeRectButton(Screen*  scr, OlgxAttrs*  pInfo,
	XtPointer label, OlSizeProc sizeProc,
	unsigned flags, Dimension* pWidth, Dimension* pHeight);


/* RootShell */

extern Boolean		OlgIs3d(Screen* screen);

#else	/* __STDC__ || __cplusplus */

extern OlgxAttrs*	OlgxCreateAttrs();
extern void		OlgxDestroyAttrs();
extern void		OlgSetStyle3D();

extern _OlgxDevice*	_OlgxGetDeviceData();
extern void		_OlgxFreeDeviceData();
extern void		_OlgxFreeDeviceDataRef();

extern void		OlgxSizeOblongButton();
extern void		OlgxSizeRectButton();

extern void		OlgxDrawTextButton();
extern void		OlgxSizeTextLabel();
extern void		OlgxDrawImageButton();
extern void		OlgxSizePixmapLabel();

extern Boolean		OlgIs3d();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLGXP_H */
