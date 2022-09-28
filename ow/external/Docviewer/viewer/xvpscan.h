#ifndef _VIEWER_XVPSCAN_H_
#define _VIEWER_XVPSCAN_H_

#ident "@(#)xvpscan.h	1.18 04/08/94 Copyright 1989-1992 Sun Microsystems, Inc."

#include "xvcanvas.h"
#include <xview/canvas.h>
#include <xview/xv_xrect.h>
#ifdef	undef
#include <xview/cms.h>
#endif
#include <X11/Xlib.h>

extern "C" {
#include <NeWS/psmacros.h>
};


 class	XVCANVAS;
 
 class XVPSCAN: public XVCANVAS {
 
 private:

	PSFILE			*psIn;
	PSFILE			*psOut;
 
 	Canvas			psCan;
 	GC			gc;
	GC			fillgc;
	Pixmap			bkgrndStipple;
	Pixmap			dataPage;

	int			pixmapDepth;
	int			pixmapWidth;
	int			pixmapHeight;
	int			function;
	float			scaleVal;
	BBox			displayBox;
 
 	static Xv_opaque	dataKey;

	STATUS			CreateBkgrndStipple(ERRSTK	&err);	
 
 	static void		EventProc(const Xv_Window	win,
 					  Event *const		event);
 
	void		        PrepCanvas(const BBox		&displayBox,
					   const float		scaleVal);

	void 			ChangeGCfunction();
	
	STATUS			GetNeWSServer(Display* dpy);

 	public: 
 
 	XVPSCAN();
 	~XVPSCAN();
 
 	void		ClearPage();
 
 	STATUS		Init(const Frame	frame,
 			     ERRSTK		&err);

	virtual void SendBitmapOperator() {}

	void AdjustPixmap(int newpixmapWidth, int newpixmapHeight);

	void UpdateCanvas();

 
 	// Send PostScript document prolog to PostScript server.
 	//
 	STATUS		DisplayData(const caddr_t start, const size_t nbytes);
 
 	// Send PostScript document page to PostScript server..
 	//
 	STATUS		DisplayPage(const float		scaleVal,
 				    const BBox		&displayBox,
 				    const caddr_t	ptr,
 				    const size_t	nbytes);
 
 	void		DrawLink(const PSLINK		&pslink,
 				 const BBox		&docbox,
 				 const float		mag);
 
 	void		DrawLinkBoxes(LIST<PSLINK*>	&links,
 				      const int		selected);
 
 	void		DrawSelectedLink(const PSLINK	&link,
 					 const BBox	&docbox,
 					 const float	mag);
 
 	void		SetSize(const int		width,
 				const int		height);
 

	void		DrawBox(const BOOL		fill,
				const BBox		&box);


	Rect&   	ConvertBBoxToXRect(const BBox &linkBox,
				   const BBox &docbox,
				   const float mag);


 	static void	RepaintProc(const Canvas	can,
 				    const Xv_Window	pwin,
 				    Display *const	dpy,
 				    const Window	xwin,
 				    Xv_xrectlist *const	rects); 

 
 	Xv_window	CanvasHandle() const
 	{
                return psCan;
 	}

	void ResetPostScript();

 };

#endif	/* _VIEWER_XVPSCAN_H_ */
