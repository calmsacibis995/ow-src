/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident  "@(#)loadsave_xv.c 1.59     93/02/25 SMI"


#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <group.h>
#include "ds_popup.h"		/* for deskset popup layout code */

#include <multimedia/audio_hdr.h>

#include "atool_i18n.h"
#include "atool_debug.h"
#include "undolist_c.h"
#include "loadsave_panel_impl.h"
#include "loadsave_panel.h"

#ifndef USE_GFM
#include "fc_exten_ui.h"	/* items for save format on Save As popup */
#include <xview/file_chsr.h>

static unsigned short audio_file_bits[] = {
#include "audio_file_glyph.icon"
};
#else /* USE_GFM */

#include "gfm.h"
#endif /* USE_GFM */


Attr_attribute	INSTANCE;	/* Devguide global key */
Attr_attribute	FILEKEY;	/* Global key for local storage */
Attr_attribute	COMPRESSKEY;	/* compression encoding key */
Attr_attribute	FCEXTENKEY;	/* file chooser extension key */
Attr_attribute  FCCANCELKEY;	/* proc to call on a cancel */


/* Create a File Panel */
int
FilePanel_INIT(
	ptr_t			owner,
	ptr_t			ifp)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	/* Init global keys, if necessary */
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (FILEKEY == 0)
		FILEKEY = xv_unique_key();
	if (COMPRESSKEY == 0)
		COMPRESSKEY = xv_unique_key();
	if (FCEXTENKEY == 0)
		FCEXTENKEY = xv_unique_key();
	if (FCCANCELKEY == 0)
		FCCANCELKEY = xv_unique_key();

	/* Set the default directories */
#ifndef SUNOS41
	if (getcwd(fp->opendir, MAXPATHLEN) == 0)
#else
	if (getwd(fp->opendir) == 0)
#endif
		strcpy(fp->opendir, "~");
	strcpy(fp->savedir, fp->opendir);

#ifndef USE_GFM
	/* 
	 * set glyph first because filter cb will be called 
	 * during xv_create 'cause it reads the directory at that
	 * time (i wish i could turn that off!).
	 */
	fp->audio_file_glyph = (ptr_t) xv_create(XV_NULL, SERVER_IMAGE,
	    SERVER_IMAGE_BITS, audio_file_bits,
	    SERVER_IMAGE_DEPTH, 1,
	    XV_WIDTH, 16,
	    XV_HEIGHT, 16,
	    NULL);
#endif /* USE_GFM */

	/* don't create any file choosers until they're actually called upon */
	return (TRUE);
}

static Xv_opaque
getpanel(
	ptr_t			ifp,
	FilePanel_Type		which)
{
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;

	switch (which) {
	default:
	case LS_Open:
		return ((Xv_opaque)fp->open_panel);
#ifndef USE_GFM
	case LS_Include:
		return ((Xv_opaque)fp->include_panel);
	case LS_Save:
	case LS_SaveAs:
		return ((Xv_opaque)fp->saveas_panel);
	case LS_None:
		return (NULL);
#endif /* !USE_GFM */
	}
}


#ifndef USE_GFM

/* File Chooser handling routines */


/*
 * FILE_CHOOSER_EXTEN_FUNC:
 * layout extension items within the given extension rect.
 */
static int
save_fmt_exten_func(
	File_chooser		fc,
	Rect			*frame_rect,
	Rect			*exten_rect, 
	int			left_edge,
	int			right_edge,
	int			max_height)
{
	fc_exten_panel_objects	*sfp;
 
	sfp = (fc_exten_panel_objects *)
	    xv_get((Xv_opaque)fc, XV_KEY_DATA, INSTANCE);;

	/* just reposition the suckka - and regroup */
	xv_set(sfp->main_group,
	       XV_X,		left_edge,
	       XV_Y,		exten_rect->r_top,
	       GROUP_LAYOUT,	TRUE,
	       PANEL_PAINT, 	PANEL_NONE,
	       NULL);

	return (-1);
}

/*
 * FILE_CHOOSER_EXTEN_FUNC:
 * layout extension items within the given extension rect.
 */
static int
show_files_exten_func(
	File_chooser		fc,
	Rect			*frame_rect,
	Rect			*exten_rect, 
	int			left_edge,
	int			right_edge,
	int			max_height)
{
	fc_exten_panel_objects	*sfp;
	int			item_width;
	int			list_width;
	Xv_opaque		list_item;
	int			item_x;
 
	sfp = (fc_exten_panel_objects *)
	    xv_get((Xv_opaque)fc, XV_KEY_DATA, INSTANCE);

	/*
	 * just reposition the suckka. try to align it center of the
	 * scrolling list.
	 */
	item_x = left_edge;	/* left edge by default */

	list_item = xv_get(fc,
	    FILE_CHOOSER_CHILD, FILE_CHOOSER_FILE_LIST_CHILD);

	if (list_item) {
		item_width = xv_get(sfp->show_files, XV_WIDTH);
		list_width = xv_get(list_item, XV_WIDTH);
		if (list_width > item_width) {
			item_x += ((list_width - item_width)/2);
		}
	}
	xv_set(sfp->show_files,
	       XV_X,		item_x,
	       XV_Y,		exten_rect->r_top,
	       PANEL_PAINT, 	PANEL_NONE,
	       NULL);
	return (-1);
}

