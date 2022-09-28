
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/dragdrop.h>
#include <xview/xv_xrect.h>

/* Global Object definitions
 *
 */

Frame        frame;
Panel        panel;
Textsw       textsw;
Panel_item   load_file;


#define FILE_NAME_ATOM			        0	
#define _SUN_AVAILABLE_TYPES_ATOM	        1	
#define XA_STRING_ATOM			        2	
#define TOTAL_ATOMS		                3		

struct
{
	Atom	atom;
	char	*name;
} atom_list[TOTAL_ATOMS] = 
{
	{0,	"FILE_NAME"},
	{0,	"_SUN_AVAILABLE_TYPES"},
	{0,	"XA_STRING"},
};

Drag_drop		drag_object; /* The drag object */

main(int argc, char **argv)
{
	Xv_Server   server;
      
        void        DnD_init(), create_user_interface(); 

 	server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
        
        create_user_interface();
   	
        
        DnD_init(server); 
	
	xv_main_loop(frame);
}

/* drop_proc: Setup the drag operation and handle the drop.
 *
 */

void
drop_proc(Xv_opaque item, unsigned int value, Event *event)
{
	long			length;
	int			format;
	char			*sel_string;
	char			*string;
	Selection_requestor	sel_req;
	char			*buff;
	int			txt_len;
	Atom			list[4];
        void             get_primary_selection(Selection_requestor sel_req);

	sel_req = xv_get(item, PANEL_DROP_SEL_REQ);
        

	printf("sel_req = %X\n", sel_req);
	switch(event_action(event))
	{
	case	ACTION_DRAG_MOVE:	/* they are moving the object */
		printf("drag move\n");
		get_primary_selection(sel_req);		
		break;

	case	ACTION_DRAG_COPY:	/* they are copying the object */
		printf("drag copy\n");
		get_primary_selection(sel_req);		
		break;

	case	LOC_DRAG:
		list[0] = atom_list[_SUN_AVAILABLE_TYPES_ATOM].atom;
		list[1] = atom_list[FILE_NAME_ATOM].atom;
		list[2] = atom_list[XA_STRING_ATOM].atom;
		list[3] = NULL;

		xv_create(drag_object, SELECTION_ITEM,
			SEL_DATA, 	list,
			SEL_FORMAT,	32,
			SEL_LENGTH,	4,
			SEL_TYPE,    atom_list[_SUN_AVAILABLE_TYPES_ATOM].atom,
			SEL_OWN,	TRUE,
			NULL);

		string = (char *)xv_get(load_file, PANEL_VALUE);

		xv_create(drag_object, SELECTION_ITEM,
			SEL_DATA, 	string,
			SEL_FORMAT,	8,
			SEL_LENGTH,	strlen(string),
			SEL_TYPE,	atom_list[FILE_NAME_ATOM].atom,
			SEL_OWN,	TRUE,
			NULL);

		txt_len = xv_get(textsw, TEXTSW_LENGTH) + 1;
		string = (char *)calloc(txt_len,1);
		xv_get(textsw,
			TEXTSW_CONTENTS, 0, string, txt_len);

		xv_create(drag_object, SELECTION_ITEM,
			SEL_DATA, 	string,
			SEL_FORMAT,	8,
			SEL_LENGTH,	strlen(string),
			SEL_TYPE,	atom_list[XA_STRING_ATOM].atom,
			SEL_OWN,	TRUE,
			NULL);

		xv_set(frame,
			FRAME_LEFT_FOOTER,	"Start draging",
			NULL);
		printf("Start draging\n");
		break;
	default:
		printf("unknown event %d\n", event_action(event));
	}

} 
void
get_primary_selection(Selection_requestor sel_req)
{
	long            length;
	int             format;
	char		*sel_string;
	char		*string;
	Atom		*list;
	int		i;

	list = NULL;
	xv_set(sel_req, SEL_TYPE, atom_list[_SUN_AVAILABLE_TYPES_ATOM].atom, 0);
	list = (Atom *) xv_get(sel_req, SEL_DATA, &length, &format);
	if (length == SEL_ERROR)
	{
		printf("*** Unable to get target list.\n");
	}
	else
	{
		printf("length = %d format = %d\n", length, format);
		while(*list)
		{
			printf("list = %X\n", list);
			for(i = 0; i < TOTAL_ATOMS; i++)
			{
				if(*list == atom_list[i].atom)
				{
					printf("supports %d %s\n", i,
							 atom_list[i].name);
					break;
				}
			}
			list++;
		}
	}
	xv_set(sel_req, SEL_TYPE, atom_list[FILE_NAME_ATOM].atom, 0);
	string = (char *) xv_get(sel_req, SEL_DATA, &length, &format);
	if (length != SEL_ERROR)
	{
		printf("length = %d format = %d\n", length, format);
		/* Create a NULL-terminated version of 'string' */
		sel_string = (char *) calloc(1, length + 1);
		strncpy(sel_string, string, length);

		xv_set(load_file, PANEL_VALUE, string, NULL);
		xv_set(textsw,
			TEXTSW_FILE,	string,
			NULL);
		return;
	}
	else
	{
		printf("*** Unable to get FILE_NAME_ATOM selection.\n");
	}

	xv_set(sel_req, SEL_TYPE, atom_list[XA_STRING_ATOM].atom, 0);
	string = (char *) xv_get(sel_req, SEL_DATA, &length, &format);
	if (length != SEL_ERROR)
	{
		printf("length = %d format = %d\n", length, format);
		/* Create a NULL-terminated version of 'string' */
		sel_string = (char *) calloc(1, length + 1);
		strncpy(sel_string, string, length);

		textsw_reset(textsw, 0, 0);
		textsw_insert(textsw, string, length);
	}
	else
	{
		printf("*** Unable to get XA_STRING_ATOM selection.\n");
	}
} 


