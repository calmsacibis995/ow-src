#ifndef lint
static char *sccsid = "@(#)props.c 3.4 93/03/05";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * props.c - property sheet for pageview.
 */

#include "pageview.h"
#include "ds_popup.h"

typedef struct {
    float       w,
                h;
}           Size;

/*
#define PAGE_NAMES "USLetter", "USLegal", "A3", "A4", "A5", "B5"
#define DPI_STRINGS "72", "85", "100", "150", "300", "400"
#define ORIENT_STRINGS "Left", "Right", "Upsidedown", "Upright"
*/

char  	*pagenames [] = { (char *) NULL, (char *) NULL, (char *) NULL,
			  (char *) NULL, (char *) NULL, (char *) NULL };

char 	*dpi_strings [] = { (char *) NULL, (char *) NULL, (char *) NULL,
			    (char *) NULL, (char *) NULL, (char *) NULL };

char 	*orient_strings [] = { (char *) NULL, (char *) NULL,
			       (char *) NULL, (char *) NULL };

/*
char       *pagenames[] = {PAGE_NAMES};
*/

static Size sizetable[] = {
    {8.500, 11.000},		/* US Letter */
    {8.500, 14.000},		/* US Legal */
    {11.693, 16.535},		/* A3 */
    {8.268, 11.693},		/* A4 */
    {5.827, 8.268},		/* A5 */
    {6.929, 9.842},		/* B5 */
};

static int  dpitable[] = {
    72, 85, 100, 150, 300, 400,
};


static void
props_apply(item, event)
    Panel_item  item;
    Event      *event;
{
    int new_method;
    int old_orient = pageview.orient;

    Panel_choice_item c_pagesize = xv_get(item, XV_KEY_DATA, UI1);
    Panel_choice_item c_dpi = xv_get(item, XV_KEY_DATA, UI2);
    Panel_choice_item c_orient = xv_get(item, XV_KEY_DATA, UI3);
    Panel_choice_item c_aa = xv_get(item, XV_KEY_DATA, UI4);
    Panel_choice_item c_timeout = xv_get (item, XV_KEY_DATA, UI5);
    Panel_choice_item c_method = xv_get (item, XV_KEY_DATA, UI8);

    setbusy();
    pageview.pagesize = xv_get(c_pagesize, PANEL_VALUE);
    pageview.dpi = xv_get(c_dpi, PANEL_VALUE);
    pageview.orient = xv_get(c_orient, PANEL_VALUE);
    pageview.aa = xv_get(c_aa, PANEL_VALUE);
    pageview.timeout = xv_get (c_timeout, PANEL_VALUE);
    new_method = xv_get (c_method, PANEL_VALUE);

    pagewidth = sizetable[pageview.pagesize].w;
    pageheight = sizetable[pageview.pagesize].h;
    dpi = dpitable[pageview.dpi];

    if (pageview.aa == 1)
       low_memory = FALSE;

    if (new_method != pageview.method) {
       pageview.method = new_method;
       CurrentPage = 1;
       MakePageTable (0);
       }

    MakePaper();
    pageview_ps_close ();
    GotoPage(CurrentPage);
    if (low_memory == FALSE)
       home_page();

    if (old_orient != pageview.orient) {
       int old_height;
       int old_width;
       int panel_height = xv_get (panel, XV_HEIGHT);
       if (((pageview.orient > 1) && (old_orient < 2)) ||
	   ((pageview.orient < 2) && (old_orient > 1))) {
	  old_height = xv_get (baseframe, XV_HEIGHT) - panel_height;
	  old_width = xv_get (baseframe, XV_WIDTH) + panel_height;
	  xv_set (baseframe,
		    XV_HEIGHT,  old_width > 850 ? pixh > 850 ? 850 
						    : pixh + panel_height
						: old_width,
		    XV_WIDTH,   old_height > 810 ? pixw > 935 ? 935 : pixw
						 : old_height, 
		    NULL);
	  }
       }

    setactive();
}

