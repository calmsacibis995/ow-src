
/*  @(#)extern.h 1.6 93/05/11 Copyright (c) Sun Microsystems Inc.
 *
 *  Copyright (c) 1987 - 1990, Sun Microsystems, Inc.  All Rights Reserved.
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

extern Vars v ;                 /* Calctool variables and options. */

extern char *base_str[] ;       /* Strings for each base value. */
extern char *calc_res[] ;       /* Calctool X resources. */
extern char *cmdstr[] ;         /* Command line options (for saving). */
extern char digits[] ;          /* Valid numerical digits. */
extern char *dtype_str[] ;      /* Strings for each display mode value. */
extern char *hstrs[] ;          /* Help strings. */
extern char *lstrs[] ;          /* Labels for various XView items. */
extern char menu_entries[] ;    /* Keyboard equivalent of menu items. */
extern char *mess[] ;           /* Message strings. */
extern char *mode_str[] ;       /* Strings for each mode value. */
extern char *mpstrs[] ;         /* MP errors (visible with -D option). */
extern char *mstrs[] ;          /* Mode titles for the popup panel. */
extern char *opts[] ;           /* Command line option strings. */
extern char *sstrs[] ;          /* Scales. */
extern char *ttype_str[] ;      /* Strings for each trig type value. */
extern char *ustrs[] ;          /* Usage message strings. */
extern char *vstrs[] ;          /* Various strings. */
extern char validkeys[] ;       /* Valid keys after an error condition. */

extern char *Rbstr[] ;          /* Base mode X resource strings. */
extern char *Rdstr[] ;          /* Display mode X resource strings. */
extern char *Rmstr[] ;          /* Mode mode X resource strings. */
extern char *Rtstr[] ;          /* Trig mode X resource strings. */

extern double max_fix[] ;       /* Maximum showable fixed values. */

extern int basevals[] ;         /* Supported arithmetic bases. */
extern int cornerR[] ;          /* Radius for corners at each scale. */
extern int cornerX[] ;          /* X offset within server image. */
extern int cornerY[] ;          /* Y offsets within server image. */
extern int disp_length[] ;      /* Character display length for each base. */
extern int bsizes[] ;           /* Border sizes at each scale. */
extern int fsizes[] ;           /* Font sizes at each scale. */
extern int gsizes[] ;           /* Gap sizes at each scale. */
extern int left_pos[] ;         /* "Left-handed" positions. */
extern int msizes[] ;           /* Menu glyph sizes at each scale. */
extern int right_pos[] ;        /* "Right-handed" positions. */
extern int scaleH[] ;           /* Minimum button height at each scale. */
extern int scaleW[] ;           /* Minimum button width at each scale. */

extern struct iteminfo items[] ;           /* Panel items. */
extern struct button buttons[] ;           /* Calculator button values. */
extern struct button mode_buttons[] ;      /* Special "mode" buttons. */
extern struct menu cmenus[] ;              /* Calculator menus. */
