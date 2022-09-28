#ifndef lint
static char sccsid[] = "@(#)props.c 1.26 93/07/14";
#endif
/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include "ds_xlib.h"
#include "props.h"
#include "display.h"
#include "imagetool.h"
#include "ui_imagetool.h"


/*
 * To add a property, add the name here.
 */
char *resource_names[] = {
	"ViewImageIn",
	"Colors",
	"DisplayPalette",
	"UseDSC"
};

char *resource_defaults[] = {
         DEFAULT_VIEW_IN,
	 DEFAULT_COLORS,
	 DEFAULT_SHOW_PALETTE,
	 DEFAULT_DSC
};

#define NRESOURCES   sizeof resource_names / sizeof resource_names[0]

ResourceInfo  resources[NRESOURCES];

int  gray_vis1 = -1;
int  color_vis1 = -1;
int  gray_vis4 = -1;
int  color_vis4 = -1;
int  gray_vis8 = -1;
int  color_vis8 = -1;
int  gray_vis16 = -1;
int  color_vis16 = -1;
int  gray_vis24 = -1;
int  color_vis24 = -1;

int   depth1 = 0, depth4 = 0, depth8 = 0, depth16 = 0, depth24 = 0;
int   gray_vis = 0;
int   color_vis = 0;

static XrmDatabase dsdb, rdb;

/*
 * Pointer to property sheet values.
 * Not to be confused with props in propsui.c which 
 * contains the ui objects.
 */
PropInfo     *current_props;


/*
 * Some utilities needed for conversion to and
 * from resource types.
 */
static int
boolean_to_int (value)
    char *value;
{
    if ((strcmp (value, "True") == 0) ||
        (strcmp (value, "TRUE") == 0) ||
	(strcmp (value, "true") == 0))
      return 1;
    else if ((strcmp (value, "False") == 0) ||
	     (strcmp (value, "FALSE") == 0) ||
	     (strcmp (value, "false") == 0))
      return 0;
    else
      return 1;

}

static char *
int_to_boolean (value)
    int value;
{
    if (value == 0)
      return UNDISPLAY_PALETTE;
    else
      return DISPLAY_PALETTE;
}

char *
int_to_view (value)
  int  value;
{
  if (value == 0 && gray_vis == TRUE)
    return VIEW_GRAY_SCALE;
  else
    return VIEW_COLOR;

}

char *
int_to_color (value)
  int  value;
{
  char buf[80];
  int  choice = -1;

  if (depth1)
    choice++;
  if (value == choice)
    return COLORS_BW;

  if (depth4)
    choice++;
  if (value == choice)
    return COLORS_16;

  if (depth8)
    choice++;
  if (value == choice)
    return COLORS_256;

  if (depth16)
    choice++;
  if (value == choice)
    return COLORS_THOUSANDS;

  if (depth24)
    choice++;
  if (value == choice)
    return COLORS_MILLIONS;

  return COLORS_256;

}

static int
view_to_int (value)
   char *value;
{
  if (gray_vis && (strcmp (value, VIEW_COLOR) == 0))
    return 1;
  else if (strcmp (value, VIEW_GRAY_SCALE) == 0)
    return 0;
  else
    return 0;

}

