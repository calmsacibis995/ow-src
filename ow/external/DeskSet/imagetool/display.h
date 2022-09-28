
/*
 * @(#)display.h 1.11 94/03/14
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>
#include <xil/xil.h>

typedef struct {
	unsigned long   canvas;		/* Canvas id			  */
	Window		win;		/* XID of window on which to draw */
	GC		win_gc;		/* GC to use for draw operations  */
	int		depth;		/* Depth of window		  */
	int		height;		/* Height of window		  */
	int		width;		/* Width of window		  */
	XilSystemState  state;		/* Xil sytem state 		  */
	XilImage	display_win;    /* Xil display window		  */
	Display	       *xdisplay;	/* X Display pointer		  */
	Visual         *visual;		/* Window visual		  */
	Pixmap		pixmap1;	/* Pixmap on which image rendered */
	int		pix_height;	/* Height of pixmap		  */
	int		pix_width;	/* Width of pixmap		  */
	Pixmap		pixmap2;	/* Second page of ps image	  */
	GC		fill_gc;	/* GC to use for fill operations  */
	GC		select_gc;	/* GC to use for select op 	  */
	float           res_x;          /* Resolution of screen in x      */
	float           res_y;          /* Resolution of screen in y      */
	float		background[3];	/* Background pixel		  */
        float   	white[4];	/* White pixel			  */
	unsigned long   black;          /* Black pixel			  */
	unsigned long	paint_win;	/* Pointer to current paint win   */
	float		pageheight;	/* Height of ps page		  */
	float		pagewidth;	/* Width of ps page		  */
} DisplayInfo;

extern DisplayInfo     *image_display;
extern DisplayInfo     *ps_display;
extern DisplayInfo     *current_display;

/* Function prototypes */
extern Pixmap		create_pixmap (DisplayInfo *, int, int, int, int);

#endif
