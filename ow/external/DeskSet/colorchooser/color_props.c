#ifndef lint
static char sccsid[] = "@(#)color_props.c	3.2 08/11/92";
#endif

/*
 * color_props.c - Color property sheet for the OpenWindows Properties program.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "colorchooser.h"
#include "color.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <xview/cursor.h>
#include <xview/fullscreen.h>
#include <xview/notify.h>
#include <xview/cms.h>
#include <xview/svrimage.h>

#include <ds_colors.h>

static int  hues[] = {0, 43, 63, 135, 180, 225, 280, 315};	/* 0 - 360 */
static int  saturations[] = {200, 400, 600, 800};		/* 0 - 1000 */
static int  brightnesses[] = {750, 900};			/* 0 - 1000 */
static int  grays[] = {300, 400, 500, 600, 700, 800};		/* 0 - 1000 */

#define NUM_BRIGHTNESSES	(sizeof(brightnesses)/sizeof(int))
#define NUM_HUES		(sizeof(hues)/sizeof(int))
#define NUM_SATURATIONS		(sizeof(saturations)/sizeof(int))
#define NUM_GRAYS		(sizeof(grays)/sizeof(int))
#define NUM_COLUMNS		(NUM_HUES+1)
#define NUM_CUSTOM		2

#define COLOR_CHOICES (NUM_HUES*NUM_SATURATIONS*NUM_BRIGHTNESSES+NUM_GRAYS+NUM_CUSTOM) /* 8*4*2+6+2=72 */
#define NUM_RWCOLORS (BlackIndex + 1)
#define PALETTE_SIZE           72
#define CUSTOM_INDEX		0
#define WORK_INDEX		0
#define WIN_INDEX		1

static unsigned long *xcolors;
static Xv_singlecolor scolors[MAX_DS_COLORS];

/* static HSV colors[COLOR_CHOICES]; */

static Cms  cms = XV_NULL;
static int  ncolors;
static char *image_string;

static Panel_item h_slider;
static Panel_item s_slider;
static Panel_item v_slider;
static Panel_item h_ticks;
static Panel_item s_ticks;
static Panel_item v_ticks;
static Panel_item palette;
static Panel_item custom;
static int  palette_x;

static int  backup_work_index;
static int  backup_win_index;
static HSV  backup_xact_win;
static HSV  backup_xact_work;

static int  slidermode;		/* palette == 0, sliders == 1 */
int  windowmode;		/* workspace == 0, window == 1 */
static int  work_index;
static int  win_index;
static int  chip1_index;
static int  chip2_index;
static HSV  xact_slider;

HSV           xact_win;
HSV           xact_work;
char 	     *iconname;
char 	     *iconmaskname;
Server_image  stipple;
Server_image  clip_mask;
int           new_image = FALSE;

static XGCValues    gcvals;
static GC           gc, bg_gc;
static Server_image preview_image;
static Pixmap       pixmap;

#define CHIP_HEIGHT	16
#define CHIP_WIDTH	16
static unsigned short chip_data[] = {
    0x7FFE, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x7FFE
};
static Server_image color_chip;

#define IMAGE_WIDTH 64
#define IMAGE_HEIGHT 64
unsigned short image_data[] = {
#include "preview.icon"
};

#define FIRST_SLIDER_Y		40
#define SLIDER_GAP		40
#define SLIDER_TICK_GAP		4
#define SLIDER_WIDTH		(NUM_COLUMNS*(CHIP_WIDTH+8))
#define SLIDER_MAX		MAXSV
#define SLIDER_MAX_H		MAXH

#define SIMAGE_WIDTH (SLIDER_WIDTH - 44)
#define SIMAGE_HEIGHT 16
static char hue_data[SIMAGE_WIDTH * SIMAGE_HEIGHT];
static char sat_data[SIMAGE_WIDTH * SIMAGE_HEIGHT];
static char val_data[SIMAGE_WIDTH * SIMAGE_HEIGHT];
Panel_item preview;
static Server_image hue_image;
static Server_image sat_image;
static Server_image val_image;
static int  huepix[NUM_HUES];
static int  satpix[NUM_SATURATIONS];
static int  valpix[NUM_GRAYS];