static int
color_to_int (value)
   char *value;
{
  if (strcmp (value, COLORS_BW) == 0)
/* If value is "BW" and 1-bit not supported,
 * find "best value".
 */
    if (depth1 == 0) {
      if (depth24)
	return color_to_int (COLORS_MILLIONS);
      else if (depth16)
	return color_to_int (COLORS_THOUSANDS);
      else if (depth8)
	return color_to_int (COLORS_256);
      else if (depth4)
	return color_to_int (COLORS_16);
    }
    else
      return 0;

  if (strcmp (value, COLORS_16) == 0) {
/* If value is "16" and 4-bit not supported,
 * find "best value".
 */
    if (depth4 == 0) {
      if (depth24)
	return color_to_int (COLORS_MILLIONS);
      else if (depth16)
	return color_to_int (COLORS_THOUSANDS);
      else if (depth8)
	return color_to_int (COLORS_256);
      else if (depth1)
	return color_to_int (COLORS_BW);
    }
    else if (depth1)
      return 1;
    else
      return 0;
  }

  if (strcmp (value, COLORS_256) == 0) {
/* If value is "256" and 8-bit not supported,
 * find "best value".
 */
    if (depth8 == 0) {
      if (depth24)
	return color_to_int (COLORS_MILLIONS);
      else if (depth16)
	return color_to_int (COLORS_THOUSANDS);
      else if (depth8)
	return color_to_int (COLORS_256);
      else if (depth1)
	return color_to_int (COLORS_BW);
    }
    else if (depth1 && depth4)
      return 2;
    else if ((depth1 + depth4) == 1)
      return 1;
    else 
      return 0;
  }

  if (strcmp (value, COLORS_THOUSANDS) == 0) {
/* If value is Thousands and 16-bit not supported,
 * find "best value".
 */
    if (depth16 == 0) {
      if (depth24)
	return color_to_int (COLORS_MILLIONS);
      else if (depth8)
	return color_to_int (COLORS_256);
      else if (depth4)
	return color_to_int (COLORS_16);
      else if (depth1)
	return color_to_int (COLORS_BW);
    }
    else if (depth1 && depth4 && depth8)
      return 3;
    else if ((depth1 + depth4 + depth8) == 2)
      return 2;
    else if ((depth1 + depth4 + depth8) == 1)
      return 1;
    else
      return 0;
  }

  if (strcmp (value, COLORS_MILLIONS) == 0) {
/* If value is Millions and 24-bit not supported,
 * find "next best value".
 */
    if (depth24 == 0) {
      if (depth16)
        return color_to_int (COLORS_THOUSANDS);
      else if (depth8)
	return color_to_int (COLORS_256);
      else if (depth4)
	return color_to_int (COLORS_16);
      else if (depth1)
	return color_to_int (COLORS_BW);
    }
    else if (depth1 && depth4 && depth8 && depth16)
      return 4;
    else if ((depth1 + depth4 + depth8 + depth16) == 3)
      return 3;
    else if ((depth1 + depth4 + depth8 + depth16) == 2)
      return 2;
    else if ((depth1 + depth4 + depth8 + depth16) == 1)
      return 1;
    else
      return 0;
  }
 
  return 0;

}

/*
 * Set the resource into the current_props struct
 * by converting the resource to ASCII and interpreting it.
 */
static void
set_property (resource)
    ResourceInfo  *resource;
{

    if (strcmp (resource->name, resource_names[RES_VIEW_IN]) == 0)
      current_props->view_in = view_to_int (resource->value);
    else if (strcmp (resource->name, resource_names[RES_COLORS]) == 0) 
      current_props->color = color_to_int (resource->value);
    else if (strcmp (resource->name, resource_names[RES_SHOW_PALETTE]) == 0) 
      current_props->show_palette = boolean_to_int (resource->value);
    else if (strcmp (resource->name, resource_names[RES_DSC]) == 0) 
      current_props->use_dsc = boolean_to_int (resource->value);

}

/*
 * read from the resource database and
 * stuff values into current_props and resources.
 */
static int
read_resources (file_name)
    char *file_name;
{

    int    i;
    char  *resource;

    for (i = 0; i < NRESOURCES; i++) {
      resource = ds_get_resource (rdb, "imagetool", resource_names[i]);
      if (resource != NULL) {
        if (resources[i].value != NULL)
          free (resources[i].value);
        resources[i].value = strdup (resource);
	set_property (&resources[i]);
      }
    }

   return 1;

}

/*
 * set props with defaults in case nothing in .desksetdefaults
 * about imagetool.
 */
