#pragma ident "@(#)mainhelp.h	1.1 91/06/18"

/*
 *      Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on this source code
 *       product does not indicate publication.
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

/************************************************************************
 * 
 * mainhelp.h:  Include file used to declare ALL help text related
 *              to the overall periodic Chart
 *
 ************************************************************************/

/**********************************************
 * Help Text for Overall Table (background, Title)
 ************************************************/

#define tablehelp	"This is a Periodic Table of the Widgets which are currently available in the OPEN LOOK Intrinsics Toolkit, Version3.4.\n\nIt is intended to give a general overview of the objects which can be used to build an OPEN LOOK user interface.\n\n"  

#define tablewarning "THIS IS NOT INTENDED TO BE USED IN PLACE OF THE STANDARD TOOLKIT DOCUMENTATION.\n\nPlease consult the following manuals for COMPLETE information on using OLIT:\n  .OPEN LOOK Intrinsics\n    Toolkit Widget Set\n    Reference Manual\n  .Xt Intrinsics\n    Programmer's Guide\n   .Xt Intrinsics \n    Reference Manual"

/**********************************************
 * Help Text for Legend
 **********************************************/

#define legendhelp	"This Legend displays the breakdown of the Classes of Widgets available in the OPEN LOOK Intrinsics Toolkit.\n\nConstraint:\nSubClass of Composite.  This Class of Widget can manage children according to some additional information associated with each child (i.e. size, position, etc).\n\n"

#define legendhelp2     "Primitive:\nSubClass of Core.\nThis Class of Widget has NO children.  Primitive Widgets are directly associated with an action.\n\nShell:\nSubClass of Composite.  Shells can have ONLY ONE child.  A Shell widget acts as an interface between the application and the Window Manager."

#define legendhelp3	"\n\nFlat:\nA Flat Widget implements a set of *like* user interface components (i.e.Rectangular Buttons), giving the appearance of many widgets, but with reduced overhead (a Flat only consumes a fraction of the Memory that the corresponding group of widgets uses).\n\nGadget:\nA Gadget is a Windowless Object (a widget that uses its parent's window and therefore conserves server-side memory)."

/**********************************************
 * Help Text for Table entry
 **********************************************/

#define entryhelp	"This is a Periodic Table entry for a Widget:\n\nIn Upper LEFT:\n Widget Symbol-\nAbbreviation for Widget\n\nIn Upper RIGHT:\n Atomic Number-\n (arbitrary)\n\nIn Center:\n A Real working example\n of the widget.\n\nAt Bottom:\n Widget Information\n Button - if pressed, will\n bring up information\n on the widget."

/*********************************************
 * Help Text for Information Button
 *********************************************/

#define buttonhelp	"Press this button to popup a window which contains information specific to that widget in the following areas:\n\n . DESCRIPTION\n . SAMPLE CODE\n . RESOURCES"


/********************************************
 * Help Text for Motif -> OLIT map
 **********************************************/

#define maphelp	"Click LEFT on this icon to bring up a table of formulas which display how each Motif widget maps to an equivelent OPEN LOOK widget (or set of widgets)."
