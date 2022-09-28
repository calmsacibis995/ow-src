#pragma ident   "@(#)NumericField.c	1.9    97/03/26 lib/libXol SMI"     /* OLIT */

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code
 *          product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include	<stdlib.h>
#include	<ctype.h>	/* isdigit() */
#include	<wctype.h>	/* iswdigit() */
#include	<widec.h>
#include	<libintl.h>
#include	<locale.h>

#include	<X11/IntrinsicP.h>
#include	<X11/StringDefs.h>
#include	<Xol/OpenLookP.h>
#include	<Xol/OlI18nP.h>
#include	<Xol/OlStrMthdsI.h>

#include	<Xol/NumericFiP.h>

	/*** Function Type Identifiers - to be moved to OpenLookP.h ***/

#define ClassMethod     static
#define Private         static
#define WidgetInternal
#define PublicInterface

	/*** Convenient interface to str_methods members ***/
#define _OLStrCmp(format, s1, s2)       str_methods[format].StrCmp(s1, s2)
#define _OLStrEmptyString(format)       str_methods[format].StrEmptyString()
#define _OLStrNumBytes(format, str)     str_methods[format].StrNumBytes(str)
#define _OLStrCpy(format, s1, s2)       str_methods[format].StrCpy(s1, s2)
#define _OLStrPrintf(format, s1, s2, d)	str_methods[format].StrPrintf(s1, s2, d)
#define _OLStrAtoi(format, s)		str_methods[format].StrAtoi(s)
#define _OLStrAtof(format, s)		str_methods[format].StrAtof(s)



#define	TAG_VALUE		100	/* 
					 * Default value for XtNsizeOf - to tag
					 * it as "not-set" thru Resource-files
					 * or thru Initialize 
					 */

/* Delta Directions */
#define INCREMENT		1
#define DECREMENT		2

#define VALUE_CHANGED		(1L)
#define DELTA_CHANGED		(1L << 1)

#define DeltaWidth(nfp, tlp)	(nfp->delta_state == OL_ABSENT ? 0 :	\
			   	 NumScrollButton_Width(tlp->pAttrs->ginfo) + tlp->gap)

#define GetDrawInfo(nfp)						\
			(nfp->delta_state == OL_INCR_INACTIVE ?		\
		 		OLGX_SCROLL_NO_BACKWARD:		\
			 	(nfp->delta_state == OL_DECR_INACTIVE ?	\
			  		OLGX_SCROLL_NO_FORWARD:		\
			   		(nfp->delta_state == OL_ACTIVE ? 0 : 0)))

#define _OLNFWarn(mssg)		OlWarning(dgettext(OlMsgsDomain, mssg))



/****************************************************************
		Private Function Declarations ....
****************************************************************/

Private void 	GetDefaultConvertProc(Widget w, int offset, XrmValue *value);

Private Boolean CvtStringToOlNFData(Display *dpy, 
				    XrmValuePtr args, Cardinal *num_args,
				    XrmValuePtr from, XrmValuePtr to, 
				    XtPointer *data
		       		   );

Private Boolean CallConverter(Widget w, OlStrRep format, 
			      XrmQuark from_type, XrmQuark to_type, 
			      XtPointer *value, 
			      XtPointer *string, Cardinal *string_length
	            	     );

Private Boolean DefaultConverter(Widget w, OlStrRep format, 
				 XrmQuark from_type, XrmQuark to_type, 
				 XtPointer *value, 
				 XtPointer *string, Cardinal *string_length
		       		);

Private void 	ValidateResources(NumericFieldPart *nfp, NumericFieldPart *old_nfp);
Private Boolean ValidateValues(NumericFieldPart *nfp, Boolean update);

Private void 	_OlNFDrawWidget(NumericFieldWidget w, unsigned long draw_info);

Private void 	Key(Widget w, OlVirtualEvent ve);
Private void 	ButtonDown(Widget w, OlVirtualEvent ve);
Private void 	ButtonUp(Widget w, OlVirtualEvent ve);

Private void 	DeltaHandler(NumericFieldWidget w, int direction, unsigned long delay);
Private int 	InvokeDeltaCallbacks(NumericFieldWidget w, int direction);
Private void 	DeltaTimer(XtPointer client_data, XtIntervalId *id);

Private void 	ValidateKey(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	ValidateField(Widget w, XtPointer client_data, XtPointer call_data);

Private Boolean IsInteger(OlStr buff, int pos, OlStrRep format);
Private Boolean IsFloat(OlStr buff, int pos, OlStrRep format);

/*******************************************************************
			Class Methods ...
*******************************************************************/

ClassMethod void 	ClassInitialize(void);
ClassMethod void 	Initialize(Widget req, 
				   Widget new, 
				   ArgList args, 
				   Cardinal *num_args
				  );

ClassMethod Boolean 	SetValues(Widget current, 
			      	  Widget request, 
			      	  Widget new,
			      	  ArgList args, 
			      	  Cardinal * num_args
			     	 );
ClassMethod void 	SetValuesAlmost(Widget old, 
					Widget new, 
				 	XtWidgetGeometry *req, 
				 	XtWidgetGeometry *reply
				       );
ClassMethod void 	Redisplay(Widget w, XEvent *event, Region region);
ClassMethod void 	Resize(Widget w);
ClassMethod void 	Destroy(Widget w);
ClassMethod void 	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);

ClassMethod Boolean 	SetString(TextLineWidget w, 
				  OlStr string, 
				  OlTLSetStringHints hints,
				  Boolean cursor
				 );


/**********************************************************************
	Static declarations & Event Handler declarations 
 **********************************************************************/
Private XrmQuark stringQ = NULLQUARK;
Private XrmQuark intQ = NULLQUARK;
Private XrmQuark floatQ = NULLQUARK;

