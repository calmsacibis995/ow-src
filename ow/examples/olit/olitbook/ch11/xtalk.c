/*****************************************************************************
 * xtalk.c: demonstrate X events
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1993
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1993 by Prentice Hall
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

#include "xtalk.h"

void
main(unsigned int argc, char **argv)
{
  Widget       toplevel, vpane, talk, listen;
  Widget       sw_listen, sw_talk;
  Arg          wargs[10];
  XtAppContext app;

  /*
   * Open display and save display and display name.
   */
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "XTalk", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);

  my_display = XtDisplay(toplevel);
  my_displayname = DisplayString(my_display);
  /*
   * Create a pane to hold all other widgets.
   */
  vpane = XtCreateManagedWidget("panel",
                                rubberTileWidgetClass,
                                toplevel, NULL,0);
  create_command_panel(vpane);
  /*
   * Create the text panes used to talk.
   */
  talk = XtCreateManagedWidget("talk", textEditWidgetClass,
                               vpane, NULL, 0);
  XtAddEventHandler(talk, KeyPressMask | KeyReleaseMask,
                    FALSE, send_to_remote, NULL);
  listen = XtCreateManagedWidget("listen", textEditWidgetClass,
                                 vpane, NULL, 0);
  /*
   * Create the message area.
   */
  XtSetArg(wargs[0], XtNgravity, SouthWestGravity);
  msg_field = XtCreateManagedWidget("messages",
                                    staticTextWidgetClass,
                                    vpane, wargs, 1);
  XtVaSetValues(msg_field, XtNstring, "No Current Connection", NULL);
  XtAddEventHandler(listen, KeyPressMask, FALSE,
                    warn_wrong_pane, NULL);
  XtAddEventHandler(listen, NoEventMask, TRUE,
                    client_message_handler, NULL);
  OlAddCallback(toplevel, (String)XtNwmProtocol, quit_callback, NULL);
  XtRealizeWidget(toplevel);
  /*
   * Store the listen window ID in a public place.
   */
  register_talker_window(listen);
  XtAppMainLoop(app);
}

void
create_command_panel(parent)
  Widget parent;
{
  Widget  command;
  Arg     wargs[3];
  int     n;

  /*
   * Create a row widget to hold the command buttons.
   */
  command = XtCreateManagedWidget("command", controlAreaWidgetClass,
                                  parent, NULL, 0);

  /*
   * Create the buttons.
   */
  connect_button = XtCreateManagedWidget("connect",
                                         oblongButtonWidgetClass,
                                         command, NULL, 0);
  XtAddCallback(connect_button, XtNselect, connect_callback, NULL);
  XtSetArg(wargs[0], XtNsensitive, FALSE);
  disconnect_button =
     XtCreateManagedWidget("disconnect", oblongButtonWidgetClass,
                           command, wargs, 1);
  XtAddCallback(disconnect_button, XtNselect,
                disconnect_callback, NULL);
  XtSetArg(wargs[0], XtNsensitive, FALSE);
  accept_button = XtCreateManagedWidget("accept",
                                        oblongButtonWidgetClass,
                                        command, wargs, 1);
  XtAddCallback(accept_button, XtNselect, accept_callback, NULL);
  /*
   * Create a text field in which the user can
   * enter new machine names.
   */
  name_field = XtCreateManagedWidget("name", textFieldWidgetClass,
                                     command, NULL, 0);
}

void
register_talker_window(w)
  Widget w;
{
  Window   window = XtWindow(w);
  Display *dpy    = XtDisplay(w);

  /*
   * Intern the atoms used for communication.
   */
  XTALK_WINDOW       = XInternAtom(dpy, XtNtalkWindow, 0);
  CONNECTION_REQUEST = XInternAtom(dpy, XtNconnectionRequest, 0);
  CONNECTION_ACCEPT  = XInternAtom(dpy, XtNconnectionAccept, 0);
  DISCONNECT_NOTIFY  = XInternAtom(dpy, XtNdisconnect, 0);
  /*
   * Store the listen window ID on our root window.
   */
  XChangeProperty(dpy, DefaultRootWindow(dpy),
                  XTALK_WINDOW, XA_WINDOW,
                  32, PropModeReplace,
                  (unsigned char *)&window, 1);
}

