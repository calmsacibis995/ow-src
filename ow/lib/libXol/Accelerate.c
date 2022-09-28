#pragma ident	"@(#)Accelerate.c	302.13	97/03/26 lib/libXol SMI"	/* mouseless:Accelerate.c 1.22	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <libintl.h>
#include <ctype.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/AcceleratP.h>
#include <Xol/Converters.h>
#include <Xol/DynamicP.h>
#include <Xol/EventObjP.h>
#include <Xol/Menu.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>	/* new multiple display support */
#include <Xol/Util.h>
#include <Xol/VendorI.h>



/*
 * Macros:
 */

/*
 * Types of actions that can be performed on an
 * accelerator or mnemonic (i.e. on a KeyEvent):
 */
#define KE_ADD			1
#define KE_FETCH		2
#define KE_REMOVE		3
#define KE_REMOVE_ALL		4   /* ingore key and data in search */
#define KE_REMOVE_USING_DATA	5   /* use only data in searching	*/


/*
 * Local routines:
 */

    static OlDefine		AddAorM (Widget w, XtPointer data, String a_or_m, Boolean is_mnemonic);
static void		RemoveAorM (Widget w, XtPointer data, Boolean ignore_data, String a_or_m, Boolean is_mnemonic);
static Widget		FetchAorM (Widget w, XtPointer *p_data, OlVirtualEvent ve, Boolean is_mnemonic);
static Widget ActOnAorM (Widget w, XtPointer data, KeyEvent *item, Boolean is_mnemonic, int action);
static KeyEvent *	StringToKeyEvent (Widget w, String str);
static KeyEvent *	OlEventToKeyEvent (Widget w, OlVirtualEvent ve);
static KeyEvent *	FindKeyEvent (
	KeyEvent *		base,
	register KeyEvent *	item,
	Cardinal		nel,
	KeyEvent **		p_insert
);
static OlVendorPartExtension	FetchVendorExtension (Widget w, Boolean is_mnemonic, Boolean *p_use_mnemonic_prefix, Boolean *p_accelerators_do_grab);
static Boolean		UseMnemonicPrefix (
	Widget			w
);
static void		GrabAccelerator (Widget w, KeyEvent *ke, Widget *shells, Cardinal nshells, Boolean grab);
void			_OlNewAcceleratorResourceValues (
	Screen *		screen,
	XtPointer		client_data
);
static void		RegisterChangeShowAccelerator (Widget w, Boolean is_mnemonic);
static void		UnregisterChangeShowAccelerator (
	Widget			w
);

/*
 * Local data:
 */

typedef struct MaskName {
	Cardinal		p_name_offset;
	Modifiers		mask;
}			MaskName;

static MaskName		mask_names[] = {

#define offset(F) XtOffsetOf(_OlAppAttributes,F)
	{ offset(shift_name),   (Modifiers)ShiftMask   },
	{ offset(lock_name),    (Modifiers)LockMask    },
	{ offset(control_name), (Modifiers)ControlMask },
	{ offset(mod1_name),    (Modifiers)Mod1Mask    },
	{ offset(mod2_name),    (Modifiers)Mod2Mask    },
	{ offset(mod3_name),    (Modifiers)Mod3Mask    },
	{ offset(mod4_name),    (Modifiers)Mod4Mask    },
	{ offset(mod5_name),    (Modifiers)Mod5Mask    },
#undef offset

};

#define KEYSYM_POSITION 1
static char		fake_accelerator[4]	= { LBRA, 'X', RBRA, 0 };

/**
 ** _OlDestroyKeyboardHooks()
 **/

void
_OlDestroyKeyboardHooks (Widget w)
{
	/*
	 * Remove all mnemonics and accelerators.
	 */
	RemoveAorM (w, (XtPointer)0, True, (String)0, True);
	RemoveAorM (w, (XtPointer)0, True, (String)0, False);

	/*
	 * Disassociate this widget from all others.
	 */
	OlUnassociateWidget (w);

	/*
	 * Remove from traversal list.
	 */
	_OlDeleteDescendant (w);

	/*
	 * If this is the default widget, make default widget NULL.
	 */
	_OlSetDefault(w, False);

	return;
}

/**
 ** _OlMakeAcceleratorText()
 **/