static PropInfo *
init_props()
{
    PropInfo    *tmp_props = (PropInfo *) calloc (1, sizeof (PropInfo));
    int          i;
    XVisualInfo  vtemplate;
    int 	 num_vis;
    int          class;
    int          screen;
    Display     *dpy;
    XVisualInfo  vinfo;

    for (i = 0; i < NRESOURCES; i++) {
       resources[i].name = strdup (resource_names[i]);
       resources[i].value = strdup (resource_defaults[i]);
    }

    dpy = image_display->xdisplay;
    screen = DefaultScreen (dpy);

    for (class = StaticGray; class <= DirectColor; class++) {

      if (XMatchVisualInfo (dpy, screen, 1, class, &vinfo)) {
	depth1 = TRUE; 
	if ((class < StaticColor) && (class > gray_vis1))
	  gray_vis1 = class;
	else if (class > color_vis1)
          color_vis1 = class;  /* shouldn't happen, 1-bit color */
      }

      if (XMatchVisualInfo (dpy, screen, 4, class, &vinfo)) {
	depth4 = TRUE;
	if (class < StaticColor && class > gray_vis4)
	  gray_vis4 = class;
	else if (class > color_vis4 && color_vis4 != PseudoColor)
          color_vis4 = class;
      }

      if (XMatchVisualInfo (dpy, screen, 8, class, &vinfo)) {
	depth8 = TRUE;
	if (class < StaticColor && class > gray_vis8)
	  gray_vis8 = class;
	else if (class > color_vis8 && color_vis8 != PseudoColor)
          color_vis8 = class;
      }

      if (XMatchVisualInfo (dpy, screen, 16, class, &vinfo)) {
	depth16 = TRUE;
	if (class < StaticColor && class > gray_vis16)
	  gray_vis16 = class;
	else if (class > color_vis16 && color_vis16 != PseudoColor)
          color_vis16 = class;
      }

      if (XMatchVisualInfo (dpy, screen, 24, class, &vinfo)) {
	depth24 = TRUE; 
	if (class < StaticColor && class > gray_vis24)
	  gray_vis24 = class;
	else if (class > color_vis24 && color_vis24 != TrueColor)
          color_vis24 = class;
      }

    }

    if (gray_vis1 != -1 || gray_vis4 != -1 || gray_vis8 != -1 ||
	gray_vis16 != -1 || gray_vis24 != -1)
      gray_vis = TRUE;
    
    if (color_vis1 != -1 || color_vis4 != -1 || color_vis8 != -1 ||
	color_vis16 != -1 || color_vis24 != -1)
      color_vis = TRUE;

    return (tmp_props);

}

static void
set_props_defaults()
{
    int frame_depth = xv_get (base_window->base_window, WIN_DEPTH);
    
    current_props = init_props();
    current_props->view_in = view_to_int (DEFAULT_VIEW_IN);
 
/*
 * Set default props to be the "best" (e.e. egret  24-bit)
 * This will be overridden if defined in .desksetdefaults
 * or on command line.
 */
    if (color_vis24 != -1)
      current_props->color = color_to_int (COLORS_MILLIONS);
    else if (color_vis16 != -1)
      current_props->color = color_to_int (COLORS_THOUSANDS);
    else if (color_vis8 != -1) 
      current_props->color = color_to_int (COLORS_256);
    else if (color_vis4 != -1) 
      current_props->color = color_to_int (COLORS_16);
    else if (color_vis1 != -1 || gray_vis1 != -1)
      current_props->color = color_to_int (COLORS_BW);
    else
      current_props->color = color_to_int (DEFAULT_COLORS);

    current_props->show_palette = boolean_to_int (DEFAULT_SHOW_PALETTE);
    current_props->use_dsc = boolean_to_int (DEFAULT_DSC);
    current_props->props_changed = FALSE;
}

/*
 * Free up resources before re-reading in again.
 * Called from Reset button.
 */
static void
free_resources()
{
    int   i;

    for (i = 0; i < NRESOURCES; i++) {
      if (resources[i].name) {
	free (resources[i].name);
        resources[i].name = NULL;
      }
      if (resources[i].value) {
	free (resources[i].value);
        resources[i].value = NULL;
      }
    }

}

/*
 * Main function to call when first
 * starting up imagetool and read in the imagetool
 * resources, if any.
 */