#ifdef NEVER
/*
 * string_to_xcolor()
 *   string may be:
 *     color name:   "Blue", "Pink", etc.
 *     hex color:   #a0bf1e
 *     sunview decimal color:   100 220 255
 *
 * if it is not one of these formats (or if the name cannot be found),
 * then string_to_xcolor() returns 1, otherwise it returns 0.
 */
static int
string_to_xcolor(s, xcolor)
    char       *s;
    XColor     *xcolor;
{
    Colormap  cmap;

    cmap = xv_get( xv_get( frame, WIN_CMS ), CMS_CMAP_ID );
    if (!XParseColor(dsp, cmap, s, xcolor)) {
	int         red,
	            green,
	            blue;
	if (sscanf(s, "%d %d %d", &red, &green, &blue) == 3) {
	    xcolor->red = red << 8;
	    xcolor->green = green << 8;
	    xcolor->blue = blue << 8;
	    xcolor->flags = DoRed | DoGreen | DoBlue;
	} else
	    return 1;
    }
    /*
     * XParseColor may not have multiplied by 257...
     */
    xcolor->red &= 0xff00;
    xcolor->green &= 0xff00;
    xcolor->blue &= 0xff00;
    return 0;
}
#endif

void
update_colors()
{
    unsigned long valuemask = 0;
    char          errmsg[2048];
    int           width  = IMAGE_WIDTH, 
                  height = IMAGE_HEIGHT;

    if ( new_image ) {
      xv_destroy_safe( stipple );
      stipple = XV_NULL;
      xv_destroy_safe( clip_mask );
      clip_mask = XV_NULL;
      xv_destroy_safe( preview_image );
      preview_image = XV_NULL;

      if ( iconname && (*iconname != NULL) && !stipple ) {
        stipple = ( Server_image )icon_load_svrim( iconname, errmsg );
        if ( !stipple ) return;
        width  = ( int )xv_get( stipple, XV_WIDTH );
        height = ( int )xv_get( stipple, XV_HEIGHT );
        gcvals.stipple = ( Pixmap )xv_get( stipple, SERVER_IMAGE_PIXMAP );
        valuemask |= GCStipple;
      }
      if ( iconmaskname && !clip_mask ) {
        clip_mask = ( Server_image )icon_load_svrim( iconmaskname, errmsg );
        if ( clip_mask ) {
          gcvals.clip_mask = ( Pixmap )xv_get( clip_mask, SERVER_IMAGE_PIXMAP );
          valuemask |= GCClipMask;
        }
        else
          gcvals.clip_mask = None;
      }
      else
        gcvals.clip_mask = None;

      preview_image = ( Server_image )xv_create( NULL, SERVER_IMAGE,
	 		  SERVER_IMAGE_COLORMAP, "palette",
		          SERVER_IMAGE_DEPTH, ( int )xv_get( frame, WIN_DEPTH ),
			  XV_WIDTH, width,
			  XV_HEIGHT, height,
                          NULL );
      pixmap = ( Pixmap )xv_get( preview_image, SERVER_IMAGE_PIXMAP );
      if ( clip_mask )
        XFillRectangle( dsp, pixmap, bg_gc, 0, 0, width, height );
      valuemask |= GCFillStyle;
      gcvals.fill_style = FillOpaqueStippled;
      new_image = FALSE;
    }

    gcvals.foreground = xcolors[ work_index + CMS_CONTROL_COLORS ];
    gcvals.background = xcolors[ win_index  + CMS_CONTROL_COLORS ];
    valuemask |= GCForeground;
    valuemask |= GCBackground;

    XChangeGC( dsp, gc, valuemask, &gcvals );
    XFillRectangle( dsp, pixmap, gc, 0, 0, width, height );
    xv_set( preview_image,
            SERVER_IMAGE_PIXMAP, pixmap,
	    NULL );
    xv_set( preview,
	    PANEL_LABEL_IMAGE, preview_image,
            NULL );

}

