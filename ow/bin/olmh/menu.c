#pragma ident "@(#)menu.c	1.6 92/10/06"
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
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#include "xmh.h"
#include "bboxint.h"


void AttachMenuToButton(Button button, Widget menu, char *menu_name)
{
    Arg		args[3];

    if (button == NULL) return;
    button->menu = menu;
    XtSetArg(args[0], XtNmenuName, XtNewString(menu_name));
    XtSetValues(button->widget, args, (Cardinal) 1);
}


/*ARGSUSED*/
void DoRememberMenuSelection(Widget widget, XtPointer client_data, XtPointer call_data)
{
#ifdef NOTDEF
    static Arg	args[] = {
	{ XtNpopupOnEntry,	(XtArgVal) NULL },
    };
    args[0].value = (XtArgVal) widget;
    XtSetValues(XtParent(widget), args, XtNumber(args));
#endif
}


void SendMenuEntryEnableMsg(Button button, char *entry_name, int value)
{
    Widget	entry;
    static Arg	args[] = { XtNsensitive, (XtArgVal) NULL };

    if ((entry = XtNameToWidget(button->menu, entry_name)) != NULL) {
	args[0].value = (XtArgVal) ((value == 0) ? False : True);
	XtSetValues(entry, args, (Cardinal) 1);
    }
}


void ToggleMenuItem(Widget entry, Boolean state)
{
#ifdef NOTDEF
    Arg		args[1];

    XtSetArg(args[0], XtNleftBitmap, (state ? MenuItemBitmap : None));
    XtSetValues(entry, args, (Cardinal) 1);
#endif
}
