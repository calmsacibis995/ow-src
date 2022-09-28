/*****************************************************************************
 * xbc.c: An X interface to bc
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

#include <stdio.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/TextEdit.h>
#include <stdio.h>

Widget create_button(String, Widget, Widget);
void   quit_bc(Widget, XtPointer, XtPointer);
void   get_from_bc(XtPointer, int *, XtInputId *);
void   send_to_bc(Widget, XtPointer, XtPointer);
void   reset_display(Widget);
void   talk_to_process(char *);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Widget       toplevel, panel, keyboard, display;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "XBc", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  /*
   * Create a ControlArea widget as a base for the 
   * rest of the calculator.
   */
  panel = XtVaCreateManagedWidget("panel", controlAreaWidgetClass,
                                  toplevel,
				  XtNlayoutType, OL_FIXEDCOLS,
				  NULL);
  /*
   * Create the calculator display.
   */
  display = XtVaCreateManagedWidget("display", textEditWidgetClass,
                                    panel,
				    XtNlinesVisible, 1,
				    XtNborderWidth,  1,
				    NULL);
  /*
   * Make the keyboard, which manages 4 columns of buttons
   */
  keyboard = XtVaCreateManagedWidget("keyboard", 
                                     controlAreaWidgetClass,
                                     panel,
				     XtNlayoutType, OL_FIXEDCOLS,
				     XtNsameSize,   OL_ALL,
				     XtNmeasure,    4,
				     NULL);
  /* 
   * Create the keyboard buttons. This order makes it 
   * look like a typical desktop calculator.
   */
  create_button("1", keyboard, display);
  create_button("2", keyboard, display);
  create_button("3", keyboard, display);
  create_button("+", keyboard, display);
  create_button("4", keyboard, display);
  create_button("5", keyboard, display);
  create_button("6", keyboard, display);
  create_button("-", keyboard, display);
  create_button("7", keyboard, display);
  create_button("8", keyboard, display);
  create_button("9", keyboard, display);
  create_button("*", keyboard, display);
  create_button("0", keyboard, display);
  create_button(".", keyboard, display);
  create_button("=", keyboard, display);
  create_button("/", keyboard, display);
  /*
   *  Add a callback that tells bc to exit.
   */
  OlAddCallback(toplevel, (String)XtNwmProtocol, quit_bc, NULL);
  /* 
   * Add callback get_from_bc() --  invoked when input 
   * is available from stdin.
   */
  XtAppAddInput(app, fileno(stdin), (XtPointer)XtInputReadMask, 
                get_from_bc, display);
  /* 
   * Exec the program "bc" and set up pipes 
   * between it and us.
   */
  talk_to_process("bc");

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

Widget
create_button(
  String name,
  Widget parent,
  Widget display)
{
  Widget button;  

  /*
   * Create a single button and attach an activate callback.
   */
  button = XtCreateManagedWidget(name, oblongButtonWidgetClass,
                                 parent, NULL, 0);
  XtAddCallback(button, XtNselect, send_to_bc, display);
  return (button);
}

void
quit_bc(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  OlWMProtocolVerify *olwmpv = (OlWMProtocolVerify *)call_data;

  if(olwmpv->msgtype == OL_WM_DELETE_WINDOW) {
    /*
     * Tell bc to quit.
     */
    fprintf(stdout, "quit\n");
    exit(0);
  }
}

void
send_to_bc(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
{
  Widget      display = (Widget)client_data;
  static int  start_new_entry = TRUE;
  char       *copybuffer;
  String      label;

  /*
   * If this is the beginning of a new operand, 
   * clear the display.
   */
  if(start_new_entry) {
    reset_display(display);
    start_new_entry = FALSE;
  }
  XtVaGetValues(widget, XtNlabel, &label, NULL);
  switch (label[0]) {
  /*
   * If the user entered and '=', send bc a newline, clear
   * the display, and get ready for a new operand.
   */
  case '=':
    OlTextEditCopyBuffer((TextEditWidget)display, &copybuffer);
    fprintf(stdout, "%s", copybuffer);
    XtFree(copybuffer);
    fprintf(stdout, "\n");
    reset_display(display);
    start_new_entry = TRUE;
    break;
  /*
   * If this is an operator, get the previous operand
   * from the display buffer, and send it to bc before 
   * sending the operand.
   */
  case '-':
  case '+':
  case '/':
  case '*':
  case '^':
    OlTextEditCopyBuffer((TextEditWidget)display, &copybuffer);
    fprintf(stdout, "%s", copybuffer);
    fprintf(stdout, "%c", label[0]);
    XtFree(copybuffer);
    reset_display(display);
    break;
  /*
   * Anything else must be a digit, so append it to the
   * display buffer.
   */
  default:
    OlTextEditInsert((TextEditWidget)display, &label[0], 1);
  }
  fflush(stdout);
}

void
get_from_bc(
  XtPointer   client_data,
  int        *fid,
  XtInputId  *id)
{
  Widget display = (Widget)client_data;
  char   buf[BUFSIZ];
  int    nbytes, i;

  /* 
   * Get all pending input and append it to the display 
   * widget. Discard lines that begin with a newline.
   */
  nbytes = read(*fid, buf, BUFSIZ);
  if (nbytes && buf[0] != '\n') {
  /*
   * Null terminate the string at the first newline,
   * or at the end of the bytes read.
   */
   for(i=0;i<nbytes;i++)
     if(buf[i] == '\n')
         buf[i] = '\0';
     buf[nbytes] = '\0'; 
     OlTextEditInsert((TextEditWidget)display, &buf[0], 1);
  }
}

void
reset_display(Widget display)
{
  /*
   * Clear the text buffer and go to position 1.
   */
  if(OlTextEditClearBuffer((TextEditWidget)display) == FALSE) {
    fprintf(stderr,"OlTextEditClearBuffer() failed\n");
  }
}

void
talk_to_process(
  char *cmd)
{
  int   to_child[2], /* pipe descriptors from parent->child */
        to_parent[2];/* pipe descriptors from child->parent */
  int   pid;
  pipe(to_child);
  pipe(to_parent);
  if (pid = fork(), pid == 0){    /* in the child   */
     close(0);                    /* redirect stdin */
     dup(to_child[0]);
     close(1);                    /* redirect stdout*/
     dup(to_parent[1]);
     close(to_child[0]);          /* close pipes    */
     close(to_child[1]);
     close(to_parent[0]);
     close(to_parent[1]);
     execlp(cmd, cmd, NULL);      /* exec the new cmd */
   }
   else if (pid > 0){             /* in the parent  */
      close(0);                   /* redirect stdin */
      dup(to_parent[0]);
      close(1);                   /* redirect stdout  */
      dup(to_child[1]);
      setbuf(stdout, NULL);       /* no buffered output */
      close(to_child[0]);         /* close pipes */
      close(to_child[1]);
      close(to_parent[0]);
      close(to_parent[1]);
    }
    else {                        /* error!       */
      fprintf(stderr,"Couldn't fork process %s\n", cmd);
      exit(1);
    }
}
