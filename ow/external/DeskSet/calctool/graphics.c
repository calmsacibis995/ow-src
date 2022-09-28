#ifndef lint
static char sccsid[] = "@(#)graphics.c 1.12 96/06/18 Copyr 1987 Sun Micro";
#endif

/*  Copyright (c) 1987 - 1990, Sun Microsystems, Inc.  All Rights Reserved.
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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "color.h"
#include "calctool.h"
#include "extern.h"

/*  Corners for the calctool buttons are stored in the following six icon
 *  files. For both color and mono, four corners are stored for normal,
 *  inverse and stencil images, at each of the four scales.
 */

static unsigned short cbut_normal_image[] = {
#include "images/button.color.normal.icon"
} ;
 
static unsigned short cbut_invert_image[] = {
#include "images/button.color.invert.icon"
} ;
 
static unsigned short cbut_stencil_image[] = {
#include "images/button.color.stencil.icon"
} ;
 
static unsigned short mbut_normal_image[] = {
#include "images/button.mono.normal.icon"
} ;
 
static unsigned short mbut_invert_image[] = {
#include "images/button.mono.invert.icon"
} ;
 
static unsigned short mbut_stencil_image[] = {
#include "images/button.mono.stencil.icon"
} ;


void
add_3D_look(fcptype, x, y, width, height, scale, bstate, mtype, tx, ty)
enum fcp_type fcptype ;
int x, y, width, height, tx, ty ;
enum scale_type scale ;
enum but_state bstate ;
enum menu_type mtype ;
{
  int c ;                 /* The radius of a corner piece at this scale. */
  int m ;                 /* The dimension of the menu glyph at this scale. */

  c = cornerR[(int) scale] ;
  if (bstate == B_NORMAL)
    {
      draw_line(fcptype, x, y + c, x, y + height - c, C_WHITE) ;
      draw_line(fcptype, x + c, y, x + width - c, y, C_WHITE) ;
      add_3D_corner(fcptype, scale, TL_CORNER, x, y, width, height, C_WHITE) ;
      add_3D_corner(fcptype, scale, TR_CORNER, x, y, width, height, C_WHITE) ;
    }
  else if (bstate == B_INVERT)
    {
      draw_line(fcptype, x, y + c, x, y + height - c, C_BLACK) ;
      draw_line(fcptype, x + c, y, x + width - c, y, C_BLACK) ;
      add_3D_corner(fcptype, scale, TL_CORNER, x, y, width, height, C_BLACK) ;
      add_3D_corner(fcptype, scale, TR_CORNER, x, y, width, height, C_BLACK) ;

      draw_line(fcptype, x + c, y + height - 1,
                         x + width - c, y + height - 1, C_WHITE) ;
      draw_line(fcptype, x + width - 1, y + c,
                         x + width - 1, y + height - c, C_WHITE) ;
      add_3D_corner(fcptype, scale, BL_CORNER, x, y, width, height, C_WHITE) ;
      add_3D_corner(fcptype, scale, BR_CORNER, x, y, width, height, C_WHITE) ;
    }
  if (mtype != M_NONE)
    {
      m = msizes[(int) scale] ;
      draw_line(fcptype, tx + m - 1, ty, tx + (m / 2), ty + m - 1, C_WHITE) ;
    }
}


char *
adjust_string(str, width)
char *str ;
int width ;
{
  char s[LABEL_LEN], temp[2] ;
  int i, len ;
  int x = 0 ;

  temp[1] = '\0' ;
  STRCPY(s, str) ;
  len = strlen(s) ;
  for (i = 0; i < len; i++)
    {
      temp[0] = s[i] ;
      x += get_strwidth(NFONT, temp) ;
      if (x > width)
        {
          s[i] = '\0' ;
          if (i > 0) s[i-1] = '>' ;
          break ;
        }
    }
  return(s) ;
}


enum but_state
button_bstate(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? v->bstate[n] :
          v->mode_bstate[MODEKEYS * ((int) v->modetype - 1) + n]) ;
}


char
button_color(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].color :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].color) ;
}


enum menu_type
button_mtype(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].mtype :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].mtype) ;
}


enum op_type
button_opdisp(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].opdisp :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].opdisp) ;
}


char *
button_str(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].str :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].str) ;
}

/* Just like button_str() except returns the English string - For Spot Help */

char *
help_button_str(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].hstr :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].hstr) ;
}

char
button_value(n)
int n ;
{
  return((v->curwin == FCP_KEY) ? buttons[n].value :
          mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].value) ;
}


