/*****************************************************************************
 * coloredit.h: Header file for coloredit
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1991
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1991 by Prentice Hall
 *  All Rights Reserved
 *
 * This code is based on the OPEN LOOK Intrinsics Toolkit (OLIT) and 
 * the X Window System
 *
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation.
 *
 * Prentice Hall and the authors disclaim all warranties with regard to 
 * this software, including all implied warranties of merchantability and 
 * fitness.
 * In no event shall Prentice Hall or the authors be liable for any special,
 * indirect or consequential damages or any damages whatsoever resulting from 
 * loss of use, data or profits, whether in an action of contract, negligence 
 * or other tortious action, arising out of or in connection with the use 
 * or performance of this software.
 *
 * OPEN LOOK is a trademark of UNIX System Laboratories.
 * X Window System is a trademark of the Massachusetts Institute of Technology
 ****************************************************************************/

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h> 
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/Slider.h>

#define MAXCOLORS    64
#define RED          1
#define GREEN        2
#define BLUE         3

Display    *dpy;
Colormap    my_colormap;
XColor      current_color;
int         ncolors, ncells;

Widget      red_slider, 
            blue_slider, 
            green_slider,  
            red_text,
            blue_text,
            green_text,
	    toplevel,
	    sliders,
	    form;
void        slider_selected();
void        slider_moved();
void        set_current_color();
void        update_color();
Widget      make_slider();
Widget      make_text();
Widget      create_color_bar();
