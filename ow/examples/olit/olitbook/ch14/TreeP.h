/*****************************************************************************
 * TreeP.h: Private header file for the Tree widget
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

#ifndef TREEP_H
#define TREEP_H
typedef struct _XsTreeClassPart {
  int         ignore;
} XsTreeClassPart;

typedef struct _XsTreeClassRec {
  CoreClassPart       core_class;
  CompositeClassPart  composite_class;
  ConstraintClassPart constraint_class;
  XsTreeClassPart     tree_class;
} XsTreeClassRec;

extern XsTreeClassRec XstreeClassRec;

typedef struct {
  Dimension  *array;
  int         size;
} TreeOffset, *TreeOffsetPtr;

typedef struct {
  Dimension      h_min_space;
  Dimension      v_min_space;
  Pixel          foreground;
  GC             gc;
  TreeOffsetPtr  horizontal;
  TreeOffsetPtr  vertical;
  Widget         tree_root;
} XsTreePart;


typedef struct _XsTreeRec {
  CorePart        core;
  CompositePart   composite;
  ConstraintPart  constraint;
  XsTreePart      tree;
} XsTreeRec;



typedef struct _TreeConstraintsPart {
  Widget        super_node;
  WidgetList    sub_nodes;
  long          n_sub_nodes;
  long          max_sub_nodes;
  Position      x, y;
} TreeConstraintsPart;

typedef struct _TreeConstraintsRec {
   TreeConstraintsPart tree;
} TreeConstraintsRec, *TreeConstraints;


#define TREE_CONSTRAINT(w) \
                   ((TreeConstraints)((w)->core.constraints))

#endif TREEP_H