void
clear_hilite()
{
  int x, y, width, height ;

  height = v->fheight[(int) v->items[(int) DISPLAYITEM].font] ;
  width  = get_strwidth(v->items[(int) DISPLAYITEM].font, v->display) ;
  x      = v->twidth - width - 10 ;
  y      = v->items[(int) DISPLAYITEM].y ;

  v->histart = -1 ;
  MEMSET(v->disp_state, FALSE, MAXLINE) ;
  color_area(FCP_KEY, x, y, width, height, W_COLOR(C_DISPCOL)) ;
  draw_text(x, y, FCP_KEY, BFONT, B_COLOR(C_TEXT), v->display) ;

  v->old_sec  = 0 ;         /* Clear the double-click */
  v->old_usec = 0 ;
}


void
do_canvas_resize()
{
  int h, w, x, y ;

  handle_resize(v->twidth, v->theight) ;
 
  get_frame_size(FCP_REG, &x, &y, &w, &h) ;
  v->rheight = ((MAXREGS - 1) * v->bgap) +
               (v->fheight[(int) MFONT] * MAXREGS) + (2 * v->bborder) ;
  v->rwidth = MAXREGCHARS * v->fwidth[(int) MFONT] + (2 * v->bborder) ;
  set_frame_size(FCP_REG, x, y, v->rwidth, v->rheight) ;
 
  get_frame_size(FCP_MODE, &x, &y, &w, &h) ;
  set_frame_size(FCP_MODE, x, y, v->mwidth, v->mheight) ;
}


void
do_display_adjust()
{
  int c ;             /* Character index into v->display. */
  int i ;
  int len ;           /* Length of the numeric display (in characters). */
  int str_height ;    /* Height of the display text. */
  int str_width ;     /* Width of the display text. */
  int x, y ;          /* Position of the display text (top-left). */

  if (v->histart == -1)
    {
      do_mouse_dragging() ;
      return ;
    }
  str_height = v->fheight[(int) v->items[(int) DISPLAYITEM].font] ;
  str_width  = get_strwidth(v->items[(int) DISPLAYITEM].font, v->display) ;
  x          = v->twidth - str_width - 10 ;
  y          = v->items[(int) DISPLAYITEM].y ;
  if (v->cury > y && v->cury <= (y + str_height) &&
      v->curx > x && v->curx <= (x + str_width))
    {
      c = get_hilite_index(v->display, x, v->curx) ;
      if (c < v->histart)
        {
          for (i = 0; i < c; i++)
            if (v->disp_state[i] == TRUE)
              hilite_char(i, x, y, str_height, FALSE) ;

          for (i = c; i < v->histart; i++)
            if (v->disp_state[i] == FALSE)
              hilite_char(i, x, y, str_height, TRUE) ;
        }
      else
        {
          len = strlen(v->display) ;
          for (i = c+1; i < len; i++)
            if (v->disp_state[i] == TRUE)
              hilite_char(i, x, y, str_height, FALSE) ;
 
          for (i = v->histart; i <= c; i++)
            if (v->disp_state[i] == FALSE) 
              hilite_char(i, x, y, str_height, TRUE) ; 
        }
    }
}


void
do_display_select()
{
  int c ;             /* Character index into v->display. */
  int i ;
  int len ;           /* Length of the numeric display (in characters). */
  int str_height ;    /* Height of the display text. */
  int str_width ;     /* Width of the display text. */ 
  int x, y ;          /* Position of the display text (top-left). */

  str_height = v->fheight[(int) v->items[(int) DISPLAYITEM].font] ;
  str_width  = get_strwidth(v->items[(int) DISPLAYITEM].font, v->display) ;
  x          = v->twidth - str_width - 10 ;
  y          = v->items[(int) DISPLAYITEM].y ;

  if (is_dblclick() == TRUE)
    {
      v->histart = 0 ;
      len = strlen(v->display) ;
      for (i = 0; i < len; i++)
        hilite_char(i, x, y, str_height, TRUE) ;
      return ;
    }

  clear_hilite() ;
  if (v->cury > y && v->cury <= (y + str_height) &&
      v->curx > x && v->curx <= (x + str_width))
    {
      c = get_hilite_index(v->display, x, v->curx) ;
      v->histart = c ;
      hilite_char(c, x, y, str_height, TRUE) ;
    }
}


