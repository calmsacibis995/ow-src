#pragma ident  "@(#)table.h	1.3 91/07/26 bin/olittable SMI"

/*
 *      Copyright (C) 1990, 1991 Sun Microsystems, Inc
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


#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Vendor.h>
#include <Xol/OpenLook.h>
#include <Xol/EventObj.h>

#include <Xol/FileCh.h>
#include <Xol/FileChSh.h>
#include <Xol/FontChSh.h>
#include <Xol/Primitive.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/DropTarget.h>
#include <Xol/Exclusives.h>
#include <Xol/FCheckBox.h>
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/Gauge.h>
#include <Xol/Menu.h>
#include <Xol/MenuButton.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/NumericFie.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/RubberTile.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/Stub.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/TextLine.h>
#include <libintl.h>


#define FIXEDFONT       "-b&h-lucidatypewriter-medium-r-*-*-*-120-*-*-*-*-iso8859-1,-*-*-medium-r-normal--14-120-75-75-c-*-*-*"
#define SIZE10FONT      "-b&h-lucidatypewriter-medium-r-*-*-*-100-*-*-*-*-iso8859-1,-*-*-medium-r-normal--14-120-75-75-c-*-*-*"
#define SIZE12FONT      "-b&h-lucida-medium-r-*-*-*-120-*-*-*-*-iso8859-1,-*-*-medium-r-normal--14-120-75-75-c-*-*-*"
#define SIZE14FONT      "-b&h-lucida-bold-r-*-*-*-140-*-*-*-*-iso8859-1,-*-*-*-r-normal--16-140-75-75-c-*-*-*"
#define SIZE18FONT      "-b&h-lucida-medium-r-*-*-*-180-*-*-*-*-iso8859-1,-*-*-medium-r-normal--22-200-75-75-c-*-*-*"
#define LABELFONT       "-b&h-lucida-bold-r-*-*-*-120-*-*-*-*-iso8859-1,-*-*-*-r-normal--*-*-75-75-c-*-*-*"

#define DUMMY	"nothingbutjunk"

#define NUMFONTS 6
#define SIZE14	0
#define SIZE18	1
#define SIZE10	2
#define SIZE12	3
#define	FIXED	4
#define LABEL	5

#define TABLE_ENTRY_HEIGHT 	110
#define TABLE_ENTRY_WIDTH  	155


#define CONSTRAINT	0
#define PRIMITIVE	1
#define SHELL		2
#define	FLAT		3
#define GADGETSYM	4
#define GADGET		5


#define NCOLORS		8

#define BACKGROUND	4
#define CONSTCHILD	5
#define SHELLBUTTON	6
#define OTHER		7

#define	TABLE_BG_COLOR		"light grey"
#define CONSTRAINT_COLOR	"medium turquoise"
#define CONST_CHILD_COLOR	"grey"
#define PRIMITIVE_COLOR		"RoyalBlue1"
#define SHELL_COLOR		"orchid2"
#define SHELL_BUTTON_COLOR	"orchid1"		
#define	FLAT_COLOR		"medium aquamarine"

#define	OlittableDomain		"SUNW_WST_OLITTABLE"

#define NUMENTRIES	34	


#define	C_BULLETINBOARD		0
#define C_CONTROLAREA		1
#define C_FORM			2
#define C_RUBBERTILE		3
#define C_TEXTFIELD		4
#define C_TEXTEDIT		5
#define C_POPUPWINDOWSHELL	6
#define C_NOTICESHELL		7
#define C_MENUSHELL		8
#define C_SCROLLEDWINDOW	9
#define C_SCROLLINGLIST		10
#define	C_NONEXCLUSIVES		11
#define C_EXCLUSIVES		12
#define C_CHECKBOX		13
#define C_SLIDER		14
#define C_MENUBUTTON		15
#define C_RECTBUTTON		16
#define C_GAUGE			17
#define C_FLATNONEXCLUSIVES	18
#define C_FLATEXCLUSIVES	19
#define C_FLATCHECKBOX		20
#define C_DRAWAREA		21
#define C_STUB			22
#define C_OBLONGBUTTON		23
#define C_SCROLLBAR		24
#define C_CAPTION		25
#define C_STATICTEXT		26
#define C_DROPTARGET		27
#define C_ABBREVMENUBUTTON	28
#define C_FOOTERPANEL		29
#define C_FONTCHOOSER          	30 
#define C_FILECHOOSER           31
#define C_TEXTLINE              32
#define C_NUMERICFIELD          33


typedef struct table_entry {
	Widget	entry; 		/* BulletinBoard */
	Widget  widget;		/* Example widget */
	Widget  entry_widget;   /* Widget to be centered in the Entry */
	Widget  info_button; 	/* Button to Popup Info Window */
	Widget  info_shell;	/* Info PopupWindowShell */
} TableEntry; 