/* Adjust the Show Files exclusive choices to be the same size */
static void
adjust_show_choices(
	fc_exten_panel_objects	*sfp)
{
	int			i;
	int			j;
	Rect			*rect;

	/* set the exclusive choices to be around the same width */
	for (j = 0, i = 0; i < 2; i++) {
		rect = (Rect*) xv_get(sfp->show_files, PANEL_CHOICE_RECT, i);
		if (rect->r_width > j)
			j = rect->r_width;
	}
	for (i = 0; i < 2; i++) {
		/* XXX - setting XV_WIDTH *should* work! */
		char	*oldname;
		char	*newname;

		while (rect = (Rect*)
		    xv_get(sfp->show_files, PANEL_CHOICE_RECT, i),
		    rect->r_width < (j - 8)) {
			oldname = (char*)
			    xv_get(sfp->show_files, PANEL_CHOICE_STRING, i);
			newname = (char*) malloc(strlen(oldname) + 3);
			(void) sprintf(newname, " %s ", oldname);
			xv_set(sfp->show_files,
			    PANEL_CHOICE_STRING, i, newname, NULL);
			(void) free(newname);
		}
	}

}

/* Put Format/Compression/File Size/Show Files extensions on save panel */
static void
add_save_format_extension(
	ptr_t			ifc)
{
	File_chooser		fc = (File_chooser) ifc;
	fc_exten_panel_objects	*sfp = NULL;
	int			i;
	int			j;
	int			frame_height;
	int			item_height;
	int			frame_width;
	int			text_width;

	/* 
	 * create the save_fmt items. the *item* is a group
	 * composed of several items in a guide gen'd file.
	 * we'll create our own guide UI object (first 
	 * re-assign a couple of the top level members
	 * from the save_as panel).
	 */
	sfp = (fc_exten_panel_objects*)
	    calloc(1, sizeof (fc_exten_panel_objects));
	if (sfp == NULL)
		return;

	xv_set(fc, XV_KEY_DATA, FCEXTENKEY, sfp, NULL);

	/* save frame width *before* adding items */
	frame_width = xv_get(fc, XV_WIDTH);

	sfp->panel = fc;
	sfp->controls = xv_get(fc, FRAME_CMD_PANEL);
	(void) fc_exten_panel_objects_initialize(sfp, NULL);

	/* set text fields so they aren't too wide */
	text_width = xv_get(sfp->format, PANEL_VALUE_DISPLAY_LENGTH);
	i = frame_width - 5;
	j = xv_get(sfp->format, XV_X);
	while (text_width > 10) {
		if ((j + xv_get(sfp->format, XV_WIDTH)) <= i)
			break;
		xv_set(sfp->format,
		    PANEL_VALUE_DISPLAY_LENGTH, --text_width, NULL);
	}
	xv_set(sfp->compression, PANEL_VALUE_DISPLAY_LENGTH, text_width, NULL);
	xv_set(sfp->file_size, PANEL_VALUE_DISPLAY_LENGTH, text_width, NULL);

	/* fix the exclusive choice button sizes */
	adjust_show_choices(sfp);

	/* get the various sizes added by the items */
	item_height = xv_get(sfp->main_group, XV_HEIGHT);
	frame_height = xv_get(fc, XV_HEIGHT);
	xv_set(fc, XV_HEIGHT, frame_height + item_height, NULL);
	frame_height = xv_get(fc, XV_HEIGHT);

	xv_set(fc, FRAME_MIN_SIZE, frame_width, frame_height,
	    XV_WIDTH, frame_width, XV_KEY_DATA, INSTANCE, sfp, NULL);

	/* Tell File Chooser to reserve layout space */
	xv_set(fc, FILE_CHOOSER_EXTEN_FUNC, save_fmt_exten_func,
	    FILE_CHOOSER_EXTEN_HEIGHT, item_height, NULL);

	if (xv_get(fc, FILE_CHOOSER_ABBREV_VIEW)) {
		/* audio files only */
		xv_set(sfp->show_files, PANEL_VALUE, 1, NULL);
	} else {
		xv_set(sfp->show_files, PANEL_VALUE, 0, NULL);
	}
	return;
}

