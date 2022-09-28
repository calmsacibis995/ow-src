#pragma ident "@(#)fish_props.c	1.13 93/03/31	SMI"

/*
 * Created by Bruce McIntyre
 */

#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <xview/server.h>
#include <xview/seln.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/byteorder.h>
#ifdef	SYSV
#include <string.h>
#else	/* SYSV */
#include <strings.h>
#endif	/* SYSV */
#include <ctype.h>
#include <sys/types.h>


/*
 * Sun raster file stuff
 */

union {
    char unf[32]; /* unformated */
    struct {
        unsigned char magic[  4];	/* magic number */
        unsigned char width[  4];	/* width of image in pixels */
        unsigned char height[ 4];	/* height of image in pixels */
        unsigned char depth[  4];	/* depth of each pixel */
        unsigned char length[ 4];	/* length of the image in bytes */
        unsigned char type[   4];	/* format of file */
        unsigned char maptype[4];	/* type of colormap */
        unsigned char maplen[ 4];	/* length of colormap in bytes */
    } fc; /* formated */
    struct {
        unsigned long magic;		/* magic number */
        unsigned long width;		/* width of image in pixels */
        unsigned long height;		/* height of image in pixels */
        unsigned long depth;		/* depth of each pixel */
        unsigned long length;		/* length of the image in bytes */
        unsigned long type;		/* format of file */
        unsigned long maptype;		/* type of colormap */
        unsigned long maplen;		/* length of colormap in bytes */
    } fi; /* formated */
} sr_header;

struct {
    unsigned long length;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
} sr_cmap;

/*
 * Following the header is the colormap (unless maplen is zero) then
 * the image.  Each row of the image is rounded to 2 bytes.
 */

#define RMAGICNUMBER 0x59a66a95 /* magic number of this file type */

/*
 * These are the possible file formats.
 */

#define ROLD       0	/* old format, see /usr/include/rasterfile.h */
#define RSTANDARD  1	/* standard format */
#define RRLENCODED 2	/* run length encoding to compress the image */

/* 
 * These are the possible colormap types.  If it's in RGB format,
 * the map is made up of three byte arrays (red, green, then blue)
 * that are each 1/3 of the colormap length.
 */

#define RNOMAP  0	/* no colormap follows the header */
#define RRGBMAP 1	/* rgb colormap */
#define RRAWMAP 2	/* raw colormap; good luck */

#define RESC 128	/* run-length encoding escape character */

typedef unsigned char	*caddrt;

#define INTENSITY(r,g,b) ((((r) / 256U) * 0.30) + \
		          (((g) / 256U) * 0.59) + \
		          (((b) / 256U) * 0.11))

#define	FISH_BUTTON_SIZE 62
#define	FISH_MSG_SIZE    32

int			fwidth[    64];
int			fheight[   64];
caddrt			fpixels[   64];
caddrt			fmsgpixels[64];
caddrt			nofishmsgpixels;