void
do_mouse_dragging()     /* Handle mouse dragging event. */
{
  enum menu_type menutype ;
  int ocol, orow ;      /* Column/row of last position of mouse. */
  int n, reply ;

  if (v->curwin == FCP_REG) return ;
  if (v->down == RIGHT_DOWN)
    {
      do_mouse_right_down() ;
      return ;
    }

  orow = v->row ;
  ocol = v->column ;
  reply = get_row_col(&v->row, &v->column) ;

  if (reply && (orow == v->row && ocol == v->column)) return ;
  n = orow * MAXCOLS + ocol ;
  if (n >= 0 && button_bstate(n) != B_NULL)
    draw_button(v->curwin, orow, ocol, B_NORMAL) ;

  if (!reply) return ;

  if (orow == v->row && ocol == v->column) return ;
  n = v->row * MAXCOLS + v->column ;
  if (n >= 0)
    if ((menutype = button_mtype(n)) != M_NONE)
      draw_def_menu(menutype, n) ;
    else if (button_bstate(n) != B_NULL)
      draw_button(v->curwin, v->row, v->column, B_INVERT) ;
}


void
do_mouse_left_down()       /* Handle mouse left button down event. */
{
  enum menu_type menutype ;
  int n ;

  if (v->curwin == FCP_REG) return ;
  v->down = LEFT_DOWN ;
  if (get_row_col(&v->row, &v->column))
    {
      n = v->row * MAXCOLS + v->column ;
      if ((menutype = button_mtype(n)) != M_NONE)
        draw_def_menu(menutype, n) ;
      else if (button_bstate(n) != B_NULL)
        draw_button(v->curwin, v->row, v->column, B_INVERT) ;
    }
}


void
do_mouse_left_up()      /* Handle mouse left button up event. */
{
  int n ;

  if (v->curwin == FCP_REG) return ;
  v->down = 0 ;
  if (get_row_col(&v->row, &v->column))
    {
      n = v->row * MAXCOLS + v->column ;
      if (button_mtype(n) != M_NONE)
        {
          STRCPY(v->opstr, v->item_text[(int) OPITEM]) ;
          handle_menu_selection(n, get_menu_def(n), FALSE) ;
          return ;
        }
      if (v->pending_op != '?' && n <= NOBUTTONS)
        if (button_bstate(n) != B_NULL)
          draw_button(v->curwin, v->row, v->column, B_NORMAL) ;
      if (v->pending)
        {
          v->current = button_value(n) ;
          do_pending() ;
        }
      else if (n >= 0 && n <= NOBUTTONS)
        process_item(n) ;
    }
}


void
do_mouse_right_down()   /* Handle mouse right button down event. */
{
  enum menu_type menutype ;
  int n, val ;

  if (v->curwin == FCP_REG) return ;
  v->down = RIGHT_DOWN ;
  if (!get_row_col(&v->row, &v->column))
    {
      if (v->curwin == FCP_KEY) (void) do_menu(v->curwin, 0, M_PROPS) ;
    }
  else
    {
      n = v->row * MAXCOLS + v->column ;
      if ((menutype = button_mtype(n)) != M_NONE)
        {
          STRCPY(v->opstr, v->item_text[(int) OPITEM]) ;
          draw_button(v->curwin, v->row, v->column, B_INVERT) ;
          val = do_menu(v->curwin, n, menutype) ;
          if (val) handle_menu_selection(n, val, FALSE) ;
          else
            {
              v->down = 0 ;
              draw_button(v->curwin, v->row, v->column, B_NORMAL) ;
            }
        }
      else if (v->curwin == FCP_KEY) (void) do_menu(v->curwin, 0, M_PROPS) ;
    }
}


void
do_repaint()     /* Redraw the calctool canvas[es]. */
{
  make_canvas(0, 0, v->twidth, v->theight, 0) ;
}


void
draw_button(fcptype, row, column, bstate)
enum fcp_type fcptype ;
int row, column ;
enum but_state bstate ;
{
  char label[LABEL_LEN] ;
  enum font_type ftype ;
  int color, len, n, nchars, spaces, strw, w ;
  int m ;                      /* Size of a menu glyph at this scale. */
  int x, y ;                   /* Position of the button. */
  int xm, ym ;                 /* Position of the menu glyph. */
  int xstr, ystr ;             /* Position of the text string. */
 
  STRCPY(label, "    ") ;
  if (row == -1 || column == -1) return ;
  n = row * MAXCOLS + column ;
  x = column * (v->bwidth  + v->bgap) + v->bborder ;
  y = row    * (v->bheight + v->bgap) + v->bborder ;
  if (fcptype == FCP_KEY) y += v->ndisplay ;
  if (button_bstate(n) == B_NULL)
    {
      if (v->iscolor) draw_stencil(fcptype, x, y, C_BACK, B_STENCIL) ;
      else grey_area(fcptype, x, y, v->bwidth, v->bheight) ;
      draw_image(fcptype, x, y, C_BLACK, B_NORMAL) ;
    } 
  else
    { 
      if (v->iscolor)
        {
          draw_stencil(fcptype, x, y, button_color(n), B_STENCIL) ;
          color = (bstate == B_INVERT && v->is_3D) ? C_GREY : C_BLACK ;
          draw_image(fcptype, x, y, color, bstate) ;
        } 
      else draw_stencil(fcptype, x, y, C_WHITE, bstate) ;
    } 