/* Put Show Files extensions on open/include panels */
static void
add_show_files_extension(
	ptr_t			ifc)
{
	File_chooser		fc = (File_chooser) ifc;
	fc_exten_panel_objects	*sfp = NULL;
	int			frame_height;
	int			item_height;
	int			frame_width;

	/* 
	 * create the show_files item. 
	 */
	sfp = (fc_exten_panel_objects*)
	    calloc(1, sizeof(fc_exten_panel_objects));
	if (sfp == NULL)
		return;

	/* save frame width *before* adding items */
	frame_width = xv_get(fc, XV_WIDTH);

	sfp->panel = fc;
	sfp->controls = xv_get(fc, FRAME_CMD_PANEL);
	sfp->show_files = fc_exten_panel_show_files_create(sfp, sfp->controls);

	/* fix the exclusive choice button sizes */
	adjust_show_choices(sfp);

	item_height = xv_get(sfp->show_files, XV_HEIGHT);
	frame_height = xv_get(fc, XV_HEIGHT);
	xv_set(fc, XV_HEIGHT, frame_height + item_height, NULL);
	frame_height = xv_get(fc, XV_HEIGHT);

	xv_set(fc, FRAME_MIN_SIZE, frame_width, frame_height,
	    XV_WIDTH, frame_width, XV_KEY_DATA, INSTANCE, sfp, NULL);

	/* Tell File Chooser to reserve layout space */
	xv_set(fc, FILE_CHOOSER_EXTEN_FUNC, show_files_exten_func,
	    FILE_CHOOSER_EXTEN_HEIGHT, item_height, NULL);

	if (xv_get(fc, FILE_CHOOSER_ABBREV_VIEW)) {
		/* audio files only */
		xv_set(sfp->show_files, PANEL_VALUE, 1, NULL);
	} else {
		xv_set(sfp->show_files, PANEL_VALUE, 0, NULL);
	}
	return;
}

/*
 * Call this everytime the directory is changed for the Save panels.
 * Display the available free space.
 */
static int
cd_func(
	File_chooser		fc,
	char			*path,
	struct stat		*sbuf,
	File_chooser_op		op)
{
	long			kb;
	struct file_panel_data	*fp;
	fc_exten_panel_objects	*sfp;
 
	fp = (struct file_panel_data *)
	    xv_get((Xv_opaque)fc, XV_KEY_DATA, FILEKEY);
			     
	if (op == FILE_CHOOSER_BEFORE_CD)
		return (XV_OK);

	/* update file size and free space */
	FilePanel_Updatefilesize(fp);
}


/* Notify routine for all cancel buttons */
static void
cancel_button_notify(
	Panel_item		item,
	Event*			event)
{
	struct file_panel_data	*fp;
	PFUNCV			cancel_proc;

	fp = (struct file_panel_data *)
	    xv_get((Xv_opaque)item, XV_KEY_DATA, FILEKEY); 

	if (fp->busy) {
		DBGOUT((2, "** cancelling load/save\n"));
		AudPanel_Stop(AudPanel_KEYDATA(fp->owner));
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
		return;
	}

	cancel_proc = (PFUNCV) xv_get(item, XV_KEY_DATA, FCCANCELKEY);
	if (cancel_proc) {
		DBGOUT((2, "** calling cancel proc\n"));
		(*cancel_proc)(item, event);
	}
}

/* Menu handler for `format_menu' */
Menu
save_format_notify(
	Menu			menu,
	Menu_generate		op)
{
	fc_exten_panel_objects *ip;
	struct file_panel_data	*fp;
	
	if (op == MENU_DISPLAY) {
		ip = (fc_exten_panel_objects*)
		    xv_get(menu, XV_KEY_DATA, INSTANCE);
		fp = (struct file_panel_data *)
		    xv_get((Xv_opaque)ip->panel, XV_KEY_DATA, FILEKEY); 

		menu = do_create_format_menu(menu, 
		    AudPanel_FormatPanel_hndl(AudPanel_KEYDATA(fp->owner)),
		    FilePanel_Formatnotify, FilePanel_Cansave, 
		    &(fp->cur_hdr), fp, MGET("Save Format"));
	}
	/* don't take down unpinned panel if button pushed */
	xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	return (menu);
}

/* Menu handler for 'compress_menu' items */
static int
compress_item_notify(
	Menu			menu,
	Menu_item		menu_item)
{
	struct file_panel_data	*fp;
	fc_exten_panel_objects	*ip;
	int			enc;
	char			*str;

	ip = (fc_exten_panel_objects*) xv_get(menu, XV_KEY_DATA, INSTANCE);
	fp = (struct file_panel_data *) xv_get(menu, XV_KEY_DATA, FILEKEY);

	if ((str = (char*)xv_get(menu_item, MENU_STRING)) && *str) {
		xv_set(ip->compression, PANEL_VALUE, str, NULL);
	}

	enc = (int)xv_get(menu_item, XV_KEY_DATA, COMPRESSKEY);
	xv_set(ip->compression, XV_KEY_DATA, COMPRESSKEY, enc, NULL);
	fp->compression = enc;

	/* update file size and free space */
	FilePanel_Updatefilesize(fp);

	/* don't take down unpinned panel if button pushed */
	xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	return (XV_OK);
}


