#include <stdio.h>
#include <pwd.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <X11/Xlib.h>
#include "ds_popup.h"
#include "base_gui.h"

#define MONO 1
#define COLOR 0
double atof();

extern XImage	*edit_image;
extern Pixmap	preview_pixmap;
extern Display	*dpy;
extern Colormap	cmap;
extern int icon_height, icon_width, preview_boarder_left, preview_boarder_upper;
extern int current_color_state;

static Frame		Print_popup;
static Panel_item 	Printer_item;
static Panel_item 	Options_item;
static Panel_item 	Directory_item;
static Panel_item 	File_item;
static Panel_item 	Destination_item;
static Panel_item 	Width_item;
static Panel_item 	Height_item;
static Panel_item 	Copies_item;
static Panel_item 	Position_left_item;
static Panel_item 	Position_bottom_item;


typedef struct {
		float	image_width;
		float	image_height;
		float	offset_from_left;
		float	offset_from_bottom;
		int	copies;
		int	width_bits;
		int	height_bits;
		int	depth;
		int	color;
		} print_params;

static
put_colorimage_function(file_handle)

FILE	*file_handle;

{
	fprintf(file_handle, "/imageraster {\n");
	fprintf(file_handle, "    dup 1 eq {\n");
	fprintf(file_handle, "	pop pop image\n");
	fprintf(file_handle, "    }{\n");
	fprintf(file_handle, "	/colorimage where {\n");
	fprintf(file_handle, "	    pop colorimage\n");
	fprintf(file_handle, "	}{\n");
	fprintf(file_handle, "	    pop pop pop		%% leaves w h d mat\n");
	fprintf(file_handle, "	    (%%%%[ color image sent to printer with no colorimage operator!\\n) print\n");
	fprintf(file_handle, "	    1 index 8 ne {\n");
	fprintf(file_handle, "		(    Can't recover--flushing file. ]%%%%\n) print\n");
	fprintf(file_handle, "		currentfile flushfile\n");
	fprintf(file_handle, "	    }{  \n");
	fprintf(file_handle, "		(    Converting to grays. ]%%%%\\n) print\n");
	fprintf(file_handle, "\n");
	fprintf(file_handle, "		/imagebuf 3 string def\n");
	fprintf(file_handle, "		/str1 1 string def\n");
	fprintf(file_handle, "		{\n");
	fprintf(file_handle, "		    str1 dup 0\n");
	fprintf(file_handle, "		    currentfile imagebuf readhexstring pop\n");
	fprintf(file_handle, "		    {} forall		%% put bytes on stack\n");
	fprintf(file_handle, "		    .11 mul exch .59 mul add exch .3 mul add\n");
	fprintf(file_handle, "		    round cvi put\n");
	fprintf(file_handle, "		} bind image \n");
	fprintf(file_handle, "	    } ifelse\n");
	fprintf(file_handle, "	} ifelse\n");
	fprintf(file_handle, "    } ifelse\n");
	fprintf(file_handle, "} def\n");
}

static FILE *
open_jobfile_with_header(file, job_def)

char		*file;
print_params	*job_def;