int
read_props()
{
    char  	 *ds, file_name [MAXPATHLEN];
    struct stat   statbuf;
    int           status, ret_value = FALSE;
    char         *view_value, *color_value;
/*
 * Set defaults in case no imagetool properties
 * are in the resource database.
 */
    set_props_defaults();

/*
 * Determine which resource file to read in: 
 * Either DESKSETDEFAULTS or $HOME/.desksetdefaults.
 */
    if ((ds = getenv ("DESKSETDEFAULTS")) == NULL) {
      sprintf (file_name, "%s/%s", getenv ("HOME"), DS_FILENAME);
      status = stat (file_name, &statbuf);
    }
    else
      status = stat (ds, &statbuf);

/*
 * Read in the resource database and overwrite
 * any imagetool properties with the defaults.
 */
    if (status != -1) {
      dsdb = ds_load_deskset_defs();
      rdb = ds_load_resources (image_display->xdisplay);
      read_resources (file_name);
      ret_value = TRUE;
    }

/*
 * Check if this is a value setting.
 * For example, if no gray visual in 24-bit is available,
 * then can't have view in = Gray && Color == 24-bit.
 */
    view_value = strdup (int_to_view (current_props->view_in));
    color_value = strdup (int_to_color (current_props->color));

    if (strcmp (color_value, COLORS_BW) == 0) {
      if (strcmp (view_value, VIEW_COLOR) == 0 && color_vis1 == -1)
        current_props->view_in = view_to_int (VIEW_GRAY_SCALE);
      else if (strcmp (view_value, VIEW_GRAY_SCALE) == 0 && gray_vis1 == -1)
        current_props->view_in = view_to_int (VIEW_COLOR);
    }
    else if (strcmp (color_value, COLORS_16) == 0) {
      if (strcmp (view_value, VIEW_COLOR) == 0 && color_vis4 == -1)
        current_props->view_in = view_to_int (VIEW_GRAY_SCALE);
      else if (strcmp (view_value, VIEW_GRAY_SCALE) == 0 && gray_vis4 == -1)
        current_props->view_in = view_to_int (VIEW_COLOR);
    }
    else if (strcmp (color_value, COLORS_256) == 0) {
      if (strcmp (view_value, VIEW_COLOR) == 0 && color_vis8 == -1)
        current_props->view_in = view_to_int (VIEW_GRAY_SCALE);
      else if (strcmp (view_value, VIEW_GRAY_SCALE) == 0 && gray_vis8 == -1)
        current_props->view_in = view_to_int (VIEW_COLOR);
    }
    else if (strcmp (color_value, COLORS_THOUSANDS) == 0) {
      if (strcmp (view_value, VIEW_COLOR) == 0 && color_vis16 == -1)
        current_props->view_in = view_to_int (VIEW_GRAY_SCALE);
      else if (strcmp (view_value, VIEW_GRAY_SCALE) == 0 && gray_vis16 == -1)
        current_props->view_in = view_to_int (VIEW_COLOR);
    }
    else if (strcmp (color_value, COLORS_MILLIONS) == 0) {
      if (strcmp (view_value, VIEW_COLOR) == 0 && color_vis24 == -1)
        current_props->view_in = view_to_int (VIEW_GRAY_SCALE);
      else if (strcmp (view_value, VIEW_GRAY_SCALE) == 0 && gray_vis24 == -1)
        current_props->view_in = view_to_int (VIEW_COLOR);
    }
	
    free (view_value);
    free (color_value);     
    
}

/*
 * This call is needed because the read_props() function
 * reads the resource database, but does not set the
 * XView Objects on the props sheet because it might have
 * not been created yet.  This call sets the props
 * sheet according to what is stored in current_props.
 *
 * The xview objects in propsui need to be created before
 * calling this because after reading in the resources
 * it sets the properties popup to display what is
 * currently defined in .desksetdefaults.
 */
int
set_props_sheet()
{
   void   *attr[10];
   void  **attr_ptr;
   int     i = 0;

/*
 * Set Gray Scale and Color for View Image In.
 */
   attr_ptr = attr;
   *attr_ptr++ = (void *) PANEL_CHOICE_STRINGS;

   if (gray_vis)
     *attr_ptr++ = (void *) LGET ("Gray Scale");

   if (color_vis)
     *attr_ptr++ = (void *) LGET ("Color");

   *attr_ptr++ = (void *) NULL;
   *attr_ptr++ = (void *) NULL;
   
   xv_set (props->view, ATTR_LIST, attr, NULL);

/*
 * Set number of colors for Colors choice.
 */
   i = 0;
   attr_ptr = attr;
   *attr_ptr++ = (void *) PANEL_CHOICE_STRINGS;

   if (depth1)
/* I18N_STRING -
   B&W - short for black and white
 */
     *attr_ptr++ = (void *) LGET ("2 (B&W)"); 
   if (depth4)
     *attr_ptr++ = (void *) LGET ("16");
   if (depth8)
     *attr_ptr++ = (void *) LGET ("256");
   if (depth16)
     *attr_ptr++ = (void *) DGET ("65536");
   if (depth24)
     *attr_ptr++ = (void *) LGET ("Millions");

   *attr_ptr++ = (void *) NULL;
   *attr_ptr++ = (void *) NULL;

   xv_set (props->colors, ATTR_LIST, attr, NULL);
   props_size_panel (props);
   
   xv_set (props->view, PANEL_VALUE, current_props->view_in, NULL);
   xv_set (props->colors, PANEL_VALUE, current_props->color, NULL);
   xv_set (props->open_palette, PANEL_VALUE, current_props->show_palette, NULL);

}