int			fnums[32] =
	{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
int			cur_number = 0;
int			cur_choice = 0;
int			maxcolors  = 0;
int			maxfish,
			maxselectedfish,
			*cmap;

int			mode,
			rate,
			increment,
			seasickness;

char			clienthome[128];
char			fishhome[  128];
char			fishfile[  128];

unsigned int		pixcolorcnt[   256];
XColor			default_colors[256];
int			intensity[     256];
char			color_match[   256];


Frame			frame;
Panel			control_panel;
Panel			fish_msg_panel;
Panel			fish_panel;
Panel_item		rate_slider;
Panel_item		increment_slider;
Panel_item		mode_choice;
Panel_item		seasickness_choice;
Panel_item		fish_message[32];
Panel_item		fish_choices;
Server_image		fish_image[64];
Server_image		fish_msg_image[64];
Server_image		nofish_msg_image;


static void
mode_notify(Panel_item panel_item, int choice, Event *event) {
    mode = choice;
}


static void
seasickness_notify(Panel_item panel_item, int choice, Event *event) {
    seasickness = choice;
}


static void
slider_notify(Panel_item panel_item, int value, Event *event) {
    switch(xv_get(panel_item, PANEL_CLIENT_DATA)) {
	case 0:
	    rate = value;
	break;
	
	case 1:
	    increment = value;
	break;
    }
}


static void
fish_notify(Panel_item panel_item, int choice, Event *event) {
    cur_choice = choice;
    if (cur_number < 32) {
        fnums[cur_number] = cur_choice;
        xv_set(fish_message[cur_number++],
		PANEL_LABEL_IMAGE, fish_msg_image[cur_choice], NULL);
    	maxselectedfish++;
    }
}

static void
apply_props_proc(Panel_item panel_item, Event *event) {
    int         result;
    Event       ie;
    FILE       *fp;
    int		i,
    		j;

    if (panel_item) {
	result = notice_prompt(frame, &ie,
			       NOTICE_MESSAGE_STRINGS,
			       "Applying your changes will modify",
			       "your ~/.fishrc file and the old",
			       "file will be lost.  Do you",
			       "want to do this?",
			       NULL,
			       NOTICE_BUTTON_YES, "Yes",
			       NOTICE_BUTTON_NO, "No",
			       NULL);

	if (result == NOTICE_NO) {
	    xv_set(panel_item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
	    return;
	}
    }
    if (panel_item) {
        if (fp = fopen(clienthome, "w")) {
	    fprintf(fp,"%i\n",(mode | (seasickness << 2)));	
	    fprintf(fp,"%i\n",rate);	
	    fprintf(fp,"%i\n",increment);	
	    i = 0;
	    while (i < maxselectedfish) { 
		fprintf(fp,"%i\n",fnums[i]);
		i++;
	    }
	    fclose(fp);
        } else {
	    result = notice_prompt(frame, &ie,
			       NOTICE_MESSAGE_STRINGS,
			       "Unable to create or write to",
			       "your ~/.fishrc file, all changes",
			       "will be lost.",
			       NULL,
			       NOTICE_BUTTON, "Continue", 1,
			       NULL);
	    return;
        }
	xv_set(panel_item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
    }

}

static void
delete_props_proc(panel_item, event)
    Panel_item	panel_item;
    Event	*event;
{
    if (cur_number > 0) cur_number--;
    if (maxselectedfish > 0) maxselectedfish--;
    xv_set(fish_message[cur_number],PANEL_LABEL_IMAGE, nofish_msg_image, NULL);
}

static void
reset_props_proc(panel_item, event)
    Panel_item	panel_item;
    Event	*event;
{
    int i;

    for (i = 0; i < 32; i++) {
	fnums[i] = 0;
        xv_set(fish_message[i], PANEL_LABEL_IMAGE, nofish_msg_image, NULL);
    }
    cur_number = 0;
    maxselectedfish = 0;
    seasickness = 0;
    xv_set(seasickness_choice, PANEL_VALUE, mode);
    mode = 0;
    xv_set(mode_choice, PANEL_VALUE, mode);
    rate = 100;
    xv_set(rate_slider, PANEL_VALUE, rate);
    increment = 100;
    xv_set(increment_slider, PANEL_VALUE, increment);

}

add_buttons()
{
    Font_string_dims apply_size;
    Font        font;
    int         i;
    Font_string_dims delete_size;
    Font_string_dims reset_size;
    int         width;

    font = xv_get(fish_panel, WIN_FONT);
    xv_get(font, FONT_STRING_DIMS, "Apply", &apply_size);
    xv_get(font, FONT_STRING_DIMS, "Delete", &delete_size);
    xv_get(font, FONT_STRING_DIMS, "Reset", &reset_size);
    width = (int) xv_get(fish_msg_panel, XV_WIDTH);

    xv_create(fish_panel, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Apply",
		  PANEL_NOTIFY_PROC, apply_props_proc,
		  PANEL_NEXT_ROW, -1,
		  XV_X, (width / 4) - (apply_size.width / 2),
		  NULL);

    xv_create(fish_panel, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Delete",
		  PANEL_NOTIFY_PROC, delete_props_proc,
		  XV_X, (2 * width) / 4 - (delete_size.width / 2),
		  NULL);

    xv_create(fish_panel, PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Reset",
		  PANEL_NOTIFY_PROC, reset_props_proc,
		  XV_X, (3 * width) / 4 - (reset_size.width / 2),
		  NULL);

    window_fit_height(fish_panel);
}

main(argc,argv)
int argc;
char *argv[];
{
  	FILE *fp;
	Xv_cmsdata	cms_data;
	unsigned char	red[256], green[256], blue[256];
	Xv_Singlecolor	fg,bg;
	int	i, j;
	char	*fhp;

    	maxfish = 40;

	bg.red = 0, bg.green = 0, bg.blue = 0;
	fg.red = 255, fg.green = 255, fg.blue = 255;

	clienthome[0] = 0;
	fishhome[0] = 0;

	strcpy(clienthome, getenv("HOME"));

	strcat(clienthome,"/.fishrc");

	fhp = (char *)getenv("FISHHOME");

	if (fhp) strcpy(fishhome, fhp);
	else {
	   fhp = (char *)getenv("OPENWINHOME");
	   fishhome[0] = 0;
	   strcpy(fishhome, fhp);
	   strcat(fishhome, "/share/images/fish");
	   fishfile[0] = 0;
	   strcat(fishfile, fishhome);
	   strcat(fishfile, "/b2rot.im8");
	   if (!(fp = fopen(fishfile, "r"))) {
	       fishhome[0] = 0;
	       strcpy(fishhome, "./fish");
	   } else fclose(fp);
	}

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

	init_cms_data(&cms_data, red, green, blue);

	frame =	xv_create(NULL, FRAME_CMD,
		FRAME_LABEL,	argv[0],
		FRAME_INHERIT_COLORS, TRUE,
		FRAME_FOREGROUND_COLOR, &fg,
		FRAME_BACKGROUND_COLOR, &bg,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		XV_LABEL, "Fish Properties",	  
		WIN_COLUMNS, 86,
		WIN_CMS_NAME,	"palette",
		WIN_CMS_DATA,	&cms_data,
		NULL);

	for (i = 0; i < maxfish; i++) {
	    fish_image[i] =	xv_create(XV_NULL, SERVER_IMAGE,
				SERVER_IMAGE_COLORMAP,	"palette",
				XV_WIDTH,		FISH_BUTTON_SIZE,
				XV_HEIGHT,		FISH_BUTTON_SIZE,
				SERVER_IMAGE_DEPTH,	8,
				SERVER_IMAGE_BITS,	fpixels[i],
				NULL);

	    if (!fish_image[i]) {
		printf("\nNot enough memory.");
		exit(1);
	    }

	    free(fpixels[i]);

	    fish_msg_image[i] =	xv_create(XV_NULL, SERVER_IMAGE,
				SERVER_IMAGE_COLORMAP,	"palette",
				XV_WIDTH,		FISH_MSG_SIZE,
				XV_HEIGHT,		FISH_MSG_SIZE,
				SERVER_IMAGE_DEPTH,	8,
				SERVER_IMAGE_BITS,	fmsgpixels[i],
				NULL);

	    if (!fish_msg_image[i]) {
		printf("\nNot enough memory.");
		exit(1);
	    }
	
	    free(fmsgpixels[i]);

	}

	nofish_msg_image =	xv_create(XV_NULL, SERVER_IMAGE,
				SERVER_IMAGE_COLORMAP,	"palette",
				XV_WIDTH,		FISH_MSG_SIZE,
				XV_HEIGHT,		FISH_MSG_SIZE,
				SERVER_IMAGE_DEPTH,	8,
				SERVER_IMAGE_BITS,	nofishmsgpixels,
				NULL);

	if (!nofish_msg_image) {
	    printf("\nNot enough memory.");
	    exit(1);
	}


	control_panel = 	xv_get(frame, FRAME_CMD_PANEL);

	mode_choice =		xv_create(control_panel, PANEL_CHOICE,
				XV_SHOW, TRUE,
				PANEL_LABEL_STRING, "Display Mode:",
				PANEL_CHOICE_STRINGS, "Single Buffered",
						      "Double Buffered",
						      "GX Double Buffered",
						      NULL,
				PANEL_NEXT_ROW, -1,
				PANEL_CHOICE_NCOLS, 3,
				PANEL_VALUE, 0,
				PANEL_NOTIFY_PROC, mode_notify,
				NULL);


	seasickness_choice =	xv_create(control_panel, PANEL_CHOICE,
				XV_SHOW, TRUE,
				PANEL_LABEL_STRING, "Sea Sickness:",
				PANEL_CHOICE_STRINGS, "Off",
						      "On",
						      NULL,
				PANEL_NEXT_ROW, -1,
				PANEL_VALUE, 0,
				PANEL_NOTIFY_PROC, seasickness_notify,
				NULL);


	rate_slider = 		xv_create(control_panel, PANEL_SLIDER,
				XV_SHOW, TRUE,
				PANEL_LABEL_STRING,
					"Time Between Updates:",
				PANEL_NEXT_ROW, -1,
				PANEL_CLIENT_DATA, 0,
				PANEL_SHOW_RANGE, TRUE,
				PANEL_SHOW_VALUE, TRUE,
				PANEL_VALUE, 100,
				PANEL_MIN_VALUE, 0,
				PANEL_MAX_VALUE, 100,
				PANEL_SLIDER_WIDTH, 100,
				PANEL_NOTIFY_PROC, slider_notify,
				NULL);

	increment_slider = 	xv_create(control_panel, PANEL_SLIDER,
				XV_SHOW, TRUE,
				PANEL_LABEL_STRING,
					"Maximum Update Increment:",
				PANEL_NEXT_ROW, -1,
				PANEL_CLIENT_DATA, 1,
				PANEL_SHOW_RANGE, TRUE,
				PANEL_SHOW_VALUE, TRUE,
				PANEL_VALUE, 100,
				PANEL_MIN_VALUE, 0,
				PANEL_MAX_VALUE, 100,
				PANEL_SLIDER_WIDTH, 100,
				PANEL_NOTIFY_PROC, slider_notify,
				NULL);

	window_fit_height(control_panel);

	fish_msg_panel = 	xv_create(frame, PANEL,
				XV_SHOW, TRUE,
				NULL);


	for (i = 0; i < 32; i++) {
	    fish_message[i] = 	xv_create(fish_msg_panel, PANEL_MESSAGE,
				PANEL_LABEL_IMAGE, nofish_msg_image,
				XV_SHOW, TRUE,
				NULL);
	}

	window_fit_height(fish_msg_panel);

	fish_panel = 		xv_create(frame, PANEL,
				XV_SHOW, TRUE,
				NULL);

	fish_choices = 		xv_create(fish_panel, PANEL_CHOICE,
				XV_SHOW, TRUE,
				PANEL_NEXT_ROW, -1,
				PANEL_CHOICE_NCOLS, 9,
				PANEL_NOTIFY_LEVEL, PANEL_ALL,
				PANEL_NOTIFY_PROC, fish_notify,
				NULL);

	for (i = 0; i < maxfish; i++) {
	    xv_set(fish_choices,
		   	PANEL_CHOICE_IMAGE, i, fish_image[i],
			NULL);
	}

	window_fit_height(fish_panel);

	add_buttons();

	cur_number = 0;
	
	maxselectedfish = 0;

	seasickness = 0;

	mode = 0;
	
	rate = 100;

	increment = 100;

        if (fp = fopen(clienthome, "r")) {
	    fscanf(fp,"%i",&mode);
	    seasickness = (mode & 0xC) >> 2;
	    mode &= 0x3;
	    fscanf(fp,"%i",&rate);
	    fscanf(fp,"%i",&increment);
	    i = -1;
	    while ((++i < 32) && (fscanf(fp,"%i",&j) != EOF)) {
	        if (j < maxfish) fnums[i] = j;
		else fnums[i] = 0;
        	xv_set(fish_message[cur_number++],
		    PANEL_LABEL_IMAGE, fish_msg_image[fnums[i]], NULL);
		maxselectedfish++;
	    }
	    xv_set(seasickness_choice, PANEL_VALUE, seasickness);
	    xv_set(mode_choice, PANEL_VALUE, mode);
	    xv_set(rate_slider, PANEL_VALUE, rate);
	    xv_set(increment_slider, PANEL_VALUE, increment);
	    xv_set(fish_choices, PANEL_VALUE, fnums[maxselectedfish - 1]);
	    fclose(fp);
        }

	window_fit(frame);

	xv_main_loop(frame);


}


char *sr_load(fp)
FILE	*fp;
{
register int	i;
register char *p, *pixels;
unsigned long fi_length;


	for (i = 0; i < 32; i++)
	    if ((sr_header.unf[i] = getc(fp)) == EOF)
		return(0);

	if (ntohl(sr_header.fi.magic) != RMAGICNUMBER) {
	    printf("\nBad magic number in raster image file.");
	    return(0);
	}

	if (ntohl(sr_header.fi.type) != RSTANDARD) {
	    printf("\nUnsupported raster image file type.");
	    return(0);
	}

	if (ntohl(sr_header.fi.maptype) != RRGBMAP) {
	    printf("\nUnsupported raster image file colormap type.");
	    return(0);
	}

	if (ntohl(sr_header.fi.maplen )== 0) {
	    printf("\nRaster image file colormap is empty.");
	    return(0);
	}

	sr_cmap.length = ntohl(sr_header.fi.maplen) / 3;

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.red[i] = getc(fp);

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.green[i] = getc(fp);

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.blue[i] = getc(fp);
	
	fi_length = ntohl(sr_header.fi.length);
	if ((pixels = malloc(fi_length)) == NULL) {
	    printf("\nNot enough memory to load raster image file.");
	    return(0);
	}	    	

	p = pixels;

	for (i = 0; i < fi_length; i++)
	    *p++ = getc(fp);

	return(pixels);
}


/* Compare two colors with a fuzz factor */
static int
clrdiff(color1, color2, fuzz)
  XColor *color1, *color2;
  int fuzz;
{
    if ((abs(color1->red - color2->red) < fuzz)
	  && (abs(color1->green - color2->green) < fuzz)
	  && (abs(color1->blue - color2->blue) < fuzz))
	return 0;		      /* Nearly the same color */
    else
	return 1;		      /* Different */
}


/* Searches default color map for closely matching color */
static int
clrclose(color)
  XColor *color;
{
  register int i, j, color_intensity, matchcount = 0;

    color_intensity = INTENSITY(color->red,color->green,color->blue);

    for (i = 0; i < 256; i++)
	if ((abs(color->red - default_colors[i].red) > 1*256) ||
	    (abs(color->green - default_colors[i].green) > 0) ||
	    (abs(color->blue - default_colors[i].blue) > 3*256))
	    color_match[i] = 0;
	else {
	    color_match[i] = 1;
	    j = i;
	    matchcount++;
	}
	
    if (matchcount) {
	    i = 0;
	    while ((matchcount > 0) && (i < 256)) {
		if (color_match[i]) {
		    matchcount--;
		    if (abs(color_intensity - intensity[i]) <
			abs(color_intensity - intensity[j]))
			    j = i;
		}
		i++; 
	    }
	    color->pixel = default_colors[j].pixel;
	    color->red = default_colors[j].red;
	    color->green = default_colors[j].green;
	    color->blue = default_colors[j].blue;
	    return 1;
    }
    return 0;
}


init_cms_data(cms_data, red, green, blue)
Xv_cmsdata	*cms_data;
unsigned char	*red, *green, *blue;
{
FILE	*fp;
register int x, y;
unsigned char *pixel;
char	*pixels;
register caddrt p,
		q;
register int 	i,
                j,
		k;
int		yprefill, ypostfill, xprefill, xpostfill;
int		linebytes, screen, black, color_intensity;
XColor		black_color, hdef;
Display		*display;
Colormap    	colormap;

    for (i = 0; i < 256; i++) pixcolorcnt[i] = 0;

    for (i = 0; i < 32; i++) fpixels[i] = NULL;

    for (i = 0; i < 32; i++) fmsgpixels[i] = NULL;

    if ((display = XOpenDisplay("")) == 0) {
	printf("\nCan't open display.");
	exit(1);
    }

    screen = DefaultScreen(display);

    black = BlackPixel(display, screen);

    black_color.red = 0;
    black_color.green = 0;
    black_color.blue = 0;

    for (i = 0; i < 256; i++)
	default_colors[i].pixel = i;

    colormap = XDefaultColormap(display, screen);

    if (colormap != NULL)
	XQueryColors(display, colormap, default_colors, 256);

    for (i = 0; i < 256; i++) {
	intensity[i] = INTENSITY(default_colors[i].red,
		       	         default_colors[i].green,
		       		 default_colors[i].blue);

	red[i] = default_colors[i].red / 256;
	green[i] = default_colors[i].green / 256;
	blue[i] = default_colors[i].blue / 256;
    }

    cmap = (int *)malloc(256 * sizeof(int));

    for (i = 0; i < 256; i++)
	cmap[i] = i;

    if (!(nofishmsgpixels =
	(caddrt) malloc(FISH_MSG_SIZE*FISH_MSG_SIZE))) {
	printf("\nNot enough memory.");
	exit(1);
    }

    for (k = 0; k < maxfish + 1; k++) {

	fishfile[0] = 0;
	strcat(fishfile, fishhome);

	switch(k) {
	    case 0:
		strcat(fishfile, "/f12_tub.im8");
	    break;

	    case 1:
		strcat(fishfile, "/f8_tub.im8");
	    break;

	    case 2:
		strcat(fishfile, "/f25_tub.im8");
	    break;

	    case 3:
		strcat(fishfile, "/f7_tub.im8");
	    break;

	    case 4:
		strcat(fishfile, "/f15_tub.im8");
	    break;

	    case 5:
		strcat(fishfile, "/f11_tub.im8");
	    break;

	    case 6:
		strcat(fishfile, "/f10_tub.im8");
	    break;

	    case 7:
		strcat(fishfile, "/f21_tub.im8");
	    break;

	    case 8:
		strcat(fishfile, "/f6_tub.im8");
	    break;

	    case 9:
		strcat(fishfile, "/f2_tub.im8");
	    break;

	    case 10:
		strcat(fishfile, "/f16_tub.im8");
	    break;

	    case 11:
		strcat(fishfile, "/f9_tub.im8");
	    break;

	    case 12:
		strcat(fishfile, "/f28_tub.im8");
	    break;

	    case 13:
		strcat(fishfile, "/f17_tub.im8");
	    break;

	    case 14:
		strcat(fishfile, "/f36_tub.im8");
	    break;

	    case 15:
		strcat(fishfile, "/f23_tub.im8");
	    break;

	    case 16:
		strcat(fishfile, "/f24_tub.im8");
	    break;

	    case 17:
		strcat(fishfile, "/f26_tub.im8");
	    break;

	    case 18:
		strcat(fishfile, "/f27_tub.im8");
	    break;

	    case 19:
		strcat(fishfile, "/f29_tub.im8");
	    break;

	    case 20:
		strcat(fishfile, "/f30_tub.im8");
	    break;

	    case 21:
		strcat(fishfile, "/f31_tub.im8");
	    break;

	    case 22:
		strcat(fishfile, "/f32_tub.im8");
	    break;

	    case 23:
		strcat(fishfile, "/f34_tub.im8");
	    break;

	    case 24:
		strcat(fishfile, "/f35_tub.im8");
	    break;

	    case 25:
		strcat(fishfile, "/f37_tub.im8");
	    break;

	    case 26:
		strcat(fishfile, "/f38_tub.im8");
	    break;

	    case 27:
		strcat(fishfile, "/f39_tub.im8");
	    break;

	    case 28:
		strcat(fishfile, "/f40_tub.im8");
	    break;

	    case 29:
		strcat(fishfile, "/f41_tub.im8");
	    break;

	    case 30:
		strcat(fishfile, "/f42_tub.im8");
	    break;

	    case 31:
		strcat(fishfile, "/f43_tub.im8");
	    break;

	    case 32:
		strcat(fishfile, "/f46_tub.im8");
	    break;

	    case 33:
		strcat(fishfile, "/f48_tub.im8");
	    break;

	    case 34:
		strcat(fishfile, "/f49_tub.im8");
	    break;

	    case 35:
		strcat(fishfile, "/f50_tub.im8");
	    break;

	    case 36:
		strcat(fishfile, "/f51_tub.im8");
	    break;

	    case 37:
		strcat(fishfile, "/f52_tub.im8");
	    break;

	    case 38:
		strcat(fishfile, "/f53_tub.im8");
	    break;

	    case 39:
		strcat(fishfile, "/swimmer_tub.im8");
	    break;

     /* this is just so swimmer_tub.im8 doesn't come last, it has a bad cmap */
	    case 40:
		strcat(fishfile, "/f12_tub.im8");
	    break;

	}

	if (!(fp = fopen(fishfile, "r"))) {
	    printf("Couldn't open %s. Try setting FISHHOME.\n", fishfile);
	    exit(1);
	}

	if (!(pixels = sr_load(fp))) {
		printf("Couldn't read raster image file.\n");
		exit(1);
	}

	linebytes = (ntohl(sr_header.fi.width) + 1) & 0xFFFFFFFE;

	fclose(fp);

	if (ntohl(sr_header.fi.depth) != 8)
		printf("Non-depth 8 image.\n");

 	fwidth[k] = ntohl(sr_header.fi.width);
 	fheight[k] = ntohl(sr_header.fi.height);

	if (fwidth[k] > FISH_BUTTON_SIZE) fwidth[k] = FISH_BUTTON_SIZE;
	if (fheight[k] > FISH_BUTTON_SIZE) fheight[k] = FISH_BUTTON_SIZE;

	if (!(fpixels[k] =
	      (caddrt) malloc(FISH_BUTTON_SIZE*FISH_BUTTON_SIZE))) {
	    printf("\nNot enough memory.");
	    exit(1);
	}

	if (!(fmsgpixels[k] =
	      (caddrt) malloc(FISH_MSG_SIZE*FISH_MSG_SIZE))) {
	    printf("\nNot enough memory.");
	    exit(1);
	}

	p = fpixels[k];

	yprefill = (FISH_BUTTON_SIZE - fheight[k]) >> 1;
	ypostfill = FISH_BUTTON_SIZE - fheight[k] - yprefill;
	xprefill = (FISH_BUTTON_SIZE - fwidth[k]) >> 1;
	xpostfill = FISH_BUTTON_SIZE - fwidth[k] - xprefill;

        if (yprefill > 0)
	    for (y = 0; y < yprefill; y++) {
		for (x = 0; x < FISH_BUTTON_SIZE; x++) *p++ = 255;
	    }

        for (y = 0; y < fheight[k]; y++) {
	    if (xprefill > 0)
		for (x = 0; x < xprefill; x++) *p++ = 255;
	    pixel = (unsigned char *) pixels + (y * linebytes);
	    for (x = 0; x < fwidth[k]; x++) {
	        pixcolorcnt[pixel[x]] += 1;
	        *p++ = pixel[x];   
	    }
	    if (xpostfill > 0)
		for (x = 0; x < xpostfill; x++) *p++ = 255;
        }

        if (ypostfill > 0)
	    for (y = 0; y < ypostfill; y++) {
		for (x = 0; x < FISH_BUTTON_SIZE; x++) *p++ = 255;
	    }

	free(pixels);
    }

    for (i = 254; i >= 0; i--) {
	if (pixcolorcnt[i] != 0) {
	    hdef.red = ((unsigned char)sr_cmap.red[i])*256;
	    hdef.green = ((unsigned char)sr_cmap.green[i])*256;
	    hdef.blue = ((unsigned char)sr_cmap.blue[i])*256;
	    if (clrdiff(&hdef,&black_color,85*256)) {
	        if (!clrclose(&hdef)) {
		    if (XAllocColor(display, colormap, &hdef)) {
		        red[hdef.pixel] = sr_cmap.red[i];
		        green[hdef.pixel] = sr_cmap.green[i];
		        blue[hdef.pixel] = sr_cmap.blue[i];
		        cmap[i] = hdef.pixel;
			intensity[i] = INTENSITY(hdef.red,
						 hdef.green,
						 hdef.blue);
		    } else {
			color_intensity = INTENSITY(hdef.red,
				   		    hdef.green,
				   		    hdef.blue);
			j = 255;
			k = 255;
			while (k > 0) {
			    if (abs(color_intensity - intensity[k]) <
				abs(color_intensity - intensity[j]))
				    j = k;
			    k--;
			}
			cmap[i] = default_colors[j].pixel;
		    }
		} else {
		cmap[i] = hdef.pixel;
		intensity[i] = INTENSITY(hdef.red, hdef.green, hdef.blue);
		XAllocColor(display, colormap, &hdef);
		}
	    } else {
	    	cmap[i] = black;
		intensity[i] = 0;
	    }
	}
    }

    cmap[255] = black;

    for (k = 0; k < maxfish; k++) {
	p = fpixels[k];
	j = FISH_BUTTON_SIZE * FISH_BUTTON_SIZE;
	for (i = 0; i < j; i++) {
	    *p = cmap[*p];
	    p++;
	}

    }    

    for (k = 0; k < maxfish; k++) {
	p = fpixels[k];
	q = fmsgpixels[k];
	for (y = 0; y < FISH_BUTTON_SIZE; y += 2) {
	    *q++ = 0;
	    for (x = 0; x < FISH_BUTTON_SIZE; x += 2) {
		*q++ = *p;
		p += 2;
	    }
	    for (x = 0; x < FISH_BUTTON_SIZE; x += 2) {
		p += 2;
	    }
	}	
	    for (x = 0; x < FISH_MSG_SIZE; x++) {
	        *q++ = 0;
	    }
    }    

    j = FISH_MSG_SIZE * FISH_MSG_SIZE;

    p = nofishmsgpixels;

    for (i = 0; i < j; i++) *p++ = 0;

    cms_data->type =	XV_STATIC_CMS;
    cms_data->size =	256;
    cms_data->rgb_count = 256;
    cms_data->index = 	0;
    cms_data->red =	red;
    cms_data->green =	green;
    cms_data->blue =	blue;

}


