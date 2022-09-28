#pragma ident "@(#)bbox.c	1.7 92/10/06"
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
 *			COPYRIGHT 1987, 1989
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

/* bbox.c -- management of buttons and buttonboxes. 
 *
 * This module implements a simple interface to buttonboxes, allowing a client
 * to create new buttonboxes and manage their contents. 
 */

#include "xmh.h"
#include "bboxint.h"

static XtTranslations	RadioButtonTranslations = NULL;


void BBoxInit(void)
{
    RadioButtonTranslations =
	XtParseTranslationTable("<Btn1Down>,<Btn1Up>:set()\n");
}


/*
 * Create a new button box.  The widget for it will be a child of the given
 * scrn's widget, and it will be added to the scrn's pane. 
 */

                                /* name of the buttonbox widgets */
ButtonBox BBoxCreate(Scrn scrn, char *name)
{
    Cardinal	n;
    ButtonBox	buttonbox = XtNew(ButtonBoxRec);
    Arg		args[5];

    n = 0;
#ifdef NOTYET
	EXPERIMENT HERE WITH RESOURCE MANAGER FIRST
    XtSetArg(args[n], XtNlayoutType, OL_FIXEDWIDTH);			n++;
#endif
    XtSetArg(args[n], XtNweight, 0);					n++;
    
#ifdef NOTDEF
    buttonbox->outer =
	XtCreateManagedWidget(name, scrolledWindowWidgetClass, scrn->widget,
			      args, 0);
    buttonbox->inner =
	XtCreateManagedWidget(name, controlAreaWidgetClass, buttonbox->outer,
			      args, (Cardinal) 0);
#endif
    buttonbox->inner =
	XtCreateManagedWidget(name, controlAreaWidgetClass, scrn->widget,
			      args, (Cardinal) n);

    buttonbox->numbuttons = 0;
    buttonbox->button = (Button *) NULL;
    buttonbox->scrn = scrn;
    return buttonbox;
}


                                /* name of the buttonbox widgets */
ButtonBox RadioBBoxCreate(Scrn scrn, char *name)
{
    return BBoxCreate(scrn, name);
}


/* Create a new button, and add it to a buttonbox. */

static void bboxAddButton(ButtonBox buttonbox, char *name, WidgetClass kind, Boolean enabled, Boolean radio)
{
    Button	button;
    Cardinal	i;
    Widget	radio_group;
    Arg		args[5];

    buttonbox->numbuttons++;
    buttonbox->button = (Button *) 
	XtRealloc((char *) buttonbox->button,
		  (unsigned) buttonbox->numbuttons * sizeof(Button));
    button = buttonbox->button[buttonbox->numbuttons - 1] = XtNew(XmhButtonRec);
    button->buttonbox = buttonbox;
    button->name = XtNewString(name);
    button->menu = (Widget) NULL;

    i = 0;
    if (!enabled) {
	XtSetArg(args[i], XtNsensitive, False);		i++;
    }

/*    if (radio && kind == toggleWidgetClass) { */
    if (radio) {
	fprintf(stderr, dgettext(OlmhDomain,
				"bboxAddButton :shouldn't get here!\n"));
	if (buttonbox->numbuttons > 1)
	    radio_group = (button == buttonbox->button[0]) 
		? (buttonbox->button[1]->widget)
		: (buttonbox->button[0]->widget);
	else radio_group = NULL;
#ifdef NOTDEF
	XtSetArg(args[i], XtNradioGroup, radio_group);		i++;
	XtSetArg(args[i], XtNradioData, button->name);		i++;
#endif
    }

    /* Prevent the folder buttons from picking up labels from resources */

    if (buttonbox == buttonbox->scrn->folderbuttons) {
	XtSetArg(args[i], XtNlabel, button->name);	i++;
    }

    button->widget =
	XtCreateManagedWidget(name, kind, buttonbox->inner, args, i);

    if (kind == menuButtonWidgetClass) {
        i = 0;
        XtSetArg(args[i], XtNmenuPane, &(button->menu));     		i++;
        XtGetValues(button->widget, args, i);
    }

    if (radio)
	XtOverrideTranslations(button->widget, RadioButtonTranslations);
}


void BBoxAddButton(ButtonBox buttonbox, char *name, WidgetClass kind, Boolean enabled)
{
    bboxAddButton(buttonbox, name, kind, enabled, False);
}    


void RadioBBoxAddButton(ButtonBox buttonbox, char *name, Boolean enabled)
{
#ifdef NOTDEF
    bboxAddButton(buttonbox, name, toggleWidgetClass, enabled, True);
#endif
}