/*
 * Callback for props Apply.
 */
void
apply_notify_proc( item, event)
    Panel_item item;
    Event *event;
{
   int   value, status;
   char *new_value;

   value = xv_get (props->view, PANEL_VALUE);
   current_props->view_in = value;
   new_value = strdup (int_to_view (value));
   free (resources [RES_VIEW_IN].value);
   resources [RES_VIEW_IN].value = new_value;
   ds_put_resource (&dsdb, "Imagetool", resources [RES_VIEW_IN].name,
	             resources [RES_VIEW_IN].value);
   
   value = xv_get (props->colors, PANEL_VALUE);
   current_props->color = value;
   new_value = strdup (int_to_color (value));
   free (resources [RES_COLORS].value);
   resources [RES_COLORS].value = new_value;
   ds_put_resource (&dsdb, "Imagetool", resources [RES_COLORS].name,
	             resources [RES_COLORS].value);

   value = xv_get (props->open_palette, PANEL_VALUE);
   current_props->show_palette = value;
   new_value = strdup (int_to_boolean (value));
   free (resources [RES_SHOW_PALETTE].value);
   resources [RES_SHOW_PALETTE].value = new_value;
   ds_put_resource (&dsdb, "Imagetool", resources [RES_SHOW_PALETTE].name,
         	     resources [RES_SHOW_PALETTE].value);

   status = ds_save_resources (dsdb);
   if (status != XV_OK)
     display_error (base_window->base_window, 
		    EGET ("Cannot save property changes."));

  current_props->props_changed = FALSE;

}

/*
 * Callback for props Reset.
 */
void
reset_notify_proc( item, event)
    Panel_item item;
    Event *event;
{

/*
 * Free up current property entries.
 */
   free_resources ();

/*
 * Read in the properties from scratch.
 */
   free (current_props);
   read_props ();

/*
 * Set items on props sheet accordingly.
 */
   xv_set (props->view, PANEL_VALUE, current_props->view_in, NULL);
   xv_set (props->colors, PANEL_VALUE, current_props->color, NULL);
   xv_set (props->open_palette, PANEL_VALUE, current_props->show_palette, NULL);

   current_props->props_changed = FALSE;

}

/*
 * Called then any of the objects on
 * the props sheet has been modified.
 * Set flag to changed so we know whether to put
 * a notice up when dismissing without applying.
 */
set_dirty_flag()
{
    current_props->props_changed = TRUE;
}


void
props_notify (item, value, event)
    Panel_item    item;
    int           value;
    Event        *event;
{
    set_dirty_flag();
}

void
props_view_notify (item, value, event)
    Panel_item    item;
    int           value;
    Event        *event;
{
    char  *view_value = NULL;
    char  *color_value = NULL;

    view_value = strdup (int_to_view (xv_get (props->view, PANEL_VALUE)));
    color_value = strdup (int_to_color (xv_get (props->colors, PANEL_VALUE)));
/*
 * If attempting to select GrayScale...
 */
    if (strcmp (view_value, VIEW_GRAY_SCALE) == 0) {

      /* Check if gray visual for
         currently selected colors */

      if ((strcmp (color_value, COLORS_MILLIONS) == 0 && gray_vis24 == -1) ||
	  (strcmp (color_value, COLORS_THOUSANDS) == 0 && gray_vis16 == -1) ||
	  (strcmp (color_value, COLORS_256) == 0 && gray_vis8 == -1) ||
	  (strcmp (color_value, COLORS_16) == 0 && gray_vis4 == -1) ||
	  (strcmp (color_value, COLORS_BW) == 0 && gray_vis1 == -1)) {
	/* Automatically select the "greatest" gray visual */
	if (gray_vis24 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_MILLIONS), NULL);
	else if (gray_vis16 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_THOUSANDS), NULL);
	else if (gray_vis8 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_256), NULL);
	else if (gray_vis4 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_16), NULL);
	else if (gray_vis1 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_BW), NULL);
      }

    }