  m = msizes[(int) v->scale] ;

/*  If the button label is greater than the button width, allow as much of
 *  it as possible, taking into consideration the possible menu mark.
 *  If the label is too big, then the large displayable character is
 *  replaced with a '>' character.
 */

  get_label(n, &spaces, &nchars) ;
  ftype = NFONT ;
  STRNCPY(label, v->pstr, nchars) ;
  label[nchars] = '\0' ;
  len = strlen(label) ;
  if (label[len-1] == '.' && len != 1) STRCAT(label, "..") ;

  w = v->bwidth ;
  if (button_mtype(n) != M_NONE) w -= m ;
  if ((strw = get_strwidth(ftype, label)) > w)
    {
      STRCPY(label, adjust_string(label, w)) ;
      strw = get_strwidth(ftype, label) ;
    }

  if (button_mtype(n) == M_NONE)
    {
      xstr = x + (v->bwidth - strw) / 2 ;
      xm = 0 ;                                  /* Not used. */
    }
  else
    {
      xstr = x + (v->bwidth - strw - m) / 3 ;
           if (nchars >  4) xm = xstr + strw ;
      else if (nchars == 4) xm = xstr + strw + (v->fwidth[(int) ftype] / 2) ;
      else                  xm = xstr + strw +  v->fwidth[(int) ftype] ;
    }

  ystr = y + (v->bheight - v->fheight[(int) ftype]) / 2 ;
  ym = y + ((v->bheight - m) / 2) + (m / 4) ;

  if (bstate == B_INVERT && (!v->iscolor || !v->is_3D)) color = C_WHITE ;
  else color = B_COLOR(C_TEXT) ;
  draw_text(xstr, ystr, fcptype, ftype, color, label) ;

  if (!strlen(label)) xm = x + (5 * v->fwidth[(int) ftype]) ;
  if (button_mtype(n) != M_NONE)
    {
      if (v->iscolor && v->is_3D)
        {
          draw_menu_stencil(fcptype, xm, ym, button_color(n), B_STENCIL) ;
          draw_menu_image(fcptype, xm, ym, bstate) ;
        }
      else
        {
          color = (bstate == B_NORMAL) ? C_BLACK : C_WHITE ;
          draw_menu_stencil(fcptype, xm, ym, color, B_NORMAL) ;
        }
    }

  if (v->iscolor && v->is_3D)
    add_3D_look(fcptype, x, y, v->bwidth, v->bheight, v->scale, bstate,
                button_mtype(n), xm, ym) ;
}


/*  The user has SELECTED a calctool button which has a menu associated
 *  with it. To conform with OPEN LOOK, the default menu item is displayed
 *  on the calctool button.
 */

void
draw_def_menu(menutype, n) /* Draw default menu value on menu button. */
enum menu_type menutype ;
int n ;
{
  int pos ;
  char butval[5] ;         /* Menu button label. */
  char defval[MAXLINE] ;   /* Default menu value. */

  STRCPY(butval, button_str(n)) ;
  pos = cmenus[(int) menutype].index + get_menu_pos(menutype, n) - 2 ; 
  switch (menutype)
    {
      case M_CON : if (!pos) STRCPY(defval, vstrs[(int) V_CONWNAME]) ;
                   else      STRCPY(defval, v->con_names[pos-1]) ;
                   break ;
      case M_FUN : if (!pos) STRCPY(defval, vstrs[(int) V_FUNWNAME]) ;
                   else      STRCPY(defval, v->fun_names[pos-1]) ;
                   break ;
      default    : STRCPY(defval, get_def_menu_str(menutype)) ;
    }
  if (v->curwin == FCP_KEY)
    STRNCPY(buttons[n].str, defval, strlen(butval)) ;
  else
    STRNCPY(mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].str,
            defval, strlen(butval)) ;
  draw_button(v->curwin, v->row, v->column, B_INVERT) ;
  if (v->curwin == FCP_KEY) STRCPY(buttons[n].str, butval) ;
  else
    STRCPY(mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].str, butval) ;
}


int
get_hilite_index(str, start, x)
char *str ;
int start, x ;
{
  char temp[2] ;
  int i, len ;

  temp[1] = '\0' ;
  len = strlen(str) ;
  for (i = 0; i < len; i++)
    {
      temp[0] = str[i] ;
      start += get_strwidth(v->items[(int) DISPLAYITEM].font, temp) ;
      if (start > x) return(i) ;
    }
/*NOTREACHED*/
}