void
_OlMakeAcceleratorText (Widget w, String str, String *qualifier_text, 
    Boolean *meta_key, String *accelerator_text)
{
	KeyEvent		*ke = StringToKeyEvent(w, str);
	Display			*dpy = XtDisplayOfObject(w);

	Cardinal		len = 0;
	Cardinal		i;

	String			detail;
	static char		single[2] = { 0 , 0 };

	Modifiers		meta_binding;

	KeyCode			keycode, foo_min_keycode;
	int			min_keycode, max_keycode, per;
	KeySym			keysym, *syms;
	_OlAppAttributes *	base	= _OlGetAppAttributesRef(w);

#define NAME(I)	*(String *)((char *)base + mask_names[(I)].p_name_offset)


	if (!ke || ke->keysym == NoSymbol)
	    return;

	if ((*single = _OlKeysymToSingleChar(ke->keysym)))
	    *accelerator_text = XtNewString(single);
	else if (detail = XKeysymToString(ke->keysym)) {
	    *accelerator_text = XtNewString(detail);
	    /*
	     * OpenLook Menu Accelerator Specifies that
	     * a 1 character alpha detail should be
	     * upper case.
	     */
	    if (islower((*accelerator_text)[0]) &&
		!(*accelerator_text)[1])
		(*accelerator_text)[0] = toupper((*accelerator_text)[0]);
	} else
	    return;

	/*
	 * Need to determine if the shift modifier has been 'folded' into
	 * the keysym by _OlCanonicalKeysym(). If this is the case,
	 * then "Shift" should be displayed in the accelerator text.
	 */
	syms = XtGetKeysymTable(dpy, &foo_min_keycode, &per);
	XDisplayKeycodes (dpy, &min_keycode, &max_keycode);

	for (keycode = (KeyCode)min_keycode, keysym = ke->keysym;
	     keycode <= (KeyCode)max_keycode;
	     keycode++, syms += per) {
	    if (per > 1 && keysym == syms[1]) {
		if (isascii(keysym) && !isalpha(keysym))
		    ke->modifiers |= ShiftMask;
		break;
	    }
	    else if (per == 1 || syms[1] == NoSymbol) {
		KeySym lower, upper;

		XtConvertCase(dpy, syms[0], &lower, &upper);
		if (keysym == lower)
		    break;
		else if (keysym == upper) {
		    if (isascii(keysym) && !isalpha(keysym))
			ke->modifiers |= ShiftMask;
		    break;
		}
	    }
	    else if (keysym == syms[0])
		break;
	}

	/*
	 * Count the number of bytes needed to form the accelerator text,
	 * then allocate that much space (plus one for our friend, the
	 * Terminating Null).
	 */
	*meta_key = False;
	meta_binding = _OlGetModifierBinding(dpy, Meta);

	if (meta_binding && ((ke->modifiers & meta_binding) == meta_binding)) {
		*meta_key = True;
		ke->modifiers &= ~meta_binding;
	}
	for (i = 0; i < XtNumber(mask_names); i++)
	    if (mask_names[i].mask & ke->modifiers)
		   	 len += Strlen(NAME(i)) + 1;

	if (len) {
	    *qualifier_text = XtMalloc(len + 1);
	    **qualifier_text = '\0';
	} else {
	    *qualifier_text = NULL;
	    return;
	}

	/*
	 * Now run through similar code, but copy the pieces this time.
	 */
	for (i = 0; i < XtNumber(mask_names); i++)
	    if (mask_names[i].mask & ke->modifiers) {
		    (void)strcat(*qualifier_text, NAME(i));
		    (void)strcat(*qualifier_text, "-");
	    }

	/* Remove last trailing "-" */
	(*qualifier_text)[len-1] = '\0';
#undef NAME
}

/**
 ** _OlAddMnemonic()
 **/

OlDefine
_OlAddMnemonic (Widget w, XtPointer data, char mnemonic)
{
	if (mnemonic) {
		fake_accelerator[KEYSYM_POSITION] = mnemonic;
		return (AddAorM(w, data, fake_accelerator, True));
	} else
		return (OL_BAD_KEY);
}

/**
 ** _OlRemoveMnemonic()
 **/

void
_OlRemoveMnemonic (Widget w, XtPointer data, Boolean ignore_data, char mnemonic)
{
	if (mnemonic) {
		fake_accelerator[KEYSYM_POSITION] = mnemonic;
		RemoveAorM (w, data, ignore_data, fake_accelerator, True);
	} else
		RemoveAorM (w, data, ignore_data, (String)0, True);
	return;
}