/* Menu handler for `compress_menu' */
Menu
save_compress_notify(
	Menu			menu,
	Menu_generate		op)
{
static int			inited = FALSE; /* initialized yet? */
	fc_exten_panel_objects	*ip;
	struct file_panel_data	*fp;
	int			i;
	int			nitems;
	Menu_item		mi;
	int			cancompress;
	Audio_hdr		hdr;
	
	if (op == MENU_DISPLAY) {
		ip = (fc_exten_panel_objects*)
		    xv_get(menu, XV_KEY_DATA, INSTANCE);
		fp = (struct file_panel_data *)
		    xv_get((Xv_opaque)ip->panel, XV_KEY_DATA, FILEKEY); 
		
		if (inited == FALSE) {
			inited == TRUE;
			if (!menu) {
				menu = xv_create(XV_NULL, MENU, NULL);
			} else {
				/* destroy all items */
				for (nitems = (int)xv_get(menu, MENU_NITEMS); 
				     nitems > 0; nitems--) {
					xv_set(menu, MENU_REMOVE, nitems, NULL);
					xv_destroy(xv_get(menu, MENU_NTH_ITEM,
					    nitems));
				}
			}
			xv_set(menu, XV_KEY_DATA, FILEKEY, (Xv_opaque)fp, NULL);
		}

		/* 
		 * XXX - for now this menu is hardwired. this should change
		 * in the future by asking the audio library "what kind of
		 * compression methods are available for this audio type"?
		 */
		mi = xv_create(XV_NULL, MENUITEM,
		    MENU_STRING, MGET("None"),
		    XV_KEY_DATA, COMPRESSKEY, AUDIO_ENCODING_NONE,
		    XV_KEY_DATA, FILEKEY, (Xv_opaque)fp,
		    MENU_NOTIFY_PROC, compress_item_notify,
		    NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
		xv_set(menu, MENU_DEFAULT_ITEM, mi, NULL);

		/* disable if current save format cannot be compressed */

		FilePanel_Setcompresshdr(fp, &hdr, AUDIO_ENCODING_G721);
		cancompress = FilePanel_Cansave(fp, &hdr);
		mi = xv_create(XV_NULL, MENUITEM,
		    MENU_STRING, audio_print_encoding(&hdr),
		    MENU_RELEASE_IMAGE,
		    XV_KEY_DATA, COMPRESSKEY, AUDIO_ENCODING_G721,
		    XV_KEY_DATA, FILEKEY, (Xv_opaque)fp,
		    MENU_NOTIFY_PROC, compress_item_notify,
		    MENU_INACTIVE, (cancompress == TRUE) ? FALSE : TRUE,
		    NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);

		FilePanel_Setcompresshdr(fp, &hdr, AUDIO_ENCODING_G723);
		cancompress = FilePanel_Cansave(fp, &hdr);
		mi = xv_create(XV_NULL, MENUITEM,
		    MENU_STRING, audio_print_encoding(&hdr),
		    MENU_RELEASE_IMAGE,
		    XV_KEY_DATA, COMPRESSKEY, AUDIO_ENCODING_G723,
		    XV_KEY_DATA, FILEKEY, (Xv_opaque)fp,
		    MENU_NOTIFY_PROC, compress_item_notify,
		    MENU_INACTIVE, (cancompress == TRUE) ? FALSE : TRUE,
		    NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
	}
	return (menu);
}

/* Notify callback function for `show_files' */
void
fc_exten_panel_show_files_notify_callback(
	Panel_item		item,
	int			value,
	Event			*event)
{
	fc_exten_panel_objects	*ip;

	ip = (fc_exten_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	switch (value) {
	case 0:
		/* set the file chooser to show all files */
		xv_set(ip->panel, FILE_CHOOSER_ABBREV_VIEW, FALSE, NULL);
		break;
	case 1:
		/* set the file chooser to show only audio files */
		xv_set(ip->panel, FILE_CHOOSER_ABBREV_VIEW, TRUE, NULL);
		break;
	}
}

/* call backs for XView File Chooser */

/* Called for each file in directory to match audio files */
static int
file_filter_notify(
	File_chooser		fc,
	char			*path,
	struct stat		sb, 
	File_chooser_op		matched,
	Server_image		*glyph,
	Xv_opaque		*cdata)
{
	struct file_panel_data	*fp;
	char			fullpath[MAXPATHLEN+1];

	fp = (struct file_panel_data *)xv_get(fc, XV_KEY_DATA, FILEKEY);

	if (fp->filt_proc) {
		/* 
		 * If it's an audio file, set our cute glyph.
		 *
		 * XXX - For now...
		 * If this is the Include file chooser, only allow files 
		 * of the same as what's loaded
		 */
		switch ((*fp->filt_proc)
		    (fp, path, (fc == (File_chooser)fp->include_panel))) {
		case 0:		/* Not an audio file */
			return (FILE_CHOOSER_IGNORE);

		case 1:		/* Audio file is ok */
			*glyph = (Server_image)fp->audio_file_glyph;
		default:
			return (FILE_CHOOSER_ACCEPT);

		case -1:	/* Audio file is not ok */
			*glyph = (Server_image)fp->audio_file_glyph;
			return (FILE_CHOOSER_IGNORE);
		}
	} else {
		return (FILE_CHOOSER_ACCEPT);
	}
}	

/* Notify proc for the Open button */
static int
open_notify(
	File_chooser		fc,
	char			*path,
	char			*file,
	Xv_opaque		cdata)
{
	struct file_panel_data	*fp;

	fp = (struct file_panel_data *)xv_get(fc, XV_KEY_DATA, FILEKEY);
	return (((*fp->open_proc)(fp, path) == TRUE) ? XV_OK : XV_ERROR);
}

/* Notify proc for the Include button */
static int
include_notify(
	File_chooser		fc,
	char			*path,
	char			*file,
	Xv_opaque		cdata)
{
	struct file_panel_data	*fp;

	fp = (struct file_panel_data *)xv_get(fc, XV_KEY_DATA, FILEKEY);
	return (((*fp->include_proc)(fp, path) == TRUE) ? XV_OK : XV_ERROR);
}

/* Notify proc for the Save button */
static int
saveas_notify(
	File_chooser		fc,
	char			*path,
	struct stat		*stats)
{
	struct file_panel_data	*fp;
	Audio_hdr		hdr;

	fp = (struct file_panel_data *)xv_get(fc, XV_KEY_DATA, FILEKEY);

	/* Modify the save format with the compression item */
	FilePanel_Setcompresshdr(fp, &hdr, fp->compression);

	return (((*fp->save_proc)(fp, path, TRUE, &hdr) == TRUE) ?
	    XV_OK : XV_ERROR);
}


/* Create a File Chooser panel variant */
static void
init_fcpanel(
	ptr_t			ifp,
	Xv_opaque		panel,
	char			*label,
	int			(*notify_proc)(),
	char*			dir)
{
	Xv_opaque		cancel_button;
	struct file_panel_data	*fp = (struct file_panel_data *)ifp; 

	/* Set the common File Chooser parameters */
	xv_set(panel,
	    XV_SHOW, FALSE,
	    XV_LABEL, label,
	    FILE_CHOOSER_NOTIFY_FUNC, notify_proc,
	    FILE_CHOOSER_DIRECTORY, dir,
	    FILE_CHOOSER_ABBREV_VIEW, TRUE,
	    XV_KEY_DATA, FILEKEY, ifp,
	    NULL);

	/* 
	 * Get a handle on the cancel button,
	 * store its notify proc as key data on the button,
	 * set our own notify proc.
	 */
	if (cancel_button = xv_get(panel,
	    FILE_CHOOSER_CHILD, FILE_CHOOSER_CANCEL_BUTTON_CHILD)) {
		xv_set(cancel_button, 
		    XV_KEY_DATA, FCCANCELKEY,
		    xv_get(cancel_button, PANEL_NOTIFY_PROC),
		    XV_KEY_DATA, FILEKEY, fp,
		    NULL);
		xv_set(cancel_button,
		    PANEL_NOTIFY_PROC, cancel_button_notify, NULL);
	}
}

/* Find a given a compression descriptor in the menu and set it */
static void
do_set_compression(
	File_chooser		fc,
	int			encoding)
{
	Menu			m;
	Menu_item		mi;
	int			i;
	int			enc;
	char			*encstr;
	fc_exten_panel_objects	*sfp;

	sfp = (fc_exten_panel_objects *)
	    xv_get((Xv_opaque)fc, XV_KEY_DATA, INSTANCE);

	if ((m = xv_get(sfp->compress_button, PANEL_ITEM_MENU)) == NULL)
		return;

	/*
	 * XXX - hack attack. make sure all the items
	 * are gen'd by doing a bogus xv_find();
	 */
	(void) xv_find(m, MENUITEM, XV_AUTO_CREATE, FALSE,
	    MENU_STRING, "", NULL);

	for (i = xv_get(m, MENU_NITEMS); ; i--) {
		if ((mi = xv_get(m, MENU_NTH_ITEM, i)) == NULL) {
			/* if not found, set no compression */
			mi = xv_get(m, MENU_NTH_ITEM, 1);
			encoding = AUDIO_ENCODING_NONE;
		}
		enc = (int)xv_get(mi, XV_KEY_DATA, COMPRESSKEY);
		if (enc == encoding) {
			encstr = (char*)xv_get(mi, MENU_STRING);
			xv_set(sfp->compression, 
			    PANEL_VALUE, encstr,
			    XV_KEY_DATA, COMPRESSKEY, encoding,
			    NULL);
			return;
		}
	}
}

/* Update the file size and filesystem free space message */
void
FilePanel_SETFILESIZESTR(
	ptr_t			ifp,
	char			*str)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp; 
	fc_exten_panel_objects	*sfp;

	if (fp->saveas_panel) {
		sfp = (fc_exten_panel_objects *) xv_get(
		    (Xv_opaque)fp->saveas_panel, XV_KEY_DATA, FCEXTENKEY);
		if (sfp != NULL)
			xv_set(sfp->file_size, PANEL_VALUE, str, NULL);
	}
	return;
}

#else /* USE_GFM */

/* XXX - From the Dark Ages before the File Chooser...


/* Set the File Panel Label */
static void
FilePanel_SETLABEL(
	ptr_t		ifp,
	FilePanel_Type	which,
	char		*str)
{
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, which)) == NULL)
		return;
	xv_set(panel, XV_LABEL, str, NULL);
}

