/*
 * text_stubs.c - Notify and event callback function stubs.
 * This file was generated by `gxv' from `./text.G'.
 * DO NOT EDIT BY HAND.
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/notice.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <xview/xv_i18n.h>
#include "gdd.h"
#include "text_gui.h"
#include "base_gui.h"
#include "base.h"

#define MAX_UNDOS 8

#define MAX_FOOTER_LENGTH 64


extern int atoi();

char pattern[] = "-*-*-*-*-*-*-*-*-*-*-*-*-*-*";

typedef struct font_node {
  char *x_font_name;
  char *full_x_font_name;
  char *foundry;
  char *family;
  char *style;
  char *weight;
  char *points;
} Font_node;

typedef struct point_node {
  char *point;
  char *x_font_name;
  char *full_x_font_name;
  int  warning_issued;
} Point_node;

typedef struct style_node {
  char *style;
  Point_node *point_info;
  int point_count;
} Style_node;

typedef struct weight_node {
  char *weight;
  Style_node *style_info;
  int style_count;
} Weight_node;

typedef struct family_node {
  char *name;
  Weight_node *weight_info;
  int weight_count;
} Family_node;


extern  text_popup_objects	*text_popup;

extern int boarder_upper, boarder_lower, boarder_left, boarder_right, height_block, width_block;
extern int icon_height, icon_width, preview_boarder_upper, preview_boarder_left;
extern XColor black, current_pen;
extern int current_undo, undos_allowed, redos_allowed;
extern int current_color_state;
extern int redo_possible;
extern Xv_server server;

Undo_struct undo_images[MAX_UNDOS];

extern Drawable		big_bit_xid;

static int	wiping_on = 0;
static int	selection_top;
static int	selection_left;
static int	selection_bottom;
static int	selection_right;

static short int ascent_string;
static short int descent_string;
static short int lbearing_string;
static short int rbearing_string;

static Family_node 	*tree_base;
static int		num_families;

static char     text_to_be_rendered[128];


extern Display *dpy;

extern GC gc, gc_rv, redraw_gc, fill_gc;
extern GC     		three_gc;    /* graphics context for most Xlib graphics */
extern Window preview_xid;
extern Pixmap preview_pixmap, big_pixmap;

extern void t_draw_text();

static int text_initialized = FALSE;

XFontSet text_font = NULL;

static Panel_item font_family_setting;
static Panel_item font_weight_setting;
static Panel_item font_style_setting;
static Panel_item font_size_setting;

char *font_old_weight;
char *font_old_style;
char *font_old_size;

static int setting_font = FALSE;

static char *last_font = NULL;

x_error_proc(dpy, event)

Display	*dpy;
XErrorEvent *event;

{
  if (setting_font && 
     ((event->request_code == X_OpenFont) && (event->error_code == BadName)))
  {
    text_font = NULL;
    return(XV_OK);
  }
  else
    return(XV_ERROR);
}

void
text_draw_init()
{
  if (!text_initialized)
    {
      init_fonts();
      text_initialized = TRUE;
  }
}


/*
 * Get the test to be printed.
 */
char *
get_text()
{
        char    *text =  (char *)xv_get(text_popup->text, PANEL_VALUE);

        return *text ? text : NULL;
}

void 
get_string_bounding_box( string, overall)
char string[];
XRectangle *overall;
{
  XFontStruct *font_struct;
  XRectangle	dummy;

  XmbTextExtents(text_font, string, strlen(string), overall, &dummy);
}


text_interpose_proc(canvas, event, arg, type)

