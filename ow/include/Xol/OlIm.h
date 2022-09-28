#ifndef _XOL_IM_H
#define _XOL_IM_H

#pragma	ident	"@(#)OlIm.h	1.8	93/05/03 include/Xol SMI"	/* OLIT	AMBER */

/*
 *	Copyright (C) 1986, 1993  Sun Microsystems, Inc
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

#include <X11/Intrinsic.h>

#include <Xol/OpenLookP.h>


/************************************************************* 
 * 
 * API for managing ICs
 * 
 *************************************************************/

/*---------------------------------------------------------------------------
 * OlCreateIC creates and returns an OlInputContextID and adds it to a
 * list of ICs in the DisplayShellWidget. It stores im_id in the OlInputContextID
 * to use when calling XCreateIC.
 * 
 * The ArgList args is for initializing pre-edit resources. Status resources
 * are expected to be initialized by the toolkit. However, if one must set
 * a status resource, use OlSetValuesIC(ic_id, FALSE, ...) etc.
 * 
 * This routine also registers a routine to XtNdestroyCallback to
 * destroy the IC when the widget is destroyed.
 *
 * It returns NULL if preedit_type is not supported by the IM corresponding
 * to the OlInputMethodID.
 */
extern OlInputContextID OlCreateIC(OlInputMethodID im_id,
				   Widget          w,
				   OlImPreeditStyle preedit_type,
                                   ArgList         args,
                                   Cardinal        num_args);

/*---------------------------------------------------------------------------
 * OlDestroyIC free storage for the OlInputContextID
 * and destroys its XIC if it had been created.
 */