/* Called for each file in directory to match audio files */
static int
file_filter_notify(
	gfm_popup_objects	*ip,
	char			*path)
{
	struct file_panel_data	*fp;
	char			fullpath[MAXPATHLEN+1];

	fp = (struct file_panel_data *)xv_get(ip->popup, XV_KEY_DATA, FILEKEY);

	if (fp->filt_proc) {
		/* XXX - tmp hack - should pass FULL path, only passes part */
		if (*path != '/') {
			strncpy(fullpath, 
				(char *) xv_get(ip->directory, PANEL_VALUE), 
				MAXPATHLEN);
			strncat(fullpath, "/", MAXPATHLEN);
			strncat(fullpath, path, MAXPATHLEN);
		} else {
			strncpy(fullpath, path, MAXPATHLEN);
		}
		return (*fp->filt_proc)(fp, fullpath, FALSE);
	} 
	return (1);
}	

/* Notify proc for the Open button */
static int
open_notify(
	gfm_popup_objects	*ip,
	char			*directory,
	char			*file)
{
	struct file_panel_data	*fp;
	char			path[MAXPATHLEN+1];

	fp = (struct file_panel_data *)xv_get(ip->popup, XV_KEY_DATA, FILEKEY);
	if (directory && *directory) {
		sprintf(path, "%s/%s", directory, file);
	} else {
		strncpy(path, file, MAXPATHLEN);
	}
	return (((*fp->open_proc)(fp, path) == TRUE) ? GFM_OK : GFM_ERROR);
}