Canvas                  canvas;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
  char *temp_text;

  static int text_pixel_width, text_pixel_height;

  static XRectangle string_geometry;

  if (event_action(event) == ACTION_SELECT)
    {
      
      if (event_is_up(event) && wiping_on)
	{
	  XDrawRectangle(dpy, big_bit_xid, three_gc, 
			 selection_left, selection_top, 
			 selection_right - selection_left, 
			 selection_bottom - selection_top); 
	  t_draw_text(selection_left, selection_bottom);
	  wiping_on = FALSE;
	}
      else
	{
	  
	  selection_bottom = (((event_y(event)-boarder_left)/width_block) 
			      * width_block) + boarder_left;
	  selection_left = (((event_x(event)-boarder_upper)/height_block)
			    * height_block + boarder_upper );
	  
	  temp_text = get_text();
	  if (temp_text)
	    {
	      strcpy(text_to_be_rendered, temp_text);
	      get_string_bounding_box( text_to_be_rendered, &string_geometry);
	      
	      text_pixel_width = string_geometry.width * width_block;
	      selection_right = selection_left + text_pixel_width; 
	      text_pixel_height = string_geometry.height * width_block;
	      selection_top = selection_bottom - text_pixel_height ;
	      
	      XDrawRectangle(dpy, big_bit_xid, three_gc, selection_left, selection_top, 
			     selection_right - selection_left, selection_bottom - selection_top);
	      wiping_on = TRUE;
	      
	    }
	  else
	    wiping_on = FALSE;
	}
      return(NOTIFY_DONE);
    }
  else if (event_id(event) == LOC_DRAG)
    {
      if (wiping_on)
	{
	  XDrawRectangle(dpy, big_bit_xid, three_gc, selection_left, selection_top, 
			 selection_right - selection_left, selection_bottom - selection_top);
	  
	  selection_bottom = (((event_y(event)-boarder_left)/width_block)*width_block) + boarder_left;
	  selection_left = (((event_x(event)-boarder_upper)/height_block)*height_block + boarder_upper);
	  
	  selection_right = selection_left + text_pixel_width; 
	  selection_top = selection_bottom - text_pixel_height ;

	  XDrawRectangle(dpy, big_bit_xid, three_gc, selection_left, selection_top, 
			 selection_right - selection_left, selection_bottom - selection_top);
	}
    }
}


void
t_draw_text(x, y)
int x;
int y;
{
  
  long pixel, temp_pixel, pixel_old;
  
  int corner_x, corner_y, actual_x, actual_y;
  
  Menu edit_menu;

  static XFontSet fontset = NULL;    /* For performance - cache fontset */
  XGCValues values;

  
  {
    
    
    corner_x = (((x-boarder_left)/width_block)*width_block) + boarder_left +1;
    corner_y = (((y-boarder_upper)/height_block)*height_block + boarder_upper +1);
    
    actual_x = ((corner_x - boarder_left)/height_block);
    actual_y = ((corner_y - boarder_upper)/width_block);
    
    XGetGCValues(dpy, fill_gc, GCFont, &values);

    XmbDrawString(dpy, preview_xid, text_font, fill_gc, 
		  actual_x+preview_boarder_left, 
		  actual_y+preview_boarder_upper, text_to_be_rendered, 
		  strlen(text_to_be_rendered));
    XmbDrawString(dpy, preview_pixmap, text_font, fill_gc, 
		  actual_x+preview_boarder_left, 
		  actual_y+preview_boarder_upper, text_to_be_rendered, 
		  strlen(text_to_be_rendered));
    common_paint_proc();
    
  }
}

Font_node *
initialize_font_storage( font_info )
     char *font_info;
{
  char temp_array[256];
  char real_temp_array[64];
  char more_temp_array[128];

  Font_node *storage;

  int i;
  int field_count;
  int foo, last;
  int temp_index;
  int temp_length;
  int old_index;

  foo = 0;
  last = 0;
  i = 0;
  field_count = 0;
  temp_index = 0;

  storage = (Font_node *)malloc( sizeof( Font_node ));

  storage->full_x_font_name = strdup(font_info);

  while (font_info[i] != '\0')
    {
      if (font_info[i] == '-')
	{
	  field_count++;

	  real_temp_array[last] = '\0';	  

	  temp_array[temp_index] = font_info[i];
	  temp_index++;

	  switch (field_count) {

	  case 2: 
	    storage->foundry = strdup(real_temp_array);	    
	    foo = 0;
	    real_temp_array[foo] = '\0';
	    break;
	  case 3: 
	    storage->family = strdup(real_temp_array);	    
	    foo = 0;
	    real_temp_array[foo] = '\0';
	    break;
	  case 4: 
	    storage->weight = strdup(real_temp_array);	    
	    foo = 0;
	    real_temp_array[foo] = '\0';
	    break;
	  case 5: 
	    storage->style = strdup(real_temp_array);	    
	    foo = 0;
	    real_temp_array[foo] = '\0';
	    break;
	  case 9:
	    storage->points =strdup(real_temp_array);	    
	    foo = 0;	
	    real_temp_array[foo] = '\0';
	    break;
	  default:
	    break;
	  }
	  if (temp_index > 1)
	    if ((temp_array[temp_index - 2] == '-') && 
		(temp_array[temp_index - 1] == '-'))
	      {
		temp_array[temp_index-1] = '*';	  
		temp_array[temp_index] = '-';	  
		temp_index++;
		foo = 0;
	      }

	}
      else
	{
	  if ( (field_count == 1) || (field_count == 2) || 
	      (field_count == 3) || 
	      (field_count == 4) || (field_count == 8) )
	    {
	      temp_array[temp_index] = font_info[i];
	      
	      switch (field_count) {
		
	      case 1: case 2: case 3: case 4: case 8:
		real_temp_array[foo] = font_info[i];
		foo++;
		last = foo;
		break;
		
	      default:
		break;
	      }
	      
	      temp_index++;
	      
	    }
	}
      i++;
    }
  temp_array[temp_index] = '*';	  
  temp_index++;
  temp_array[temp_index] = '\0';	  

  storage->x_font_name = strdup(temp_array);


  strcpy(font_info, temp_array);

  temp_length = strlen(storage->family);
  for (temp_index = 0; temp_index < temp_length; temp_index++)
    more_temp_array[temp_index] = storage->family[temp_index];

  more_temp_array[temp_index] = ' '; temp_index++;
  more_temp_array[temp_index] = '('; temp_index++;
  more_temp_array[temp_index] = ' '; temp_index++;

  old_index = temp_index;
  temp_length = strlen(storage->foundry);
  for (temp_index = 0; temp_index < temp_length; temp_index++)
    more_temp_array[temp_index + old_index] = storage->foundry[temp_index];

  more_temp_array[temp_index + old_index] = ' '; temp_index++;
  more_temp_array[temp_index + old_index] = ')'; temp_index++;
  more_temp_array[temp_index + old_index] = '\0'; 

  storage->family = strdup(more_temp_array);

  return (storage);

}