{
	FILE	*file_handle;
	struct passwd	*passwd_ent = getpwuid((uid_t) getuid());

	file_handle = fopen(file, "w");

	if (!file_handle)
		return(NULL);

	/* eps header comments */

	fprintf(file_handle, "%%!PS-Adobe-2.0 EPSF-2.0\n");
	fprintf(file_handle, "%%%%Title: %s\n",file);
	fprintf(file_handle, "%%%%Creator: iconedit (DeskSet Rulz!)\n");
	fprintf(file_handle, "%%%%For: %s\n", 
		(passwd_ent != NULL) ? 
		passwd_ent->pw_gecos : 
		(char *) gettext("NONE"));
	fprintf( file_handle, "%%%%CreationDate: \n\n");
	fprintf( file_handle, "%%%%Pages: 1\n\n");
	

	/* check to see make sure my boss and the PostScript police aren't */
	/* looking, that is, uid 6312 or 7827, and then insert extraneous */
	/* comments if I can get away with it.  (Not yet implemented) */

	fprintf(file_handle, "%%%%BoundingBox ");
	fprintf(file_handle, "%d %d ", 
		(int) job_def->offset_from_left * 72, 
		(int) job_def->offset_from_bottom * 72);
	fprintf(file_handle, "%d %d \n", 
		(int) job_def->image_width * 72, 
		(int) job_def->image_height * 72);

	fprintf( file_handle, "%%%%End Comments\n\n");

	/* begin definition of imageraster operation */

	if (job_def->depth > 1)
	  {
	    put_colorimage_function(file_handle);
	    fprintf(file_handle, "\n/picstr %d string def\n\n", 
		    job_def->width_bits );
	  }
	else
	  fprintf(file_handle, "\n/picstr %d string def\n\n", 
		  job_def->width_bits);

	/* finished defining new imageraster operator */

	/* number of copies */

	fprintf(file_handle, "\n\n/#copies %d def", job_def->copies);

	fprintf(file_handle, "\n%%%%EndProlog");
	fprintf(file_handle, "\n%%%%Page: 1 1");

	/* begin setup for data stream */

	/* translate out the right number of inches */

	fprintf(file_handle, "\n\n%d %d translate\n", 
		(int) job_def->offset_from_left * 72, 
		(int) job_def->offset_from_bottom * 72);

	/* scale up to the right size */

	fprintf(file_handle, "\n\n%d %d scale\n", 
		(int) job_def->image_width * 72, 
		(int) job_def->image_height * 72);

	/* prep for the imageraster operator */

	fprintf(file_handle, "\n\n%d %d %d\n", job_def->width_bits, job_def->height_bits, 8);

	/* transformation matrix */

	fprintf(file_handle, "\n\n[ %d 0 0 -%d 0 %d ]", job_def->width_bits, job_def->height_bits, job_def->width_bits);


	/* get data routine */

	fprintf(file_handle, "\n\n{currentfile picstr readhexstring pop }");

	/* finish prep */

	if (job_def->depth > 1)
		fprintf(file_handle, "\n\nfalse %d\nimageraster\n", job_def->color ? 3 : 1);
	else
		fprintf(file_handle, "\n\nimage\n");

	return(file_handle);
}

static
finish_job(file_handle)

FILE	*file_handle;

{

	fprintf(file_handle, "\n\nshowpage\n");

	fclose(file_handle);
}

static
write_data(file_handle, job_def)

FILE 		*file_handle;
print_params    *job_def;

{
	int	i, j;
	XColor	*color_array;
	extern XColor	white;
	base_window_objects     *base_window;

	edit_image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, 
				icon_width, icon_height, -1, ZPixmap); 

	/* if the image is a color image, shuttle the bytes out 
	   as RGB values for the colorimage operator */

	if (job_def->depth > 1)
	{
		color_array = (XColor *) calloc(job_def->width_bits, sizeof(XColor));
	
	
		for (i = 0; i < job_def->height_bits; i++)
		{
			for (j = 0; j < job_def->width_bits; j++)
				color_array[j].pixel = XGetPixel(edit_image, j, i);
	
			XQueryColors(dpy, cmap, color_array, job_def->width_bits);
	
			for (j = 0; j < job_def->width_bits; j++)
			{
				fprintf(file_handle, "%.2x%.2x%.2x", 
					color_array[j].red >> 8,
					color_array[j].green >> 8,
					color_array[j].blue >> 8);
			}
			fprintf(file_handle, "\n");
		}
	
		free(color_array);
	}
	else
	{
		/* if the image is a monochrome image, shuttle the 
		   bytes out as a monochrome picture. */

      		for (i = 0; i < job_def->height_bits; i++)
		{
			for (j = 0; j < job_def->width_bits; j++)
	  		{

				if (white.pixel != XGetPixel(edit_image, j, i))
	    				fprintf(file_handle,"00");
				else
	    				fprintf(file_handle,"ff");
	  		}
			fprintf(file_handle, "\n");
      		}
	}
}

static
print_to_file(file_name, job_def)
 
char            *file_name;   
print_params    *job_def;

{
	FILE	*file_handle;
	extern int	errno;
	extern char	*sys_errlist[];

	if (!(file_handle = open_jobfile_with_header(file_name, job_def)))
	{
		xv_set(Print_popup, FRAME_LEFT_FOOTER, sys_errlist[errno], 0);
		return(XV_ERROR);
	}

	write_data(file_handle, job_def);
	finish_job(file_handle);
	
	return(NULL);
}


static
print_to_printer(printer_name, printer_options, job_def)

char		*printer_name;
char		*printer_options;
print_params	*job_def;

