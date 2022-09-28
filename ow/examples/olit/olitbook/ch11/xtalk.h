/*****************************************************************************
 *  xtalk.h: declarations used by xtalk
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

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/RubberTile.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
/* 
 *   Atoms used for communication
 */
Atom      XTALK_WINDOW, CONNECTION_REQUEST, 
          CONNECTION_ACCEPT, DISCONNECT_NOTIFY;
Display  *remote_display = NULL;
Display  *my_display;
Window    remote_talker_window;
/* 
 *  Various widgets
 */
Widget    name_field, msg_field, 
          connect_button, disconnect_button, 
          accept_button;
char     *othermachine;
char     *my_displayname;
int       connection_accepted = FALSE;
/*
 *  Define the strings used to create atoms
 */
#define XtNdisconnect        "Disconnect Notify"
#define XtNconnectionAccept  "Connection Accept"
#define XtNconnectionRequest "Connection Request"
#define XtNtalkWindow        "XTalk Window"
/* 
 * Declare the callbacks used in xtalk
 */
void    client_message_handler();
void    register_talker_window();
void    create_command_panel();
void    warn_wrong_pane();
void    accept_callback();
void    connect_callback();
void    disconnect_callback();
void    send_to_remote();
void    quit_callback();