/* Notify proc for the Save button */
static int
saveas_notify(
	gfm_popup_objects	*ip,
	char			*directory,
	char			*file)
{
	struct file_panel_data	*fp;
	char			path[MAXPATHLEN+1];

	if (directory && *directory) {
		sprintf(path, "%s/%s", directory, file);
	} else {
		strncpy(path, file, MAXPATHLEN);
	}
	fp = (struct file_panel_data *)xv_get(ip->popup, XV_KEY_DATA, FILEKEY);
	return (((*fp->save_proc)(fp, path, TRUE, NULL) == TRUE)
		? GFM_OK : GFM_ERROR);
}

/* Notify proc for the Include button */
static int
include_notify(
	gfm_popup_objects	*ip,
	char			*directory,
	char			*file)
{
	struct file_panel_data	*fp;
	char			path[MAXPATHLEN+1];

	if (directory && *directory) {
		sprintf(path, "%s/%s", directory, file);
	} else {
		strncpy(path, file, MAXPATHLEN);
	}
	fp = (struct file_panel_data *)xv_get(ip->popup, XV_KEY_DATA, FILEKEY);
	return (((*fp->include_proc)(fp, path) == TRUE) ? GFM_OK : GFM_ERROR);
}

/* Update the file size and filesystem free space message */
void
FilePanel_SETFILESIZESTR(
	ptr_t			ifp,
	char			*str)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp; 
	gfm_popup_objects	*ip;
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, LS_SaveAs)) == NULL)
		return;
	ip = (gfm_popup_objects *) xv_get(panel, XV_KEY_DATA, INSTANCE);
	gfm_setfilesizestr(ip, str);
}

/* stubs for 4.x or non-filechooser builds */

static Menu
save_format_notify(Menu menu, Menu_generate op)
{
}

static void
compression_notify(Panel_item item, int value, Event *event)
{
}

#endif /* USE_GFM */



/* Pop up the requested panel */
void
FilePanel_SHOW(
	ptr_t		ifp,
	FilePanel_Type	which)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp; 