{
  char	filename[30];
  char	command[256];
  
  strcpy(filename,"/tmp/icprXXXXXX");
  
  mktemp(filename);
  
  if (print_to_file(filename, job_def))
    return(XV_ERROR);
  
#ifdef SVR4
  sprintf(command, "lp -c -d%s %s %s", printer_name,printer_options, filename);
#else /* SVR4 */
  sprintf(command, "lpr -P%s %s %s", printer_name,printer_options, filename);
#endif /* SVR4 */
  
  system(command);
  
  unlink(filename);
}


static Panel_setting
validate_number(item, event)

Panel_item      item;
Event           *event;

{
  char    *v_ptr = (char*)xv_get(item, PANEL_VALUE);
  int     decimal_seen = FALSE;
  int     error = FALSE;
  Panel_setting   status;
  int     input = event_action(event);
  
  while (*v_ptr)
    {
      if (*v_ptr == '.')
	decimal_seen = TRUE;
      
      v_ptr++;
    }

  /* determine if the new string fits within out criteria for
     a floating point number.  if not, put it back to what it
     was before. */


  switch (input) {
    
  case '.':
    if (decimal_seen)
      if (event_is_down(event))
	error = TRUE;
    break;
    
  default:
    if (isascii(input))
      if ( !isdigit(input))
	if ((input != '\n') && (input != '\t') 
	    && (input != '\r') && (input != '\b'))
	  if (event_is_down(event))
	    error = TRUE;
    break;
  }
  
  if (error)
    {
      /* produce some sort of notice.here */
      
      xv_set(Print_popup, FRAME_LEFT_FOOTER,  
	     gettext("You must type a number here") , 0);
      return(PANEL_NONE);
    }
  else
    {
      if (event_is_down(event))
	xv_set(Print_popup, FRAME_LEFT_FOOTER, "", 0);
      return(panel_text_notify(item, event));
    }
}

static
change_popup_state(item, value, event)

        Panel_item      item;
        int             value;
        Event           *event;
{
  if (!value)
    {
      xv_set(Directory_item, XV_SHOW, FALSE, 0);
      xv_set(File_item, XV_SHOW, FALSE, 0);
      xv_set(Printer_item, XV_SHOW, TRUE, 0);
      xv_set(Options_item, XV_SHOW, TRUE, 0);
    }
  else
    {
      xv_set(Printer_item, XV_SHOW, FALSE, 0);
      xv_set(Options_item, XV_SHOW, FALSE, 0);
      xv_set(Directory_item, XV_SHOW, TRUE, 0);
      xv_set(File_item, XV_SHOW, TRUE, 0);
    }
}

static
print_icon(button, event)

Panel_item	button;
Event		*event;

