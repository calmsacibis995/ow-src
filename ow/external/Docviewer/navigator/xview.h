#ifndef	_XVIEW_H
#define	_XVIEW_H

#ident "@(#)xview.h	1.39 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/svrimage.h>
#include <xview/icon.h>
#include <xview/font.h>
#include <xview/defaults.h>
#include "spothelp.h"


// Misc XView utilities.
//
void	InitWindows(int *argc, char **argv);
void	SetMinFrameSize(Frame, int width, int height);

#endif	_XVIEW_H