#ifndef USE_GFM
	/* 
	 * Create the requested panel, if necessary, before popping it up.
	 * Always create the open panel first.
	 */
	if (fp->open_panel == NULL) {
		char	*cp;
		char	*path;
		char	*save_path;

		fp->open_panel = (ptr_t) xv_create((Xv_opaque)fp->owner,
		    FILE_CHOOSER_OPEN_DIALOG,
		    NULL);
		init_fcpanel(ifp, (Xv_opaque)fp->open_panel,
		    MGET("Audio Tool: Open"), open_notify, fp->opendir);
		xv_set((Xv_opaque)fp->open_panel,
		    FILE_CHOOSER_FILTER_FUNC, file_filter_notify,
		    FILE_CHOOSER_FILTER_MASK, FC_MATCHED_FILES_MASK,
		    NULL);

		ds_position_popup((Xv_opaque)fp->owner,
		    (Xv_opaque)fp->open_panel, DS_POPUP_LOR);

		/* pre-load the history list with $AUDIOPATH (if set) */
		if ((cp = getenv("AUDIOPATH")) &&
		    (save_path = path = strdup(cp))) {
			/* skip over any initial ":"'s */
			while (*path == ':') {
				path++;
			}

			while (path && *path) {
				if (cp = strchr(path, ':')) {
					/*  cp pts to next path elem */
					*cp++ = NULL; 
				}

				/* only add if not "." */
				if (*path && (strcmp(path, ".") != 0) &&
				    (strcmp(path, "./") != 0)) {
					xv_set((Xv_opaque)fp->open_panel,
					    FILE_CHOOSER_APP_DIR, path, path,
					    NULL);
				}

				/* go on to the next one */
				path = cp;
			}
			free (save_path);
		}
		add_show_files_extension(fp->open_panel);
	}

	switch (which) {
	default:
	case LS_Open:
		/* The open panel was created first */
		xv_set((Xv_opaque)fp->open_panel, FRAME_LEFT_FOOTER, "",
		    XV_SHOW, TRUE, NULL);
		break;

	case LS_Include:
		if (fp->include_panel == NULL) {
			fp->include_panel = (ptr_t)
			    xv_create((Xv_opaque)fp->owner,
			    FILE_CHOOSER_OPEN_DIALOG,
			    FILE_CHOOSER_CUSTOMIZE_OPEN,
			    MGET("Include"), NULL, FILE_CHOOSER_SELECT_FILES,
			    NULL);
			init_fcpanel(ifp, (Xv_opaque)fp->include_panel,
			    MGET("Audio Tool: Include"), include_notify,
			    (char*)xv_get((Xv_opaque)fp->open_panel,
			    FILE_CHOOSER_DIRECTORY));
			xv_set((Xv_opaque)fp->include_panel,
			    FILE_CHOOSER_FILTER_FUNC, file_filter_notify,
			    FILE_CHOOSER_FILTER_MASK, FC_MATCHED_FILES_MASK,
			    NULL);

			add_show_files_extension(fp->include_panel);

			ds_position_popup((Xv_opaque)fp->owner,
			    (Xv_opaque)fp->include_panel, DS_POPUP_LOR);
		}

		/*
		 * for some reason this always comes up with the same
		 * list of files as the load panel. re-read the directory
		 * so it re-runs the filter proc and filters out files
		 * that are not of the same type.
		 */
		xv_set((Xv_opaque)fp->include_panel, FRAME_LEFT_FOOTER, "",
		    FILE_CHOOSER_UPDATE, XV_SHOW, TRUE, NULL);
		break;

	case LS_Save:
	case LS_SaveAs:
		if (fp->saveas_panel == NULL) {
			fp->saveas_panel = (ptr_t)
			    xv_create((Xv_opaque)fp->owner,
			    FILE_CHOOSER_SAVEAS_DIALOG,
			    NULL);
			init_fcpanel(ifp, (Xv_opaque)fp->saveas_panel,
			    MGET("Audio Tool: Save As"), saveas_notify,
			    fp->savedir);
			xv_set((Xv_opaque)fp->saveas_panel,
			    FILE_CHOOSER_CD_FUNC, cd_func,
			    FILE_CHOOSER_FILTER_FUNC, file_filter_notify,
			    FILE_CHOOSER_FILTER_MASK, FC_MATCHED_FILES_MASK,
			    NULL);

			add_save_format_extension(fp->saveas_panel);

			ds_position_popup((Xv_opaque)fp->owner,
			    (Xv_opaque)fp->saveas_panel, DS_POPUP_LOR);

			/* set the initial format to the saved value */
			(void) FilePanel_SETFORMAT(fp, &fp->cur_hdr);
			FilePanel_SETCOMPRESSION(fp, fp->compression);

			/* make sure file size msg is up-to-date */
			AudPanel_Displaysize(AudPanel_KEYDATA(fp->owner));
		}
		xv_set((Xv_opaque)fp->saveas_panel, FRAME_LEFT_FOOTER, "",
		    XV_SHOW, TRUE, NULL);
		break;
	}

#else /* USE_GFM */
	gfm_popup_objects *ip;

	/* The first time, create the panel and position it */
	if (fp->open_panel == NULL) {
		ip = gfm_initialize(NULL, (Xv_opaque)fp->owner,
		MGET("Audio Tool: File Operations"));
		xv_set(ip->popup, XV_KEY_DATA, FILEKEY, fp, NULL);
		fp->open_panel = (ptr_t) ip->popup;

		ds_position_popup((Xv_opaque)fp->owner,
		    (Xv_opaque)fp->open_panel, DS_POPUP_LOR);
	}

	ip = (gfm_popup_objects *)
	    xv_get((Xv_opaque)fp->open_panel, XV_KEY_DATA, INSTANCE);

	switch (which) {
	case LS_Save:
	case LS_SaveAs:
		FilePanel_SETLABEL(fp, which, MGET("Audio Tool: SaveAs"));
		gfm_activate(ip, NULL, NULL, file_filter_notify, saveas_notify,
		    NULL, GFM_SAVE);
		break;
	case LS_Include:
		FilePanel_SETLABEL(fp, which, MGET("Audio Tool: Include"));
		gfm_activate(ip, NULL, NULL, file_filter_notify, include_notify,
		    NULL, GFM_LOAD);
		gfm_set_action(ip, MGET("Include"));
		break;
	case LS_Open:
	default:
		FilePanel_SETLABEL(fp, which, MGET("Audio Tool: Open"));
		gfm_activate(ip, NULL, NULL, file_filter_notify, open_notify,
		    NULL, GFM_LOAD);
		break;
	}
#endif /* USE_GFM */
}


/* 
 * This is what ultimately gets called when an item from the
 * SaveAs->Format menu is selected.
 * Returns FALSE if unidentifiable format.
 */
int
FilePanel_SETFORMAT(
	ptr_t			ifp,
	Audio_hdr		*hdrp)
{
#ifndef USE_GFM
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;
	fc_exten_panel_objects	*sfp;
	char			*label;

	label = (char*) FormatPanel_Getformatname(
	    AudPanel_FormatPanel_hndl(AudPanel_KEYDATA(fp->owner)), hdrp);
	if (label == NULL)
		return (FALSE);

	if (fp->saveas_panel) {
		sfp = (fc_exten_panel_objects *) xv_get(
		    (Xv_opaque)fp->saveas_panel, XV_KEY_DATA, FCEXTENKEY);
		if (sfp != NULL)
			xv_set(sfp->format, PANEL_VALUE, label, NULL);
	}
#endif /* !USE_GFM */
	return (TRUE);
}

