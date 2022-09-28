#ident "@(#)dpscan.h	1.20 11/11/93 Copyright 1992 Sun Microsystems, Inc."


#ifndef	_VIEWER_DPSCAN_H_
#define _VIEWER_DPSCAN_H_
 
#include "xvcanvas.h"
#include <xview/canvas.h>
#include <xview/xv_xrect.h>
#ifdef	undef
#include <xview/cms.h>
#endif
#include <X11/Xlib.h>

#include <DPS/dpsclient.h>
#include <DPS/dpsops.h>
#include "dpsdebug.h"


 class	XVCANVAS;
 
 class DPSCAN : public XVCANVAS {
 
 private:

 	DPSContext		ctxt;
 
 	GC			gc;
	GC			fillgc;
	Pixmap			bkgrndStipple;
	Pixmap			dataPage;

	int			pixmapDepth;
	int			pixmapWidth;
	int			pixmapHeight;
	int			function;
 
 	Canvas			dpsCan;
 
 	static Xv_opaque	dataKey;

	STATUS			CreateBkgrndStipple(ERRSTK	&err);	

 	static void		EventProc(const Xv_Window	win,
 					  Event *const		event);
 
	void		        PrepCanvas(const BBox		&displayBox,
					   const float			scaleVal);

	DPSOperation            operation;
	void                    SetOperation(DPSOperation currOperation)
	                        {operation = currOperation;}

	void ChangeGCfunction();
 	public: 

 
 	DPSCAN();
 	~DPSCAN();
 
 	void		ClearPage();
 
 	STATUS		Init(const Frame	frame,
 			     ERRSTK		&err);


	void SendBitmapOperator();

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
 
 	void		DrawLink(const PSLINK		&link,
 					 const BBox	&docbox,
 					 const float	mag);
 
 	void		DrawLinkBoxes(LIST<PSLINK*>	&links,
 				      const int		selected);
 
 	void		DrawSelectedLink(const PSLINK	&link,
 					 const BBox	&docbox,
 					 const float	mag);
 
 	void		SetSize(const int		width,
 				const int		height);
 

	void		DrawBox(const BOOL		fill,
				const BBox		&box);


	Rect&   ConvertBBoxToXRect(const BBox &linkBox,
				   const BBox &docbox,
				   const float mag);
 

 	static void		RepaintProc(const Canvas	can,
 					    const Xv_Window	pwin,
 					    Display *const	dpy,
 					    const Window	xwin,
 					    Xv_xrectlist *const	rects); 

 	Xv_window	CanvasHandle() const
 	{
                return dpsCan;
 	}

	void ResetPostScript();

	STRING* nameOperation(DPSOperation anOperation);
	void    DbgDPSOperation(DPSOperation anOperation);
 };

#endif	/* _VIEWER_DPSCAN_H_ */
