#ifndef	_XOL_FILECHP_H
#define	_XOL_FILECHP_H

#pragma	ident	"@(#)FileChP.h	1.10	93/02/25 include/Xol SMI"	/* OLIT 493	*/

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


/************************************************************************
 *
 *	Private interface of the file chooser panel widget
 *		
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <limits.h>		/* MAX_PATH */

#ifndef	MAX_PATH
	#include <sys/param.h>	/* MAXPATHLEN */
#endif	/* MAX_PATH */

#include <sys/types.h>		/* boolean_t, uid_t, gid_t */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>

#include <Xol/OpenLookP.h>

#include <Xol/ManagerP.h>
#include <Xol/RubberTilP.h>
#include <Xol/ScrollingL.h>	/* OlListToken */

/* Import the public API of this widget */
#include <Xol/FileCh.h>


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *	Widget private manifest constants and macros definitions
 *
 ************************************************************************/

#ifdef	MAX_PATH
	#define	BIGGEST_PATH	(MAX_PATH + 1)
#else
	#define	BIGGEST_PATH	(MAXPATHLEN)
#endif	/* MAX_PATH */

/* Dynamic resources bit masks */
#define	OL_FILECH_FOREGROUND	(1 << 0)
#define	OL_FILECH_FONTCOLOR	(1 << 1)


/* Define the instance part */
typedef struct _FileChooserPart {

/* New resource fields */
	
	/* State */
	OlStrRep		text_format;
	OlFont			font;
	Pixel			font_color;
	Pixel			foreground;
	Pixel			input_focus_color;
	int			scale;
	OlDefine		operation;
	String			current_folder;
	String			last_document_name;
	Cardinal		list_visible_item_min_count;
	Cardinal		list_visible_item_count;
	Boolean			show_glyphs;
	Boolean			follow_symlinks;
	Boolean			no_type_in_acceleration;
	Boolean			allow_incremental_search;
	
	/* Standard Callbacks */
	XtCallbackList		open_folder_callback;
	XtCallbackList		input_document_callback;
	XtCallbackList		output_document_callback;
	
	/* Filtering */
	String			filter_string;
	XtCallbackProc		filter_proc;
	Boolean			show_inactive;
	Boolean			hide_dot_files;
	
	/* Go to */
	String			home_folder;
	Cardinal		application_folders_max_count;
	OlFolderList		application_folders;
	Cardinal		user_folders_max_count;
	OlFolderList		user_folders;
	Cardinal		history_folders_min_count;
	Cardinal		history_folders_max_count;
	OlFolderList		history_folders;
	
	/* Sorting */
	OlComparisonFunc	comparison_func;
	
	/* Path Name Processing */
	Boolean			expand_tilde;
	Boolean			substitute_shell_variables;
	
	/* Accelerators and Mnemonics */
	String			goto_home_accelerator;
	String			go_up_one_folder_accelerator;
	String			cancel_accelerator;
	String			open_accelerator;
	String			open_folder_accelerator;
	String			save_accelerator;
	String			save_as_accelerator;
	String			include_accelerator;
	String			command_accelerator;
	
	OlMnemonic		goto_home_mnemonic;
	OlMnemonic		cancel_mnemonic;
	OlMnemonic		open_mnemonic;
	OlMnemonic		open_folder_mnemonic;
	OlMnemonic		save_mnemonic;
	OlMnemonic		save_as_mnemonic;
	OlMnemonic		include_mnemonic;
	OlMnemonic		command_mnemonic;
	
	/* Extension Container */
	String			extension_name;
	WidgetClass		extension_class;
	Widget			extension_widget;

	/* Component Access */
	Widget			goto_prompt_widget;
	Widget			goto_button_widget;
	Widget			goto_home_button_widget;
	Widget			goto_type_in_widget;
	Widget			current_folder_label_widget;
	Widget			current_folder_widget;
	Widget			list_prompt_widget;
	Widget			document_list_widget;
	Widget			document_name_type_in_widget;
	Widget			open_button_widget;
	Widget			cancel_button_widget;
	Widget			command_button_widget;
	
	/* Extensibility Callbacks */
	XtCallbackList		list_choice_callback;
	XtCallbackList		folder_opened_callback;
	XtCallbackList		cancel_callback;
	
	/* Labels */
	OlStr			goto_prompt_string;
	OlStr			goto_label;
	OlStr			goto_home_label;

	OlStr			current_folder_label_string;

	OlStr			open_prompt_string;
	OlStr			folder_prompt_string;
	OlStr			document_prompt_string;
	OlStr			command_prompt_string;

	OlStr			go_up_one_folder_label;
	
	OlStr			open_folder_label;
	OlStr			cancel_label;
	OlStr			open_label;
	OlStr			save_label;
	OlStr			save_as_label;
	OlStr			include_label;
	OlStr			command_label;
	
	OlStr			default_document_name;
	OlStr			default_document_suffix;

	/* New internal fields */
	
	/* The API does not provide access to these widgets */
	Widget			shell;
	Widget			notice_text_widget;

	Widget			goto_menu_widget;
	Widget			user_folders_spacer_b;
	Cardinal		user_folders_count;
	WidgetList		user_folders_oba;
	Widget			application_folders_spacer_b;
	WidgetList		application_folders_oba;
	Cardinal		application_folders_count;
	Widget			history_folders_spacer_b;
	WidgetList		history_folders_oba;
	Cardinal		folder_index;

	Widget			goto_f,
				current_folder_f,
				document_f,
				command_center_rt,
					buttons_left_c,
					buttons_right_c,
				buttom_spacer_c
				;

	Dimension*		x_dimensions;
	Dimension*		y_dimensions;
	
	OlStr			list_prompt_string;
	OlStr			document_name_type_in_label;
	OlStr			document_name_type_in_string;

	unsigned char		dynamic_resources_flags;

	OlStat			home_folder_stat_bufferp;
	OlStat			current_folder_stat_bufferp;

	XtPointer		ring;

	/* scrolling list state */
	OlDefine		show_mnemonics;
	Modifiers		mnemonic_modifiers;
	Boolean			filled;
	OlFNavNode		chosen_item_node;
	XtPointer		tree;

	/* glyphs */
	OlGlyph			folder_glyph;
	OlGlyph			document_glyph;
	OlGlyph			go_up_glyph;

	/* user identification info */
	uid_t			euid;
	gid_t			egid;
	const gid_t*		groups;
	int			ngroups;

} FileChooserPart;

/* Define the full instance record */
typedef struct _FileChooserRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	RubberTilePart		rubber_tile;
	FileChooserPart		file_chooser;
} FileChooserRec;

/* constraint record */
typedef struct _FileChooserConstraintPart {
	XtPointer		extension;
} FileChooserConstraintPart;

typedef struct _FileChooserConstraintRec {
	RubberTileConstraintRec	rubber_tile;
	FileChooserConstraintPart
				file_chooser;
} *FileChooserConstraint, FileChooserConstraintRec;


/* Define new types for new class methods */
/* None */

/* Define class part structure */
typedef struct _FileChooserClassPart {
	XtPointer		extension;
} FileChooserClassPart;

/* Define the full class record */
typedef struct _FileChooserClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	RubberTileClassPart	rubber_tile_class;
	FileChooserClassPart	file_chooser_class;
} FileChooserClassRec;

/* External definition for class record */
extern FileChooserClassRec	fileChooserClassRec;

/* Inheritance constants for new class methods */
/* None */


#ifdef	__cplusplus
}
#endif


/* end of %M */
#endif	/* _XOL_FILECHP_H */