count_points(list, begin, end)

Font_node	**list;
int		begin;
int		end;

{
        return(end - begin + 1);
}


count_weights(list, begin, end)

Font_node	**list;
int		begin;
int		end;

{
	int	count, i;
	char	*base_weight = list[begin]->weight;
	
	if (begin == end)
		return(1);
	
	for (count = 1, i = begin + 1; i <= end; i++)
	{
		if (strcmp(base_weight, list[i]->weight))
		{
			count++;
			base_weight = list[i]->weight;
		}
	}

	return(count);
}

count_styles(list, begin, end)

Font_node	**list;
int		begin;
int		end;

{
	int	count, i;
	char	*base_style = list[begin]->style;
	
	if (begin == end)
		return(1);

	for (count = 1, i = begin + 1; i <= end; i++)
	{
		if (strcmp(base_style, list[i]->style))
		{
			count++;
			base_style = list[i]->style;
		}
	}

	return(count);
}

count_families(list, begin, end)

Font_node	**list;
int		begin;
int		end;

{
	int	count, i;
	char	*base_family = list[begin]->family;
	
	if (begin == end)
		return(1);
	
	for (count = 1, i = begin + 1; i <= end; i++)
	{
		if (strcmp(base_family, list[i]->family))
		{
			count++;
			base_family = list[i]->family;
		}
	}

	return(count);
}

build_points(list, begin, end, base)

Font_node	**list;
int		begin;
int		end;
Point_node 	**base;

{
	int		point_count;
	Point_node	*point;
	int		i;
	int		index;
	int		original_index;
	char		*base_name;

	point_count = count_points(list, begin, end);

#ifdef FONT_TRACE
printf("    build_points, range %d to %d\n", begin, end);
printf("    found %d point sizes\n", point_count);
#endif FONT_TRACE

	*base = point = (Point_node *) calloc(point_count, sizeof(Point_node));

	for (i = 0; i < point_count; i++)
	{
		point[i].point = list[begin + i]->points;
		point[i].x_font_name = list[begin + i]->x_font_name;
		point[i].full_x_font_name = list[begin + i]->full_x_font_name;
		point[i].warning_issued = FALSE;
	}

	return(point_count);

}

build_styles(list, begin, end, base)

Font_node	**list;
int		begin;
int		end;
Style_node 	**base;

{
	int		style_count;
	Style_node	*style;
	int		i;
	int		index;
	int		original_index;
	char		*base_name;

	style_count = count_styles(list, begin, end);
#ifdef FONT_TRACE
printf("   build_styles, range %d to %d\n", begin, end);
printf("   found %d styles\n", style_count);
#endif FONT_TRACE

	*base = style = (Style_node *) calloc(style_count, sizeof(Style_node));
	for (i = 0, index = begin; (i < style_count) && (index <= end); i++)
	{
		base_name = list[index]->style;
		original_index = index;

		while (!strcmp(base_name, list[index + 1]->style) && (index < end))
			index++;

		style[i].point_count = build_points(list, original_index, index, &(style[i].point_info)); 
		style[i].style = base_name;

		index++;
	}

	return(style_count);

}