/*
 * Notify callback function for `filename'. This routine loads the
 * named file into the textpane.
 */

Panel_setting
load_file_proc(Panel_item item, Event *event)
{
	char *	value = (char *) xv_get(item, PANEL_VALUE);
	
	fprintf(stderr, "DnD_demo: load_file: value: %s\n", value);
	

	xv_set(textsw,
		TEXTSW_FILE,	value,
		NULL);

	return panel_text_notify(item, event);
} 


/* 
 *  DnD_init: Create a drop site, and a drag object.
 */
void
DnD_init(Xv_Server server)
{
	Xv_drop_site	drop_site;
	Xv_opaque	drop_glyph;
	Xv_opaque	busy_glyph;

        static unsigned short   drop_icon[] = {
#include "drop_site.icon"
        };
        static unsigned short   busy_icon[] = {
#include "busy_site.icon"
        };

	int	i;        

	for(i = 0; i < TOTAL_ATOMS; i++)
	{
		atom_list[i].atom = xv_get(server,
					SERVER_ATOM, 
					atom_list[i].name);
	}

	atom_list[XA_STRING_ATOM].atom = XA_STRING;
 
	drag_object = xv_create(panel, DRAGDROP, NULL);

        drop_glyph = xv_create(XV_NULL, SERVER_IMAGE,
                SERVER_IMAGE_BITS, drop_icon,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 32,
                XV_HEIGHT, 32,
                NULL);

        busy_glyph = xv_create(XV_NULL, SERVER_IMAGE,
                SERVER_IMAGE_BITS, busy_icon,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 32,
                XV_HEIGHT, 32,
                NULL);

	xv_create(panel,                        PANEL_DROP_TARGET,
			PANEL_DROP_DND,		drag_object,
			PANEL_DROP_GLYPH,	drop_glyph,
			PANEL_DROP_BUSY_GLYPH,	busy_glyph,
			PANEL_NOTIFY_PROC,	drop_proc,
			PANEL_DROP_FULL,	TRUE,
			NULL); 
}
 

/* 
 *  create_user_interface: Create the user interface components.
 */
 
void
create_user_interface()
{
        Panel_setting    load_file_proc();
	frame = xv_create(NULL,       FRAME,
 	                    XV_LABEL, "Drag-n-Drop Demo",
                            XV_WIDTH,  600,
                            XV_HEIGHT, 300,
                            FRAME_SHOW_FOOTER, TRUE,
                            NULL);

        panel = xv_create(frame,      PANEL,
                            XV_X,     0,
                            XV_Y,     0,
                            XV_WIDTH, WIN_EXTEND_TO_EDGE,
                            XV_HEIGHT,50,
                            NULL);

        load_file = xv_create(panel,  PANEL_TEXT,
                                PANEL_VALUE_DISPLAY_LENGTH, 45,
                                PANEL_VALUE_STORED_LENGTH, 80,
                                PANEL_LABEL_STRING, "Filename:",
                                PANEL_LAYOUT,PANEL_HORIZONTAL,
                                PANEL_READ_ONLY, FALSE,
                                PANEL_NOTIFY_PROC, load_file_proc,
                                NULL); 

                     
         textsw = xv_create(frame,        TEXTSW,
                              WIN_BELOW,  panel,
                              XV_WIDTH,   WIN_EXTEND_TO_EDGE,
                              XV_HEIGHT,  WIN_EXTEND_TO_EDGE,
                              NULL); 
}