static void
update_choices()
{
    if (slidermode) {
	if (windowmode) {
	    xact_slider = xact_win;
	} else {
	    xact_slider = xact_work;
	}
	xv_set(h_slider,
	       PANEL_VALUE, xact_slider.h,
	       NULL);
	xv_set(s_slider,
	       PANEL_VALUE, xact_slider.s,
	       NULL);
	xv_set(v_slider,
	       PANEL_VALUE, xact_slider.v,
	       NULL);
    } else {
	int         the_index;

	the_index = (windowmode) ? 1 : 0;

	xv_set(palette, PANEL_VALUE, the_index, NULL);
    }
}

static char *
hsv_to_string(hsv)
    HSV        *hsv;
{
    XColor      xcolor;
    static char s[8];

    hsv_to_xcolor(hsv, &xcolor);
    sprintf(s, "#%02x%02x%02x",
	    xcolor.red >> 8, xcolor.green >> 8, xcolor.blue >> 8);
    return strdup(s);
}


void
store_custom_colors()
{
    Xv_singlecolor scolor;
    RGB    rgb;
    int    i;
    /*  
     * Put fg and  bg color in first two color chips if colors exist.
     */
    hsv_to_rgb( &xact_work, &rgb );
    scolor.red   = ( u_char )rgb.r;
    scolor.green = ( u_char )rgb.g;
    scolor.blue  = ( u_char )rgb.b;
    chip1_index = ds_cms_index( cms, &scolor ) - CMS_CONTROL_COLORS;
    work_index = chip1_index;

    hsv_to_rgb( &xact_win, &rgb );
    scolor.red   = ( u_char )rgb.r;
    scolor.green = ( u_char )rgb.g;
    scolor.blue  = ( u_char )rgb.b;
    chip2_index = ds_cms_index( cms, &scolor ) - CMS_CONTROL_COLORS;
    win_index = chip2_index;

    xv_set( palette,
            PANEL_VALUE,        windowmode ? 1 : 0,
            PANEL_CHOICE_IMAGE, 0, color_chip,
            PANEL_CHOICE_IMAGE, 1, color_chip, 
            PANEL_CHOICE_COLOR, 0, CMS_CONTROL_COLORS + chip1_index,
            PANEL_CHOICE_COLOR, 1, CMS_CONTROL_COLORS + chip2_index,
            NULL);

}

void
backup_colors()
{
    backup_work_index = work_index;
    backup_win_index = win_index;
    backup_xact_win = xact_win;
    backup_xact_work = xact_work;
}

static void
restore_colors()
{
    work_index = backup_work_index;
    win_index = backup_win_index;
    xact_win = backup_xact_win;
    xact_work = backup_xact_work;
}

void
apply_colors( scolor )
    Xv_singlecolor *scolor;
{
    XColor         xcolor;

    backup_colors();
    /*
     * Convert to RGB and return it.
     */
    if (windowmode)
      hsv_to_xcolor(&xact_win, &xcolor);
    else
      hsv_to_xcolor(&xact_work, &xcolor );

    scolor->red   = xcolor.red >> 8;
    scolor->green = xcolor.green >> 8;
    scolor->blue  = xcolor.blue >> 8;

}

void
current_color( scolor )
    Xv_singlecolor *scolor;
{
    XColor         xcolor;

    /*
     * Convert to RGB and return it.
     */
    if (windowmode)
      hsv_to_xcolor(&xact_win, &xcolor);
    else
      hsv_to_xcolor(&xact_work, &xcolor );

    scolor->red   = xcolor.red >> 8;
    scolor->green = xcolor.green >> 8;
    scolor->blue  = xcolor.blue >> 8;

}

void
reset_colors()
{
    restore_colors();
    update_choices();
    update_colors();
}