build_weights(list, begin, end, base)

Font_node	**list;
int		begin;
int		end;
Weight_node 	**base;

{
	int		weight_count;
	Weight_node	*weight;
	int		i;
	int		index;
	int		original_index;
	char		*base_name;

	weight_count = count_weights(list, begin, end);
#ifdef FONT_TRACE
printf("  build_weights, range %d to %d\n", begin, end);
printf("  found %d weights\n", weight_count);
#endif FONT_TRACE

	*base = weight = (Weight_node *) calloc(weight_count, sizeof(Weight_node));
	for (i = 0, index = begin; (i < weight_count) && (index <= end); i++)
	{
		base_name = list[index]->weight;
		original_index = index;

		while (!strcmp(base_name, list[index + 1]->weight) && (index < end))
			index++;

		weight[i].style_count = build_styles(list, original_index, index, &(weight[i].style_info)); 
		weight[i].weight = base_name;

		index++;
	}

	return(weight_count);

}

build_families(list, begin, end, base)

Font_node	**list;
int		begin;
int		end;
Family_node 	**base;

{
	int		family_count;
	Family_node	*family;
	int		i;
	int		index;
	int		original_index;
	char		*base_name;

	family_count = count_families(list, begin, end);

#ifdef FONT_TRACE
printf(" build_failies, range %d to %d\n", begin, end);
printf(" found %d families\n", family_count);
#endif FONT_TRACE

	*base = family = (Family_node *) calloc(family_count, sizeof(Family_node));
	for (i = 0, index = begin; (i < family_count) && (index <= end); i++)
	{
		base_name = list[index]->family;
		original_index = index;

		while (!strcmp(base_name, list[index + 1]->family) && (index < end))
			index++;

		family[i].weight_count = build_weights(list, original_index, index, &(family[i].weight_info));
		family[i].name = base_name;

		index++;

	}

	return(family_count);

}

#ifdef FONT_TRACE
dump_tree(families, tree_base)

int		families;
Family_node	*tree_base;

{
	int	i, j, k, l;

	for (i = 0; i < families; i++)
	{
		printf("%s\n", tree_base[i].name);
		for (j = 0; j < tree_base[i].weight_count; j++)
		{
			printf("	%s\n", tree_base[i].weight_info[j].weight);
			for (k = 0; k < tree_base[i].weight_info[j].style_count; k++)
			{
				printf("	%s\n", tree_base[i].weight_info[j].style_info[k].style);
				for (l = 0; l <  tree_base[i].weight_info[j].style_info[k].point_count; l++)
				{
					printf("		%s\n", tree_base[i].weight_info[j].style_info[k].point_info[l].point);
					printf("		%s\n", tree_base[i].weight_info[j].style_info[k].point_info[l].x_font_name);
					printf("		%s\n", tree_base[i].weight_info[j].style_info[k].point_info[l].full_x_font_name);
				}
			}
		}
	}
}
#endif FONT_TRACE

int 
dummy( info1, info2)
	Font_node **info1;
	Font_node **info2;
{
  int foo;

  if (foo = strcmp((*info1)->family, (*info2)->family))
    return (foo);
   else
    if (foo = strcmp((*info1)->weight, (*info2)->weight))
      return (foo);
  else
    if (foo = strcmp((*info1)->style, (*info2)->style))
      return (foo);
  else
      return (atoi((*info1)->points) - atoi((*info2)->points));

}

init_fonts()
{

int maxnames;
int count, i, unique_count;
char 		**list;
Font_node	**new_list;

maxnames = 2048;

list = XListFonts(dpy, pattern, maxnames, &count);

new_list = (Font_node **)calloc( count, sizeof( Font_node *));

/* copy to a sortable type structure */

for ( i = 0; i < count; i++ ) 
  {
    new_list[i] = initialize_font_storage( list[i] );
  }

/* Throw away the X font structure */

XFreeFontNames( list );

/* sort the new list */

qsort( (char *) new_list, count, sizeof( char *), dummy);

/* compress out to unique entries */

unique_count = 0;
 
for ( i = 1; i < count; i++ )
{
  if ( strcmp( new_list[unique_count]->x_font_name, 
	       new_list[i]->x_font_name ) != 0 )
    {
      unique_count++;
      new_list[unique_count] = new_list[i] ;
    }
}

num_families = build_families(new_list, 0, unique_count, &tree_base);

#ifdef FONT_TRACE
 fprintf(stderr, "num_families = %d\n", num_families);

 dump_tree(num_families, tree_base); 
#endif FONT_TRACE


set_family_item();
}