/**
 ** _OlFetchMnemonicOwner()
 **/

Widget
_OlFetchMnemonicOwner (Widget w, XtPointer *p_data, OlVirtualEvent virtual_event)
{
	if (virtual_event && OlQueryMnemonicDisplay(w) != OL_INACTIVE)
		return (FetchAorM(w, p_data, virtual_event, True));
	else
		return (0);
}

/**
 ** _OlAddAccelerator()
 **/

OlDefine
_OlAddAccelerator (Widget w, XtPointer data, String accelerator)
{
	OlDefine retval;
	char *buf;

	if (!_OlMenuAccelerators(w))
		return (OL_NEVER);

	if (!accelerator)
		return (OL_BAD_KEY);

	retval = AddAorM(w, data, accelerator, False);
	if (retval == OL_DUPLICATE_KEY) {
		/* Fix for 4008133 - Security risk in lib Xol	*/
		if (buf = malloc(256)) {
			snprintf (buf, 256, dgettext(OlMsgsDomain,
				"duplicate accelerator specified : %1$s"),
				accelerator);
			OlWarning (buf);
			free(buf);
		}
	}

	return (retval);
}

/**
 ** _OlRemoveAccelerator()
 **/

void
_OlRemoveAccelerator (Widget w, XtPointer data, Boolean ignore_data, String accelerator)
{
	RemoveAorM (w, data, ignore_data, accelerator, False);
	return;
}

/**
 ** _OlFetchAcceleratorOwner()
 **/

Widget
_OlFetchAcceleratorOwner (Widget w, XtPointer *p_data, OlVirtualEvent virtual_event)
{
	if (virtual_event && OlQueryAcceleratorDisplay(w) != OL_INACTIVE)
		return (FetchAorM(w, p_data, virtual_event, False));
	else
		return (0);
}

/**
 ** AddAorM()
 **/

static OlDefine
AddAorM (Widget w, XtPointer data, String a_or_m, Boolean is_mnemonic)
{
	KeyEvent *		ke	= StringToKeyEvent(w, a_or_m);

	Widget			owner;


	if (!ke)
		return (OL_BAD_KEY);
	else {
		owner = ActOnAorM(w, data, ke, is_mnemonic, KE_ADD);
		if (owner && owner != w)
			return (OL_DUPLICATE_KEY);
		if (!owner)
			RegisterChangeShowAccelerator (w, is_mnemonic);
		return (OL_SUCCESS);
	}
}

/**
 ** RemoveAorM()
 **/

static void
RemoveAorM (Widget w, XtPointer data, Boolean ignore_data, String a_or_m, Boolean is_mnemonic)
{
	KeyEvent *		ke;

	if (a_or_m) {
		if (ke = StringToKeyEvent(w, a_or_m))
			(void)ActOnAorM (w, data, ke, is_mnemonic, KE_REMOVE);
	} else if (ignore_data) {
		(void)ActOnAorM (
			w,
			data,
			(KeyEvent *)0,
			is_mnemonic,
			KE_REMOVE_ALL
		);
		UnregisterChangeShowAccelerator (w);
	} else
		(void)ActOnAorM (
			w,
			data,
			(KeyEvent *)0,
			is_mnemonic,
			KE_REMOVE_USING_DATA
		);

	return;
}

/**
 ** FetchAorM()
 **/

static Widget
FetchAorM (Widget w, XtPointer *p_data, OlVirtualEvent ve, Boolean is_mnemonic)
{
	KeyEvent *		ke	= OlEventToKeyEvent(w, ve);

	if (ke)
		return (ActOnAorM(w, (XtPointer)p_data, ke, is_mnemonic, KE_FETCH));
	else
		return (0);
}

/**
 ** ActOnAorM()
 **/