static void
color_notify(panel_item, choice, event)
    Panel_item  panel_item;
    int         choice;
    Event      *event;
 {
    RGB     rgb;
    int     cms_index;

    if (windowmode) {
      if ( choice == 0 )
        win_index = chip1_index;
      else if ( choice == 1 )
        win_index = chip2_index;
      else
	win_index = choice;
      rgb.r = ( int )scolors[win_index].red;
      rgb.g = ( int )scolors[win_index].green;
      rgb.b = ( int )scolors[win_index].blue;
      rgb_to_hsv( &rgb, &xact_win );
  } else {
        if ( choice == 0 ) 
          work_index = chip1_index;
        else if ( choice == 1 )
          work_index = chip2_index;
        else
	  work_index = choice;
        rgb.r = ( int )scolors[work_index].red;
        rgb.g = ( int )scolors[work_index].green;
        rgb.b = ( int )scolors[work_index].blue;
        rgb_to_hsv( &rgb, &xact_work );
    }
    update_colors();
}

static void
slider_notify(panel_item, value, event)
    Panel_item  panel_item;
    int         value;
    Event      *event;
{
    int            save_index = work_index;
    RGB            rgb;
    Xv_singlecolor scolor;

    switch (xv_get(panel_item, PANEL_CLIENT_DATA)) {
    case 1:
	xact_slider.h = value;
	break;
    case 2:
	xact_slider.s = value;
	break;
    case 3:
	xact_slider.v = value;
	break;
    }
    hsv_to_rgb( &xact_slider, &rgb );
    scolor.red   = ( u_char )rgb.r;
    scolor.green = ( u_char )rgb.g;
    scolor.blue  = ( u_char )rgb.b;

    if (windowmode) {
        win_index = ds_cms_index( cms, &scolor ) - CMS_CONTROL_COLORS;
        if ( win_index != save_index ) {
          update_colors();
          rgb.r = ( int )scolors[win_index].red;
          rgb.g = ( int )scolors[win_index].green;
          rgb.b = ( int )scolors[win_index].blue;
          rgb_to_hsv( &rgb, &xact_slider );
          rgb_to_hsv( &rgb, &xact_win );
        }
    } else {
        work_index = ds_cms_index( cms, &scolor ) - CMS_CONTROL_COLORS;
        if ( work_index != save_index ) {
          update_colors();
          rgb.r = ( int )scolors[work_index].red;
          rgb.g = ( int )scolors[work_index].green;
          rgb.b = ( int )scolors[work_index].blue;
          rgb_to_hsv( &rgb, &xact_slider );
          rgb_to_hsv( &rgb, &xact_work );
        }
    }
}

static void
switch_modes()
{
    if (slidermode) {
	xv_set(palette, XV_SHOW, FALSE, NULL);
	update_choices();
	xv_set(h_slider, XV_SHOW, TRUE, NULL);
	xv_set(s_slider, XV_SHOW, TRUE, NULL);
	xv_set(v_slider, XV_SHOW, TRUE, NULL);
	xv_set(h_ticks, XV_SHOW, TRUE, NULL);
	xv_set(s_ticks, XV_SHOW, TRUE, NULL);
	xv_set(v_ticks, XV_SHOW, TRUE, NULL);
    } else {
	xv_set(h_slider, XV_SHOW, FALSE, NULL);
	xv_set(s_slider, XV_SHOW, FALSE, NULL);
	xv_set(v_slider, XV_SHOW, FALSE, NULL);
	xv_set(h_ticks, XV_SHOW, FALSE, NULL);
	xv_set(s_ticks, XV_SHOW, FALSE, NULL);
	xv_set(v_ticks, XV_SHOW, FALSE, NULL);
	update_choices();
	xv_set(palette, XV_SHOW, TRUE, NULL);
    }
}


static void
custom_notify(panel_item, choice, event)
    Panel_item  panel_item;
    int         choice;
    Event      *event;
{
    if (slidermode && !choice) {
	store_custom_colors();
    }
    slidermode = choice;
    switch_modes();
}