/*  Get button index for given character value, setting curwin,
 *  row and column appropriately. Note that if the value isn't found,
 *  then a value of TITEMS is returned. This is "chucked out" by
 *  process_item as being invalid.
 *
 *  XXX: This routine can be improved by using a hash lookup table.
 */

int
get_index(ch)
char ch ;
{
  int n, val ;

  for (n = 0; n < TITEMS; n++)
    if (ch == buttons[n].value) break ;
  if (n < TITEMS) v->curwin = FCP_KEY ;
  else if (v->modetype != BASIC)
    { 
      for (n = 0; n < MODEKEYS; n++)
        {
          val = mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].value ;
          if (ch == val) break ;
        }
      if (n == MODEKEYS) return(TITEMS) ;
      v->curwin = FCP_MODE ;
    }
  v->row = n / MAXCOLS ;
  v->column = n - (v->row * MAXCOLS) ;
  return(n) ;
}


int
get_row_col(row, column)    /* Return row/column for current x/y position. */
int *row, *column ;
{
  int c, r, x, y ;

  x = v->curx ;
  y = v->cury ;
  *row = *column = -1 ;
  if (v->curwin == FCP_KEY) y -= v->ndisplay ;
  for (r = 0; r < MAXROWS; r++)
    for (c = 0; c < MAXCOLS; c++)
      if ((x > (c * (v->bwidth  + v->bgap) + v->bborder)) &&
          (x < (c * (v->bwidth  + v->bgap) + v->bborder + v->bwidth)) &&
          (y > (r * (v->bheight + v->bgap) + v->bborder)) &&
          (y < (r * (v->bheight + v->bgap) + v->bborder + v->bheight)))
        {
          *row = r ;
          *column = c ;
          return(1) ;
        }
  return(0) ;
}


void
grey_buttons(base)     /* Grey out numeric buttons depending upon base. */
enum base_type base ;
{
  enum but_state state ;      /* State of current button being checked. */
  enum fcp_type scurwin ;     /* Used to save the current window value. */
  char val ;
  int column, i, n, row ;

  scurwin = v->curwin ;
  v->curwin = FCP_KEY ;
  for (i = 0; i < 16; i++)
    {
      val = digits[i] ;
      if (isupper(val)) val = tolower(val) ;
      for (n = 0; n < TITEMS; n++)
        if (val == buttons[n].value) break ;           
      row = n / MAXCOLS ;
      column = n - (row * MAXCOLS) ;

      if (i < basevals[(int) base]) state = B_NORMAL ;
      else                          state = B_NULL ;
      if (state != v->bstate[n])
        {
          v->bstate[n] = state ;
          draw_button(FCP_KEY, row, column, B_NORMAL) ;
        }
    }                    
  v->curwin = scurwin ;
}


void
handle_menu_selection(n, item, pinned)   /* Process menu selection. */
int n, item, pinned ;
{
  if (item != -1)
    {
      if (IS_KEY(v->pending, KEY_LPAR))     /* Are we inside parentheses? */
        {
          v->current = button_value(n) ;
          do_paren() ;
          v->current = item ;
          do_paren() ;
        }
      else
        {
          save_pending_values(button_value(n)) ;
          v->current = item ;
          v->ismenu = 1 ;       /* To prevent grey buttons being redrawn. */
          do_pending() ;
          v->ismenu = 0 ;
        }
      v->down = 0 ;
    }
  if (!pinned) draw_button(v->curwin, v->row, v->column, B_NORMAL) ;
}


void
handle_resize(width, height)
int width, height ;
{
  int i ;
  int s ;                       /* Current scale. */

  s = (int) S_SMALL ;           /* Start with the smallest scale. */

  v->bborder = bsizes[s] ;
  v->bgap    = gsizes[s] ;

  v->bwidth = (width - (2 * v->bborder) - ((BCOLS-1) * v->bgap)) / BCOLS ;
  if (v->bwidth % 2)
    {
      v->bwidth-- ;
      v->bborder += (BCOLS / 2) ;
    }

  v->ndisplay = height / 4 ;
  if (v->ndisplay % 2) v->ndisplay-- ;

  v->bheight = ((height - v->ndisplay) -
               (2 * v->bborder) - ((BROWS-1) * v->bgap)) / BROWS ;
  if (v->bheight % 2)
    {
      v->bheight-- ;
      v->ndisplay += BROWS ;
    }

  for (i = 0; i < MAXSCALES; i++)
    if (v->bwidth > scaleW[i] && v->bheight > scaleH[i])
      v->scale = (enum scale_type) i ;

  init_fonts(v->scale) ;
  init_other_dims() ;
  init_panel_item_sizes() ;
  make_buttons() ;
}