Private int defIntDelta = 1;
Private float defFloatDelta = .1;

Private OlEventHandlerRec event_procs[] = {
	{ KeyPress,             Key     },
	{ ButtonRelease,        ButtonUp },
	{ ButtonPress,          ButtonDown  }
};


/***********************************************************************
			Resource Declarations
***********************************************************************/

Private XtResource resources[] = {
#define OFFSET(field) XtOffsetOf(NumericFieldRec, numericField.field)

 {
	XtNtype, XtCType, XtRString, sizeof(String),
	OFFSET(type), XtRImmediate, XtRInt
 },{
	XtNconvertProc, XtCConvertProc, XtROlNFConvertProc, sizeof(OlNFConvertProc), 
	OFFSET(convert_proc), XtRCallProc, (XtPointer) GetDefaultConvertProc
 },{
	XtNsizeOf, XtCSizeOf, XtRCardinal, sizeof(Cardinal),
	OFFSET(size_of), XtRImmediate, (XtPointer)TAG_VALUE
 },{
	XtNvalue, XtCValue, XtROlNFData, sizeof(OlNFData),
	OFFSET(value), XtRImmediate, (XtPointer)NULL
 },{
	XtNdelta, XtCDelta, XtROlNFData, sizeof(OlNFData),
	OFFSET(delta), XtRImmediate, (XtPointer)NULL
 },{
	XtNdeltaState, XtCDeltaState, XtROlDefine, sizeof(OlDefine),
	OFFSET(delta_state), XtRImmediate, (XtPointer)OL_ACTIVE
 },{
	XtNmaxValue, XtCMaxValue, XtROlNFData, sizeof(OlNFData),
	OFFSET(max_value), XtRImmediate, (XtPointer)NULL
 },{
	XtNminValue, XtCMinValue, XtROlNFData, sizeof(OlNFData),
	OFFSET(min_value), XtRImmediate, (XtPointer)NULL
 },{
	XtNdeltaCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	OFFSET(delta_callback), XtRCallback, (XtPointer)NULL
 },{
	XtNvalidateCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	OFFSET(validate_callback), XtRCallback, (XtPointer)NULL
 },
#undef OFFSET
};

/******************************************************************
       Class Record Structure ...
******************************************************************/

NumericFieldClassRec numericFieldClassRec = {
 {		/* Core Class */
	
	/* superclass		*/ 	(WidgetClass)	&textLineClassRec,
	/* class_name 		*/ 			"NumericField",
	/* widget_size 		*/			sizeof(NumericFieldRec),
	/* class_init		*/			ClassInitialize,
	/* class_part_init 	*/			NULL,
	/* class_inited 	*/			False,
	/* initialize		*/			Initialize,
	/* initialize_hook	*/			NULL,
	/* realize		*/			XtInheritRealize,
	/* actions		*/			NULL,
	/* num_actions		*/			0,
	/* resources		*/			resources,
	/* num_resources      	*/    			XtNumber(resources),
	/* xrm_class		*/			NULLQUARK,
	/* compress_motion	*/			True,
	/* compress_exposure	*/			True,
	/* compress_enterleave	*/			True,
	/* visible_interest	*/			False,
	/* destroy		*/			Destroy,
	/* resize		*/			Resize,
	/* expose		*/			Redisplay,
	/* set_values 		*/			SetValues,
	/* set_values_hook 	*/			NULL,
	/* set_values_almost	*/			SetValuesAlmost,
	/* get_values_hook	*/			GetValuesHook,
	/* accept_focus		*/			XtInheritAcceptFocus,
	/* version		*/			XtVersion,
	/* callback_private	*/	(XtPointer)	0,
	/* tm_table		*/			XtInheritTranslations,
	/* query_geometry	*/			XtInheritQueryGeometry,
	/* display_accelerator	*/	(XtStringProc)	_XtInherit,
	/* extension		*/	(XtPointer)	0
 },
 {		/* Primitive Class */
	/* reserved             */ 	(XtPointer)	NULL,
	/* highlight_handler	*/			XtInheritHighlightHandler,
	/* traversal_handler	*/			NULL,
	/* register_focus	*/			NULL,
	/* activate		*/			NULL,
	/* event_procs		*/			event_procs,
	/* num_event_procs	*/			XtNumber(event_procs),
	/* version		*/			OlVersion,
	/* extension		*/			NULL,
	/* dyn_data		*/			{0, 0},
	/* transparent_proc	*/			NULL,
	/* query_sc_locn_proc   */			NULL,
 },
 {		/* TextLine Class */
	/* set_string		*/			SetString,
	/* get_string		*/			XtInheritOlTLGetStringProc,
	/* extension		*/	(XtPointer)	NULL,
 },
 {		/* NumericField Class */
	/* extension		*/	(XtPointer)	NULL,
 }
};

PublicInterface WidgetClass 
numericFieldWidgetClass = (WidgetClass) & numericFieldClassRec;


/****************************************************************************
	GetDefaultConvertProc 

 	We are forced to a default_proc here, since the widget's
  default-resource converter would be needed while loading the resource-database
  and hence we can't wait till Initialize to fill this field ...

 ****************************************************************************/
Private void
GetDefaultConvertProc(Widget w, int offset, XrmValue *value)
{
	static XtPointer p = (XtPointer)DefaultConverter;
	if ((strcmp(((NumericFieldWidget)w)->numericField.type, XtRInt) == 0) ||
	    (strcmp(((NumericFieldWidget)w)->numericField.type, XtRFloat) == 0))
	    	value->addr = (caddr_t)&p;
	else
		value->addr = (caddr_t)NULL;
}


/******************************************************
	CvtStringToOlNFData 

 ******************************************************/