static Widget
ActOnAorM (Widget w, XtPointer data, KeyEvent *item, Boolean is_mnemonic, int action)
{
	_OlAppAttributes *	resources = _OlGetAppAttributesRef(w);

	OlVendorPartExtension	pe;

	KeyEvent *		base;
	KeyEvent *		insert;
	KeyEvent *		old;
	KeyEvent *		new;
	KeyEvent *		p	= 0;

	Cardinal		n;
	Cardinal		nel;
	Cardinal		insert_index;

	Widget			ret;

	Boolean			use_prefix;
	Boolean			do_grabs;

	Widget			shell;
	Widget			*shells;
	Cardinal		num_shells;
	Arg			args[2];


	pe = FetchVendorExtension(w, is_mnemonic, &use_prefix, &do_grabs);
	if (!pe)
		return (0);
	if (pe->accelerator_list) {
		base = pe->accelerator_list->base;
		nel  = pe->accelerator_list->nel;
	} else {
		base = 0;
		nel  = 0;
	}

	shell = _OlGetScreenShellOfWidget(w);
	XtSetArg(args[0], XtNshells, &shells);
	XtSetArg(args[1], XtNnumShells, &num_shells);
	XtGetValues(shell, args, XtNumber(args));
	
	if (item) {
		/*
		 * Mnemonic events get adjusted slightly:
		 *
		 *	- Mnemonics for objects inside (e.g.) menus
		 *	  don't need (but can optionally have) a prefix
		 *	  such as Alt.
		 *
		 *	- Mnemonics are case-insensitive.
		 *
		 * Since we get to this code for both registration and
		 * fetching of mnemonics, we are adjusting the events
		 * consistently. Note, though, that we don't add the
		 * mnemonic prefix for the FETCH case, otherwise what
		 * would be the point?
		 */
		if (is_mnemonic) {
			item->modifiers &= ~ShiftMask;
			if (use_prefix && action != KE_FETCH)
			    item->modifiers |= resources->mnemonic_modifiers;
			else if (!use_prefix && action == KE_FETCH)
			    item->modifiers &= ~resources->mnemonic_modifiers;
		}

		p = FindKeyEvent(base, item, nel, &insert);
	}

	switch (action) {

	case KE_ADD:
		if (p)
			return (p->w);
		nel++;

		/*
		 * Careful: "base" may change due to reallocation,
		 * so we have to realign "insert"--but only if it
		 * is not null!
		 */
		if (insert)
			insert_index = insert - base;
		base = Array((char*)base, KeyEvent, nel);
		if (insert)
			insert = base + insert_index;

		/*
		 * Note: "nel" is now equal to the new length of the list.
		 * "insert - base" is equal to the number of items before
		 * where the new item goes. Thus the difference is one
		 * more than the number of items after the new item. The
		 * "OlMemMove" below will move the latter items up, to
		 * make room for the new item at "insert".
		 */
		if (insert)
			(void) memmove(insert + 1, insert, 
				(nel - insert_index - 1) * sizeof(KeyEvent));
		else
			insert = base;

		*insert = *item;
		insert->is_mnemonic = is_mnemonic;
		insert->data        = data;
		insert->w           = w;

		/*
		 * If this is an accelerator and we have DoGrab turned
		 * on, then set grabs for this key on all top-level
		 * widgets.
		 */
		if (
			!is_mnemonic
		     && do_grabs
		     && shells
		     && num_shells
		) {
			GrabAccelerator (
				w,
				item,
				shells,
				num_shells,
				True
			);
			insert->grabbed = True;
		} else
			insert->grabbed = False;

		ret = 0;
		break;

	case KE_REMOVE:
		if (!p)
			return (0);

		/*
		 * If this was an accelerator and we had DoGrab turned
		 * on, then turn off grabs for this key on all top-level
		 * widgets.
		 */
		if (
			!p->is_mnemonic
		     && p->grabbed
		     && shells
		     && num_shells
		)
			GrabAccelerator (
				w,
				p,
				shells,
				num_shells,
				False
			);

		nel--;

		/*
		 * Note: "nel" is now the new size of the list (sans
		 * item to be deleted).
		 * "p - base" equals the number of items before the
		 * item to be deleted. Thus the difference is
		 * the number of items after the one to be deleted.
		 * The following will move the latter items down, to
		 * cover up (delete) the defunct item.
		 */
		if (nel)
			(void) memmove(p, p + 1, 
				(nel - (p - base)) * sizeof(KeyEvent));
		base = Array((char*)base, KeyEvent, nel);

		ret = 0;
		break;

	case KE_REMOVE_USING_DATA:		/* FALLTHROUGH */
	case KE_REMOVE_ALL:
		for (old = new = base, n = 0; n < nel; old++, n++) {
			if (
				old->w == w
			     && old->is_mnemonic == is_mnemonic
			     && (action == KE_REMOVE_ALL || old->data == data)
			) {
				if (
					!old->is_mnemonic
				     && old->grabbed
				     && shells
				     && num_shells
				)
					GrabAccelerator (
						w,
						old,
						shells,
						num_shells,
						False
					);
			} else
				*new++ = *old;
		}
		nel = new - base;
		base = Array((char*)base, KeyEvent, nel);
		break;

	case KE_FETCH:
		if (p && p->is_mnemonic == is_mnemonic) {
			if (data)
				*(XtPointer *)data = p->data;
			return (p->w);
		} else
			return (0);

	}

	/*
	 * Getting here means we added or deleted an item, or
	 * have freed the storage space.
	 * Failures, or the fetch case, have already returned.
	 */
	if (base) {
		if (!pe->accelerator_list)
			pe->accelerator_list = XtNew(OlAcceleratorList);
		pe->accelerator_list->base = base;
		pe->accelerator_list->nel  = nel;
	} else {
		if (pe->accelerator_list)
			XtFree((char*)pe->accelerator_list);
		pe->accelerator_list = 0;
	}

	return (ret);
}