void
hilite_char(index, x, y, height, state)
int index, x, y, height, state ;
{
  char str[2] ;            /* Character being hilited. */
  char temp[2] ;
  int color, i, width ;

  str[0] = v->display[index] ;
  str[1] = '\0' ;
  v->disp_state[index] = state ;
  temp[1] = '\0' ;
  for (i = 0; i < index; i++)
    {
      temp[0] = v->display[i] ;
      x += get_strwidth(v->items[(int) DISPLAYITEM].font, temp) ;
    }
  width = get_strwidth(v->items[(int) DISPLAYITEM].font, str) ;

  if (state == TRUE) color = B_COLOR(C_TEXT) ;
  else               color = W_COLOR(C_DISPCOL) ;
  color_area(FCP_KEY, x, y, width, height, color) ;

  if (state == TRUE) color = W_COLOR(C_DISPCOL) ;
  else               color = B_COLOR(C_TEXT) ;
  draw_text(x, y, FCP_KEY, BFONT, color, str) ;
}


void
init_fonts(scale)
enum scale_type scale ;
{
  int size ;

  size = fsizes[(int) scale] ;
  get_font(size + 2, BFONT) ;
  get_font(size,     MFONT) ;
  get_font(size,     NFONT) ;
  get_font(size - 2, SFONT) ;
}


void
init_other_dims()
{
  v->theight = (BROWS*v->bheight) + ((BROWS-1) * v->bgap) +
               v->ndisplay + (2*v->bborder) ;
  v->twidth  = (BCOLS*v->bwidth)  + ((BCOLS-1) * v->bgap) + (2*v->bborder) ;
 
  v->rheight = ((MAXREGS - 1) * v->bgap) +
               (v->fheight[(int) MFONT] * MAXREGS) + (2 * v->bborder) ;
  v->rwidth = MAXREGCHARS * v->fwidth[(int) MFONT] + (2 * v->bborder) ;

  v->mheight = (MROWS * v->bheight) + ((MROWS-1) * v->bgap) + (2*v->bborder) ;
  v->mwidth  = (MCOLS * v->bwidth)  + ((MCOLS-1) * v->bgap) + (2*v->bborder) ;
}


void
init_panel_item_sizes()
{
  int n ;      /* Current pseudo panel item. */

  n = (int) BASEITEM ;
  v->items[n].x = v->bborder ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) DISPLAYITEM ;
  v->items[n].x = 0 ;
  v->items[n].y = (v->ndisplay - v->fheight[(int) v->items[n].font]) / 2 ;

  n = (int) TTYPEITEM ;
  v->items[n].x = v->bborder + 1 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) NUMITEM ;
  v->items[n].x = v->bborder + 2 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) HYPITEM ;
  v->items[n].x = v->bborder + 3 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) INVITEM ;
  v->items[n].x = v->bborder + 4 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) OPITEM ;
  v->items[n].x = v->bborder + 5 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;

  n = (int) MODEITEM ;
  v->items[n].x = v->bborder + 6 * (v->bwidth + v->bgap) ;
  v->items[n].y = v->ndisplay - v->fheight[(int) v->items[n].font] ;
}


void
load_corners()
{
  if (v->iscolor && v->is_3D)
    {
      get_but_corners(B_NORMAL,  cbut_normal_image) ;
      get_but_corners(B_INVERT,  cbut_invert_image) ;
      get_but_corners(B_STENCIL, cbut_stencil_image) ;
    }
  else
    {
      get_but_corners(B_NORMAL,  mbut_normal_image) ;
      get_but_corners(B_INVERT,  mbut_invert_image) ;
      get_but_corners(B_STENCIL, mbut_stencil_image) ;
    }
}


void
make_buttons()
{
  make_button(B_NORMAL,  v->scale, v->bwidth, v->bheight) ;
  make_button(B_INVERT,  v->scale, v->bwidth, v->bheight) ;
  make_button(B_STENCIL, v->scale, v->bwidth, v->bheight) ;
}


