#pragma ident "@(#)globals.h	1.5 92/10/06"
/*
 *      Copyright (C) 1991  Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on some portions of this source
 *       code product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */
/*
 *
 *
 *		       COPYRIGHT 1987, 1989
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */

#ifdef MAIN
#define ext
#else
#define ext extern
#endif

ext Display	*theDisplay;	/* Display variable. */
ext Widget	toplevel;	/* The top level widget (A hack %%%). */
ext char	*progName;	/* Program name. */
ext char	*homeDir;	/* User's home directory. */
ext Atom	wm_delete_window;	/* see ICCCM on Window Deletion */

ext struct _resources {
    Boolean	debug;
    char	*mail_path;		/* mh's mail directory. */
    char	*temp_dir;		/* Directory for temporary files. */
    char	*mh_path;		/* Path for mh commands. */
    char	*initial_folder_name;	/* Initial folder to use. */
    char	*initial_inc_file;	/* -file for inc on initial folder */
    char	*insert_filter;		/* Insert message filter command */
    char	*drafts_folder_name;	/* Folder for drafts. */
    int		send_line_width;	/* How long to break lines on send. */
    int		break_send_line_width;	/* Minimum length of a line before
					   we'll break it. */
    char	*print_command;	/* Printing command. */
    int		toc_width;	/* How many characters wide to use in tocs */
    Boolean	skip_deleted;		/* If true, skip over deleted msgs. */
    Boolean	skip_moved;		/* If true, skip over moved msgs. */
    Boolean	skip_copied;		/* If true, skip over copied msgs. */
    Boolean	hide_boring_headers;
    char	*geometry;	/* Default geometry to use for things. */
    char	*toc_geometry;
    char	*view_geometry;
    char	*comp_geometry;
    char	*pick_geometry;
    int		toc_percentage;
    Boolean	new_mail_check;		/* Whether to check for new mail. */
    Boolean	make_checkpoints;     /* Whether to create checkpoint files. */
    int		check_frequency;	/* in minutes, of new mail check */
    int		mail_waiting_flag;	/* If true, change icon on new mail */
    Cursor	cursor;			/* application cursor */
    Pixel	pointer_color;		/* application cursor color */
    Boolean	sticky_menu;		/* command menu entries are sticky? */
    Boolean	prefix_wm_and_icon_name;/* prefix wm names with progName ? */
    Boolean	reverse_read_order;	/* decrement counter to next msg ? */
    Boolean	block_events_on_busy;	/* disallow user input while busy ? */
    Cursor	busy_cursor;		/* the cursor while input blocked */
    Pixel	busy_pointer_color;	/* busy cursor color */
    int		command_button_count;	/* number of buttons in command box */
    int		app_defaults_version;	/* for sanity check */
    char 	*banner;		/* defaults to xmh version string */
} app_resources;

ext Boolean     allowPopdown;   /* Allow the CreateFolder popup to come down */
ext char	*draftFile;	/* Filename of draft. */
ext char	*xmhDraftFile;	/* Filename for sending. */
ext Toc		*folderList;	/* Array of folders. */
ext int		numFolders;	/* Number of entries in above array. */
ext Toc		InitialFolder;	/* Toc containing initial folder. */
ext Toc		DraftsFolder;	/* Toc containing drafts. */
ext Scrn	*scrnList;	/* Array of scrns in use. */
ext int		numScrns;	/* Number of scrns in above array. */
ext Widget	NoMenuForButton;/* Flag menu widget value: no menu */
ext Widget	LastMenuButtonPressed;	/* to `toggle' menu buttons */
ext Widget      NullSource;	/* null text widget source */
ext Dimension	rootwidth;	/* Dimensions of root window.  */
ext Dimension	rootheight;
ext Pixmap	NoMailPixmap;	/* Icon pixmap if no new mail. */
ext Pixmap	NewMailPixmap;	/* Icon pixmap if new mail. */
ext Pixmap	MenuItemBitmap;	/* Options menu item checkmark */
ext XtTranslations NoTextSearchAndReplace; /* no-op ^S and ^R in Text */

ext struct _LastInput {
    Window win;
    int x;
    int y;
} lastInput;

ext Boolean	subProcessRunning; /* interlock for DoCommand/CheckMail */

#define PNullSource (NullSource != NULL ? NullSource : \
(NullSource = (Widget)  CreateFileSource(scrn->viewlabel, "/dev/null", False)))


typedef struct _XmhMenuEntry {
    char	*name;			/* menu entry name */
    void   	(*function)();		/* menu entry callback function */
} XmhMenuEntryRec, *XmhMenuEntry;	


typedef struct _XmhMenuButtonDesc {
    char	*button_name;		/* menu button name */
    char	*menu_name;		/* menu name */
    int		id;			/* an internal key */
    XmhMenuEntry entry;			/* list of menu entries */
    Cardinal	num_entries;		/* count of menu entries in list */
} XmhMenuButtonDescRec, *XmhMenuButtonDesc;


extern XmhMenuEntryRec	folderMenu[];
extern XmhMenuEntryRec	tocMenu[];
extern XmhMenuEntryRec	messageMenu[];
extern XmhMenuEntryRec	sequenceMenu[];
extern XmhMenuEntryRec	viewMenu[];
extern XmhMenuEntryRec	optionMenu[];

extern XmhMenuButtonDescRec	MenuBoxButtons[];

/* Used as indices into MenuBoxButtons; these must correspond. */

#define XMH_FOLDER	0
#define XMH_TOC		1
#define XMH_MESSAGE	2
#define	XMH_SEQUENCE	3
#define XMH_VIEW	4
#define XMH_OPTION	5