set_family_item()

{
	int	i;
      	font_family_setting = (Panel_item) text_popup->font_family_setting;

	xv_set(font_family_setting, PANEL_CHOICE_STRINGS, "", NULL, NULL);

	for (i = 0; i < num_families; i++)
		xv_set(font_family_setting, PANEL_CHOICE_STRING, i, tree_base[i].name, 0);

	xv_set(font_family_setting, PANEL_VALUE, 0, 0);

	window_fit(text_popup->controls);
	window_fit(text_popup->popup);

	set_weight_item();

}

set_weight_item()

{
	int	i;
	int	num_weights = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_count;
	int     new_value;
	int     flag = 0;

      	font_weight_setting = (Panel_item) text_popup->font_weight_setting;

	xv_set(font_weight_setting, PANEL_CHOICE_STRINGS, "", NULL, NULL);

	for (i = 0; i < num_weights; i++)
		xv_set(font_weight_setting, 
		       PANEL_CHOICE_STRING, 
		       i, 
		       tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[i].weight, 
		       0);

	if (font_old_weight)
	  for (i = 0; i < num_weights; i++)
	    {
	      if (!strcmp((char *)xv_get(font_weight_setting, PANEL_CHOICE_STRING, i),
			  font_old_weight))
		{
		  new_value = i;
		  i = num_weights;
		  flag = 1;
		}
	    }

	if (!flag)
	  font_old_weight = strdup((char *)xv_get(font_weight_setting, 
					  PANEL_CHOICE_STRING, 0));
	xv_set(font_weight_setting, PANEL_VALUE, new_value, 0);
	
	set_style_item();
}

set_style_item()

{
	int	i;
	int     new_value;
	int flag = 0;
	int	num_styles = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_count;

      	font_style_setting = (Panel_item) text_popup->font_style_setting;

	xv_set(font_style_setting, PANEL_CHOICE_STRINGS, "", NULL, NULL);

	for (i = 0; i < num_styles; i++)
	{
		char	*style_value = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[i].style;

		if (!strcmp(style_value, "r"))
			xv_set(font_style_setting, PANEL_CHOICE_STRING, i, gettext("regular"), 0);
		else if (!strcmp(style_value, "o"))
			xv_set(font_style_setting, PANEL_CHOICE_STRING, i, gettext("oblique"), 0);
		else if (!strcmp(style_value, "i"))
			xv_set(font_style_setting, PANEL_CHOICE_STRING, i, gettext("italic"), 0);
		else
			xv_set(font_style_setting, PANEL_CHOICE_STRING, i, style_value, 0);
	}

	if (font_old_style)
	  for (i = 0; i < num_styles; i++)
	    {
	      if (!strcmp((char *)xv_get(font_style_setting, PANEL_CHOICE_STRING, i),
			  font_old_style))
		{
		  new_value = i;
		  i = num_styles;
		  flag = 1;
		}
	    }

	if (!flag)
	  font_old_style = strdup((char *)xv_get(font_style_setting, 
					 PANEL_CHOICE_STRING, 0));
	xv_set(font_style_setting, PANEL_VALUE, new_value, 0);

	set_point_item();
}

set_point_item()

{
	int	i;
	int     new_value;
	int     flag = 0;
	int	num_points = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_count;

      	font_size_setting = (Panel_item) text_popup->font_size_setting;

	xv_set(font_size_setting, PANEL_CHOICE_STRINGS, "", NULL, NULL);

	for (i = 0; i < num_points; i++)
		if (strcmp(tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[i].point, "0"))
			xv_set(font_size_setting, PANEL_CHOICE_STRING, i, tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[i].point, 0);
		else
			xv_set(font_size_setting, PANEL_CHOICE_STRING, i, "default", 0);

	if (font_old_size)
	  for (i = 0; i < num_points; i++)
	    {
	      if (!strcmp((char *)xv_get(font_size_setting, PANEL_CHOICE_STRING, i),
			  font_old_size))
		{
		  new_value = i;
		  i = num_points;
		  flag = 1;
		}
	    }

	if (!flag)
	  font_old_size = strdup((char *)xv_get(font_size_setting, 
					PANEL_CHOICE_STRING, 0));
	xv_set(font_size_setting, PANEL_VALUE, new_value, 0);

	point_event_proc(0);

}

