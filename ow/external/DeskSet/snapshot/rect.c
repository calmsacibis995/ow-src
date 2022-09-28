#ifndef lint
static char sccsid[] = "@(#)rect.c	3.1 04/03/92 Copyright 1987-1990 Sun Microsystem, Inc." ;
#endif

/*  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include "snapshot.h"
#include <xview/rect.h>

extern Vars v ;

static void adjustbottom P((Rect *, int, int *)) ;
static void adjustleft P((Rect *, int, int *)) ;
static void adjustright P((Rect *, int, int *)) ;
static void adjusttop P((Rect *, int, int *)) ;


static void
adjustbottom(r, y, grasp)
Rect *r ;
int y, *grasp ;
{
  r->r_height += y - rect_bottom(r) ;
  if (r->r_height < 0)
    {
      r->r_top += r->r_height + 1 ;
      r->r_height = -r->r_height ;
      reverse_y(grasp) ;
    }
}


static void
adjustleft(r, x, grasp)
Rect *r ;
int x, *grasp ;
{
  r->r_width += r->r_left - x ;
  r->r_left = x ;
  if (r->r_width < 0)
    {
      r->r_left += r->r_width - 1 ;
      r->r_width = -r->r_width ;
      reverse_x(grasp) ;
    }
}


static void
adjustright(r, x, grasp)
Rect *r ;
int x, *grasp ;
{
  r->r_width += x - rect_right(r) ;
  if (r->r_width < 0)
    {
      r->r_left += r->r_width + 1 ;
      r->r_width = -r->r_width ;
      reverse_x(grasp) ;
    }
}


static void
adjusttop(r, y, grasp)
Rect *r ;
int y, *grasp ;
{
  r->r_height += r->r_top - y ;
  r->r_top = y ;
  if (r->r_height < 0)
    {
      r->r_top += r->r_height - 1 ;
      r->r_height = -r->r_height ;
      reverse_y(grasp) ;
    }
}


int
computegrasp(r, x, y)
Rect *r ;
int x, y ;
{
  int row, col ;

       if (x < r->r_left +  r->r_width / 3)      col = 0 ;
  else if (x > r->r_left + (r->r_width / 3) * 2) col = 2 ;
  else                                           col = 1 ;

       if (y < r->r_top +  r->r_height / 3)      row = 0 ;
  else if (y > r->r_top + (r->r_height / 3) * 2) row = 2 ;
  else                                           row = 1 ;
     
  if (row == 1 && col == 1)
    {
      if (x <= r->r_left + r->r_width / 2) col = 0 ;
      else                                 col = 2 ;
     
      if (y <= r->r_top + r->r_height / 2) col = 0 ;
      else                                 col = 2 ;
    }
  return (v->grasps[row][col]) ;
}


void
dofeedback(x, y, r, grasp)
int x, y, *grasp ;
Rect *r ;
{    
  drawbox(r) ;
     
  switch (*grasp)
    {
      case NW : adjustleft(r, x, grasp) ;
      case N  : adjusttop(r,  y, grasp) ;
                break ;
      case NE : adjusttop(r,   y, grasp) ;
      case E  : adjustright(r, x, grasp) ;
                break ;
      case SE : adjustright(r,  x, grasp) ;
      case S  : adjustbottom(r, y, grasp) ;
                break ;
      case SW : adjustbottom(r, y, grasp) ;
      case W  : adjustleft(r,   x, grasp) ;
    }

  drawbox(r) ;
}


void
reverse_x(c)      /* Change the grasp c to its opposite */
int *c ;
{
  switch (*c)
    {
      case E  : *c = W ;
                break ;
      case W  : *c = E ;
                break ;
      case NE : *c = NW ;
                break ;
      case SE : *c = SW ;
                break ;
      case NW : *c = NE ;
                break ;
      case SW : *c = SE ;
    }
}


void
reverse_y(c)
int *c ;
{
  switch (*c)
    {
      case N  : *c = S ;
                break ;
      case S  : *c = N ;
                break ;
      case NE : *c = SE ;
                break ;
      case SE : *c = NE ;
                break ;
      case NW : *c = SW ;
                break ;
      case SW : *c = NW ;
    }
}