Private Boolean
CvtStringToOlNFData(Display *dpy, XrmValuePtr args, Cardinal *num_args,
			XrmValuePtr from, XrmValuePtr to, XtPointer *data)
{
	NumericFieldWidget w = *(NumericFieldWidget *)args[0].addr;
	NumericFieldPart *nfp = &w->numericField;
	Boolean status = True;
	static XtPointer value;
	
	value = (XtPointer)NULL;
	status = CallConverter((Widget)w, w->primitive.text_format,
				  XrmPermStringToQuark(XtRString), 
				  XrmPermStringToQuark(nfp->type),
				  &value, (XtPointer *)&from->addr, 
				  &from->size);
	
	if (status) {
		if (to->addr == NULL)
			to->addr = (caddr_t)&value;
		else
		if (to->size < sizeof(XtPointer))
			status = False;
		else
			*(XtPointer *)to->addr = value;
		to->size = sizeof(XtPointer);
	}

	return status;
}

/******************************************************
	StringToOlNFDestructor 

 ******************************************************/
Private void 
StringToOlNFDestructor(XtAppContext app, XrmValuePtr to, XtPointer conv_data,
			XrmValuePtr args, Cardinal *num_args)
{
	if (*(XtPointer *)(to->addr) != NULL)
		XtFree(*(XtPointer *)(to->addr));
}

/******************************************************
	CallConverter 

 ******************************************************/
Private Boolean
CallConverter(Widget w, OlStrRep format, XrmQuark from_type, 
		XrmQuark to_type, XtPointer *value, XtPointer *string, 
		Cardinal *string_length)
{
	NumericFieldWidget nw = (NumericFieldWidget)w;

	if (nw->numericField.convert_proc != NULL)
		return ((*nw->numericField.convert_proc)(w, 
				nw->primitive.text_format, from_type, to_type,
				value, string, string_length));
	else {
		_OLNFWarn("NumericFieldWidget: Converter not installed");
		return False;
	}
}

/******************************************************
	SetString - set_string class-method of TextLine

 ******************************************************/
ClassMethod Boolean
SetString(TextLineWidget w, OlStr string, OlTLSetStringHints hints, Boolean cursor)
{
	if (hints != TLSetVal || hints != TLInit)
		return ((*textLineClassRec.textLine_class.set_string)
				(w, string, hints, cursor));
	else
		return False;
}


/******************************************************
	DefaultConverter 

 ******************************************************/
Private Boolean
DefaultConverter(Widget w, OlStrRep format, XrmQuark from_type, XrmQuark to_type,
		 XtPointer *value, XtPointer *string, Cardinal *string_length)
{

#define CheckStringSize(size)						\
	if (_OLStrCmp(format, *string, _OLStrEmptyString(format)) == 0)	\
		*string = XtMalloc(size);				\
	else if (*string_length < size) 				\
		*string = XtRealloc((char *)*string, size);		\
	*string_length = size;						\

#define	NUM_SIZE 50

	static char buff[NUM_SIZE];

	if (to_type == stringQ) {	/* from_type is Int/Float */
		OlStr bufp = (OlStr)buff;
		if (from_type == intQ) {
			int i = *(int *)(*value);

			_OLStrPrintf(format, bufp, "%d", i);
			CheckStringSize(_OLStrNumBytes(format, bufp));
			_OLStrCpy(format, *string, bufp);
		} else if (from_type == floatQ) {
			double d = *(float *)(*value);

			_OLStrPrintf(format, bufp, "%f", d);
			CheckStringSize(_OLStrNumBytes(format, bufp));
			_OLStrCpy(format, *string, bufp);
		}
		else 
			_OLNFWarn("NumericFieldWidget: Unknown type in converter");
	}
	else if (to_type == intQ) {
		if (*value == (XtPointer)NULL)
			*value = XtMalloc(sizeof(int));
		*(int *)(*value) = _OLStrAtoi(format, *string);
	}
	else if (to_type == floatQ) {
		if (*value == (XtPointer)NULL)
			*value = XtMalloc(sizeof(float));
		*(float *)(*value) = _OLStrAtof(format, *string);
	}
	else 
		_OLNFWarn("NumericFieldWidget: Unknown type in converter");
	return True;
}
		
/******************************************************
	ClassInitialize - class_initialize class-method

 ******************************************************/
ClassMethod void
ClassInitialize(void)
{
	static XtConvertArgRec converterArgs[] = {
		{ XtBaseOffset, 0,	/* We want the whole widget ! */
		  sizeof(NumericFieldWidget)
		},
	};

	if (stringQ == NULLQUARK)
		stringQ = XrmPermStringToQuark(XtRString);
	if (intQ == NULLQUARK)
		intQ = XrmPermStringToQuark(XtRInt);
	if (floatQ == NULLQUARK)
		floatQ = XrmPermStringToQuark(XtRFloat);

	XtSetTypeConverter(XtRString, XtROlNFData, CvtStringToOlNFData,
		converterArgs, XtNumber(converterArgs), XtCacheNone,
		StringToOlNFDestructor);
	_OlAddOlDefineType( "active", OL_ACTIVE);
	_OlAddOlDefineType( "absent", OL_ABSENT);
	_OlAddOlDefineType( "incr_inactive", OL_INCR_INACTIVE);
	_OlAddOlDefineType( "decr_inactive", OL_DECR_INACTIVE);
}

/******************************************************
	ValidateResources 

 ******************************************************/