/**
 ** StringToKeyEvent()
 **/

static KeyEvent *
StringToKeyEvent (Widget w, String str)
{
	DeclareConversionClass (XtDisplayOfObject(w), "stringToKeyEvent",
					(String)0);

	static KeyEvent		ret	= { 0 };

	XrmValue		from;
	XrmValue		to;

	OlKeyDef *		kd;

	/* if its starts with 'coreset', look for coreset resource */
	if (!strncasecmp(str, "coreset", sizeof("coreset") - 1)) {
	    char *funcname = NULL;
		char *resname = NULL;
	    XrmValue value;
	    char *strtype;
	    
		if ((funcname = (char *) malloc(100)) &&
			(resname = (char *) malloc(100))) {
			*funcname = '\0';
			sscanf(str, "%*s%s", funcname);
			snprintf(resname, 100, "OpenWindows.MenuAccelerator.%s", funcname);
			if( False == XrmGetResource(XtDatabase(XtDisplayOfObject(w)),
						resname, "*", &strtype, &value))
			return(0);
			/*
			 * If the database contains a match for this coreset name,
			 * then pass this string to the type converter.
			 */
			from.addr = value.addr;
			from.size = Strlen(value.addr) + 1;
			to.addr = 0;
		}
		if (funcname) free (funcname);
		if (resname) free (resname);
		
	}
	else { /* not a coreset binding */
	    from.addr = (XtPointer)str;
	    from.size = Strlen(str) + 1;
	    to.addr   = 0;
	}

	if (!XtCallConverter(
			     XtDisplayOfObject(w),
			     _OlStringToOlKeyDef,
			     (XrmValuePtr)NULL,
			     (Cardinal)0,
			     &from,
			     &to,
			     (XtCacheRef)NULL
			     ))
	    return (NULL);
	
	kd = (OlKeyDef *)to.addr;
	if (kd->used != 1) {
	    static String		_args[2]  = { 0 , 0 };
	    Cardinal		_num_args = 1;
	    
	    _args[0] = str;
	    ConversionWarning (
			       "illegalSyntax",
			       "String to KeyEvent found multiple keys: %s",
			       _args,
			       &_num_args
			       );
	    return (0);
	}
	
	ret.modifiers = kd->modifier[0];
	ret.keysym    = kd->keysym[0];

	if (ret.keysym == NoSymbol)
		return (0);
	else
		return (&ret);
}

/**
 ** OlEventToKeyEvent()
 **/

