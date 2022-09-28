#pragma       ident   "@(#)OlIm.c 1.20     97/03/26 SMI"        /* OLIT */

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

#include <stdarg.h>		/* for varargs */
#include <libintl.h>		/* for dgettext */
#include <assert.h>
#include <string.h>

#include <Xol/OlImI.h>

/************************************************************* 
 * 
 * Static variables
 * 
 *************************************************************/

static IcResourceInfo pe_ic_resources[] = {
    { XNArea,			(XrmName) NULL,	sizeof(XRectangle) },
    { XNAreaNeeded,		(XrmName) NULL,	sizeof(XRectangle) },
    { XNSpotLocation,		(XrmName) NULL,	sizeof(XPoint) },
    { XNColormap,		(XrmName) NULL, 0 },
    { XNForeground,		(XrmName) NULL, 0 },
    { XNBackground,		(XrmName) NULL, 0 },
    { XNBackgroundPixmap,	(XrmName) NULL, 0 },
    { XNFontSet,		(XrmName) NULL, 0 },
    { XNLineSpace,		(XrmName) NULL, 0 },
    { XNCursor,			(XrmName) NULL, 0 },
    { XNPreeditStartCallback,	(XrmName) NULL, sizeof(XIMCallback) },
    { XNPreeditDoneCallback,	(XrmName) NULL, sizeof(XIMCallback) },
    { XNPreeditDrawCallback,	(XrmName) NULL, sizeof(XIMCallback) },
    { XNPreeditCaretCallback,	(XrmName) NULL, sizeof(XIMCallback) }
};
    
static IcResourceInfo st_ic_resources[] = {
    { XNArea,			(XrmName) NULL,	sizeof(XRectangle) },
    { XNAreaNeeded,		(XrmName) NULL,	sizeof(XRectangle) },
    { XNSpotLocation,		(XrmName) NULL,	sizeof(XPoint) },
    { XNColormap,		(XrmName) NULL, 0 },
    { XNForeground,		(XrmName) NULL, 0 },
    { XNBackground,		(XrmName) NULL, 0 },
    { XNBackgroundPixmap,	(XrmName) NULL, 0 },
    { XNFontSet,		(XrmName) NULL, 0 },
    { XNLineSpace,		(XrmName) NULL, 0 },
    { XNCursor,			(XrmName) NULL, 0 },
    { XNStatusStartCallback,	(XrmName) NULL, sizeof(XIMCallback) },
    { XNStatusDoneCallback,	(XrmName) NULL, sizeof(XIMCallback) },
    { XNStatusDrawCallback,	(XrmName) NULL, sizeof(XIMCallback) },
};
/************************************************************* 
 * 
 * Private function declarations
 * 
 *************************************************************/
static void		initialize_resource_info(void);

static InputContextRec *create_ic(ImVSInfo *	  ivs,
				  OlInputMethodID im_id,
				  Widget	  w,
				  XIMStyle	  style);
static void		destroy_ic(InputContextRec * ic_id);

static XIMStyle	        xim_style_for_preedit_type(
				      ImVSInfo *      ivs,
			              OlInputMethodID im_id,
			              Widget          w,
			              OlImPreeditStyle preedit_type);

static void		destroy_ic_cb(Widget	w,
				      XtPointer client_data,
				      XtPointer call_data);

static void             im_filter_dummy_event_handler(
				      Widget	w,
				      XtPointer	client_data,
				      XEvent *	event,
				      Boolean *	continue_to_dispatch);

static XIC		create_xic(InputContextRec * ic_rec);

static Cardinal		count_va_arg_list(va_list var);

static ArgList		va_create_arg_list(va_list      var,
					   register int count);

static int		find_preedit_res_index(String		name,
					       unsigned int *	size);

static int		find_status_res_index(String		name,
					      unsigned int *	size);

static void		set_resource(OlInputContextID ic_id,
				     Boolean          for_preedit,
				     Arg	      arg);

static Boolean		set_resources_on_xic(OlInputContextID ic_id);

static void		compile_sv_resources(OlInputContextID ic_id,
					     XVaNestedList *  pe_attr,
					     XVaNestedList *  st_attr);

static void		compile_gv_resources(Boolean		for_preedit,
					     ArgList		args,
					     Cardinal		num_args,
					     XVaNestedList *	attr);

static void		set_status_resources(Widget		shw,
					     OlInputContextID	ic_id);

#ifdef DEBUG
static void		show_resources(XPointer *at, int count);
#endif

/************************************************************* 
 * 
 * Private function definitions
 * 
 *************************************************************/

#ifdef DEBUG