Private void
ValidateResources(NumericFieldPart *nfp, NumericFieldPart *old_nfp)
{
	if (nfp->delta_state != OL_ACTIVE &&
	    nfp->delta_state != OL_INCR_INACTIVE &&
	    nfp->delta_state != OL_DECR_INACTIVE &&
	    nfp->delta_state != OL_ABSENT) {
		_OLNFWarn("NumericField : Invalid value for XtNdeltaState");
		nfp->delta_state = OL_ACTIVE;
	}

	
	if (old_nfp) {	/* Non - settable resources */
		if (nfp->type != old_nfp->type) {
			_OLNFWarn("NumericFieldWidget: XtNtype is not settable");
			nfp->type = old_nfp->type;
		}
		if (nfp->size_of != old_nfp->size_of) {
			_OLNFWarn("NumericFieldWidget: XtNsizeOf is not settable");
			nfp->size_of = old_nfp->size_of;
		}
	}
}

/*************************************************************************
	ValidateValues 

 	Validates that XtNvalue is <= XtNmaxValue & >= XtNminValue. Corrects 
  	XtNvalue if "update" is True. Updates XtNdeltaState if reqd.
 
 *************************************************************************/
Private Boolean
ValidateValues(NumericFieldPart *nfp, Boolean update)
{
	Boolean is_valid = True;

	if (nfp->value == (XtPointer)NULL)
		return is_valid;

	if (nfp->type_quark == intQ) {
		int max_value, min_value;
		int value = *(int *)(nfp->value);

		if (nfp->max_value) {	/* validate only if max_value exits */
			max_value = *(int *)nfp->max_value;
			if (value > max_value) {
				is_valid = False;
				if (update)
					*(int *)(nfp->value) = max_value;
			}
		}
		if (nfp->min_value) {	/* validate only if min_value exits */
			min_value = *(int *)nfp->min_value;
			if (value < min_value) {
				is_valid = False;
				if (update)
					*(int *)(nfp->value) = min_value;
			}
		}

		if (nfp->delta_state != OL_ABSENT) {  /* Only if buttons are present */
			if (nfp->max_value && *(int *)(nfp->value) >=  max_value)
				nfp->delta_state = OL_INCR_INACTIVE;
			else if (nfp->min_value && *(int *)(nfp->value) <=  min_value)
				nfp->delta_state = OL_DECR_INACTIVE;
			else 
				nfp->delta_state = OL_ACTIVE;
		}
	}
	else if (nfp->type_quark == floatQ) {
		double max_value, min_value;
		double value = *(float *)(nfp->value);

		if (nfp->max_value) {	/* validate only if max_value exits */
			max_value = *(float *)(nfp->max_value);
			if (value > max_value) {
				is_valid = False;
				if (update)
					*(float *)(nfp->value) = max_value;
			}
		}
		if (nfp->min_value) {	/* validate only if min_value exits */
			min_value = *(float *)(nfp->min_value);
			if (value < min_value) {
				is_valid = False;
				if (update)
					*(float *)(nfp->value) = min_value;
			}
		}

		if (nfp->delta_state != OL_ABSENT) {  /* Only if buttons are present */
			if (nfp->max_value && *(float *)nfp->value >= max_value) 
				nfp->delta_state = OL_INCR_INACTIVE;
			if (nfp->min_value && *(float *)nfp->value <=  min_value)
				nfp->delta_state = OL_DECR_INACTIVE;
			else 
				nfp->delta_state = OL_ACTIVE;
		}
	}
	return is_valid;
}

/******************************************************
	Initialize - Initialize class-method

 ******************************************************/
ClassMethod void
Initialize(Widget req, Widget new, ArgList args, Cardinal *num_args)
{
	NumericFieldPart *nfp 	= &((NumericFieldWidget)new)->numericField;
	TextLinePart *tlp 	= &((TextLineWidget)new)->textLine;
	OlStrRep format 	= ((NumericFieldWidget)new)->primitive.text_format;

	ValidateResources(nfp, (NumericFieldPart *)NULL);

	/* Set private data */
	nfp->delta_timer 	= NULL;
	nfp->delta_direction 	= 0;
	nfp->delta_width 	= DeltaWidth(nfp, tlp);
	nfp->type_quark 	= XrmPermStringToQuark(nfp->type);
	
	if (nfp->size_of == TAG_VALUE) { 	/* Has'nt been set .. yet .. */
		if (nfp->type_quark == intQ)
			nfp->size_of = sizeof(int);
		else if (nfp->type_quark == floatQ)
			nfp->size_of = sizeof(float);
	}

	/* "store" is a piece of scratch memory allocated now - which can
	 * be used to hold XtNvalue for "misc" purposes ... Currently , we
	 * use this in the deltaCallback handler. 
	 */
	if (nfp->size_of)
		nfp->store = XtMalloc(nfp->size_of);
	else
		nfp->store = (XtPointer)NULL;

	/* Make copies - if we are told so .. */
#define COPY(value, size)					\
	if (value && size) {					\
		char *p = XtMalloc(size);			\
		memmove((void *)p, (void *)value, size);	\
		value = p;					\
	}

	COPY(nfp->value, nfp->size_of)
	COPY(nfp->delta, nfp->size_of)
	COPY(nfp->max_value, nfp->size_of)
	COPY(nfp->min_value, nfp->size_of)

#undef COPY

	/* Install default deltaValues & modifyCallbacks if type == INT/FLOAT */
	if (nfp->type_quark == intQ) {
		if (nfp->delta == (XtPointer)NULL)
			nfp->delta = (XtPointer)&defIntDelta;
		XtAddCallback(new, XtNpreModifyCallback, ValidateKey, (XtPointer)intQ);
	} 
	else if (nfp->type_quark == floatQ) {
		if (nfp->delta == (XtPointer)NULL)
			nfp->delta = (XtPointer)&defFloatDelta;
		XtAddCallback(new, XtNpreModifyCallback, ValidateKey, (XtPointer)floatQ);
	}
	
	XtAddCallback(new, XtNcommitCallback, ValidateField, NULL);

	(void)ValidateValues(nfp, True);

	/* Convert "value" into "string" , insert into textLine's buffer */
	if (nfp->value != (XtPointer)NULL) {
		Cardinal str_len = _OLStrNumBytes(format, tlp->string);
		if (CallConverter(new, format, nfp->type_quark, stringQ, 
				  &(nfp->value), &(tlp->string), &str_len))
			SetString((TextLineWidget)new, tlp->string, NFInit, True);
		else
			_OLNFWarn("NumericFieldWidget: Conversion failed");
	}

	new->core.width = ((TextLineWidget)new)->textLine.real_width + nfp->delta_width;
	new->core.height = ((TextLineWidget)new)->textLine.real_height;
}