static KeyEvent *
OlEventToKeyEvent (Widget w, OlVirtualEvent ve)
{
	static KeyEvent		ret	= { 0 };

	KeySym			lower;
	KeySym			upper;

	KeySym *		syms;

	KeyCode			min_keycode;	/* not int, when Xt */

	int			per;

	/*
	 * Save the detail (even if the event is a ``virtual'' one),
	 * because we may have to compare this to a specific (non-virtual)
	 * accelerator.
	 *
	 * We ignore the "keysym" given in the virtual event structure,
	 * because it may have some of the modifiers already ``folded
	 * in''. We instead convert the naked KeyCode to a KeySym.
	 * Furthermore, because the KeyCode itself may have some implied
	 * modifiers (some servers give a different KeyCode for <A>
	 * than for <a>, where <A> == Shift<a>), we also convert to
	 * the lowest denominator, canonical form.
	 *
	 * Note: If we do have the situation where the KeyCode is <A>,
	 * the modifiers will already include ShiftMask.
	 *
	 * Further note: For some shifted keys we fold the ShiftMask
	 * back into the keysym, for example:
	 *
	 *	Shift<1>   ->	<!>
	 *	Shift<'>   ->	<">
	 *
	 * but we leave these alone:
	 *
	 *	Shift<m>, not <M>
	 *	Shift<F2>, not <F14>
	 *
	 * See _OlCanonicalKeysym() in Dynamic.c.
	 *
	 * (I18N)
	 */

	ret.modifiers = ve->xevent->xkey.state & ~ve->dont_care;

	syms = XtGetKeysymTable(XtDisplayOfObject(w), &min_keycode, &per);
	syms += (ve->xevent->xkey.keycode - min_keycode) * per;

	if (per > 1 && syms[1] != NoSymbol) {
		lower = syms[0];
		upper = syms[1];
	} else
		XtConvertCase (XtDisplayOfObject(w), syms[0], &lower, &upper);
	if (
		(ret.modifiers & ShiftMask)
	     && isascii(upper)		/* e.g. not <F14> */
	     && !isalpha(upper)		/* e.g. not <M>   */
	) {
		ret.keysym = upper;
		ret.modifiers &= ~ShiftMask;
	} else
		ret.keysym = lower;

	return (&ret);
}

/**
 ** FindKeyEvent()
 **/

static KeyEvent *
FindKeyEvent (KeyEvent *base, register KeyEvent *item, Cardinal nel, KeyEvent **p_insert)
{
    register KeyEvent *	low		= base;
    register KeyEvent *	high		= low + (nel - 1);
    register KeyEvent *	p;

    /*
     * The easy case.
     */
    if (!base) {
	if (p_insert)
	    *p_insert = 0;
	return (0);
    }

#define K_EQ(a,b) (a->keysym == b->keysym)
#define M_EQ(a,b) (a->modifiers == b->modifiers)
#define K_LT(a,b) (a->keysym < b->keysym)
#define M_LT(a,b) (a->modifiers < b->modifiers)
    
#define SPEC_EQ(a,b) (K_EQ(a,b) && M_EQ(a,b))
#define SPEC_LT(a,b) (K_LT(a,b) || K_EQ(a,b) && M_LT(a,b))
    
    while (high >= low) {
	p = low + ((high - low) / 2);
	
	/*
	 * If we find that the specific-event information matches,
	 * AND it's real, that's good.
	 */
	if (SPEC_EQ(item, p) && p->keysym != NoSymbol)
	    return (p);
	
	/*
	 * At this point, "p" always points within the ordered
	 * list of items. When the loop ends with no match,
	 * the item we failed to find can be inserted either
	 * immediately before or immediately after "p".
	 */
	if (SPEC_LT(item, p))
	    high = p - 1;
	else
	    low = p + 1;
    }
    
    if (p_insert) {
	/*
	 * As suggested above, the new item should be inserted
	 * either after or before the item at "p". Adjust "p",
	 * if needed, so that it points to where the item should
	 * be inserted, once the existing items starting with "p"
	 * are shifted up to make room.
	 */
	if (!SPEC_LT(item, p))
	    p++;
	*p_insert = p;
    }
    return (0);
}

/**
 ** FetchVendorExtension()
 **/

static OlVendorPartExtension
FetchVendorExtension (Widget w, Boolean is_mnemonic, Boolean *p_use_mnemonic_prefix, Boolean *p_accelerators_do_grab)
{
	Widget			vsw	= _OlFindVendorShell(w, is_mnemonic);

	OlVendorPartExtension	pe	= 0;


	if (vsw) {
		pe = _OlGetVendorPartExtension(vsw);
		if (p_use_mnemonic_prefix)
			*p_use_mnemonic_prefix = UseMnemonicPrefix(vsw);
		if (p_accelerators_do_grab) {
			static Arg	arg = {	XtNacceleratorsDoGrab };

			arg.value = (XtArgVal)p_accelerators_do_grab;
			XtGetValues (vsw, &arg, 1);
		}
	}
	return (pe);
}

/**
 ** UseMnemonicPrefix()
 **/