void
make_canvas(x, y, width, height, toggle)
int x, y, width, height, toggle ;
{
  int first_col, first_row, no_cols, no_rows ;

  if (toggle) v->tstate = !v->tstate ;
  if (v->iscolor) color_area(FCP_KEY, x, y, width, height, C_BACK) ;
  else
    {
      if (x % 2)         /* Make sure grey pattern in aligned. */
        {
          x-- ;
          width++ ;
        }
      if (y % 2)
        {
          y-- ;
          height++ ;
        }
      grey_area(FCP_KEY, x, y, width, height) ;
    }
  if (y < v->ndisplay)
    {
      color_area(FCP_KEY, 0, 0, v->twidth, v->ndisplay, W_COLOR(C_DISPCOL)) ;
      draw_line(FCP_KEY, 0, v->ndisplay, v->twidth, v->ndisplay, C_BLACK) ;
      set_item(DISPLAYITEM, v->display) ;
      set_item(NUMITEM,     dtype_str[(int) v->dtype]) ;
      set_item(OPITEM,      v->item_text[(int) OPITEM]) ;
      set_item(TTYPEITEM,   ttype_str[(int) v->ttype]) ;
      set_item(HYPITEM,     (v->hyperbolic) ? vstrs[(int) V_HYP] : "    ") ;
      set_item(INVITEM,     (v->inverse)    ? vstrs[(int) V_INV] : "    ") ;
      set_item(MODEITEM,    mode_str[(int) v->modetype]) ;
      set_item(BASEITEM,    base_str[(int) v->base]) ;
    }

  first_row = (y - v->ndisplay - v->bborder) / (v->bheight + v->bgap) ;
  first_col = (x - v->bborder) / (v->bwidth  + v->bgap) ;
  if (first_row < 0) first_row = 0 ;
  if (first_col < 0) first_col = 0 ;

  no_rows = (height + v->bheight) / (v->bheight + v->bgap) + 1 ;
  no_cols = (width  + v->bwidth)  / (v->bwidth  + v->bgap) + 1 ;
  if ((first_row + no_rows) > BROWS) no_rows = BROWS - first_row ;
  if ((first_col + no_cols) > BCOLS) no_cols = BCOLS - first_col ;

  for (v->row = first_row; v->row < (first_row + no_rows); v->row++)
    for (v->column = first_col; v->column < (first_col + no_cols); v->column++)
        draw_button(FCP_KEY, v->row, v->column, B_NORMAL) ;

  if (x < (v->bborder + (3 * (v->bwidth + v->bgap)))) grey_buttons(v->base) ;
}


void
make_modewin(x, y, width, height)  /* Draw special mode frame plus buttons. */
int x, y, width, height ;
{
  int column, first_col, first_row, no_cols, no_rows, row ;

  if (v->iscolor) color_area(FCP_MODE, x, y, width, height, C_BACK) ;
  else grey_area(FCP_MODE, x, y, width, height) ;

  first_row = (y - v->bborder) / (v->bheight + v->bgap) ;
  first_col = (x - v->bborder) / (v->bwidth  + v->bgap) ;

  no_rows = (height + v->bheight) / (v->bheight + v->bgap) + 1 ;
  no_cols = (width  + v->bwidth)  / (v->bwidth  + v->bgap) + 1 ;
  if ((first_row + no_rows) > MROWS) no_rows = MROWS - first_row ;
  if ((first_col + no_cols) > MCOLS) no_cols = MCOLS - first_col ;

  for (row = first_row; row < (first_row + no_rows); row++)
    for (column = first_col; column < (first_col + no_cols); column++)
      if (button_bstate(row * MCOLS + column) != B_NULL)
        draw_button(FCP_MODE, row, column, B_NORMAL) ;
}


void
make_registers()           /* Calculate memory register frame values. */
{
  char line[MAXLINE] ;     /* Current memory register line. */
  int n, y ;

  if (!v->rstate) return ;
  color_area(FCP_REG, 0, 0, v->rwidth, v->rheight, W_COLOR(C_MEMORY)) ;
  for (n = 0; n < MAXREGS; n++)
    {
      SPRINTF(line, "%1d   %s", n,  make_number(v->MPmvals[n]))  ;
      y = v->bborder + ((n-1) * v->bgap) + (v->fheight[(int) MFONT] * n) ;
      draw_text(v->bborder, y, FCP_REG, MFONT, B_COLOR(C_TEXT), line) ;
    }
}