/******************************************************
	SetValues - set_values class-method

 ******************************************************/
ClassMethod Boolean
SetValues(Widget current,Widget request, Widget new,ArgList args,Cardinal * num_args)
{
	NumericFieldPart *newnfp = &((NumericFieldWidget)new)->numericField;
	NumericFieldPart *oldnfp = &((NumericFieldWidget)current)->numericField;
	TextLinePart *newtlp	 = &((TextLineWidget)new)->textLine;
	TextLinePart *oldtlp	 = &((TextLineWidget)current)->textLine;
	OlStrRep format		 = ((NumericFieldWidget)new)->primitive.text_format;
	Boolean value_changed 	 = False;
	Boolean resize 		 = False;
	Boolean realized 	 = XtIsRealized(new);
	unsigned long changes	 = 0;
	int i;

#define CHANGED_PRIMITIVE(field) (((PrimitiveWidget)new)->primitive.field != \
				  ((PrimitiveWidget)current)->primitive.field)
#define CHANGED_TEXT(field) 	 (newtlp->field != oldtlp->field)
#define CHANGED_NUM(field) 	 (newnfp->field != oldnfp->field)

#define	MIN_MAX		(1L)
#define	VALUE		(1L << 1)

#define COPY(oldvalue, newvalue, size)				\
	if (size) {						\
		char *p;					\
		if (!(oldvalue))				\
			p = XtMalloc(size);			\
		else						\
			p = oldvalue;				\
		memmove((void *)p, (void *)newvalue, size);	\
		newvalue = p;					\
	}

	ValidateResources(newnfp, oldnfp);

	if (CHANGED_PRIMITIVE(scale)) {
		/* Dimensions of DeltaButtons would change .. So resize */
		newnfp->delta_width = DeltaWidth(newnfp, newtlp);
		resize = True;
	}
					
	for (i=0; i < *num_args; i++) {
		if (strcmp(args[i].name, XtNmaxValue) == 0) {
			COPY(oldnfp->max_value, newnfp->max_value, newnfp->size_of);
			changes |= MIN_MAX;
		}
		if (strcmp(args[i].name, XtNminValue) == 0) {
			COPY(oldnfp->min_value, newnfp->min_value, newnfp->size_of);
			changes |= MIN_MAX;
		}
		if (strcmp(args[i].name, XtNdelta) == 0) {
			COPY(oldnfp->delta, newnfp->delta, newnfp->size_of);
		}
		if (strcmp(args[i].name, XtNvalue) == 0) {
			COPY(oldnfp->value, newnfp->value, newnfp->size_of);
			changes |= VALUE;
		}
	}
	if (changes & VALUE) {
		value_changed = True;
		(void)ValidateValues(newnfp, True);
	}
	else if (changes & MIN_MAX)
		value_changed = !ValidateValues(newnfp, True); 

#undef MIN_MAX
#undef VALUE
#undef COPY

	if (CHANGED_TEXT(real_width) || CHANGED_TEXT(real_height)) 
		resize = True;

	if (CHANGED_NUM(delta_state)) { 
	/* delta_state could be changed thru XtSetValues by the programmer OR thru 
	 * ValidateValues() above ... Note that ValidateValues takes precedence
	 */
		if (newnfp->delta_state ==OL_ABSENT || oldnfp->delta_state ==OL_ABSENT) {
			/* change between delta_exist & delta_no_exist .. So Resize */
			resize = True;
			newnfp->delta_width = DeltaWidth(newnfp, newtlp);
		}

		/* Redraw deltaButtons ... However, if we are going to be resized,
		 * don't bother redrawing since we'll get an Xpose event
		 */
		if (!resize && realized) 
			_OlNFDrawWidget((NumericFieldWidget)new, GetDrawInfo(newnfp));
	}

	if (value_changed) {
		Cardinal str_len = _OLStrNumBytes(format, newtlp->string);
		if (CallConverter(new, format, newnfp->type_quark, stringQ, 
				  &(newnfp->value), &(newtlp->string), &str_len))
			/* Setup string & redraw it. But,if we are going to be resized,
			 * don't bother redrawing since we'll get an Xpose event.
			 */
			SetString((TextLineWidget)new, 
				  newtlp->string, 
				  (resize || !realized) ? NFSetVal : NFOther, 
				  False
				 );
		else
			_OLNFWarn("NumericFieldWidget: Conversion failed");
	}

	if (resize) {
		new->core.width = newtlp->real_width + newnfp->delta_width;
		new->core.height = newtlp->real_height;
	}
	
	return False;

#undef CHANGED_TEXT
#undef CHANGED_NUM
}

/***********************************************************
	SetValuesAlmost - set_values_almost class-method

 **********************************************************/