/* Set encoding compression in the save panel */
void
FilePanel_SETCOMPRESSION(
	ptr_t			ifp,
	int			encoding)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

#ifndef USE_GFM
	if (fp->saveas_panel) {
		do_set_compression((Xv_opaque)fp->saveas_panel, encoding);
	}
#else
	gfm_popup_objects	*ip;
	int			setting;
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, LS_SaveAs)) == NULL)
		return;
	ip = (gfm_popup_objects *) xv_get(panel, XV_KEY_DATA, INSTANCE);
	if (encoding > 0) {
		xv_set(ip->format, PANEL_VALUE, encoding, NULL);
		fp->compression = encoding;
	}
#endif
}

/* Return the current directory for a File panel */
char*
FilePanel_GETDIRECTORY(
	ptr_t			ifp,
	FilePanel_Type		which)
{
#ifndef USE_GFM
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, which)) == NULL)
		return ("");
	return ((char*) xv_get(panel, FILE_CHOOSER_DIRECTORY));
#else
	return ("");
#endif
}

/* Set left footer label */
void
FilePanel_SETLEFTFOOTER(
	ptr_t			ifp,
	FilePanel_Type		which,
	char			*str)
{
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, which)) == NULL)
		return;
	xv_set(panel, FRAME_LEFT_FOOTER, str, NULL);
}

/* Take down the specified panel only if it is unpinned */
void
FilePanel_TAKEDOWN(
	ptr_t			ifp,
	FilePanel_Type		which)
{
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, which)) == NULL)
		return;
	if (xv_get(panel, FRAME_CMD_PIN_STATE) == FRAME_CMD_PIN_OUT) {
		xv_set(panel, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT,
		    XV_SHOW, FALSE, NULL);
	}
}

/* Return TRUE if panel exists and is being displayed */
int
FilePanel_ISMAPPED(
	ptr_t			ifp,
	FilePanel_Type		which)
{
	Xv_opaque		panel;

	if ((panel = getpanel(ifp, which)) == NULL)
		return (FALSE);
	return (xv_get(panel, XV_SHOW));
}


/* Set all panels busy except for their Cancel buttons */
void
FilePanel_ALLBUSY(
	ptr_t			ifp,
	int			busy)
{
#ifndef USE_GFM
	Xv_opaque		button;
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	fp->busy = busy;
	if (fp->open_panel) {
		if (button = xv_get((Xv_opaque)fp->open_panel,
		    FILE_CHOOSER_CHILD, FILE_CHOOSER_OPEN_BUTTON_CHILD)) {
			xv_set(button, PANEL_INACTIVE, busy, NULL);
		}
	}
	if (fp->include_panel) {
		if (button = xv_get((Xv_opaque)fp->include_panel,
		    FILE_CHOOSER_CHILD, FILE_CHOOSER_OPEN_BUTTON_CHILD)) {
			xv_set(button, PANEL_INACTIVE, busy, NULL);
		}
	}
	if (fp->saveas_panel) {
		if (button = xv_get((Xv_opaque)fp->saveas_panel,
		    FILE_CHOOSER_CHILD, FILE_CHOOSER_SAVE_BUTTON_CHILD)) {
			xv_set(button, PANEL_INACTIVE, busy, NULL);
		}
	}
#endif /* !USE_GFM */
}

/* Force a re-scan of all file panel directories */
void
FilePanel_ALLRESCAN(
	ptr_t			ifp)
{
#ifndef USE_GFM
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	if (fp->open_panel) {
		xv_set((Xv_opaque)fp->open_panel, FILE_CHOOSER_UPDATE, NULL);
	}
	if (fp->saveas_panel) {
		xv_set((Xv_opaque)fp->saveas_panel, FILE_CHOOSER_UPDATE, NULL);
	}
	if (fp->include_panel) {
		xv_set((Xv_opaque)fp->include_panel, FILE_CHOOSER_UPDATE, NULL);
	}
#endif /* !USE_GFM */
}


/* Dismiss all the file panels */
void
FilePanel_ALLUNSHOW(
	ptr_t			ifp)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	if (fp->open_panel) {
		xv_set((Xv_opaque)fp->open_panel, FRAME_CMD_PIN_STATE,
		    FRAME_CMD_PIN_OUT, XV_SHOW, FALSE, NULL);
	}
	if (fp->saveas_panel) {
		xv_set((Xv_opaque)fp->saveas_panel, FRAME_CMD_PIN_STATE,
		    FRAME_CMD_PIN_OUT, XV_SHOW, FALSE, NULL);
	}
	if (fp->include_panel) {
		xv_set((Xv_opaque)fp->include_panel, FRAME_CMD_PIN_STATE,
		    FRAME_CMD_PIN_OUT, XV_SHOW, FALSE, NULL);
	}
}