check_font_set(font_name, tail_string)

char	*font_name;
char	*tail_string;

{
	if (!strcmp(tail_string, &font_name[strlen(font_name) - strlen(tail_string)]))
		return(TRUE);
	else
		return(FALSE);
}

point_event_proc(point_size)

int	point_size;

{
	extern base_window_objects     *base_window;
	extern int Edit_mode;
	int answer;
	char *fontfamily;
  	char **missing_charset_list;
  	char *def_string;
  	char isofont[] = "-*-*-*-*-*-*-*-*-*-*-*-*-iso8859-1";
  	int missing_charset_count;

	char *font_name = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[point_size].x_font_name;
	char *full_font_name = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[point_size].full_x_font_name;
	char warned = tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[point_size].warning_issued;


	/* test to see if the font is an expensive one, and if 
	   so, warn the user */

	if (!check_font_set(full_font_name, "iso8859-1") && 
	    !check_font_set(full_font_name, "sunolglyph-1") && 
	    !check_font_set(full_font_name, "sunolcursor-1") && 
            !check_font_set(full_font_name, "adobe-fontspecific") && 
            !check_font_set(full_font_name, "sun-fontspecific") && 
            !check_font_set(full_font_name, "symbol") && 
	    !warned)
	{
		tree_base[xv_get(font_family_setting, PANEL_VALUE)].weight_info[xv_get(font_weight_setting, PANEL_VALUE)].style_info[xv_get(font_style_setting, PANEL_VALUE)].point_info[point_size].warning_issued = TRUE;
                
      		answer = notice_prompt(base_window->window, NULL,
                             		NOTICE_MESSAGE_STRINGS,
                             		gettext("The font you have selected may take some time to load.  Do you wish to continue?"),
                             		0,
                             		NOTICE_BUTTON_YES, gettext("Cancel"),
                             		NOTICE_BUTTON_NO, gettext("Continue"),
                             		0);
 
 
      		if (answer == NOTICE_YES)
        	{
			Edit_mode = 0;
			xv_set(base_window->brush_setting, PANEL_VALUE, Edit_mode, 0);
          		return;
        	}
		xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
		xv_set(text_popup->popup, FRAME_BUSY, TRUE, 0);

	}
	/* get the rightfont */

	if (font_name == last_font)
		return;

	if (text_font)
        {
		XFreeFontSet(dpy, text_font);
                text_font = NULL;
	}

	/* open it */

        setting_font = TRUE;
	fontfamily = (char *)malloc(strlen(font_name)+strlen(isofont)+2);
	strcpy(fontfamily, font_name);
	strcat(fontfamily, ",");
	strcat(fontfamily, isofont);
	text_font = XCreateFontSet(dpy, fontfamily, &missing_charset_list, &missing_charset_count, &def_string);
	xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	xv_set(text_popup->popup, FRAME_BUSY, FALSE, 0);
	free(fontfamily);
	if ( missing_charset_count != 0 ) 
		pop_notice(gettext("The font you have selected may not\ndisplay certain characters in this locale."));

	XSync(dpy, FALSE);
        setting_font = FALSE;

        if (!text_font)
        {
    		pop_notice(gettext("The font you have selected is unavailable.\nText entry is disabled until you select a valid font."));
		Edit_mode = 0;
		xv_set(base_window->brush_setting, PANEL_VALUE, Edit_mode, 0);
        }
	else
		last_font = font_name;
}

family_setting_proc(item, value, event)

Panel_item	item;
int		value;
Event		*event;

{
	set_weight_item();
}

weight_setting_proc(item, value, event)

Panel_item	item;
int		value;
Event		*event;

{
        char *foo;
	foo = (char *)xv_get(font_weight_setting, PANEL_CHOICE_STRING, value);
	if (foo)
	    font_old_weight = strdup(foo);
	set_style_item();

}

style_setting_proc(item, value, event)

Panel_item	item;
int		value;
Event		*event;

{
        char *foo;
	foo = (char *)xv_get(font_style_setting, PANEL_CHOICE_STRING, value);
	if (foo)
	    font_old_style = strdup(foo);
	set_point_item();

}

size_setting_proc(item, value, event)

Panel_item	item;
int		value;
Event		*event;

{
        char *foo;
	point_event_proc(value);
	foo = (char *)xv_get(font_size_setting, PANEL_CHOICE_STRING, value);
	if (foo)
	    font_old_size = strdup(foo);
}