ClassMethod void
SetValuesAlmost(Widget old, Widget new, XtWidgetGeometry *req, XtWidgetGeometry *reply)
{
	Dimension width = ((NumericFieldWidget)new)->numericField.delta_width;

        /* Fake new geometry for super-class */
        new->core.width -= width;
        old->core.width -= width;
        req->width      -= width;
        reply->width    -= width;

        (*textLineWidgetClass->core_class.set_values_almost) (old, new, req, reply);

        /* Undo above modfications */
        new->core.width += width;
        old->core.width += width;
        req->width      += width;
        reply->width    += width;
}

/******************************************************
	Redisplay - redisplay class-method

 ******************************************************/
ClassMethod void 
Redisplay(Widget w, XEvent *event, Region region)
{
	NumericFieldPart *nfp = &((NumericFieldWidget)w)->numericField;

	if (event->type == Expose || event->type == GraphicsExpose) {
		if (nfp->delta_state != OL_ABSENT)  /* DeltaButton present */
			_OlNFDrawWidget((NumericFieldWidget)w, GetDrawInfo(nfp));
		(*textLineWidgetClass->core_class.expose)(w, event, region);
	}
}

/******************************************************
	_OlNFDrawWidget 

 ******************************************************/
Private void
_OlNFDrawWidget(NumericFieldWidget w, unsigned long draw_info)
{
	NumericFieldPart *nfp = &w->numericField;
	Graphics_info *ginfo = w->textLine.pAttrs->ginfo;
	int x = w->core.width - (nfp->delta_width - w->textLine.gap);
	int y = w->core.height - NumScrollButton_Height(ginfo);

	if (!XtIsSensitive((Widget)w))
		draw_info = OLGX_INACTIVE;	/* override draw_info */
	
	olgx_draw_numscroll_button(ginfo, XtWindow(w), x, y, draw_info);
}

/******************************************************
	Resize - resize class-method

 ******************************************************/
ClassMethod void 
Resize(Widget w)
{
	Dimension width = w->core.width;
	Widget shell = _OlGetShellOfWidget(w);

	if (shell != (Widget)NULL)
		XtVaSetValues(shell, XtNdoDSDMFetchesAsync, (Boolean)True, NULL);

	/* Fake new geometry for super-class (TextLine ) */
	w->core.width -= ((NumericFieldWidget)w)->numericField.delta_width;

	(*textLineWidgetClass->core_class.resize)(w);

	w->core.width = width;

	if (shell != (Widget)NULL)
		XtVaSetValues(shell, XtNdoDSDMFetchesAsync, (Boolean)False, NULL);
}

/******************************************************
	GetValuesHook - get_values_hook class-method

 ******************************************************/
ClassMethod void 
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	NumericFieldPart *nfp = &((NumericFieldWidget)w)->numericField;
	OlStrRep format = ((PrimitiveWidget)w)->primitive.text_format;
	OlStr string;
	Cardinal str_len;
	int i;

	for (i = 0; i < *num_args; i++)
		if (strcmp(args[i].name, XtNvalue) == 0) {
			string = (*((TextLineWidgetClass)XtClass(w))->textLine_class.get_string)
			((TextLineWidget)w);
			str_len = _OLStrNumBytes(format, string);
			if (CallConverter(w, format, stringQ, nfp->type_quark,
					  &nfp->value, &string, &str_len))
				*((XtPointer *)(args[i].value)) = nfp->value;
			else
				*((XtPointer *)(args[i].value)) = (XtPointer)NULL;
			return;
		}
}

/******************************************************
	Destroy - destroy class-method

 ******************************************************/
ClassMethod void
Destroy(Widget w)
{
	NumericFieldPart *nfp = &((NumericFieldWidget)w)->numericField;

	if (nfp->store)
		XtFree(nfp->store);
	if (nfp->delta_timer)
		XtRemoveTimeOut(nfp->delta_timer);
}

/******************************************************
	Key 

 ******************************************************/
Private void
Key(Widget w, OlVirtualEvent ve)
{
	/* Implement mouseless behaviour */
}

/******************************************************
	ButtonDown 

 ******************************************************/
Private void
ButtonDown(Widget w, OlVirtualEvent ve)
{
	NumericFieldWidget nw 	= (NumericFieldWidget)w;
	NumericFieldPart *nfp 	= &nw->numericField;
	XEvent *ev 		= ve->xevent;
	int x 			= ev->xbutton.x;
	int y			= ev->xbutton.y;
	Dimension dwidth 	= nfp->delta_width - nw->textLine.gap;

	if (nfp->delta_state == OL_ABSENT || (x < (int)(w->core.width - dwidth)) ||
	    (y < (int)(w->core.height - NumScrollButton_Height(nw->textLine.pAttrs->ginfo))))
		/* Event belongs to SuperClass - don't consume ... */
		return;
	
	ve->consumed = True;
	if (x < (int)(w->core.width - dwidth/2) && (nfp->delta_state != OL_INCR_INACTIVE)) {
		/* ButtonPress on the INCR button */
		nfp->delta_direction = INCREMENT;
		_OlNFDrawWidget(nw, OLGX_SCROLL_BACKWARD | 
				(nfp->delta_state == OL_DECR_INACTIVE ?
			 	 	OLGX_SCROLL_NO_FORWARD : 0));
		DeltaHandler(nw, INCREMENT, nw->textLine.initial_delay);
	}
	else if (x > (int)(w->core.width - dwidth/2) && nfp->delta_state != OL_DECR_INACTIVE) {
		nfp->delta_direction = DECREMENT;
		_OlNFDrawWidget(nw, OLGX_SCROLL_FORWARD | 
				(nfp->delta_state == OL_INCR_INACTIVE ?
			 	 	OLGX_SCROLL_NO_BACKWARD : 0));
		DeltaHandler(nw, DECREMENT, nw->textLine.initial_delay);
	}
}

/******************************************************
	DeltaHandler 

 ******************************************************/