static void
props_reset(item, event)
    Panel_item  item;
    Event      *event;
{
    Panel_choice_item pagesize = xv_get(item, XV_KEY_DATA, UI1);
    Panel_choice_item dpi_item = xv_get(item, XV_KEY_DATA, UI2);
    Panel_choice_item orient = xv_get(item, XV_KEY_DATA, UI3);
    Panel_choice_item aa = xv_get(item, XV_KEY_DATA, UI4);
    Panel_choice_item timeout = xv_get (item, XV_KEY_DATA, UI5);
    Panel_choice_item method = xv_get (item, XV_KEY_DATA, UI8);

    xv_set(pagesize, PANEL_VALUE, pageview.pagesize, NULL);
    xv_set(dpi_item, PANEL_VALUE, pageview.dpi, NULL);
    xv_set(orient, PANEL_VALUE, pageview.orient, NULL);
    xv_set(aa, PANEL_VALUE, pageview.aa, NULL);
    xv_set(timeout, PANEL_VALUE, pageview.timeout, NULL);
    xv_set(method, PANEL_VALUE, pageview.method, NULL);
}


static void
props_defaults(item, event)
    Panel_item  item;
    Event      *event;
{
    Panel_choice_item pagesize = xv_get(item, XV_KEY_DATA, UI1);
    Panel_choice_item dpi_item = xv_get(item, XV_KEY_DATA, UI2);
    Panel_choice_item orient = xv_get(item, XV_KEY_DATA, UI3);
    Panel_choice_item aa = xv_get(item, XV_KEY_DATA, UI4);
    Panel_choice_item timeout = xv_get (item, XV_KEY_DATA, UI5);
    Panel_choice_item method = xv_get (item, XV_KEY_DATA, UI8);

    xv_set(pagesize, PANEL_VALUE, DEFAULT_PAGESIZE, NULL);
    xv_set(dpi_item, PANEL_VALUE, DEFAULT_DPI, NULL);
    xv_set(orient, PANEL_VALUE, DEFAULT_ORIENT, NULL);
    xv_set(aa, PANEL_VALUE, DEFAULT_AA, NULL);
    xv_set(timeout, PANEL_VALUE, DEFAULT_TIMEOUT, NULL);
    xv_set(method, PANEL_VALUE, DEFAULT_METHOD, NULL);
}

static    Panel_choice_item aa;

