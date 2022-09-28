/*****************************************************************************
 * sort.c: Display a binary sort tree using the Tree widget.
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
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h> 
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h> 
#include <Xol/StaticText.h>
#include <Xol/Form.h>
#include <Xol/ScrolledWi.h>
#include "Tree.h"

/*
 * Define the structure for a node in the binary sort tree.
 */
typedef struct _node {
  int            key;
  struct _node  *left;
  struct _node  *right;
} node;

extern node *insert_node();
extern node *make_node();
main(argc, argv)
  int argc;
  char **argv;
{
  Widget       toplevel, form, sw, tree;
  int          i;
  node        *head = NULL;
  int          digit;   
  Arg          wargs[10];
  int          n;
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Sort", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  /*
   * Scrolled Window behave better when the child of a ControlArea
   */
  form = XtCreateManagedWidget("form", formWidgetClass,
                              toplevel, NULL, 0);
  /*
   * Put the tree in a scrolled window, to handle large trees.
   */
  n = 0;
  XtSetArg(wargs[n], XtNxAttachRight, TRUE); n++;
  XtSetArg(wargs[n], XtNyAttachBottom, TRUE); n++;
  XtSetArg(wargs[n], XtNxResizable, TRUE); n++;
  XtSetArg(wargs[n], XtNyResizable, TRUE); n++;
  sw = XtCreateManagedWidget("swindow", scrolledWindowWidgetClass,
                             form, wargs, n);
  /*
   * Create the tree widget.
   */
  tree = XtCreateManagedWidget("tree", XstreeWidgetClass, 
                               sw, NULL, 0);
  /*
   * Create a binary sort tree from data read from stdin.
   */
  while(scanf("%d", &digit) != EOF)
    head = insert_node(digit, head);
  /*
   * Create the widgets representing the tree.
   */
  show_tree(tree, head, NULL);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

node *
insert_node(key, head)
  int   key;
  node *head;
{
  node *prev, *ptr  = head;
  /*
   * If the tree doesn't exist, just create and 
   * return a new node.
   */
  if(!head)
    return (make_node(key));
  /*
   * Otherwise, find a leaf node, always following the 
   * left branches if the key is less than the value in each 
   * node, and the right branch otherwise.
   */
  while(ptr != NULL){ 
    prev = ptr; 
    ptr = (key < ptr->key) ? ptr->left : ptr->right;
  } 
  /*
   * Make a new node and attach it to the appropriate branch.
   */
  if (key < prev->key)
    prev->left = make_node(key);
  else 
    prev->right = make_node(key);
   return (head);
}

node *
make_node(key)
  int   key;
{
  node  *ptr = (node *) malloc(sizeof(node));

  ptr->key  = key;
  ptr->left = ptr->right = NULL;

  return (ptr);
}

show_tree(parent, branch, super_node)
  Widget   parent;     
  node    *branch;     
  Widget   super_node; 
{
  Widget   w;
  Arg      wargs[3];
  int      n = 0;
  char     str[32];
  /*
   * If we've hit a leaf, return.
   */
  if(!branch) return;
  /*
   * Create a widget for the node, specifying the
   * given super_node constraint.
   */
  n = 0;
  XtSetArg(wargs[n], XtNsuperNode, super_node); n++;
  w  =  XtCreateManagedWidget("node", staticTextWidgetClass, 
                              parent, wargs, n);
  sprintf(str, "%d", branch->key);
  XtVaSetValues(w, XtNstring, str, NULL);
  /*
   * Recursively create the subnodes, giving this node's 
   * widget as the super_node.
   */
  show_tree(parent, branch->left,  w);
  show_tree(parent, branch->right, w);
}