Private void
DeltaHandler(NumericFieldWidget w, int direction, unsigned long delay)
{
	XtAppContext ac 	= XtWidgetToApplicationContext((Widget)w);
	NumericFieldPart *nfp 	= &w->numericField;
	TextLinePart *tlp 	= &w->textLine;
	OlStrRep format		= w->primitive.text_format;
	unsigned long draw_info	= 0;
	int state;

	state = InvokeDeltaCallbacks(w, direction);

	if (state & VALUE_CHANGED) {
		Cardinal str_len = _OLStrNumBytes(format, tlp->string);
		if (CallConverter((Widget)w, format, nfp->type_quark, stringQ, 
				  &(nfp->value), &(tlp->string), &str_len))
			SetString((TextLineWidget)w, tlp->string, NFOther, False);
		else
			_OLNFWarn("NumericFieldWidget: Conversion failed");
	}

	if (state & DELTA_CHANGED) {
		if ((direction == INCREMENT && nfp->delta_state == OL_INCR_INACTIVE) ||
		    (direction == DECREMENT && nfp->delta_state == OL_DECR_INACTIVE))
		    /* We have reached our limits - Cease Delta ... */
		    	nfp->delta_direction = NULL;
		else 	
			nfp->delta_timer = XtAppAddTimeOut(ac, delay, DeltaTimer, w);

		draw_info = GetDrawInfo(nfp) | (nfp->delta_direction == INCREMENT ?
						OLGX_SCROLL_BACKWARD :
					       (nfp->delta_direction == DECREMENT ?
						OLGX_SCROLL_FORWARD : 0));
		_OlNFDrawWidget(w, draw_info);
	}
	else	/* No delta_change - continue delta-ing */
		nfp->delta_timer = XtAppAddTimeOut(ac, delay, DeltaTimer, w);
}

/*************************************************************************
	InvokeDeltaCallbacks 

	For default types, do the default delta processing. 
	Invoke any XtNdeltaCallbacks. 
	Returns VALUE_CHANGED 	if XtNvalue has been changed
		DELTA_CHANGED 	if XtNdeltaState has changed
		0	      	if failure ...

 *************************************************************************/
Private int 
InvokeDeltaCallbacks(NumericFieldWidget w, int direction)
{
	NumericFieldPart *nfp = &w->numericField;
	OlNFDeltaCallbackStruct cb;
	OlStrRep format = w->primitive.text_format;
	OlStr string; 
	Cardinal str_len; 
	int return_state = 0;

	string = (*((TextLineWidgetClass)XtClass(w))->textLine_class.get_string)
					((TextLineWidget)w);
	str_len = _OLStrNumBytes(format, string);
	if (CallConverter((Widget)w, format, stringQ, nfp->type_quark, &(nfp->value),
			  &string, &str_len) == False)
		return 0;
	
	cb.reason = (direction == INCREMENT ? OL_REASON_INCREMENT: OL_REASON_DECREMENT);
	cb.event = (XEvent *)NULL;
	cb.delta = nfp->delta;

	/* Make a copy of the current nfp->value ... */
	if (nfp->store) {
		memmove((void *)(nfp->store), (void *)(nfp->value), nfp->size_of);
		cb.current_value = nfp->store;
	} else
		cb.current_value = nfp->value;

	cb.current_delta_state = nfp->delta_state;
	cb.update = False;

	/* Compute the default new_values for the default datatypes */
	if (nfp->type_quark == intQ) {
		if (direction == INCREMENT)
			*(int *)(nfp->value) += *(int *)nfp->delta;
		else
			*(int *)(nfp->value) -= *(int *)nfp->delta;
		(void) ValidateValues(nfp, True);
		cb.update = True;	/* Indicating that nfp->value has changed */

	} else if (nfp->type_quark == floatQ) {
		if (direction == INCREMENT)
			*(float *)(nfp->value) += *(float *)nfp->delta;
		else
			*(float *)(nfp->value) -= *(float *)nfp->delta;
		(void) ValidateValues(nfp, True);
		cb.update = True;       /* Indicating that nfp->value has changed */
	}
	cb.new_value = nfp->value;
	cb.new_delta_state = nfp->delta_state;

	XtCallCallbackList((Widget)w, nfp->delta_callback, &cb);
	nfp->delta_state = cb.new_delta_state;

	if (cb.update)
		return_state |= VALUE_CHANGED;
	if (cb.current_delta_state != cb.new_delta_state)
		return_state |= DELTA_CHANGED;

	return return_state;
}

/******************************************************
	DeltaTimer

 ******************************************************/
Private void
DeltaTimer(XtPointer client_data, XtIntervalId *id)
{	
	NumericFieldWidget w = (NumericFieldWidget)client_data;
	NumericFieldPart *nfp = &w->numericField;

	nfp->delta_timer  =  NULL;
	if (nfp->delta_direction)
		DeltaHandler(w, nfp->delta_direction, w->textLine.repeat_rate);
}

/******************************************************
	ButtonUp

 ******************************************************/
Private void
ButtonUp(Widget w, OlVirtualEvent ve)
{
	NumericFieldPart *nfp 	= &((NumericFieldWidget)w)->numericField;
	
	if (nfp->delta_timer) {
		XtRemoveTimeOut(nfp->delta_timer);
		nfp->delta_direction = NULL;
		nfp->delta_timer = NULL;
		_OlNFDrawWidget((NumericFieldWidget)w, GetDrawInfo(nfp));
		ve->consumed = True;
	}
}

/******************************************************
	ValidateKey

 ******************************************************/
