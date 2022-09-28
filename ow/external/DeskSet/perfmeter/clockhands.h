/*  @(#)clockhands.h 1.1 92/04/05 Copyright (c) Sun Microsystems Inc.  */

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

/* Structure defining data points for hands of clocktool. */

struct  hands {
  char x1, y1 ;            /* First origin for this position */
  char x2, y2 ;            /* Second origin for this position */
  char sec_x, sec_y ;      /* Second-hand origin for this minute */
  char min_x, min_y ;      /* End of minute and second hands */
  char hour_x, hour_y ;    /* End of hour hand */
} ;

extern struct hands hand_points[] ;

/* The following hands stuff is for the roman face. */

struct  endpoints {
  char  x ;
  char  y ;
} ;

extern struct endpoints ep_min[] ;
extern struct endpoints ep_hr[] ;
extern struct endpoints ep_sec[] ;
extern struct endpoints ep_secorg[] ;
