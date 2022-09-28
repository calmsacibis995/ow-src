#pragma ident "@(#)bbox.h	1.5 92/10/06"
/*
 *      Copyright (C) 1991  Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on some portions of this source
 *       code product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of Digital Equipment Corporation not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.
 */

#ifndef _bbox_h
#define _bbox_h

	/* for radio button boxes only */

extern ButtonBox RadioBBoxCreate      (  Scrn, char *  );
extern void      RadioBBoxAddButton   (  ButtonBox, char *, Boolean );
extern void      RadioBBoxSet         (  Button  );
extern char *    RadioBBoxGetCurrent  (  ButtonBox  );
extern void      RadioBBoxDeleteButton(  Button  );

        /* for other kinds of button boxes */

extern ButtonBox BBoxCreate           (  Scrn, char *  );
extern void      BBoxAddButton ( ButtonBox, char *, WidgetClass, Boolean );
extern void      BBoxDeleteButton     (  Button  );

        /* for all kinds of button boxes */

extern void      BBoxInit             (  void  );
extern void      BBoxEnable           (  Button  );
extern void      BBoxDisable          (  Button  );
extern Button    BBoxFindButtonNamed  (  ButtonBox, char *  );
extern Button    BBoxFindButton       (  ButtonBox, Widget  );
extern Button    BBoxButtonNumber     (  ButtonBox, int  );
extern int       BBoxNumButtons       (  ButtonBox  );
extern char *    BBoxNameOfButton     (  Button  );
extern Widget    BBoxMenuOfButton     (  Button  );
extern Widget    BBoxBBox             (  ButtonBox );
extern void      BBoxLockSize         (  ButtonBox  );
extern Boolean   BBoxIsGrandparent    (  ButtonBox, Widget  );

#endif /* _bbox_h */