Private void
ValidateKey(Widget w, XtPointer client_data, XtPointer call_data)
{
	NumericFieldWidget nfw = (NumericFieldWidget)w;
	NumericFieldPart *nfp = &nfw->numericField;
	OlTLPreModifyCallbackStruct *cb = (OlTLPreModifyCallbackStruct *)call_data;
	OlStrRep format = nfw->primitive.text_format;
	XrmQuark type = (XrmQuark)client_data;

	if (type == intQ && cb->insert_length) {
		if (IsInteger(cb->insert_buffer, cb->start, format))
			;
		else {
			cb->valid = False;
			_OlBeepDisplay(w, 1);
		}
	}
	else if (type == floatQ && cb->insert_length) {
		if (IsFloat(cb->insert_buffer, cb->start, format))
			;
		else {
			cb->valid = False;
			_OlBeepDisplay(w, 1);
		}
	}
}

/******************************************************
	IsInteger

 ******************************************************/
Private Boolean
IsInteger(OlStr buff, int pos, OlStrRep format)
{
	if (format == OL_SB_STR_REP) {
		char *cp = (char *)buff;

        /* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
                if (*cp == '-' && pos == 0)
                        cp++;

		while (*cp) {
			if (!isdigit((int)(*cp)))
				return False;
			cp++;
		}
		return True;
	}
	else if (format == OL_MB_STR_REP) {
		char *cp = (char *)buff;
 		int len;
		wchar_t wc;

	/* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
		if ((len = mbtowc(&wc, cp, MB_CUR_MAX)) > 0 &&
                        (wc == L'-') && (pos == 0))
                    cp += len;

		for (; (len = mbtowc(&wc, cp, MB_CUR_MAX)) > 0; cp += len) {
			if (!iswdigit((wint_t)(wc)))
				return False;
		}
		return True;
	}
	else if (format == OL_WC_STR_REP) {
		wchar_t *wcp = (wchar_t *)buff;

	/* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
		if (*wcp == L'-' && pos == 0)
                        wcp++;

		while (*wcp) {
			if (!iswdigit((wint_t)(*wcp)))
				return False;
			wcp++;
		}
		return True;
	}
}

/******************************************************
	IsFloat

 ******************************************************/
Private Boolean
IsFloat(OlStr buff, int pos, OlStrRep format)
{
	struct lconv *lconv = localeconv();
	char *dp = lconv->decimal_point;

	if (format == OL_SB_STR_REP) {
		char *cp = (char *)buff;

	/* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
                if (*cp == '-' && pos == 0)
                        cp++;

		while (*cp) {
			if (!isdigit((int)(*cp)) && *cp != *(char *)dp)
				return False;
			cp++;
		}
		return True;
	}
	else if (format == OL_MB_STR_REP) {
		char *cp = (char *)buff; 
		int len;
		wchar_t wc;

	/* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
		if ((len = mbtowc(&wc, cp, MB_CUR_MAX)) > 0 &&
                        (wc == L'-') && (pos == 0))
                    cp += len;

		for (; (len = mbtowc(&wc, cp, MB_CUR_MAX)) > 0; cp += len) {
			if (!iswdigit((wint_t)(wc)) && strncmp(cp, dp, len))
				return False;
		}
		return True;
	}
	else if (format == OL_WC_STR_REP) {
		wchar_t wdp;
		wchar_t *wcp = (wchar_t *)buff;

	/* Convert the decimal-seperator (is always in multi_byte) to wide_char */
		if (mbtowc(&wdp, (char *)dp, MB_CUR_MAX) == -1)
			return False;

	/* If first character is '-' && insert-pos == 0, its
         * "probably" a valid negative sign ...
         */
		if (*wcp == L'-' && pos == 0)
			wcp++;

		while (*wcp) {
			if (!iswdigit((wint_t)(*wcp)) && *wcp != wdp)
				return False;
			wcp++;
		}
		return True;
	}
}
	
/******************************************************
	ValidateField

 ******************************************************/
Private void
ValidateField(Widget w, XtPointer client_data, XtPointer call_data)
{
	NumericFieldWidget nfw 		= (NumericFieldWidget)w;
	NumericFieldPart *nfp  		= &nfw->numericField;
	OlStrRep format 		= nfw->primitive.text_format;
	OlTLCommitCallbackStruct *ccb 	= (OlTLCommitCallbackStruct *)call_data;
	OlStr string 			= ccb->buffer;
	Cardinal str_len 		= _OLStrNumBytes(format, string);
	OlDefine old_delta_state	= nfp->delta_state;
	OlNFValidateCallbackStruct vcb;


	if (CallConverter(w, format, stringQ, nfp->type_quark, &(nfp->value),
			 	&string, &str_len) == False)
		return;
	
	vcb.reason 	= OL_REASON_VALIDATE;
	vcb.event  	= ccb->event;
	vcb.value  	= nfp->value;
	vcb.delta_state = nfp->delta_state;
	vcb.valid  	= ValidateValues(nfp, False);
	vcb.update	= False;
	XtCallCallbackList(w, nfp->validate_callback, &vcb);

	if (vcb.update) {
		TextLinePart *tlp = &nfw->textLine;
		str_len = _OLStrNumBytes(format, tlp->string);
		if (CallConverter(w, format, nfp->type_quark, stringQ,
				  &(nfp->value), &(tlp->string), &str_len))
			SetString((TextLineWidget)w, tlp->string, NFOther, False);
		else
			_OLNFWarn("NumericFieldWidget: Conversion failed");
		nfp->delta_state = vcb.delta_state;
	}

	/* nfp->delta_state might have changed either thru the callback or thru
	 * ValidateValues ... In either case, redraw the buttons ..
	 */
	if (old_delta_state != nfp->delta_state)
		_OlNFDrawWidget(nfw, GetDrawInfo(nfp));

	ccb->valid = vcb.valid;
}