static void
show_resources(XPointer *at, int count)
{
    register int i;

    for (i = 0; i < count; i++) {
	fprintf(stderr, "\t%s\t= ", at[i]);
        if (!strcmp(XNInputStyle, at[i])) {
	    char *preedit_str;
	    char *status_str;
	    XIMStyle im_style = (XIMStyle) at[++i];

	    if (im_style & XIMPreeditArea)
		preedit_str = "XIMPreeditArea";
	    else if (im_style & XIMPreeditNothing)
		preedit_str = "XIMPreeditNothing";
	    else if (im_style & XIMPreeditNone)
		preedit_str = "XIMPreeditNone";
	    else if (im_style & XIMPreeditPosition)
		preedit_str = "XIMPreeditPosition";
	    else if (im_style & XIMPreeditCallbacks)
		preedit_str = "XIMPreeditCallbacks";

	    if (im_style & XIMStatusArea)
		status_str = "XIMStatusArea";
	    else if (im_style & XIMStatusNothing)
		status_str = "XIMStatusNothing";
	    else if (im_style & XIMStatusNone)
		status_str = "XIMStatusNone";
	    else if (im_style & XIMStatusCallbacks)
		status_str = "XIMStatusCallbacks";

	    fprintf(stderr, " %s | %s", preedit_str, status_str);
	}
        else if (!strcmp(XNClientWindow, at[i])) {
	    fprintf(stderr, " %ld", (Window) at[++i]);
	}
        else if (!strcmp(XNFocusWindow, at[i])) {
	    fprintf(stderr, " %ld", (Window) at[++i]);
	}
        else if (!strcmp(XNPreeditAttributes, at[i])) {
	    fprintf(stderr, " %p", at[++i]);
	}
        else if (!strcmp(XNStatusAttributes, at[i])) {
	    fprintf(stderr, " %p", at[++i]);
	}
        else if (!strcmp(XNArea, at[i])) {
	    XRectangle x = *(XRectangle *)at[++i];
	    fprintf(stderr, " { %d %d %d %d }", x.x, x.y, x.width, x.height);
	}
        else if (!strcmp(XNAreaNeeded, at[i])) {
	    XRectangle x = *(XRectangle *)at[++i];
	    fprintf(stderr, " { %d %d %d %d }", x.x, x.y, x.width, x.height);
	}
        else if (!strcmp(XNSpotLocation, at[i])) {
	    XPoint x = *(XPoint *)at[++i];
	    fprintf(stderr, " { %d %d }", x.x, x.y);
	}
        else if (!strcmp(XNColormap, at[i])) {
	    fprintf(stderr, " %p", at[++i]);
	}
        else if (!strcmp(XNForeground, at[i])) {
	    fprintf(stderr, " %ld", (Pixel) at[++i]);
	}
        else if (!strcmp(XNBackground, at[i])) {
	    fprintf(stderr, " %ld", (Pixel) at[++i]);
	}
        else if (!strcmp(XNBackgroundPixmap, at[i])) {
	    fprintf(stderr, " %p", (Pixmap) at[++i]);
	}
        else if (!strcmp(XNFontSet, at[i])) {
	    fprintf(stderr, " %p", at[++i]);
	}
        else if (!strcmp(XNLineSpace, at[i])) {
	    fprintf(stderr, " %d", (int) at[++i]);
	}
        else if (!strcmp(XNCursor, at[i])) {
	    fprintf(stderr, " %p", (Cursor) at[++i]);
	}
        else if (!strcmp(XNPreeditStartCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNPreeditDoneCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNPreeditDrawCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNPreeditCaretCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNStatusStartCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNStatusDoneCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	}
        else if (!strcmp(XNStatusDrawCallback, at[i])) {
	    XIMCallback x = *(XIMCallback *)at[++i];
	    fprintf(stderr, " { %p %p }", x.client_data, x.callback);
	} else
	    i++;
	fprintf(stderr, "\n");
    }
}

#endif

static void
initialize_resource_info(void)
{
    register IcResourceInfo *	res_info = &(pe_ic_resources[0]);
    register int		i;

    if (res_info->name == (XrmName) NULL) {
	for (i = 0; i < XtNumber(pe_ic_resources); i++) {
	    res_info = &(pe_ic_resources[i]);
	    if ((res_info->name = XrmStringToName(res_info->string))
		  == (XrmName) NULL)
		OlError(dgettext(OlMsgsDomain,
			"OlIM: IC Preedit Resource Info List Corrupt."));
	}
	for (i = 0; i < XtNumber(st_ic_resources); i++) {
	    res_info = &(st_ic_resources[i]);
	    if ((res_info->name = XrmStringToName(res_info->string))
		  == (XrmName) NULL)
		OlError(dgettext(OlMsgsDomain,
			"OlIM: IC Status Resource Info List Corrupt."));
	}
    }
}
 
static InputContextRec *
create_ic(ImVSInfo *	  ivs,
	  OlInputMethodID im_id,
	  Widget	  w,
	  XIMStyle	  style)
{
    InputContextRec *	temp;
    
    temp = (InputContextRec *) XtMalloc(sizeof(InputContextRec));
    temp->ivs         = ivs;
    temp->w           = w;
    temp->im_id       = im_id;
    temp->xic         = (XIC) NULL;
    temp->input_style = style;
    temp->dirty       = ICA_CLIENTW | ICA_FOCUSW;
    temp->preedit_res = (IcResourceList) XtCalloc(
			  XtNumber(pe_ic_resources), sizeof(IcResource));
    temp->status_res  = (IcResourceList) XtCalloc(
			  XtNumber(st_ic_resources), sizeof(IcResource));
    initialize_resource_info();	/* maybe */
    return(temp);
}

static void
destroy_ic(InputContextRec * ic_id)
{
    register int	i;
    
    XtRemoveCallback(ic_id->w, XtNdestroyCallback, destroy_ic_cb, ic_id);

    if (ic_id->xic != (XIC) NULL) {
	XDestroyIC(ic_id->xic);
	/* If this is the last ic using the im remove the filter event
	 * part of the event mask from the shell
	 */
	if (ic_id->im_id->num_xics == 1 &&
	    ic_id->im_id->is_mask_augmented)
	{
	    XtRemoveEventHandler(ic_id->ivs->vw, ic_id->im_id->augment_mask,
				 FALSE, im_filter_dummy_event_handler,
				 (XtPointer) ic_id->im_id);
	    ic_id->im_id->is_mask_augmented = FALSE;
	}
	ic_id->im_id->num_xics--;
    }
    if (ic_id->im_id->last_ic_used == ic_id) {
	ic_id->im_id->last_ic_used       = (OlInputContextID) NULL;
	ic_id->im_id->is_last_ic_focused = FALSE;
    }
    
    for (i = 0; i < XtNumber(pe_ic_resources); i++)
	if (pe_ic_resources[i].deep_size != 0			&&
	    ic_id->preedit_res[i].value != (XtArgVal) NULL)
	{
	    XtFree((XtPointer) ic_id->preedit_res[i].value);
	}
    for (i = 0; i < XtNumber(st_ic_resources); i++)
	if (st_ic_resources[i].deep_size != 0			&&
	    ic_id->status_res[i].value != (XtArgVal) NULL)
	{
	    XtFree((XtPointer) ic_id->status_res[i].value);
	}

    XtFree((XtPointer) ic_id->preedit_res);
    XtFree((XtPointer) ic_id->status_res);
    XtFree((XtPointer) ic_id);
}

/*
 * Check if im and w are on the same display.
 *
 * Returns XIMStyle mask if im supports preedit_style + status_style.
 * 0 otherwise
 */
static XIMStyle
xim_style_for_preedit_type(ImVSInfo *	   ivs,
			   OlInputMethodID im_id,
			   Widget          w,
			   OlImPreeditStyle preedit_type)
{
    if (XtDisplay(w) == XDisplayOfIM(OlXIMOfIM(im_id))) {
	OlImStatusStyle im_status_style;
	Arg	       args[1];
	XIMStyle *     supported_styles =
	    im_id->supported_styles->supported_styles;
	int            count            =
	    im_id->supported_styles->count_styles;
	XIMStyle       required_style   = (XIMStyle) 0;
	register int   i;

	XtSetArg(args[0], XtNimStatusStyle, &im_status_style);
	XtGetValues(ivs->vw, args, 1);
	switch (im_status_style) {
	case OL_IM_DISPLAYS_IN_CLIENT:
	    required_style |= XIMStatusArea;
	    break;
	case OL_IM_DISPLAYS_IN_ROOT:
	    required_style |= XIMStatusNothing;
	    break;
	case OL_NO_STATUS:
	    required_style |= XIMStatusNone;
	    break;
	}
    
	switch (preedit_type) {
	case OL_OVER_THE_SPOT:
	    required_style |= XIMPreeditPosition;
	    break;
	case OL_ON_THE_SPOT:
	    required_style |= XIMPreeditCallbacks;
	    break;
	case OL_ROOT_WINDOW:
	    required_style |= XIMPreeditNothing;
	    break;
	case OL_NO_PREEDIT:
	    required_style |= XIMPreeditNone;
	    break;
	}

	for (i = 0; i < count; i++)
	    if ((supported_styles[i] & required_style) == required_style)
		return(required_style);
    }
    return((XIMStyle) 0);
}

/*
 * This callback is registered by OlCreateIC in XtNdestroyCallback
 * of the widget.
 */
static void
destroy_ic_cb(Widget	w,
	      XtPointer client_data,
	      XtPointer call_data)
{
    OlInputContextID ic_id = (OlInputContextID) client_data;

    OlDestroyIC(ic_id);
}

/*
 * This event handler is added for the event mask got by intersecting
 * the shell widget's event mask and the one returned from the
 * XNFilterEvents from the XIC. This seems to be the most acceptable
 * way of augmenting the event mask for the shell widget.
 */
static void
im_filter_dummy_event_handler(Widget	w,
			      XtPointer	client_data,
			      XEvent *	event,
			      Boolean *	continue_to_dispatch)
{
    /*
     * We will not affect the way this event is handled otherwise
     */
    *continue_to_dispatch = TRUE;
}

/*
 * Create XIC with required arguments:
 *   XNInputStyle
 *   XNClientWindow
 *   XNFocusWindow
 *  PE:
 *  - XNSpotLocation for XIMPreeditPosition
 *  + XNFontSet for XIMPreeditPosition & XIMPreeditArea
 *  - PreeditCallbacks for XIMPreeditCallback
 *  Status:
 *  + XNFontSet for XIMStatusArea
 *  - StatusCallbacks for XIMStatusCallback
 *
 *  + Pick these from cache or default to Vendor resources
 *  - Pick these from cache only. So, ensure presence or flag error.
 *
 *  Returns NULL if it fails due to any reason (to be detailed)
 */
static XIC
create_xic(InputContextRec * ic_rec)
{
    XVaNestedList	pe_attr;
    XVaNestedList	st_attr;
    XPointer            ic[11];
    Widget              vw                 = ic_rec->ivs->vw;
    Boolean		to_set_filter_mask = FALSE;
    int                 i = 0;
				/*
				 * Five resources:
				 *  XNInputStyle
				 *  XNClientWindow
				 *  XNFocusWindow
				 *  XNPreeditAttributes
				 *  XNStatusAttributes
				 */
    /*
     * Add Status resources by looking at resources
     * in the VendorShell
     */
    if (ic_rec->input_style & XIMStatusArea)
	set_status_resources(vw, ic_rec);
    
    compile_sv_resources(ic_rec, &pe_attr, &st_attr);

    ic[i++] = (XPointer) XNInputStyle;
    ic[i++] = (XPointer) ic_rec->input_style;

    if (XtIsRealized(vw)) {
	ic[i++] = (XPointer) XNClientWindow;
	ic[i++] = (XPointer) XtWindow(vw);
	ic_rec->dirty &= ~ICA_CLIENTW;

	to_set_filter_mask = !(ic_rec->im_id->is_mask_augmented) &&
	                      (ic_rec->im_id->num_xics == 0);
    }
    if (XtIsRealized(ic_rec->w)) {
	ic[i++] = (XPointer) XNFocusWindow;
	ic[i++] = (XPointer) XtWindow(ic_rec->w);
	ic_rec->dirty &= ~ICA_FOCUSW;
    }
    if (pe_attr != (XVaNestedList) NULL) {
	ic[i++] = (XPointer) XNPreeditAttributes;
	ic[i++] = (XPointer) pe_attr;
    }
    if (st_attr != (XVaNestedList) NULL) {
	ic[i++] = (XPointer) XNStatusAttributes;
	ic[i++] = (XPointer) st_attr;
    }
    ic[i] = (XPointer) NULL;

#ifdef DEBUG
    if (i) {
	fprintf(stderr, "XCreateIC(\n");
	show_resources(ic, i);
	fprintf(stderr, ");\n");
    }
#endif

    ic_rec->xic = XCreateIC(OlXIMOfIM(ic_rec->im_id),
		    ic[0],  ic[1],  ic[2],  ic[3],
		    ic[4],  ic[5],  ic[6],  ic[7],
		    ic[8],  ic[9],  ic[10]);

    if (ic_rec->xic != (XIC) NULL) {
	ic_rec->im_id->num_xics++;
	/* If first xic for im, indicate xic resource propagation */
	if (ic_rec->im_id->num_xics == 1) {
	    ic_rec->im_id->last_ic_used       = ic_rec;
	    ic_rec->im_id->is_last_ic_focused = FALSE;
	}
	/* Augment the event mask for the shell window if necessary */
	if (to_set_filter_mask) {
	    /*
	     * The following are true at this point:
	     *  + the shell is realized
	     *  + the first xic has just been successfully created
	     *  + the event_mask for IM has not yet been augmented
	     *    to the shell
	     */
	    EventMask	fil_ev_mask;
	
	    if (XGetICValues(ic_rec->xic,
			     XNFilterEvents, &fil_ev_mask,
			     NULL) != (char *) NULL)
		OlError(dgettext(OlMsgsDomain,
			 "OlIM: cannot do getvalues on XNFilterEvents."));
	    else {
		EventMask	w_ev_mask = XtBuildEventMask(vw);
		EventMask	aug_mask  = (EventMask) 0;

		/* bits ON in fil_ev_mask that are
		 * not already ON in w_ev_mask */
		aug_mask = fil_ev_mask & ~w_ev_mask;
		if (aug_mask != (EventMask) 0)
		    XtAddEventHandler(vw, aug_mask, FALSE,
				      im_filter_dummy_event_handler,
				      (XtPointer) ic_rec->im_id);
		ic_rec->im_id->is_mask_augmented = TRUE;
		ic_rec->im_id->augment_mask      = aug_mask;
	    }
	}
    }
    if (pe_attr != (XVaNestedList) NULL)
	XtFree((XtPointer) pe_attr);
    if (st_attr != (XVaNestedList) NULL)
	XtFree((XtPointer) st_attr);

    return(ic_rec->xic);
}

/* 
 *   Given a variable length attribute-value list, count_va_arg_list()
 *   counts and returns the number of attribute-value pairs in it.
 */
static Cardinal
count_va_arg_list(va_list var)
{
    int		count = 0;
#if defined(__ppc)
    XtArgVal tmpval;
#endif
    String	attr;

    /*
     * Count the number of attribute-value pairs in the list.
     * Note: The count is required only to allocate enough space to store
     * the list.
     */
    for(attr = va_arg(var, String) ; attr != NULL;
                        attr = va_arg(var, String)) {
        ++count;
#if defined(__ppc)
	tmpval =
#endif
	va_arg(var, XtArgVal);
    }
    return count;
}

static ArgList
va_create_arg_list(va_list var, register int count)
{
    String	attr;
    ArgList	avlist;

    avlist = (ArgList) XtCalloc((int)count + 1, (unsigned)sizeof(Arg));

    for(attr = va_arg(var, String), count = 0; attr != NULL; 
		attr = va_arg(var, String)) {
	avlist[count].name = attr;
	avlist[count].value = va_arg(var, XtArgVal);
	++count;
    }
    avlist[count].name = NULL;
    return avlist;
}

static int
find_preedit_res_index(String		name,
		       unsigned int *	size)
{
    register int	i;
    XrmName		Qname = XrmStringToName(name);
    
    for (i = 0; i < XtNumber(pe_ic_resources); i++) {
	IcResourceInfo *	res_info = &(pe_ic_resources[i]);

	if (Qname == res_info->name) {
	    *size = res_info->deep_size;
	    return(i);
	}
    }
    return(-1);
}

static int
find_status_res_index(String		name,
		      unsigned int *	size)
{
    register int	i;
    XrmName		Qname = XrmStringToName(name);
    
    for (i = 0; i < XtNumber(st_ic_resources); i++) {
	IcResourceInfo *	res_info = &(st_ic_resources[i]);

	if (Qname == res_info->name) {
	    *size = res_info->deep_size;
	    return(i);
	}
    }
    return(-1);
}

static void
set_resource(OlInputContextID ic_id,
	     Boolean          for_preedit,
	     Arg	      arg)
{
    int			idx;
    unsigned int	size;

    if (for_preedit) {
	if ((idx = find_preedit_res_index(arg.name, &size)) >= 0) {
	    if (size) {
		if (ic_id->preedit_res[idx].value == (XtArgVal) NULL)
		    ic_id->preedit_res[idx].value = (XtArgVal) XtMalloc(size);
		memcpy((void *) ic_id->preedit_res[idx].value,
		       (void *) arg.value, (size_t) size);
	    } else
		ic_id->preedit_res[idx].value   = arg.value;
	    
	    ic_id->preedit_res[idx].changed = TRUE;
	    ic_id->dirty |= ICA_PREEDIT;
	}
    } else {
	if ((idx = find_status_res_index(arg.name, &size)) >= 0) {
	    if (size) {
		if (ic_id->status_res[idx].value == (XtArgVal) NULL)
		    ic_id->status_res[idx].value = (XtArgVal) XtMalloc(size);
		memcpy((void *) ic_id->status_res[idx].value,
		       (void *) arg.value, (size_t) size);
	    } else
		ic_id->status_res[idx].value   = arg.value;
	    
	    ic_id->status_res[idx].changed = TRUE;
	    ic_id->dirty |= ICA_STATUS;
	}
    }
}

/*
 * If any resources are present in the cache, propagate them
 * to the XIC(creating it if necessary) and flag the cache clean.
 * Return TRUE if it succeeds else FALSE.
 */
static Boolean
set_resources_on_xic(OlInputContextID ic_id)
{
    XVaNestedList	pe_attr     = (XVaNestedList) NULL;
    XVaNestedList	st_attr     = (XVaNestedList) NULL;
    XPointer		ic_attr[9];
    int			i           = 0;
    char *              ret_val     = (char *) NULL;
    
    /* Create the XIC if not yet created */
    if (ic_id->xic == (XIC) NULL) {
	if ((ic_id->xic = create_xic(ic_id)) == (XIC) NULL)
	    return(FALSE);
    }
    if (ic_id->dirty & ICA_CLIENTW) {
	Widget	vw = ic_id->ivs->vw;

	if (XtIsRealized(vw)) {
	    ic_attr[i++] = (XPointer) XNClientWindow;
	    ic_attr[i++] = (XPointer) XtWindow(vw);
	    ic_id->dirty &= ~ICA_CLIENTW;
	}
    }
    if (ic_id->dirty & ICA_FOCUSW) {
 	if (XtIsRealized(ic_id->w)) {
	    ic_attr[i++] = (XPointer) XNFocusWindow;
	    ic_attr[i++] = (XPointer) XtWindow(ic_id->w);
	    ic_id->dirty &= ~ICA_FOCUSW;
	}
    }
    if (!(ic_id->dirty & ICA_CLIENTW)) {
	compile_sv_resources(ic_id, &pe_attr, &st_attr);
	if (pe_attr != (XVaNestedList) NULL) {
	    ic_attr[i++] = (XPointer) XNPreeditAttributes;
	    ic_attr[i++] = (XPointer) pe_attr;
	    ic_id->dirty &= ~ICA_PREEDIT;
	}
	if (st_attr != (XVaNestedList) NULL) {
	    ic_attr[i++] = (XPointer) XNStatusAttributes;
	    ic_attr[i++] = (XPointer) st_attr;
	    ic_id->dirty &= ~ICA_STATUS;
	}
    }
    ic_attr[i] = (XPointer) NULL;

#ifdef DEBUG
    if (i) {
	fprintf(stderr, "XSetICValues(\n");
	show_resources(ic_attr, i);
	fprintf(stderr, ");\n");
    }
#endif

    if (i)
	ret_val = XSetICValues(ic_id->xic,
		    ic_attr[0], ic_attr[1],
		    ic_attr[2], ic_attr[3],
		    ic_attr[4], ic_attr[5],
		    ic_attr[6], ic_attr[7],
		    ic_attr[8]);

    if (pe_attr != (XVaNestedList) NULL)
	XtFree((XtPointer) pe_attr);
    if (st_attr != (XVaNestedList) NULL)
	XtFree((XtPointer) st_attr);
    
    return(ret_val == (char *) NULL);
}

#define MAX_NUM_PE_RES 14
#define MAX_NUM_ST_RES 14

static void
compile_sv_resources(OlInputContextID ic_id,
		     XVaNestedList *  pe_attr,
		     XVaNestedList *  st_attr)
{
    int		  num_pe_res = XtNumber(pe_ic_resources);
    int		  num_st_res = XtNumber(st_ic_resources);
    register int  i;
    register int  j;

    /* 
     * We have to provide these two assertions as a safeguard for the 
     * following:
     * 
     * XVaCreateNestedList takes a variable number of arguments. We need 
     * to provide a *varying* number of such arguments while calling it. 
     * As there is no API to programmatically construct such a varying 
     * list of varargs, we are forced to make a kludge, viz. to
     * enumerate all the elements of a 
     * large enough array as the varargs list while taking care to 
     * insert a NULL after the last valid arg-value pair.
     * 
     * These two assertions ensure that we always enumerate enough 
     * elements and never enumerate a non-allocated one.
     *
     */
    assert(MAX_NUM_PE_RES >= num_pe_res && MAX_NUM_PE_RES >= 14);
    assert(MAX_NUM_ST_RES >= num_st_res && MAX_NUM_ST_RES >= 14);
    
    *pe_attr = (XVaNestedList) NULL;
    if (ic_id->dirty & ICA_PREEDIT) {
	XPointer * at =
	    (XPointer *) XtMalloc(sizeof(XPointer) * (MAX_NUM_PE_RES*2 + 1));

	for (i = j = 0; i < num_pe_res; i++)
	    if (ic_id->preedit_res[i].changed) {
		at[j++] = (XPointer) pe_ic_resources[i].string;
		at[j++] = (XPointer) ic_id->preedit_res[i].value;
		ic_id->preedit_res[i].changed = FALSE;
	    }
	at[j] = (XPointer) NULL;
	*pe_attr = XVaCreateNestedList(0,
		     at[0],  at[1],  at[2],  at[3],
		     at[4],  at[5],  at[6],  at[7],
		     at[8],  at[9],  at[10], at[11],
		     at[12], at[13], at[14], at[15],
		     at[16], at[17], at[18], at[19],
		     at[20], at[21], at[22], at[23],
		     at[24], at[25], at[26], at[27],
		     at[28]);
#ifdef DEBUG
	if (j) {
	    fprintf(stderr, "PE_ATTR =\n");
	    show_resources(at, j);
	    fprintf(stderr, "\n");
	}
#endif
		
	XtFree((XtPointer) at);
    }
    
    *st_attr = (XVaNestedList) NULL;
    if (ic_id->dirty & ICA_STATUS) {
	XPointer * at =
	    (XPointer *) XtMalloc(sizeof(XPointer) * (MAX_NUM_ST_RES*2 + 1));

	for (i = j = 0; i < num_st_res; i++)
	    if (ic_id->status_res[i].changed) {
		at[j++] = (XPointer) st_ic_resources[i].string;
		at[j++] = (XPointer) ic_id->status_res[i].value;
		ic_id->status_res[i].changed = FALSE;
	    }
	at[j] = (XPointer) NULL;
	*st_attr = XVaCreateNestedList(0,
		     at[0],  at[1],  at[2],  at[3],
		     at[4],  at[5],  at[6],  at[7],
		     at[8],  at[9],  at[10], at[11],
		     at[12], at[13], at[14], at[15],
		     at[16], at[17], at[18], at[19],
		     at[20], at[21], at[22], at[23],
		     at[24], at[25], at[26], at[27],
		     at[28]);
#ifdef DEBUG
	if (j) {
	    fprintf(stderr, "ST_ATTR =\n");
	    show_resources(at, j);
	    fprintf(stderr, "\n");
	}
#endif

	XtFree((XtPointer) at);
    }
}

static void
compile_gv_resources(Boolean		for_preedit,
		     ArgList		args,
		     Cardinal		num_args,
		     XVaNestedList *	attr)
{
    register int  i;
    register int  j;
    unsigned int  dum;

    assert(MAX_NUM_PE_RES >= 14 && MAX_NUM_ST_RES >= 14);

    if (num_args > 14)
	num_args = 14;
    
    *attr = (XVaNestedList) NULL;
    if (for_preedit) {
	XPointer * at =
	    (XPointer *) XtMalloc(sizeof(XPointer) * (MAX_NUM_PE_RES*2 + 1));

	for (i = j = 0; i < num_args; i++)
	    if (find_preedit_res_index(args[i].name, &dum) >= 0) {
		at[j++] = (XPointer) args[i].name;
		at[j++] = (XPointer) args[i].value;
	    }
	at[j] = (XPointer) NULL;
	if (j)
	    *attr = XVaCreateNestedList(0,
		      at[0],  at[1],  at[2],  at[3],
		      at[4],  at[5],  at[6],  at[7],
		      at[8],  at[9],  at[10], at[11],
		      at[12], at[13], at[14], at[15],
		      at[16], at[17], at[18], at[19],
		      at[20], at[21], at[22], at[23],
		      at[24], at[25], at[26], at[27],
		      at[28]);
	XtFree((XtPointer) at);
    } else {
	XPointer * at =
	    (XPointer *) XtMalloc(sizeof(XPointer) * (MAX_NUM_PE_RES*2 + 1));

	for (i = j = 0; i < num_args; i++)
	    if (find_status_res_index(args[i].name, &dum) >= 0) {
		at[j++] = (XPointer) args[i].name;
		at[j++] = (XPointer) args[i].value;
	    }
	at[j] = (XPointer) NULL;
	if (j)
	    *attr = XVaCreateNestedList(0,
		      at[0],  at[1],  at[2],  at[3],
		      at[4],  at[5],  at[6],  at[7],
		      at[8],  at[9],  at[10], at[11],
		      at[12], at[13], at[14], at[15],
		      at[16], at[17], at[18], at[19],
		      at[20], at[21], at[22], at[23],
		      at[24], at[25], at[26], at[27],
		      at[28]);
	XtFree((XtPointer) at);
    }
}
    

#undef MAX_NUM_PE_RES
#undef MAX_NUM_ST_RES

static void
set_status_resources(Widget		vw,
		     OlInputContextID	ic_id)
{
    Arg	   	  	args[7];
    int	   	  	i;
    int	   	  	j;
    Pixel  	  	im_fg;
    Pixel  	  	im_bg;
    Pixmap		im_bgpix;
    OlStrRep 	  	str_rep;
    XFontSet	  	im_fontset;
    XRectangle *  	im_rect;
    Boolean		setArea = FALSE;

    i = 0;
    XtSetArg(args[i], XtNforeground,    	&im_fg);      i++;
    XtSetArg(args[i], XtNbackground,    	&im_bg);      i++;
    XtSetArg(args[i], XtNbackgroundPixmap,    &im_bgpix);   i++;
    XtSetArg(args[i], XtNimRect,          	&im_rect);    i++;
    XtSetArg(args[i], XtNimFontSet,          	&im_fontset); i++;
    XtGetValues(vw, args, i);

    i = 0;
    XtSetArg(args[i], XNForeground,        im_fg);   i++;
    XtSetArg(args[i], XNBackground,        im_bg);   i++;
    XtSetArg(args[i], XNFontSet,  im_fontset);       i++;
    if (im_bgpix != XtUnspecifiedPixmap) {
	XtSetArg(args[i], XNBackgroundPixmap, im_bgpix);   i++;
    }
    if (im_rect != (XRectangle *) NULL) {
	XtSetArg(args[i], XNArea,              im_rect); i++;
	setArea = TRUE;
    }
    for (j = 0; j < i; j++)
	set_resource(ic_id, FALSE, args[j]);
    if (ic_id->xic != (XIC) NULL)
	set_resources_on_xic(ic_id);
    if (setArea && ic_id->im_id->last_ic_used != (OlInputContextID) NULL &&
	ic_id->im_id->last_ic_used->im_id->is_last_ic_focused)
	    XSetICFocus(ic_id->im_id->last_ic_used->xic);
}

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
OlInputContextID
OlCreateIC(OlInputMethodID im_id,
	   Widget          w,
	   OlImPreeditStyle preedit_type,
           ArgList         args,
           Cardinal        num_args)
{
    XIMStyle             style;
    InputContextRec *	 temp  = (OlInputContextID) NULL;
    ImVSInfo * 		 ivs   = _OlGetImVSInfo(w);
    
    /* check if im exists and pre-edit style is supported by im */
    if (im_id != (OlInputMethodID) NULL &&
	(style = xim_style_for_preedit_type(ivs, im_id, w, preedit_type))
	!= (XIMStyle) 0)
    {
	int        i;

	/* create and initialize an InputContextRec */
	temp = create_ic(ivs, im_id, w, style);
	
	/* Attach temp to list of ICs in DisplayShell */
	temp->next   = ivs->ic_list;
	ivs->ic_list = temp;
	ivs->num_ics++;

	/*
	 * Process ArgList of Preedit Resources.
	 * Status resources will be added just before XIC
	 * creation to reflect latest values in the Shell
	 */
	for (i = 0; i < num_args; i++)
	    set_resource(temp, TRUE, args[i]);

	/* Add callback to destroy ic when widget is destroyed */
	XtAddCallback(w, XtNdestroyCallback,
		      destroy_ic_cb, (XtPointer) temp);

	/* Tell the Shell to bring up its IM footer area the first time */
	if (ivs->num_ics == 1) {
	    Arg			args[1];
	    OlImStatusStyle	im_status_style;

	    XtSetArg(args[0], XtNimStatusStyle, &im_status_style);
	    XtGetValues(ivs->vw, args, 1);
	    if (im_status_style == OL_IM_DISPLAYS_IN_CLIENT) {
		Arg	args[1];

		XtSetArg(args[0], XtNimFooterPresent, TRUE);
		XtSetValues(ivs->vw, args, 1);
	    }
	    if (XtIsRealized(w) && !set_resources_on_xic(temp)) {
		OlDestroyIC(temp);
		OlWarning(dgettext(OlMsgsDomain,
		 "IM Management: Could not realize first ic."));
		temp = (OlInputContextID) NULL;
	    }
	}
    }
    return(temp);
}

/*---------------------------------------------------------------------------
 * OlDestroyIC frees storage for the OlInputContextID
 * and destroys its XIC if it had been created.
 */
void
OlDestroyIC(OlInputContextID ic_id)
{
    ImVSInfo *                 ivs  = ic_id->ivs;
    register InputContextRec * trav;
    register InputContextRec * prev = (InputContextRec *) NULL;

    if (ivs->num_ics <= 0) {
	OlError(dgettext(OlMsgsDomain,
	 "OlIM: Internal error: IC count is non-positive while destroying."));
	return;
    }
    
    for (trav = ivs->ic_list;
	 trav != ic_id && trav != (InputContextRec *) NULL;
	 prev = trav, trav = trav->next)
	;
    if (trav == (InputContextRec *) NULL)	/* should never happen */
	OlError(dgettext(OlMsgsDomain,
		"OlIM: Internal error: IC List is empty while destroying."));

    if (prev == (InputContextRec *) NULL) {
	/* At the head of the list */
	ivs->ic_list = trav->next;
    } else {
	/* Somewhere in the middle of the list */
	prev->next = trav->next;
    }

    /* Tell the Shell to bring down its IM footer area the last time */
    if (ivs->num_ics == 1) {
	Arg		args[1];
	OlImStatusStyle	im_status_style;

	XtSetArg(args[0], XtNimStatusStyle, &im_status_style);
	XtGetValues(ivs->vw, args, 1);
	if (im_status_style == OL_IM_DISPLAYS_IN_CLIENT) {
	    XtSetArg(args[0], XtNimFooterPresent, FALSE);
	    XtSetValues(ivs->vw, args, 1);
	}
    }
    
    destroy_ic(trav);
    ivs->num_ics--;
}

/*---------------------------------------------------------------------------
 * OlRealizeIC returns TRUE, for ic_id, if its XIC is initially NULL and
 * attempting to create it succeeds.
 * Otherwise, it returns FALSE.
 */
Boolean
OlRealizeIC(OlInputContextID ic_id)
{
    return (!OlIsICRealized(ic_id) && set_resources_on_xic(ic_id));
}

/*---------------------------------------------------------------------------
 * OlWidgetHasIC returns TRUE, for a widget w, if a call to OlCreateIC has
 * been made with w as its widget parameter and the IC has not yet been destroyed
 * either through the destroy callback or by calling OlDestroyIC.
 * Otherwise, it returns FALSE.
 */
Boolean
OlWidgetHasIC(Widget w)
{
    ImVSInfo *                 ivs  = _OlGetImVSInfo(w);
    register InputContextRec * trav;

    for (trav = ivs->ic_list;
	 trav != (InputContextRec *) NULL && trav->w != w;
	 trav = trav->next)
	;
    return(trav != (InputContextRec *) NULL);
}

/*---------------------------------------------------------------------------
 * OlIsICRealized returns TRUE, for ic_id, if the XIC in it is non-NULL.
 * Otherwise, it returns FALSE.
 */
Boolean
OlIsICRealized(OlInputContextID ic_id)
{
    return(ic_id->xic != (XIC) NULL);
}

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
Boolean
OlSetValuesIC(OlInputContextID ic_id,
	      Boolean	       for_preedit,
	      ArgList          args,
	      Cardinal         num_args)
{
    register int	i;

    for (i = 0; i < num_args; i++)
	set_resource(ic_id, for_preedit, args[i]);

  /*----------------------------------------
    if (ic_id == ic_id->im_id->last_ic_used)
   */

    return(set_resources_on_xic(ic_id));

  /*----------------------------------------
    else
	return(TRUE);
   */
}

/*---------------------------------------------------------------------------
 * OlVaSetValuesIC is a varargs version of OlSetValuesIC
 */
Boolean
OlVaSetValuesIC(OlInputContextID ic_id,
		Boolean	         for_preedit,
		...)
{
    ArgList	arg_list;
    va_list	var;
    Cardinal	count;
    Boolean     ret_val;

    va_start(var, for_preedit);
    count = count_va_arg_list(var);
    va_end(var);

    va_start(var, for_preedit);
    arg_list = va_create_arg_list(var, count);
    va_end(var);

    ret_val = OlSetValuesIC(ic_id, for_preedit, arg_list, count);
    XtFree((XtPointer) arg_list);

    return(ret_val);
}

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
Boolean
OlGetValuesIC(OlInputContextID ic_id,
	      Boolean	       for_preedit,
	      ArgList          args,
	      Cardinal         num_args)
{
    XVaNestedList   attr;
    char *	    ret_val = (char *) NULL;
    
    if (!set_resources_on_xic(ic_id))
	return(FALSE);
    
    compile_gv_resources(for_preedit, args, num_args, &attr);
    if (attr != (XVaNestedList) NULL) {
	if (for_preedit)
	    ret_val = XGetICValues(ic_id->xic,
				   XNPreeditAttributes, attr,
				   NULL);
	else
	    ret_val = XGetICValues(ic_id->xic,
				   XNStatusAttributes, attr,
				   NULL);
	XFree((XPointer) attr);
    }
    return(ret_val == (char *) NULL);
}

/*---------------------------------------------------------------------------
 * OlVaGetValuesIC is a varargs version of OlGetValuesIC.
 */
Boolean
OlVaGetValuesIC(OlInputContextID ic_id,
		Boolean	         for_preedit,
		...)
{
    ArgList	arg_list;
    va_list	var;
    Cardinal	count;
    Boolean     ret_val;

    va_start(var, for_preedit);
    count = count_va_arg_list(var);
    va_end(var);

    va_start(var, for_preedit);
    arg_list = va_create_arg_list(var, count);
    va_end(var);

    ret_val = OlGetValuesIC(ic_id, for_preedit, arg_list, count);
    XtFree((XtPointer) arg_list);

    return(ret_val);
}

/*---------------------------------------------------------------------------
 * OlGetInputStyleOfIC returns the value of the resource XNInputStyle
 * only if the IC is realized.
 * Otherwise it returns (XIMStyle) 0.
 */
XIMStyle
OlGetInputStyleOfIC(OlInputContextID ic_id)
{
    XIMStyle	ic_style;
    
    if (ic_id->xic != (XIC) NULL &&
	XGetICValues(ic_id->xic, XNInputStyle, &ic_style) != (char *) NULL)
	return(ic_style);
    else
	return((XIMStyle) 0);
}

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
Boolean
OlSetFocusIC(OlInputContextID ic_id)
{
    if (ic_id != (OlInputContextID) NULL) {
	OlInputContextID  last_ic;

	if (!set_resources_on_xic(ic_id))
	    return(FALSE);

	last_ic = ic_id->im_id->last_ic_used;
	if (last_ic != (OlInputContextID) NULL &&
	    ic_id->im_id->is_last_ic_focused)
	    OlUnsetFocusIC(last_ic);

	XSetICFocus(ic_id->xic);
	ic_id->im_id->last_ic_used       = ic_id;
	ic_id->im_id->is_last_ic_focused = TRUE;
	return(TRUE);
    }
}

/*---------------------------------------------------------------------------
 * OlUnsetFocusIC does XUnsetICFocus on the XIC of ic_id.
 * If !OlIsRealized(ic_id), then this is a no-op.
 */
void
OlUnsetFocusIC(OlInputContextID ic_id)
{
    if (ic_id->xic != (XIC) NULL &&
	ic_id->im_id->last_ic_used == ic_id &&
	ic_id->im_id->is_last_ic_focused)
    {
	XUnsetICFocus(ic_id->xic);
	ic_id->im_id->is_last_ic_focused = FALSE;
    }
}

/*---------------------------------------------------------------------------
 * OlXICOfIC returns the XIC of ic_id.
 * If !OlIsRealized(ic_id), it returns NULL.
 */
XIC
OlXICOfIC(OlInputContextID ic_id)
{
    return(ic_id->xic);
}

/************************************************************* 
 * 
 * Private API for shell's IC management
 * 
 *************************************************************/

/**************************************************************************
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
 ****************************procedure*header*****************************/

void
OlRealizeFirstIC(Widget vw)
{
    if (XtIsVendorShell(vw)) {
	ImVSInfo *			ivs = _OlGetImVSInfo(vw);
	register OlInputContextID	trav;
	register OlInputContextID	prev = (OlInputContextID) NULL;

	for (trav = ivs->ic_list;
	     trav != (OlInputContextID) NULL;
	     prev = trav, trav = trav->next)
	{
	    if (OlIsICRealized(trav)) {
		prev = NULL;
		break;
	    }
	}
	if (prev != (OlInputContextID) NULL) {
	    (void) set_resources_on_xic(prev);
	}
    }
}

/**************************************************************************
 *
 *	OlSetIMStatusOnAllICs
 *
 *             passes the status attributes from the vendor shell to
 *             the ICs in the ImVSInfo record in the DisplayShell
 *
 ****************************procedure*header*****************************/

void
OlSetIMStatusOnAllICs(Widget	vw,
		      ArgList	args,
		      Cardinal	num_args)
{
    if (XtIsVendorShell(vw)) {
	ImVSInfo *		   ivs  = _OlGetImVSInfo(vw);
	register InputContextRec * trav;
	
	for (trav = ivs->ic_list; trav != (InputContextRec *) NULL;
	     trav = trav->next)
	{
	    OlSetValuesIC(trav, FALSE, args, num_args);
	}
    }
}

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
 * Returns NULL if it fails to open an IM connection.
 */
OlInputMethodID
OlCreateIM(Display * 	display,
	   String    	im_modifier,
	   XrmDatabase	rdb)
{
    InputMethodRec *	temp;
    XIM                 xim;
    XIMStyles      *    supp_styles;
    String		res_name;
    String		res_class;
    
    /* Set the im modifier if specified  */
    if (im_modifier && *im_modifier) {
	char * modif_str;
	char * prefix_str = "@im=";

	modif_str = XtMalloc(strlen(prefix_str) + strlen(im_modifier) + 1);
	strcpy(modif_str, prefix_str);
	strcat(modif_str, im_modifier);

	if (XSetLocaleModifiers(modif_str) == (char *) NULL) {
	    XtFree(modif_str);
	    return((OlInputMethodID) NULL);
	}
	XtFree(modif_str);
    }

    XtGetApplicationNameAndClass(display, &res_name, &res_class);
    /* Open the IM connection */
    if ((xim = XOpenIM(display, rdb, res_name, res_class)) == (XIM) NULL) {
	return((OlInputMethodID) NULL);
    }

    /* Create and initialize an InputMethodRec */
    temp = (InputMethodRec *) XtMalloc(sizeof(InputMethodRec));
    temp->xim                = xim;
    temp->num_xics           = (Cardinal) 0;
    temp->is_mask_augmented  = FALSE;
    temp->augment_mask       = (EventMask) 0;
    temp->last_ic_used       = (OlInputContextID) NULL;
    temp->is_last_ic_focused = FALSE;

    /* Query the IM for list of supported styles */
    if (XGetIMValues(temp->xim,
		     XNQueryInputStyle, &supp_styles,
		     NULL)
	!= (char *) NULL)
    {
	/* Query failed */
	XCloseIM(temp->xim);
	XtFree((XtPointer) temp);
	temp = (OlInputMethodID) NULL;
    } else {
	int count = supp_styles->count_styles;
	if (count > 0) {
	    int i;
	
	    temp->supported_styles =
		(XIMStyles *) XtMalloc(sizeof(XIMStyles));
	    temp->supported_styles->supported_styles = (XIMStyle *)
		XtMalloc(sizeof(XIMStyle) * count);
	    for (i = 0; i < count; i++)
		temp->supported_styles->supported_styles[i] =
		    supp_styles->supported_styles[i];
	    temp->supported_styles->count_styles = i;
	} else {
	    XCloseIM(temp->xim);
	    XtFree((XtPointer) temp);
	    temp = (OlInputMethodID) NULL;
	}
	XFree(supp_styles);
    }
    return(temp);
}

/*---------------------------------------------------------------------------
 * OlDestroyIM closes the IM connection corresponding to im_id and frees
 * storage for it.
 */
void
OlDestroyIM(OlInputMethodID im_id)
{
    XtFree((XtPointer) (im_id->supported_styles->supported_styles));
    XtFree((XtPointer) (im_id->supported_styles));
    XCloseIM(im_id->xim);
    XtFree((XtPointer) im_id);
}

/*---------------------------------------------------------------------------
 * OlXIMOfIM returns the XIM corresponding to im_id.
 */
XIM
OlXIMOfIM(OlInputMethodID im_id)
{
    return(im_id->xim);
}

/*---------------------------------------------------------------------------
 * OlDefaultIMOfWidget returns the default OlInputMethodID for the
 * display to which widget w belongs. The first call to this function
 * calls OlCreateIM.
 */
OlInputMethodID
OlDefaultIMOfWidget(Widget w)
{
    ImVSInfo *	ivs  = _OlGetImVSInfo(w);
    XrmDatabase	rdb  = (XrmDatabase) NULL;

    /* If default_im_id is null, and open not attempted earlier
     * do an OlCreateIM and assign its return to default_im_id
     */
    if (ivs->default_im_id == (OlInputMethodID) NULL &&
	!ivs->default_im_open_attempted)
    {
	String default_im_name;
	Arg    args[1];

	XtSetArg(args[0], XtNdefaultImName, &default_im_name);
	XtGetValues(ivs->vw, args, 1);
	rdb = XtScreenDatabase(XtScreen(w));
	ivs->default_im_id = OlCreateIM(XtDisplay(w), default_im_name,
					rdb);
	ivs->default_im_open_attempted = TRUE;
    }
    return(ivs->default_im_id);
}
