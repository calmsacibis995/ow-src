#ifndef	_XOL_DYNAMICP_H
#define	_XOL_DYNAMICP_H

#pragma	ident	"@(#)DynamicP.h	302.6	93/12/16 include/Xol SMI"	/* olmisc:DynamicP.h 1.13	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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


#include <Xol/Dynamic.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define XtROlKeyDef		"OlKeyDef"
#define XtROlBtnDef		"OlBtnDef"

#define MAXDEFS			2
#define MORESLOTS               4
#define BUFFER_SIZE		64

#define ABS_DELTA(x1, x2)	(x1 < x2 ? x2 - x1 : x1 - x2)

#define IsDampableKey(flag, k)	(flag == True && IsCursorKey(k))

#define CanBeBound(flag,keysym,modifier) \
				(flag == False || keysym >= 0x1000 || \
				(modifier & ~(ShiftMask | dont_care_bits)))


typedef unsigned int	BtnSym;

typedef struct _OlKeyDef {
	int			used;
	Modifiers		modifier[MAXDEFS];
	KeySym			keysym[MAXDEFS];
}			OlKeyDef;

typedef struct _OlBtnDef {
	int			used;
	Modifiers		modifier[MAXDEFS];
	BtnSym			button[MAXDEFS];
}			OlBtnDef;

#if	defined(__STDC__) || defined(__cplusplus)

	typedef struct _OlKeyBinding {
		const char*	name;		/* XtN string */
		const char*	default_value;	/* `,' sperated string, 
						   two most */
		OlInputEvent	ol_event;
		OlKeyDef	def;
	}			OlKeyBinding;

	typedef struct _OlBtnBinding {
		const char*	name;		/* XtN string */
		const char*	default_value;	/* `,' sperated string,
							   two most */
		OlInputEvent	ol_event;
		OlBtnDef	def;
	}			OlBtnBinding;

	typedef struct mapping {
		const char*	s;
		unsigned long	m;
	}			mapping;

#else	/* __STDC__ || __cplusplus */

	typedef struct _OlKeyBinding {
		char*		name;
		char*		default_value;
		OlInputEvent	ol_event;
		OlKeyDef	def;
	}			OlKeyBinding;

	typedef struct _OlBtnBinding {
		char*		name;
		char*		default_value;
		OlInputEvent	ol_event;
		OlBtnDef	def;
	}			OlBtnBinding;

	typedef struct mapping {
		char*		s;
		unsigned long	m;
	}			mapping;

#endif	/* __STDC__ || __cplusplus */

typedef enum { Alt, Meta, Hyper, Super, NumLock, ModeSwitch } ModifierType;

typedef struct {
	ModifierType		modifier_type;
	String			modifier_name;
	KeySym			left_keysym;
	KeySym			right_keysym;
	KeyCode			left_keycode;
	KeyCode			right_keycode;
	Modifiers		modifier;
}			ModifierInfo;

typedef struct {
	KeySym			keysym;
	KeyCode*			keycodelist;
	Cardinal		keycount;
}			KeypadInfo;

typedef struct _btn_mapping {
	unsigned long		button;
	unsigned long		button_mask;
}			btn_mapping;

typedef struct _Token {
	short			i;
	short			j;
}			Token;

typedef enum _OlDBType {
	CoreDB = (1 << 0),
	TextDB = (1 << 1),
	UnboundDB = (1 << 8),
	WidgetDB = (1 << 9)
} OlDBType;

#define IsWidgetDB(db_type)	((db_type & WidgetDB) == WidgetDB)
#define IsUnboundDB(db_type)	((db_type & UnboundDB) == UnboundDB)
#define IsCoreDB(db_type)       ((db_type & CoreDB) == CoreDB)
#define IsTextDB(db_type)       ((db_type & TextDB) == TextDB)

typedef struct _OlVirtualEventInfo {
	Display*		dpy;
	OlDBType		db_type;
	int			refcnt;
	OlKeyBinding*		key_bindings;
	OlBtnBinding*		btn_bindings;
	char			num_key_bindings;
	char			num_btn_bindings;
	Token*			sorted_key_db;
}			OlVirtualEventInfo;

typedef struct _OlClassSearchRec {
	WidgetClass		wc;
	OlVirtualEventInfo*	db;
}			OlClassSearchRec,* OlClassSearchInfo;

typedef struct _OlWidgetSearchRec {
	Widget			w;
	OlVirtualEventInfo*	db;
}			OlWidgetSearchRec,* OlWidgetSearchInfo;

typedef struct GrabbedVirtualKey {
	Widget			w;
	OlVirtualName		vkey;
	OlKeyBinding*		kb;
	OlKeyDef		as_grabbed;
	Boolean			grabbed;
	Boolean			owner_events;
	int			pointer_mode;
	int			keyboard_mode;
}			GrabbedVirtualKey;

typedef struct {
	Cardinal		mouse_damping_factor;
	Cardinal		multi_click_timeout;
	Cardinal		key_remap_timeout;
}			LocalData;

typedef struct _DynamicCallback {
	OlDynamicCallbackProc	CB;
	XtPointer		data;
}			DynamicCallback;

typedef struct CharKeysymMap {
	char			single;
	KeySym			keysym;
} CharKeysymMap;

typedef struct _ComposeData {
	Widget		widget;
	Widget		vendor;
	Time		time;
	char		buf[10];
	int		buf_len;
	KeySym		keysym;
	XComposeStatus	cstatus;
} ComposeData, *ComposeDataPtr;

/*
 * new for multiple display support
 */
typedef struct _PerDisplayVEDBInfo {
	Display*			dpy;
	Widget			appl_shell;
	Boolean			doing_copy;

	OlVirtualEventTable*	db_stack;
	short			db_stack_slots_left;
	short			db_stack_slots_alloced;
	short			db_stack_entries;

	OlVirtualEventTable*	avail_dbs;
	short			avail_dbs_slots_left;
	short			avail_dbs_slots_alloced;
	short			avail_dbs_entries;

	OlWidgetSearchInfo	wid_list;
	short			wid_list_slots_left;
	short			wid_list_slots_alloced;
	short			wid_list_entries;

	OlClassSearchInfo	wc_list;
	short			wc_list_slots_left;
	short			wc_list_slots_alloced;
	short			wc_list_entries;

	OlVirtualEventTable	OlCoreDB;
	OlVirtualEventTable	OlTextDB;

	ModifierInfo*		mod_info;
	KeypadInfo*		keypad_table;
}			PerDisplayVEDBInfo,* PerDisplayVEDBInfoPtr;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlCleanupPDVEDB(Display* dpy);
extern String		_OlGetModifierNames(Display* dpy, Modifiers modifier);
extern BtnSym		_OlStringToButton(String);
extern Modifiers	_OlGetModifierBinding(Display *dpy,
					      ModifierType modifier_type);
				
#else	/* __STDC__ || __cplusplus */

extern void		_OlCleanupPDVEDB();
extern String _OlGetModifierNames();
extern BtnSym		_OlStringToButton();
extern Modifiers	_OlGetModifierBinding();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DYNAMICP_H */
