#ident "@(#)xview.cc	1.35 07/06/93 Copyright 1990-1992 Sun Microsystems, Inc."



#include <doc/common.h>
#include "xview.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>


// Default double-click timeout (X 1/10 sec).
//
static const int	DEFAULT_DOUBLE_CLICK(4);


void
InitWindows(int *argc, char **argv)
{
	xv_init(XV_INIT_ARGC_PTR_ARGV, argc, argv,
		XV_USE_LOCALE, TRUE,
		NULL);
}

void
SetMinFrameSize(Frame frame, int width, int height)
{
	XSizeHints	hints;

	DbgMed("SetMinFrameSize:"
		<< "  min width = "	<< width
		<< ", min height = "	<< height
		<< endl);

	hints.flags      = PMinSize|PPosition;
	hints.min_width  = width;
	hints.min_height = height;

	XSetWMNormalHints(	(Display *) xv_get(frame, XV_DISPLAY),
				(Window)    xv_get(frame, XV_XID),
				&hints);
}