void
create_palette()
{
    int         i,
                h,
                s,
                v;
    RGB       rgb;

    i = NUM_CUSTOM;		/* leave room for the user's current defaults */

    /* make the gray ramp. */

    for (v = 0; v < NUM_GRAYS; v++) {
	valpix[v] = i;
	i++;
    }

    /* make the color cube. */
    for (h = 0; h < NUM_HUES; h++) {
	for (v = 0; v < NUM_BRIGHTNESSES; v++) {
	    for (s = 0; s < NUM_SATURATIONS; s++) {
		if (v == 1 && h == 6)
		    satpix[s] = i;
		if (v == 1 && s == 1)
		    huepix[h] = i;
		i++;
	    }
	}
    }


    cms = ds_cms_create( frame );
    if ( !cms ) {
      fprintf( stderr, MGET( "Color Chooser: Unable to create Colormap Segment\n" ) );
      exit( 0 );
    }
    xv_set( cms, CMS_NAME, "palette", XV_NULL );

    /*
     * Force all colors to be allocated now.
     */
    ncolors = ds_alloc_colors( frame );

    work_index = WORK_INDEX;
    win_index  = WIN_INDEX;

}


void
create_color_panel()
{
    int            i,
                   j,
                   ncols;
    unsigned long  valuemask;

    create_palette();

    xv_set(panel,
	   WIN_CMS, cms,
	   XV_HELP_DATA, "colorchooser:ColorPanelInfo",
	   NULL);
    /*
     * Return the XColors for the cms.
     * XColors are needed for Xlib drawing.
     */
    xcolors = ( unsigned long * )xv_get( panel, WIN_X_COLOR_INDICES );
    /*
     * Return the Xv_singlecolors for the cms and
     * omit the CMS_CONTROL_COLORS.
     * Xv_singlecolors are needed to return the RGB values to other apps.
     */
    xv_get( cms, CMS_COLORS, scolors );

    for ( i = 0; i < ncolors; i ++ )
      scolors[i] = scolors[i + CMS_CONTROL_COLORS];

    /*
     * Create a background gc for panel items.
     */
    valuemask = GCForeground;
    gcvals.foreground = xcolors[ 1 ];
    bg_gc = XCreateGC( dsp, drawable, valuemask, &gcvals );
 
    /* 
     * Create a default image in case no icon file is specified.
     */
    valuemask         = GCFillStyle;
    gcvals.fill_style = FillSolid;
    gc = XCreateGC( dsp, drawable, valuemask, &gcvals );

    preview_image = ( Server_image )xv_create( NULL, SERVER_IMAGE,
			  SERVER_IMAGE_COLORMAP, "palette",
		          SERVER_IMAGE_DEPTH, ( int )xv_get( frame, WIN_DEPTH ),
			  XV_WIDTH, IMAGE_WIDTH,
			  XV_HEIGHT, IMAGE_HEIGHT,
                          NULL );
    pixmap = ( Pixmap )xv_get( preview_image, SERVER_IMAGE_PIXMAP );

    preview = xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, preview_image,
			XV_Y, 20,
			XV_HELP_DATA, "colorchooser:AppImageInfo",
			NULL);

    custom = xv_create(panel, PANEL_CHOICE,
		       PANEL_CHOICE_STRINGS,
		         MGET("Palette"), MGET("Custom"),
		       NULL,
		       PANEL_NOTIFY_PROC, custom_notify,
		       PANEL_NEXT_ROW, -1,
		       PANEL_CHOICE_NCOLS, 1,
		       PANEL_VALUE, 0,
		       XV_HELP_DATA, "colorchooser:CustomColorInfo",
		       XV_SHOW, FALSE,
		       NULL);

    palette_x = xv_get(custom, XV_WIDTH) + 40;

    xv_set(preview,
	   XV_X, (palette_x - IMAGE_WIDTH) / 2,
	   NULL);
    xv_set(custom,
	   XV_X, (palette_x - xv_get(custom, XV_WIDTH)) / 2,
	   NULL);

    if ( ( ( ncolors - 1 ) / 8 + 1 ) > 16 )
      ncols = ( ncolors - 1 ) / 16 + 1;
    else
      ncols = ( ncolors - 1 ) / 8 + 1;

   /*
    * Inactive the custom button is the number of palette
    * colors is < 16 independent of the framebuffer type.
    */
   if ( ncolors <= 16 )
     xv_set( custom, XV_SHOW, FALSE, NULL );
    
    palette = xv_create(panel, PANEL_CHOICE,
			XV_SHOW, FALSE,
			PANEL_CHOICE_NCOLS, ncols,
			PANEL_LAYOUT, PANEL_VERTICAL,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			PANEL_NOTIFY_PROC, color_notify,
			XV_HELP_DATA, "colorchooser:PaletteInfo",
			XV_X, palette_x,
			XV_Y, 10,
			NULL);

    h_slider = xv_create(panel, PANEL_SLIDER,
			 XV_SHOW, FALSE,
			 PANEL_LABEL_STRING, MGET( "Hue:" ),
                         PANEL_ITEM_COLOR, 2,
			 PANEL_CLIENT_DATA, 1,
			 PANEL_NOTIFY_LEVEL, PANEL_ALL,
			 PANEL_SHOW_RANGE, FALSE,
			 PANEL_SHOW_VALUE, FALSE,
			 PANEL_VALUE, 0,
			 PANEL_MIN_VALUE, 0,
			 PANEL_MAX_VALUE, SLIDER_MAX_H,
			 PANEL_NOTIFY_PROC, slider_notify,
			 XV_HELP_DATA, "colorchooser:HueSlider",
			 NULL);
    for (j = 0; j < SIMAGE_HEIGHT; j++)
	for (i = 0; i < SIMAGE_WIDTH; i++)
	    hue_data[j * SIMAGE_WIDTH + i] = CMS_CONTROL_COLORS +
		huepix[i * NUM_HUES / SIMAGE_WIDTH];
    hue_image = xv_create(XV_NULL, SERVER_IMAGE,
			  SERVER_IMAGE_COLORMAP, "palette",
			  XV_WIDTH, SIMAGE_WIDTH,
			  XV_HEIGHT, SIMAGE_HEIGHT,
			  SERVER_IMAGE_DEPTH, ( int )xv_get( frame, WIN_DEPTH ),
			  SERVER_IMAGE_X_BITS, hue_data,
			  NULL);
    h_ticks = xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, hue_image,
			NULL);


    s_slider = xv_create(panel, PANEL_SLIDER,
			 XV_SHOW, FALSE,
			 PANEL_LABEL_STRING, MGET( "Saturation:" ),
                         PANEL_ITEM_COLOR, 2,
			 PANEL_CLIENT_DATA, 2,
			 PANEL_NOTIFY_LEVEL, PANEL_ALL,
			 PANEL_SHOW_RANGE, FALSE,
			 PANEL_SHOW_VALUE, FALSE,
			 PANEL_VALUE, 0,
			 PANEL_MIN_VALUE, 0,
			 PANEL_MAX_VALUE, SLIDER_MAX,
			 PANEL_NOTIFY_PROC, slider_notify,
			 XV_HELP_DATA, "colorchooser:SaturationSlider",
			 NULL);
    for (j = 0; j < SIMAGE_HEIGHT; j++)
	for (i = 0; i < SIMAGE_WIDTH; i++)
	    sat_data[j * SIMAGE_WIDTH + i] = CMS_CONTROL_COLORS +
		satpix[i * NUM_SATURATIONS / SIMAGE_WIDTH];
    sat_image = xv_create(XV_NULL, SERVER_IMAGE,
			  SERVER_IMAGE_COLORMAP, "palette",
 			  XV_WIDTH, SIMAGE_WIDTH,
			  XV_HEIGHT, SIMAGE_HEIGHT,
			  SERVER_IMAGE_DEPTH, ( int )xv_get( frame, WIN_DEPTH ),
			  SERVER_IMAGE_X_BITS, sat_data,
			  NULL);
    s_ticks = xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, sat_image,
			NULL);


    v_slider = xv_create(panel, PANEL_SLIDER,
			 XV_SHOW, FALSE,
			 PANEL_LABEL_STRING, MGET( "Brightness:" ),
                         PANEL_ITEM_COLOR, 2,
			 PANEL_CLIENT_DATA, 3,
			 PANEL_NOTIFY_LEVEL, PANEL_ALL,
			 PANEL_SHOW_RANGE, FALSE,
			 PANEL_SHOW_VALUE, FALSE,
			 PANEL_VALUE, 0,
			 PANEL_MIN_VALUE, 0,
			 PANEL_MAX_VALUE, SLIDER_MAX,
			 PANEL_NOTIFY_PROC, slider_notify,
			 XV_HELP_DATA, "colorchooser:BrightnessSlider",
			 NULL);
    for (j = 0; j < SIMAGE_HEIGHT; j++)
	for (i = 0; i < SIMAGE_WIDTH; i++)
	    val_data[j * SIMAGE_WIDTH + i] = CMS_CONTROL_COLORS +
		valpix[i * NUM_GRAYS / SIMAGE_WIDTH];
    val_image = xv_create(XV_NULL, SERVER_IMAGE,
			  SERVER_IMAGE_COLORMAP, "palette",
			  XV_WIDTH, SIMAGE_WIDTH,
			  XV_HEIGHT, SIMAGE_HEIGHT,
			  SERVER_IMAGE_DEPTH, ( int )xv_get( frame, WIN_DEPTH ),
			  SERVER_IMAGE_X_BITS, val_data,
			  NULL);
    v_ticks = xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_IMAGE, val_image,
			NULL);


    i = xv_get(s_slider, PANEL_LABEL_WIDTH);
    j = SLIDER_GAP + xv_get(h_slider, XV_HEIGHT);
    xv_set(h_slider,
	   PANEL_VALUE_X, palette_x + i,
	   PANEL_VALUE_Y, FIRST_SLIDER_Y,
	   PANEL_SLIDER_WIDTH, SLIDER_WIDTH - j,
	   NULL);
    xv_set(s_slider,
	   PANEL_VALUE_X, palette_x + i,
	   PANEL_VALUE_Y, xv_get(h_slider, PANEL_VALUE_Y) + j,
	   PANEL_SLIDER_WIDTH, SLIDER_WIDTH - j,
	   NULL);
    xv_set(v_slider,
	   PANEL_VALUE_X, palette_x + i,
	   PANEL_VALUE_Y, xv_get(s_slider, PANEL_VALUE_Y) + j,
	   PANEL_SLIDER_WIDTH, SLIDER_WIDTH - j,
	   NULL);

    xv_set(h_ticks,
	   XV_X, palette_x + i,
	   XV_Y, xv_get(h_slider, PANEL_VALUE_Y)
	   + xv_get(h_slider, XV_HEIGHT)
	   + SLIDER_TICK_GAP,
	   NULL);
    xv_set(s_ticks,
	   XV_X, palette_x + i,
	   XV_Y, xv_get(h_ticks, XV_Y) + j,
	   NULL);
    xv_set(v_ticks,
	   XV_X, palette_x + i,
	   XV_Y, xv_get(s_ticks, XV_Y) + j,
	   NULL);

    color_chip = xv_create(XV_NULL, SERVER_IMAGE,
			   XV_WIDTH, CHIP_WIDTH,
			   XV_HEIGHT, CHIP_HEIGHT,
			   SERVER_IMAGE_DEPTH, 1,
			   SERVER_IMAGE_BITS, chip_data,
			   NULL);
    for (i = 0; i < ncolors; i++)
	xv_set(palette,
	       PANEL_CHOICE_IMAGE, i, color_chip,
	       PANEL_CHOICE_COLOR, i, CMS_CONTROL_COLORS + i,
	       NULL);

    xv_set(custom, PANEL_VALUE, slidermode, NULL);
store_custom_colors();
    update_colors();
    switch_modes();
    backup_colors();
    window_fit_width(panel);

}