{

  print_params	job_def;
  char		name[MAXPATHLEN];
  char		*printer_name;
  
  xv_set(Print_popup, FRAME_LEFT_FOOTER, "", 0);
  
  job_def.image_width = (float)atof((char *)xv_get(Width_item, PANEL_VALUE));
  job_def.image_height = (float)atof((char *)xv_get(Height_item, PANEL_VALUE));
  
  job_def.offset_from_left = (float)atof((char *)xv_get(Position_left_item, 
							PANEL_VALUE));
  job_def.offset_from_bottom = (float)atof((char *)xv_get(Position_bottom_item,
							  PANEL_VALUE));

  job_def.copies = xv_get(Copies_item, PANEL_VALUE);
  job_def.width_bits = icon_width;
  job_def.height_bits = icon_height;
  if (current_color_state == MONO)
    job_def.depth = 1;
  else
    job_def.depth = 8;
  
  job_def.color = 1;
  
  if (xv_get(Destination_item, PANEL_VALUE))
    {
      sprintf(name, "%s/%s", xv_get(Directory_item, PANEL_VALUE), 
	      xv_get(File_item, PANEL_VALUE));
      if (print_to_file(name, &job_def))
	xv_set(button, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
    }
  else
    {
      printer_name = (char *)xv_get(Printer_item, PANEL_VALUE);
      if (printer_name && *printer_name)
	{
	  if (print_to_printer(printer_name, 
			       xv_get(Options_item, PANEL_VALUE), 
			       &job_def))
	    {
	      xv_set(button, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	    }
	}
      else
	{
	  xv_set(Print_popup, FRAME_LEFT_FOOTER,  
		 gettext("No printer specified"), 0);
	  xv_set(button, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	}
    }
}

static
create_print_popup(base_frame)

Frame	base_frame;

{
  Panel		panel;
  Panel_item	button_item;
  Rect		*rect;
  char 		pathname[MAXPATHLEN];
  char		*printer_var;
  
  if (Print_popup)
    return;
  
  Print_popup = xv_create(base_frame, FRAME_CMD, 
			  FRAME_SHOW_FOOTER, TRUE,
			  FRAME_SHOW_LABEL, TRUE,
#ifdef OW_I18N
			  WIN_USE_IM, TRUE,
#endif
			  XV_LABEL,  gettext("Iconedit: Print") ,
			  0);
  
  panel = xv_get(Print_popup, FRAME_CMD_PANEL, 0);
  
  Destination_item = xv_create(panel, PANEL_CHOICE,
			       PANEL_LABEL_STRING,   gettext("Destination:"),
			       PANEL_CHOICE_STRINGS, 
			       gettext("Printer") , 
			       gettext("File"), 
			       0,
			       PANEL_NOTIFY_PROC, change_popup_state,
			       PANEL_VALUE, 0,
			       XV_Y, xv_row(panel, 0),
			       XV_HELP_DATA, "iconedit:FilePrinterChoice",
			       0);
  
  printer_var = (char *) getenv("PRINTER");
  Printer_item = xv_create(panel, PANEL_TEXT,
			   PANEL_LABEL_STRING,  gettext("Printer:"),
			   PANEL_VALUE, printer_var ? printer_var : "",
			   PANEL_VALUE_DISPLAY_LENGTH, 30,
			   PANEL_VALUE_STORED_LENGTH, 30,
			   XV_Y, xv_row(panel, 1),
			   XV_HELP_DATA, "iconedit:PrinterFillin",
			   0);
  
  Options_item = xv_create(panel, PANEL_TEXT,
			   PANEL_LABEL_STRING,  gettext("Options:"),
			   PANEL_VALUE, "",
			   PANEL_VALUE_DISPLAY_LENGTH, 30,
			   PANEL_VALUE_STORED_LENGTH, 100,
			   XV_Y, xv_row(panel, 2),
			   XV_HELP_DATA, "iconedit:PrintOptsFillin",
			   0);
  
  getcwd(pathname, MAXPATHLEN);
  
  Directory_item = xv_create(panel, PANEL_TEXT,
			     PANEL_LABEL_STRING,  gettext("Directory:"),
			     PANEL_VALUE, pathname,
			     PANEL_VALUE_DISPLAY_LENGTH, 30,
			     PANEL_VALUE_STORED_LENGTH, 100,
			     XV_Y, xv_row(panel, 1),
			     XV_HELP_DATA, "iconedit:PrintDirectoryFillin",
			     0);
  
  
  File_item = xv_create(panel, PANEL_TEXT,
			PANEL_LABEL_STRING,  gettext("File:") ,
			PANEL_VALUE,  gettext("iconedit.ps"),
			PANEL_VALUE_DISPLAY_LENGTH, 30,
			PANEL_VALUE_STORED_LENGTH, 100,
			XV_Y, xv_row(panel, 2),
			XV_HELP_DATA, "iconedit:PrintFileFillin",
			0);
  
  Width_item = xv_create(panel, PANEL_TEXT,
			 PANEL_LABEL_STRING,  gettext("Width:"),
			 PANEL_VALUE_DISPLAY_LENGTH, 6,
			 PANEL_VALUE_STORED_LENGTH, 6,
			 PANEL_VALUE, "1.00",
			 PANEL_NOTIFY_PROC, validate_number,
			 PANEL_NOTIFY_LEVEL, PANEL_ALL,
			 XV_Y, xv_row(panel, 3),
			 XV_HELP_DATA, "iconedit:PrintWidthFillin",
			 0);
  
  Position_left_item = xv_create(panel, PANEL_TEXT,
				 PANEL_LABEL_STRING,  gettext("Position:"),
				 PANEL_VALUE_DISPLAY_LENGTH, 6,
				 PANEL_VALUE_STORED_LENGTH, 6,
				 PANEL_VALUE, "1.00",
				 PANEL_NOTIFY_LEVEL, PANEL_ALL,
				 PANEL_NOTIFY_PROC, validate_number,
				 XV_Y, xv_row(panel, 4),
				 XV_HELP_DATA, "iconedit:PrintPositionFillin",
				 0);
  
  Position_bottom_item = xv_create(panel, PANEL_TEXT,
				   PANEL_VALUE_DISPLAY_LENGTH, 6,
				   PANEL_VALUE_STORED_LENGTH, 6,
				   PANEL_VALUE, "1.00",
				   PANEL_NOTIFY_LEVEL, PANEL_ALL,
				   PANEL_NOTIFY_PROC, validate_number,
				   XV_Y, xv_row(panel, 5),
				   XV_HELP_DATA, 
				   "iconedit:PrintPositionFillin",
				   0);
  
  Copies_item = xv_create(panel, PANEL_NUMERIC_TEXT,
			  PANEL_LABEL_STRING,  gettext("Copies:"),
			  PANEL_VALUE_DISPLAY_LENGTH, 6,
			  PANEL_VALUE_STORED_LENGTH, 6,
			  PANEL_VALUE, 1,
			  PANEL_MIN_VALUE, 1,
			  PANEL_MAX_VALUE, 99,
			  XV_Y, xv_row(panel, 6),
			  XV_HELP_DATA, "iconedit:PrintCopiesFillin",
			  0);
  
  ds_justify_items(panel, TRUE);
  
  
  xv_set(Directory_item,
	 PANEL_VALUE_DISPLAY_LENGTH, 30,
	 0);
  
  xv_set(File_item,
	 PANEL_VALUE_DISPLAY_LENGTH, 30,
	 0);
  
  xv_set(Options_item,
	 PANEL_VALUE_DISPLAY_LENGTH, 30,
	 0);
  
  rect = (Rect *) xv_get(Width_item, PANEL_ITEM_RECT);
  
  Height_item = xv_create(panel, PANEL_TEXT,
			  PANEL_LABEL_STRING,  gettext("Height:"),
			  PANEL_VALUE_DISPLAY_LENGTH, 6,
			  PANEL_VALUE_STORED_LENGTH, 6,
			  PANEL_VALUE, "1.00",
			  PANEL_NOTIFY_LEVEL, PANEL_ALL,
			  PANEL_NOTIFY_PROC, validate_number,
			  XV_Y, xv_row(panel, 3),
			  XV_X, 
			  rect->r_left 
			  + rect->r_width 
			  + xv_get(panel, PANEL_ITEM_X_GAP),
			  XV_HELP_DATA, "iconedit:PrintHeightFillin",
			  0);
  
  rect = (Rect *) xv_get(Position_left_item, PANEL_ITEM_RECT);
  xv_create(panel, PANEL_MESSAGE,
	    PANEL_LABEL_STRING,  gettext("Inches from left"),
	    PANEL_LABEL_BOLD, TRUE,
	    XV_Y, xv_row(panel, 4),
	    XV_X, 
	    rect->r_left + rect->r_width + xv_get(panel, PANEL_ITEM_X_GAP),
	    XV_HELP_DATA, "iconedit:PrintPositionFillin",
	    0);
  
  rect = (Rect *) xv_get(Position_bottom_item, PANEL_ITEM_RECT);
  xv_create(panel, PANEL_MESSAGE,
	    PANEL_LABEL_STRING,  gettext("Inches from bottom"),
	    PANEL_LABEL_BOLD, TRUE,
	    XV_Y, xv_row(panel, 5),
	    XV_X, 
	    rect->r_left + rect->r_width + xv_get(panel, PANEL_ITEM_X_GAP),
	    XV_HELP_DATA, "iconedit:PrintPositionFillin",
	    0);
  

  button_item = xv_create(panel, PANEL_BUTTON,
			  PANEL_LABEL_STRING,  gettext("Print"),
			  PANEL_NOTIFY_PROC, print_icon,
			  XV_Y, xv_row(panel, 7),
			  XV_HELP_DATA, "iconedit:PrintButton",
			  0);
  
  window_fit(panel);
  
  ds_center_items(panel, 7, button_item, 0);
  
  change_popup_state(0, 0, 0);
  
  window_fit(Print_popup);
  
  ds_position_popup(base_frame, Print_popup, DS_POPUP_LOR);
}

show_print_popup(item, op)
     
     Menu_item	item;
     Menu_generate	op;
     
{
  base_window_objects * ip = 
    (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    create_print_popup(ip->window);
    xv_set(Print_popup, 
	   XV_SHOW, TRUE, 
	   0);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}