/* Set the current button in a radio buttonbox. */

void RadioBBoxSet(Button button)
{
#ifdef NOTDEF
    XawToggleSetCurrent(button->widget, button->name);
#endif
}


/* Get the name of the current button in a radio buttonbox. */

char *RadioBBoxGetCurrent(ButtonBox buttonbox)
{
#ifdef NOTDEF
    return ((char *) XawToggleGetCurrent(buttonbox->button[0]->widget));
#endif
}


/* Remove the given button from its buttonbox, and free all resources
 * used in association with the button.  If the button was the current
 * button in a radio buttonbox, the current button becomes the first 
 * button in the box.
 */

void BBoxDeleteButton(Button button)
{
    ButtonBox	buttonbox;
    int		i, found;
    
    if (button == NULL) return;
    buttonbox = button->buttonbox;
    found = False;

    for (i=0 ; i<buttonbox->numbuttons; i++) {
	if (found)
	    buttonbox->button[i-1] = buttonbox->button[i];
	else if (buttonbox->button[i] == button) {
	    found = True;
 
	    /* Free the resources used by the given button. */

	    if (button->menu != NULL && button->menu != NoMenuForButton)
		XtDestroyWidget(button->menu);
	    XtDestroyWidget(button->widget);
	    XtFree(button->name);
	    XtFree((char *) button);
	} 
    }
    if (found)
	buttonbox->numbuttons--;
}


void RadioBBoxDeleteButton(Button button)
{
    ButtonBox	buttonbox;
    Boolean	reradio = False;
    char *	current;

    if (button == NULL) return;
    buttonbox = button->buttonbox;
    current = RadioBBoxGetCurrent(buttonbox);
    if (current) reradio = ! strcmp(current, button->name);
    BBoxDeleteButton(button);

    if (reradio && BBoxNumButtons(buttonbox))
	RadioBBoxSet(buttonbox->button[0]);
}


/* Enable or disable the given button widget. */

                                /* TRUE for enable, FALSE for disable. */
static void SendEnableMsg(Widget widget, int value)
{
    static Arg arglist[] = {XtNsensitive, NULL};
    arglist[0].value = (XtArgVal) value;
    XtSetValues(widget, arglist, XtNumber(arglist));
}


/* Enable the given button (if it's not already). */

void BBoxEnable(Button button)
{
    SendEnableMsg(button->widget, True);
}


/* Disable the given button (if it's not already). */

void BBoxDisable(Button button)
{
    SendEnableMsg(button->widget, False);
}


/* Given a buttonbox and a button name, find the button in the box with that
   name. */

Button BBoxFindButtonNamed(ButtonBox buttonbox, char *name)
{
    register int i;
    for (i=0 ; i<buttonbox->numbuttons; i++)
	if (strcmp(name, buttonbox->button[i]->name) == 0)
	    return buttonbox->button[i];
    return (Button) NULL;
}


/* Given a buttonbox and a widget, find the button which is that widget. */

Button BBoxFindButton(ButtonBox buttonbox, Widget w)
{
    register int i;
    for (i=0; i < buttonbox->numbuttons; i++)
	if (buttonbox->button[i]->widget == w)
	    return buttonbox->button[i];
    return (Button) NULL;
}


/* Return the nth button in the given buttonbox. */

Button BBoxButtonNumber(ButtonBox buttonbox, int n)
{
    return buttonbox->button[n];
}


/* Return how many buttons are in a buttonbox. */

int BBoxNumButtons(ButtonBox buttonbox)
{
    return buttonbox->numbuttons;
}


/* Given a button, return its name. */

char *BBoxNameOfButton(Button button)
{
    return button->name;
}


/* Given a button, return its menu. */

Widget  BBoxMenuOfButton(Button button)
{
    return button->menu;
}

Widget BBoxBBox(ButtonBox buttonbox)
{
    return buttonbox->inner;
}


/* Set maximum size for a bbox so that it cannot be resized any taller 
 * than the space needed to stack all the buttons on top of each other.
 * Allow the user to set the minimum size.
 */

void BBoxLockSize(ButtonBox buttonbox)
{
#ifdef NOTDEF
    Dimension	maxheight;
    Arg		args[1];

    if (buttonbox == NULL) return;
    maxheight = (Dimension) GetHeight(buttonbox->outer);
    XtSetArg(args[0], XtNmax, maxheight);	/* for Paned widget */
    XtSetValues(buttonbox->outer, args, (Cardinal) 1);
#endif
}


Boolean BBoxIsGrandparent(ButtonBox buttonbox, Widget widget)
{
    return (XtParent(XtParent(widget)) == buttonbox->inner);
}