/*
 * Attempting to select Color... 
 */
    else if (strcmp (view_value, VIEW_COLOR) == 0) {
      if ((strcmp (color_value, COLORS_MILLIONS) == 0 && color_vis24 == -1) ||
	  (strcmp (color_value, COLORS_THOUSANDS) == 0 && color_vis16 == -1) ||
	  (strcmp (color_value, COLORS_256) == 0 && color_vis8 == -1) ||
	  (strcmp (color_value, COLORS_16) == 0 && color_vis4 == -1) ||
	  (strcmp (color_value, COLORS_BW) == 0 && color_vis1 == -1)) {
	/* Automatically select the "greatest" color visual */
	if (color_vis24 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_MILLIONS), NULL);
	else if (color_vis16 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_THOUSANDS), NULL);
	else if (color_vis8 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_256), NULL);
	else if (color_vis4 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_16), NULL);
	else if (color_vis1 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_BW), NULL);
      }
    }

    if (view_value)
      free (view_value);

    if (color_value)
      free (color_value);

   set_dirty_flag();
}

void
props_colors_notify (item, value, event)
    Panel_item    item;
    int           value;
    Event        *event;
{
    char  *color_value = NULL;
    char  *view_value = NULL;

    color_value = strdup (int_to_color (xv_get (props->colors, PANEL_VALUE)));
    view_value = strdup (int_to_view (xv_get (props->view, PANEL_VALUE)));

    if (strcmp (view_value, VIEW_GRAY_SCALE) == 0) {
      if ((strcmp (color_value, COLORS_MILLIONS) == 0 && gray_vis24 == -1) ||
          (strcmp (color_value, COLORS_THOUSANDS) == 0 && gray_vis16 == -1) ||
          (strcmp (color_value, COLORS_256) == 0 && gray_vis8 == -1) ||
          (strcmp (color_value, COLORS_16) == 0 && gray_vis4 == -1) ||
          (strcmp (color_value, COLORS_BW) == 0 && gray_vis1 == -1)) {
	display_error (props->props, 
		       EGET ("Invalid Colors set for Gray Scale viewing."));
	/* Automatically select the "greatest" gray visual */
	if (gray_vis24 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_MILLIONS), NULL);
	else if (gray_vis16 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_THOUSANDS), NULL);
	else if (gray_vis8 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_256), NULL);
	else if (gray_vis4 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_16), NULL);
	else if (gray_vis1 != -1)
	  xv_set (props->colors, 
		  PANEL_VALUE, color_to_int (COLORS_BW), NULL);
      }
    }
    else {
      if ((strcmp (color_value, COLORS_MILLIONS) == 0 && color_vis24 == -1) ||
          (strcmp (color_value, COLORS_THOUSANDS) == 0 && color_vis16 == -1) ||
          (strcmp (color_value, COLORS_256) == 0 && color_vis8 == -1) ||
          (strcmp (color_value, COLORS_16) == 0 && color_vis4 == -1) ||
          (strcmp (color_value, COLORS_BW) == 0 && color_vis1 == -1)) {
	display_error (props->props,
		       EGET ("Invalid Colors set for Color viewing."));
	/* Automatically select the "greatest" color visual */
	if (color_vis24 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_MILLIONS), NULL);
	else if (color_vis16 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_THOUSANDS), NULL);
	else if (color_vis8 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_256), NULL);
	else if (color_vis4 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_16), NULL);
	else if (color_vis1 != -1)
	  xv_set (props->colors,
		  PANEL_VALUE, color_to_int (COLORS_BW), NULL);
      }
    }

    if (color_value)
      free (color_value);

    if (view_value)
      free (view_value);

   set_dirty_flag();
}

/*
 * Put up notice if props has been modified.
 */
void
props_done_proc (frame)
    Frame  frame;
{
    int  notice_val;

    if (!current_props->props_changed) {
      xv_set (frame, XV_SHOW, FALSE, NULL);
      return;
    }

    notice_val = notice_prompt (frame, NULL,
				NOTICE_MESSAGE_STRINGS,
				  EGET ("You have made changes to Properties\nsettings that have not been applied.\nDo you still want to dismiss this window?"),
				  NULL,
				NOTICE_BUTTON_YES, LGET ("Dismiss"),
				NOTICE_BUTTON_NO, LGET ("Cancel"),
				NULL);
    switch (notice_val) {  
    case NOTICE_YES:
      xv_set (frame, FRAME_CMD_PUSHPIN_IN, FALSE,
	             XV_SHOW, FALSE, NULL);
      current_props->props_changed = FALSE;
      break;

    case NOTICE_NO:
      xv_set (frame, FRAME_CMD_PUSHPIN_IN, TRUE, 	
	             XV_SHOW, TRUE, NULL);
      break;

    case NOTICE_FAILED:
      break;
    }

}
