/*
 * @(#)props.h 1.5 93/07/14
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef PROPS_H
#define PROPS_H

#define DS_FILENAME ".desksetdefaults"

#define VIEW_GRAY_SCALE          "GrayScale"
#define VIEW_COLOR               "Color"

#define COLORS_BW                "BW"             /*  1-bit */
#define COLORS_16                "16"             /*  4-bit */
#define COLORS_256               "256"            /*  8-bit */
#define COLORS_THOUSANDS         "Thousands"      /* 16-bit */
#define COLORS_MILLIONS          "Millions"       /* 24-bit */

#define DISPLAY_PALETTE          "True"
#define UNDISPLAY_PALETTE        "False"

#define USE_DSC                  "True"
#define NO_DSC                   "False"

#define DEFAULT_VIEW_IN 	 VIEW_COLOR
#define DEFAULT_COLORS           COLORS_256
#define DEFAULT_SHOW_PALETTE	 DISPLAY_PALETTE
#define DEFAULT_DSC              NO_DSC

#define RES_VIEW_IN         0
#define RES_COLORS          1
#define RES_SHOW_PALETTE    2
#define RES_DSC             3

typedef struct {
        char              *name;
        char              *value;
} ResourceInfo;

typedef struct {
        int        	   view_in;
	int                color;
	int	           show_palette;
/*	FrameBufTypes      framebuf; */
	int                use_dsc;
	int		   props_changed;
} PropInfo;

extern PropInfo   *current_props;

/* Function prototypes */
extern int 		read_props();
extern void             props_size_panel();

#endif