void
connect_callback(w, client_data, call_data)
  Widget   w;
  XtPointer  client_data, call_data;
{
  int            format, fail;
  unsigned long  nitems, left;
  unsigned char *retdata;
  Arg            wargs[2];
  char          *msg;
  Atom           type, REMOTE_XTALK_WINDOW;
  int            size;

  /*
   * Get the name of the display to connect to.
   */
  othermachine = OlTextFieldGetString((TextFieldWidget)name_field, &size);
  /*
   * Make sure the string isn't empty, so we don't connect
   * to ourselves.
   */
  if((int)strlen(othermachine) > 0) {
    XtVaSetValues(msg_field, 
		  XtNstring, "Trying To Open Connection", 
		  NULL);
    /*
     * Attempt to open the remote display.
     */
    if((remote_display = XOpenDisplay(othermachine)) == NULL) {
	XtVaSetValues(msg_field, 
		      XtNstring, "Connection Failed", 
		      NULL);
        return;
    }
    /*
     *  Get the REMOTE property containing THEIR listen ID.
     */
    REMOTE_XTALK_WINDOW  =
          XInternAtom(remote_display, XtNtalkWindow, 0);
    if(XGetWindowProperty(remote_display,
                          DefaultRootWindow(remote_display),
                          REMOTE_XTALK_WINDOW,
                          0, 4, FALSE, XA_WINDOW,
                          &type, &format, &nitems, &left,
                          &retdata) == Success &&
         type == XA_WINDOW) {
      remote_talker_window = *(Window *)retdata;
      /*
       *  If all went well, request a connection.
       */
      XtVaSetValues(msg_field, 
		    XtNstring, "Waiting for a response", 
		    NULL);
      connection_accepted = FALSE;
      send_message(remote_display, remote_talker_window,
                   XtNconnectionRequest, my_displayname);
     }
    /*
     *  If something went wrong, disconnect.
     */
    else
      disconnect_callback(disconnect_button, NULL, NULL);
  }
}

void
client_message_handler(w, client_data, event)
  Widget          w;
  XtPointer       client_data;
  XEvent         *event;
{
  Arg wargs[10];
  char str[32];

  if(event->type != ClientMessage) 
    return;
  if(event->xclient.message_type == CONNECTION_REQUEST) {
   /*
    * Notify the user of the incoming request and
    * enable the "accept" button.
    */
    XBell(XtDisplay(w), 0);
    othermachine = XtNewString(event->xclient.data.b);
    sprintf(str, "Connection Request from: %s",
                       othermachine);
    XtVaSetValues(msg_field, XtNstring, str, NULL);
    XtSetSensitive(accept_button, TRUE);
  } else if(event->xclient.message_type == CONNECTION_ACCEPT) {
    /*
     * Notify the user that the connection has
     * been accepted. Enable the "disconnect" button
     * and disable the "connect" button.
     */
    XBell(XtDisplay(w), 0);
    connection_accepted = TRUE;
    othermachine = XtNewString(event->xclient.data.b);
    sprintf(str, "Connected to %s", othermachine);
    XtVaSetValues(msg_field, XtNstring, str, NULL);
    XtSetSensitive(connect_button, FALSE);
    XtSetSensitive(disconnect_button, TRUE);
    } else if(event->xclient.message_type == DISCONNECT_NOTIFY) {
    /*
     * Close the remote display and reset
     * all command buttons to their initial state.
     */
    XBell(XtDisplay(w), 0);
    if(remote_display)
      XCloseDisplay(remote_display);
    remote_display = NULL;
    connection_accepted = FALSE;
    othermachine = NULL;
    XtVaSetValues(msg_field, XtNstring, "Disconnected", NULL);
    XtSetSensitive(connect_button, TRUE);
    XtSetSensitive(disconnect_button, FALSE);
  }
}

