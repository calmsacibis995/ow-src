#ifndef _XOL_IM_I_H
#define _XOL_IM_I_H

#pragma       ident   "@(#)OlImI.h 1.4     92/04/22 SMI"        /* OLIT */

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

#include <locale.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/txtbufCA.h>
#include <Xol/RootShellP.h>
#include <Xol/OlIm.h>
#include <Xol/OlImP.h>

#define ICA_PREEDIT 0x01
#define ICA_STATUS  0x02
#define ICA_CLIENTW 0x04
#define ICA_FOCUSW  0x08


typedef	struct _InputMethodRec {
    XIM			      xim;
    OlInputContextID	      last_ic_used; /* for propagating attribs */
    Boolean		      is_last_ic_focused;
    Cardinal		      num_xics; /* number of XICs (not ic_ids)
					 * created using this im
					 */
    Boolean		      is_mask_augmented;
    EventMask		      augment_mask;
    XIMStyles *               supported_styles;
} InputMethodRec;		/* OlInputMethodID is an opaque pointer to this */

typedef struct _IcResourceInfo {
    char *	string;
    XrmName	name;
    Cardinal	deep_size;
} IcResourceInfo, *IcResourceInfoList;

typedef struct _IcResourceRec {
    XtArgVal	value;
    Boolean	changed;
} IcResource, *IcResourceList;

typedef struct _InputContextRec {
    struct _ImVSInfo *	      ivs; /* record containing it, for efficiency */
    Widget		      w; /* the widget to which this IC is attached */
    OlInputMethodID	      im_id; /* the im to be used for this IC */
    XIC			      xic;   /* NULL implies not realized */
    XIMStyle		      input_style;
  /* Cached Attributes */
    char		      dirty; /* mask for preedit, status, clientwin, focuswin */
    IcResourceList	      preedit_res; /* malloc space based on num_resources */
    IcResourceList	      status_res; /* malloc space based on num_resources */
    struct _InputContextRec * next;
} InputContextRec;		/* OlInputContextID is an opaque pointer to this */


#endif	/* _XOL_IM_I_H */