void
process_event(type)       /* Process this event. */
int type ;
{
  int ival ;

  switch (type)
    {
      case KEYBOARD_DOWN   : clear_hilite() ;
                             if (v->pending)
                               {
                                 v->current = v->cur_ch ;
                                 do_pending() ;
                               }
                             else
                               {
                                 ival = get_index(v->cur_ch) ;
                                 if (ival < TITEMS)
                                   draw_button(v->curwin,
                                               v->row, v->column, B_INVERT) ;
                                 process_item(ival) ;
                               }
                             break ;

      case KEYBOARD_UP     : ival = get_index(v->cur_ch) ;
                             if (ival < TITEMS)
                               draw_button(v->curwin,
                                           v->row, v->column, B_NORMAL) ;
                             break ;

      case MOUSE_DRAGGING  : if (v->cury < v->ndisplay && v->curwin == FCP_KEY)
                               do_display_adjust() ;
                             else
                               {
                                 clear_hilite() ;
                                 do_mouse_dragging() ;
                               }
                             break ;

      case LEFT_DOWN       : if (v->cury < v->ndisplay && v->curwin == FCP_KEY)
                               {
                                 do_display_select() ;

/*  Setup values for a possible double click. */

                                 v->oldx     = v->curx ;
                                 v->oldy     = v->cury ;
                                 v->old_sec  = v->sec ;
                                 v->old_usec = v->usec ;
                               }
                             else
                               {
                                 clear_hilite() ;
                                 do_mouse_left_down() ;
                               }
                             break ;

      case MIDDLE_DOWN     : if (v->cury < v->ndisplay &&
                                 v->curwin == FCP_KEY  &&
                                 v->down != RIGHT_DOWN)
                               do_display_adjust() ;
                             break ;

      case RIGHT_DOWN      : clear_hilite() ;
                             do_mouse_right_down() ;
                             break ;

      case LEFT_UP         : do_mouse_left_up() ;
                             break ;

      case MIDDLE_UP       : break ;

      case RIGHT_UP        : break ;

      case TAKE_FROM_SHELF : clear_hilite() ;
                             handle_selection() ;
                             if (v->issel) process_str(v->selection) ;
                             break ;

      case PUT_ON_SHELF    : get_display() ;
                             break ;

      case SHOWHELP        : do_help() ;
    }                           
}


/* Process a portion of the parentheses stack. */

void
process_stack(startop, startnum, n)
int startop ;         /* Initial position in the operand stack. */
int startnum ;        /* Initial position in the numeric stack. */
int n ;               /* Number of items to process. */
{
  char sdisp[MAXLINE] ;     /* Used to save display contents. */
  int i ;
  int nptr ;                /* Pointer to next number from numeric stack. */

  STRCPY(sdisp, v->display) ;  /* Save current display. */
  nptr = startnum ;
  v->pending = 0 ;
  v->cur_op = '?' ;            /* Current operation is initially undefined. */
  for (i = 0; i < n; i++)
    {
      if (v->opstack[startop + i] == -1)
        {
          mpstr(v->MPnumstack[nptr++], v->MPdisp_val) ;
        }
      else
        {
          v->cur_ch = v->opstack[startop + i] ;
          if (v->cur_ch == '^')                    /* Control character? */
            {
              i++ ;
              v->cur_ch = CTL(v->opstack[startop + i]) ;
            }
          if (v->pending)
            {
              v->current = v->cur_ch ;
              do_pending() ;
            }
          else process_item(get_index(v->cur_ch)) ;
        }
    }
  v->numsptr = startnum ;
  push_num(v->MPdisp_val) ;
  v->opsptr = startop - 1 ;
  push_op(-1) ;
  save_pending_values(KEY_LPAR) ;
  STRCPY(v->display, sdisp) ;  /* Restore current display. */
}


void
process_str(str)
char *str ;
{
  int i, len ;

  len = strlen(str) ;
  for (i = 0 ; i < len; i++)
    {
      if (v->error) return ;
      if (v->pending)
        {
          v->current = str[i] ;
          do_pending() ;
        }
      else process_item(get_index(str[i])) ;
    }
}


void
set_item(itemno, str)
enum item_type itemno ;
char *str ;
{
  enum font_type ftype ;
  char *old_text ;
  int x, y ;
 
/*  If we are in the middle of processing parentheses input, then we
 *  should immediately return. The display would look a mess otherwise.
 *  There is one exception to this; when we want to show the current
 *  characters typed in during parenthesis processing. This can be
 *  determined by checking the show_paren flag.
 */

  if (v->opsptr && !v->show_paren) return ;

  old_text = v->item_text[(int) itemno] ;
  if (itemno == DISPLAYITEM)
    x = v->twidth - get_strwidth(v->items[(int) itemno].font, old_text) - 10 ;
  else x = v->items[(int) itemno].x ;
  y = v->items[(int) itemno].y ;
  ftype = v->items[(int) itemno].font ;
  draw_text(x, y, FCP_KEY, ftype, W_COLOR(C_DISPCOL), old_text) ;

  if (itemno == DISPLAYITEM)
    x = v->twidth - get_strwidth(v->items[(int) itemno].font, str) - 10 ;
  draw_text(x, y, FCP_KEY, ftype, B_COLOR(C_TEXT), str) ;
  STRCPY(v->item_text[(int) itemno], str) ;
}