void
accept_callback(w, client_data, call_data)
  Widget    w;
  XtPointer client_data, call_data;
{
  int      format, fail;
  unsigned long  nitems, left;
  unsigned char  *retdata;
  Atom     type, REMOTE_XTALK_WINDOW;
  Arg      wargs[10];
  char     str[32];

  /*
   * Make sure there really is another machine.
   */
  if((int)strlen(othermachine) > 0 ) {
   /*
    * Attempt to open the remote display.
    */
    if((remote_display = XOpenDisplay(othermachine)) == NULL) {
	XtVaSetValues(msg_field, XtNstring, "Connection Failed", NULL);
        return;
    }
    /*
     *  Get the window ID of the remote xtalk program
     */
    REMOTE_XTALK_WINDOW  =
                XInternAtom(remote_display, XtNtalkWindow, 0);
    if(XGetWindowProperty(remote_display,
                          DefaultRootWindow(remote_display),
                          REMOTE_XTALK_WINDOW,
                          0, 4, FALSE, XA_WINDOW,
                          &type, &format, &nitems, &left,
                          &retdata) == Success &&
            type ==  XA_WINDOW) {
      connection_accepted = TRUE;
      remote_talker_window = *(Window *)retdata;
      /*
       * Notify the remote xtalk that we accept the connection.
       */
      connection_accepted = TRUE;
      send_message(remote_display, remote_talker_window,
                      XtNconnectionAccept, my_displayname);
      sprintf(str, "Connected to %s", othermachine);
      XtVaSetValues(msg_field, XtNstring, str, NULL);

      XtSetSensitive(accept_button, FALSE);
      XtSetSensitive(connect_button, FALSE);
      XtSetSensitive(disconnect_button, TRUE);
    } else
      disconnect_callback(disconnect_button, NULL, NULL);
  }
}

void
send_to_remote(w, client_data, event)
  Widget     w;
  XtPointer  client_data;
  XEvent    *event;
{
  XEvent newevent;

  if(remote_display && remote_talker_window && connection_accepted) {
    newevent.xkey.type = event->xkey.type;
    newevent.xkey.subwindow = event->xkey.subwindow;
    newevent.xkey.time = event->xkey.time;
    newevent.xkey.x = event->xkey.x;
    newevent.xkey.y = event->xkey.y;
    newevent.xkey.display = remote_display;
    newevent.xkey.window = remote_talker_window;
    newevent.xkey.state = event->xkey.state;
    newevent.xkey.keycode = event->xkey.keycode;
    XSendEvent(remote_display, remote_talker_window,
               TRUE, (unsigned long)0, &newevent);
    XFlush(remote_display);
  }
}

void
warn_wrong_pane(w, client_data, event)
  Widget     w;
  XtPointer  client_data;
  XEvent    *event;
{

  /*
   * Just beep if the user types into the wrong pane.
   */
  if (!event->xany.send_event)
    XBell(XtDisplay(w), 0);
}

void
disconnect_callback(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data, call_data;
{
  Arg wargs[10];

  /*
   * Send a disconnect notice and close the display.
   */
  if(remote_display) {
    connection_accepted = FALSE;
    send_message(remote_display, remote_talker_window,
                 XtNdisconnect, my_displayname);
    XCloseDisplay(remote_display);
    XtVaSetValues(msg_field, XtNstring, "Disconnected", NULL);
    othermachine = NULL;
    remote_display = NULL;
    XtSetSensitive(connect_button, TRUE);
    XtSetSensitive(disconnect_button, FALSE);
  }
}

void
quit_callback(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data, call_data;
{
  Display *dpy = XtDisplay(w);
  OlWMProtocolVerify *olwmpv = (OlWMProtocolVerify *)call_data;

  if(olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
    /*
     * Inform the remote connection that we are shutting down.
     */
    if(remote_display && remote_talker_window) {
      connection_accepted = FALSE;
      send_message(remote_display, remote_talker_window,
		      XtNdisconnect, my_displayname);
    }
    /*
     * Clean up.
     */
    XDeleteProperty(dpy, DefaultRootWindow(dpy), XTALK_WINDOW);
    exit(0);
  }
}

send_message(display, window, msg_name, data)
  Display *display;
  Window   window;
  char    *msg_name;
  char    *data;
{
  XClientMessageEvent event;
  Atom                MSG_ATOM;

  /*
   * Get the atom used 
   * by the display.
   */
  MSG_ATOM = XInternAtom(display, msg_name, FALSE);
  /*
   * Fill out the client message event structure.
   */
  event.display = display; 
  event.window  = window;
  event.type    = ClientMessage;
  event.format  = 8;
  event.message_type = MSG_ATOM;
  strcpy(event.data.b, data);
  /*
   * Send it and flush.
   */
  XSendEvent(display, window, 
             TRUE, (long)0, (XEvent *)&event);
  XFlush(display);
}