static Boolean
UseMnemonicPrefix (Widget w)
{
	return (!XtIsSubclass(w, menuShellWidgetClass));
}

/**
 ** GrabAccelerator()
 **/

static void
GrabAccelerator (Widget w, KeyEvent *ke, Widget *shells, Cardinal nshells, Boolean grab)
{
    Cardinal j;
    KeyCode keycode;
    Modifiers modifiers = ke->modifiers;
    
#define GRAB_KIND False,GrabModeAsync,GrabModeAsync
    
#define XTGRAB(W,K,M,G) \
    (G?  XtGrabKey((W),(K),(M),GRAB_KIND) \
     : XtUngrabKey((W),(K),(M)))
	
    (void)_OlCanonicalKeysym (
			      XtDisplayOfObject(w),
			      ke->keysym,
			      &keycode,
			      &modifiers
			      );
    if (!keycode)
	return;

    for (j = 0; j < nshells; j++)
	XTGRAB (shells[j], keycode, modifiers, grab);
    
    return;
}

/**
 ** _OlNewAcceleratorResourceValues()
 **/

void
_OlNewAcceleratorResourceValues (Screen *screen, XtPointer client_data)
{
	Widget			vsw	  = (Widget)client_data;
	_OlAppAttributes *	resources = _OlGetAppAttributesRef(vsw);
	OlVendorPartExtension	pe	  = _OlGetVendorPartExtension(vsw);
	register KeyEvent *	p;
	register Cardinal	nel;


	if (!pe || !pe->accelerator_list || !UseMnemonicPrefix(vsw))
		return;

	p   = pe->accelerator_list->base;
	nel = pe->accelerator_list->nel;

	while (nel--) {
		if (p->is_mnemonic)
			p->modifiers = resources->mnemonic_modifiers;
		p++;
	}

	return;
}

/**
 ** RegisterChangeShowAccelerator()
 ** UnregisterChangeShowAccelerator()
 **/

static _OlArrayRec	mnemonic_array		= _OL_ARRAY_INITIAL;
static _OlArrayRec	accelerator_array	= _OL_ARRAY_INITIAL;


static void		ChangeShowAccelerator (
	Screen *		screen,
	XtPointer		client_data
);
static void		ExposeWidgets (
	Screen	    *		screen,
	Widget	    *		array,
	Cardinal		num
);

static void
RegisterChangeShowAccelerator (Widget w, Boolean is_mnemonic)
{
	/*
	 * Register a callback to handle dynamic changes in the
	 * method of showing mnemonics and accelerators.
	 */

	OlAddDynamicScreenCB(XtScreenOfObject(w),
			     ChangeShowAccelerator, (XtPointer)w);


	if (is_mnemonic)
		_OlArrayUniqueAppend (&mnemonic_array, w);
	else
		_OlArrayUniqueAppend (&accelerator_array, w);

	return;
} /* RegisterChangeShowAccelerator */

static void
UnregisterChangeShowAccelerator (Widget w)
{
	int			i;

	OlRemoveDynamicScreenCB(XtScreenOfObject(w),
			     ChangeShowAccelerator, (XtPointer)w);

	if ((i = _OlArrayFind(&mnemonic_array, w)) != _OL_NULL_ARRAY_INDEX)
		_OlArrayDelete (&mnemonic_array, i);
	if ((i = _OlArrayFind(&accelerator_array, w)) != _OL_NULL_ARRAY_INDEX)
		_OlArrayDelete (&accelerator_array, i);

	return;
} /* UnregisterChangeShowAccelerator */

static void
ChangeShowAccelerator (Screen *screen, XtPointer client_data)
{
	ExposeWidgets (screen, (Widget *)&client_data, 1);
} /* ChangeShowAccelerator */

static void
ExposeWidgets (Screen *screen, Widget *array, Cardinal num)
{
	Cardinal		i;

	Widget			w;


	for (i = 0; i < num; i++) {
		w = array[i];
		if (DisplayOfScreen(screen) == XtDisplayOfObject(w) &&
		    XtIsRealized(w))
			XClearArea (
				XtDisplayOfObject(w),
				XtWindowOfObject(w),
				_OlXTrans(w, 0),
				_OlYTrans(w, 0),
				w->core.width,
				w->core.height,
				True
			);
	}
	return;
} /* ExposeWidgets */