Frame
init_props(parent)
    Frame       parent;
{
    Frame       frame;
    Panel       panel;
    Panel_choice_item pagesize,
		dpi_item,
                orient,
		timeout,
		method;
	Panel_item apply_but;
	Panel_item reset_but;
	Panel_item default_but;
    char        fr_label [100];
    char       *pr_label;

    pagenames [0] = LGET ("USLetter");
    pagenames [1] = LGET ("USLegal");
    pagenames [2] = LGET ("A3");
    pagenames [3] = LGET ("A4");
    pagenames [4] = LGET ("A5");
    pagenames [5] = LGET ("B5");

    dpi_strings [0] = LGET ("72");
    dpi_strings [1] = LGET ("85");
    dpi_strings [2] = LGET ("100");
    dpi_strings [3] = LGET ("150");
    dpi_strings [4] = LGET ("300");
    dpi_strings [5] = LGET ("400");

    orient_strings [0] = LGET ("Left");
    orient_strings [1] = LGET ("Right");
    orient_strings [2] = LGET ("Upsidedown");
    orient_strings [3] = LGET ("Upright");

    pr_label = LGET ("Page Properties");

    sprintf (fr_label, "%s: %s", PV_Name, pr_label);
    frame = (Frame) xv_create(parent, FRAME_CMD,
			      FRAME_LABEL, fr_label,
			      NULL);
    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL);

    pagesize = xv_create(panel, PANEL_CHOICE,
			 XV_X, xv_col(panel, 0),
			 XV_Y, xv_row(panel, 0),
			 PANEL_LABEL_STRING, LGET("Size:"),
			 PANEL_VALUE, pageview.pagesize,
			 PANEL_CHOICE_STRINGS, pagenames [0],
					       pagenames [1], 
					       pagenames [2], 
					       pagenames [3], 
					       pagenames [4], 
					       pagenames [5], 
					       NULL,
			 XV_HELP_DATA, "pageview:pagesize",
			 NULL);
    dpi_item = xv_create(panel, PANEL_CHOICE,
		    XV_X, xv_col(panel, 0),
		    XV_Y, xv_row(panel, 1),
		    PANEL_LABEL_STRING, LGET("DPI:"),
		    PANEL_VALUE, pageview.dpi,
		    PANEL_CHOICE_STRINGS, dpi_strings [0],
					  dpi_strings [1],
					  dpi_strings [2],
					  dpi_strings [3],
					  dpi_strings [4],
					  dpi_strings [5],
					  NULL,
		    XV_HELP_DATA, "pageview:dpi",
		    NULL);

    aa = xv_create(panel, PANEL_CHECK_BOX,
		       PANEL_LABEL_STRING, LGET("AntiAlias:"),
		       PANEL_VALUE, pageview.aa,
		       XV_HELP_DATA, "pageview:aa",
		       NULL);

    orient = xv_create(panel, PANEL_CHOICE,
		       XV_X, xv_col(panel, 0),
		       XV_Y, xv_row(panel, 2),
		       PANEL_LABEL_STRING, LGET("Orientation:"),
		       PANEL_VALUE, pageview.orient,
		       PANEL_CHOICE_STRINGS, orient_strings [0],
					     orient_strings [1],
					     orient_strings [2],
					     orient_strings [3],
					     NULL,
		       XV_HELP_DATA, "pageview:orient",
		       NULL);

    timeout = xv_create (panel, PANEL_SLIDER,
			XV_X, xv_col(panel, 0),
			XV_Y, xv_row(panel, 3),
		        PANEL_LABEL_STRING, LGET("Job Timeout (sec):"),
		        PANEL_VALUE, pageview.timeout,
		        XV_HELP_DATA, "pageview:timeout",
                        PANEL_MIN_VALUE, 5,
                        PANEL_MAX_VALUE, 180,
                        PANEL_SLIDER_END_BOXES, True,
			NULL);

    method = xv_create(panel, PANEL_CHECK_BOX,
		       PANEL_LABEL_STRING, 
			  LGET("Ignore PostScript Structuring Comments:"),
		       PANEL_VALUE, pageview.method,
		       XV_X, xv_col(panel, 0),
		       XV_Y, xv_row(panel, 4),
		       XV_HELP_DATA, "pageview:method",
		       NULL);

    apply_but = xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("Apply"),
		     PANEL_NOTIFY_PROC, props_apply,
		     XV_KEY_DATA, UI1, pagesize,
		     XV_KEY_DATA, UI2, dpi_item,
		     XV_KEY_DATA, UI3, orient,
		     XV_KEY_DATA, UI4, aa,
		     XV_KEY_DATA, UI5, timeout,
		     XV_KEY_DATA, UI8, method,
			 XV_X, xv_col(panel, 12),
		     XV_Y, xv_row(panel, 5),
		     XV_HELP_DATA, "pageview:apply",
		     NULL);
    reset_but = xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("Reset"),
		     PANEL_NOTIFY_PROC, props_reset,
		     XV_KEY_DATA, UI1, pagesize,
		     XV_KEY_DATA, UI2, dpi_item,
		     XV_KEY_DATA, UI3, orient,
		     XV_KEY_DATA, UI4, aa,
		     XV_KEY_DATA, UI5, timeout,
		     XV_KEY_DATA, UI8, method,
		     XV_Y, xv_row(panel, 5),
		     XV_HELP_DATA, "pageview:reset",
		     NULL);
    default_but = xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("Defaults"),
		     PANEL_NOTIFY_PROC, props_defaults,
		     XV_KEY_DATA, UI1, pagesize,
		     XV_KEY_DATA, UI2, dpi_item,
		     XV_KEY_DATA, UI3, orient,
		     XV_KEY_DATA, UI4, aa,
		     XV_KEY_DATA, UI5, timeout,
		     XV_KEY_DATA, UI8, method,
		     XV_Y, xv_row(panel, 5),
		     XV_HELP_DATA, "pageview:defaults",
		     NULL);
    window_fit(panel);

    xv_set(frame,
	   XV_WIDTH, xv_get(panel, XV_WIDTH),
	   XV_HEIGHT, xv_get(panel, XV_HEIGHT),
	   NULL);

	ds_center_items(panel, -1, apply_but, reset_but, default_but, 0);
    return (frame);
}

void
props_AAinactivate()
{
    xv_set(aa, PANEL_INACTIVE, TRUE, NULL);
    pageview.aa = 0;
}