extern void		OlDestroyIC(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlRealizeIC returns TRUE, for ic_id, if its XIC is initially NULL and
 * attempting to create it succeeds.
 * Otherwise, it returns FALSE.
 */
extern Boolean		OlRealizeIC(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlWidgetHasIC returns TRUE, for a widget w, if a call to OlCreateIC has
 * been made with w as its widget parameter and the IC has not yet been destroyed
 * either through the destroy callback or by calling OlDestroyIC.
 * Otherwise, it returns FALSE.
 */
extern Boolean		OlWidgetHasIC(Widget w);

/*---------------------------------------------------------------------------
 * OlIsICRealized returns TRUE, for ic_id, if the XIC in it is non-NULL.
 * Otherwise, it returns FALSE.
 */
extern Boolean		OlIsICRealized(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlSetValuesIC caches values of attributes,
 * passed through the ArgList, in the OlInputContextID and flags the cache dirty.
 * If for_preedit is TRUE, the resources are interpreted as Preedit resources
 * else they are interpreted as Status resources.
 * If the XIC has been
 * created and is in focus, it propagates the cached values to the XIC
 * stored in the entry and flags the cache clean.
 *
 * Note: The following resources cannot be accessed using this routine.
 *      XNInputStyle
 *	XNClientWindow
 *	XNFocusWindow
 *	XNResourceName
 *	XNResourceName
 *	XNResourceClass
 *	XNGeometryCallback
 *	
 */
extern Boolean		OlSetValuesIC(OlInputContextID ic_id,
				      Boolean	       for_preedit,
				      ArgList          args,
				      Cardinal         num_args);

/*---------------------------------------------------------------------------
 * OlVaSetValuesIC is a varargs version of OlSetValuesIC
 */
extern Boolean		OlVaSetValuesIC(OlInputContextID ic_id,
					Boolean	         for_preedit,
					...);

/*---------------------------------------------------------------------------
 * OlGetValuesIC gets information about requested attributes,
 * passed through the ArgList, from the OlInputContextID.
 * If for_preedit is TRUE, the resources are interpreted as Preedit resources
 * else they are interpreted as Status resources.
 * If the cache is dirty, it propagates these
 * fields to the XIC (creating it if necessary exactly as in OlSetFocusIC),
 * flags the cache clean,
 * extracts the attributes from the XIC and returns TRUE (FALSE if creating
 * the IC failed).
 *
 * Note: The following resources cannot be accessed using this routine.
 *      XNInputStyle
 *	XNClientWindow
 *	XNFocusWindow
 *	XNResourceName
 *	XNResourceName
 *	XNResourceClass
 *	XNGeometryCallback
 */
extern Boolean		OlGetValuesIC(OlInputContextID ic_id,
				      Boolean	       for_preedit,
				      ArgList          args,
				      Cardinal         num_args);

/*---------------------------------------------------------------------------
 * OlVaGetValuesIC is a varargs version of OlGetValuesIC.
 */
extern Boolean		OlVaGetValuesIC(OlInputContextID ic_id,
					Boolean	         for_preedit,
					...);

/*---------------------------------------------------------------------------
 * OlGetInputStyleOfIC returns the value of the resource XNInputStyle
 * only if the IC is realized.
 * Otherwise it returns (XIMStyle) 0.
 */
extern XIMStyle		OlGetInputStyleOfIC(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlSetFocusIC does the following. If the XIC has not
 * been created, attempt to create it with all the attributes stored in the
 * cache and flag the cache clean. In addition, it will also set the values
 * for the following resources:
 *            XNClientWindow
 *            XNFocusWindow
 *            XNInputStyle
 * Note: The above resources cannot be accessed by OlSetICValues.
 *
 * If unable to create it, return FALSE. If
 * successful, do XSetICFocus on the XIC and return TRUE.
 */
extern Boolean		OlSetFocusIC(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlUnsetFocusIC does XUnsetICFocus on the XIC of ic_id.
 * If !OlIsRealized(ic_id), then this is a no-op.
 */
extern void		OlUnsetFocusIC(OlInputContextID ic_id);

/*---------------------------------------------------------------------------
 * OlXICOfIC returns the XIC of ic_id.
 * If !OlIsRealized(ic_id), it returns NULL.
 */
extern XIC		OlXICOfIC(OlInputContextID ic_id);

/************************************************************* 
 * 
 * Private API for shell's IC management
 * 
 *************************************************************/

/*---------------------------------------------------------------------------
 *
 *	OlRealizeFirstIC
 *
 *             get the im related record for the vendor shell from
 *             its display shell and create the XIC for the first IC
 *             if none of have been created yet.
 *             This routine is to be called from the Realize method of
 *             the vendor shell. This is to ensure that the status gets
 *             displayed correctly.
 *
 */
extern void	OlRealizeFirstIC(Widget vw);

/*---------------------------------------------------------------------------
 *
 *	OlSetIMStatusOnAllICs
 *
 *             passes the status attributes from the vendor shell to
 *             the ICs in the ImVSInfo record in the DisplayShell
 *
 */
extern void	OlSetIMStatusOnAllICs(Widget	vw,
				      ArgList	args,
				      Cardinal	num_args);

/************************************************************* 
 * 
 * API for managing IMs.
 * 
 *************************************************************/

/*---------------------------------------------------------------------------
 * OlCreateIM does the following. If im_modifier is not null it does an
 * XSetLocaleModifiers with the string "@im=<im_modifier>". It then opens
 * an IM connection for display.
 * If opening a connection succeeds, it creates an OlInputMethodID and stores
 * the XIM value in it. Additionally, it queries the XIM for all supported
 * pre-edit styles and stores it in the OlInputMethodID. Finally it returns
 * the OlInputMethodID.
 *
 * It also registers a routine to XtNdestroyCallback of the DisplayShell
 * to destroy the OlInputMethodID.
 *
 * Returns NULL if it fails to open an IM connection.
 */
extern OlInputMethodID  OlCreateIM(Display*	display,
				   String	im_modifier,
				   XrmDatabase	rdb);

/*---------------------------------------------------------------------------
 * OlDestroyIM closes the IM connection corresponding to im_id and frees
 * storage for it.
 */
extern void		OlDestroyIM(OlInputMethodID im_id);

/*---------------------------------------------------------------------------
 * OlXIMOfIM returns the XIM corresponding to im_id.
 */
extern XIM		OlXIMOfIM(OlInputMethodID im_id);

/*---------------------------------------------------------------------------
 * OlDefaultIMOfWidget returns the default OlInputMethodID for the
 * display to which widget w belongs. The first call to this function
 * calls OlCreateIM.
 */
extern OlInputMethodID	OlDefaultIMOfWidget(Widget w);

#endif	/* _XOL_IM_H */
