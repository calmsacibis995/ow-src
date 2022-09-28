#pragma ident	"@(#)RootShell.c	302.43	97/03/26 lib/libXol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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


#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicI.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>

#include <Xol/Converters.h>
#include <Xol/DynamicP.h>
#include <Xol/OlgxP.h>
#include <Xol/OlCursorsP.h>
#include <Xol/OpenLook.h>
#include <Xol/OpenLookI.h>
#include <Xol/RootShellP.h>
#include <Xol/OlImP.h>
#include <Xol/Util.h>

static void RootClassInitialize(void);

static void RootClassPartInitialize ( WidgetClass );

static void RootInitialize   ( Widget, Widget, ArgList, Cardinal* );

static Boolean RootSetValues ( Widget, Widget, Widget, ArgList, 
				       Cardinal* );

static void RootGetValuesHook ( Widget, ArgList, Cardinal * );

static XtGeometryResult GeometryManager     (Widget, XtWidgetGeometry *,
					     XtWidgetGeometry *),
			RootGeometryManager (Widget, XtWidgetGeometry *,
					     XtWidgetGeometry *);
static void RootDestroy    ( Widget );

static void AddShell    ( RootShellWidget, Widget ),
	    DeleteShell ( RootShellWidget, Widget );

static void AddRoot    ( RootShellWidget, RootShellWidgetClass ),
	    DeleteRoot ( RootShellWidget, RootShellWidgetClass );

static Widget   RootShellOfScreen (WidgetClass, Screen *);

static void _rootPropChanged ( Widget, XtPointer, XEvent *, 
				       Boolean * );

static void _Depth    ( Widget, int, XrmValue * ),
	    _Colormap ( Widget, int, XrmValue * ),
	    _Visual   ( Widget, int, XrmValue * ),
	    _Width    ( Widget, int, XrmValue * ),
	    _Height   ( Widget, int, XrmValue * );

static void _LoadAppAttributes ( _OlAppAttributes *,
					 DisplayShellWidget,
					 ScreenShellWidget );

static void _OlGetMetaKey(Widget, int , XrmValue *);

static ShellClassExtensionRec rootShellClassExtRec = {
    NULL,
    NULLQUARK,
    XtShellExtensionVersion,
    sizeof(ShellClassExtensionRec),
    RootGeometryManager
};


/***************************************************************************
 *
 * RootShell class record
 *
 ***************************************************************************/

#define Offset(x) (XtOffsetOf(RootShellRec, x))

static XtResource rootShellResources[]= {
        { XtNx, XtCPosition, XtRPosition, sizeof(Position),
            Offset(core.x), XtRImmediate, (XtPointer)0 },
            
        { XtNy, XtCPosition, XtRPosition, sizeof(Position),
            Offset(core.y), XtRImmediate, (XtPointer)0 },
            
        { XtNwidth, XtCWidth, XtRDimension, sizeof(Dimension),
            Offset(core.width), XtRCallProc, (XtPointer)_Width },
            
        { XtNheight, XtCHeight, XtRDimension, sizeof(Dimension),
            Offset(core.height), XtRCallProc, (XtPointer)_Height },
            
        { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
            Offset(core.border_width), XtRImmediate, (XtPointer)NULL },
            
        { XtNdepth, XtCDepth, XtRInt, sizeof(int),
            Offset(core.depth), XtRCallProc, (XtPointer)_Depth },

        { XtNcolormap, XtCColormap, XtRColormap, sizeof(Colormap),
            Offset(core.colormap), XtRCallProc, (XtPointer)_Colormap },

        { XtNallowShellResize, XtCAllowShellResize, XtRBoolean,
            sizeof(Boolean), Offset(shell.allow_shell_resize),
            XtRImmediate, (XtPointer)False },
            
        { XtNsaveUnder, XtCSaveUnder, XtRBoolean, sizeof(Boolean),
            Offset(shell.save_under), XtRImmediate, (XtPointer)False },
            
        { XtNoverrideRedirect, XtCOverrideRedirect,
            XtRBoolean, sizeof(Boolean), Offset(shell.override_redirect),
            XtRImmediate, (XtPointer)True },
            
        { XtNvisual, XtCVisual, XtRVisual, sizeof(Visual*),
            Offset(shell.visual), XtRCallProc, (XtPointer)_Visual},

        { XtNshells, XtCReadOnly, XtRInt, sizeof(Widget*),
            Offset(root.shells), XtRImmediate, (XtPointer)NULL },
            
        { XtNnumShells, XtCReadOnly, XtRInt, sizeof(Cardinal),
            Offset(root.num_shells), XtRImmediate, (XtPointer)NULL },

	{ XtNrootProperty, XtCReadOnly, XtRAtom, sizeof(Atom),
	    Offset(root.root_property), XtRString, (XtPointer)NULL },
	    
	{ XtNpropertyString, XtCReadOnly, XtRString, sizeof(String),
	    Offset(root.property_string), XtRString, (XtPointer)NULL },
	    
	{ XtNlastUpdate, XtCReadOnly, XtRInt, sizeof(int),
	    Offset(root.last_update), XtRImmediate, (XtPointer)NULL },

	{ XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	    Offset(root.user_data), XtRPointer, (XtPointer)NULL },

	{ XtNpropNotifyCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	  Offset(root.prop_notify_callback), XtRCallback, (XtPointer)NULL },

	{ XtNapplShell, XtCApplShell, XtRPointer, sizeof(Widget),
	  Offset(root.appl_shell), XtRImmediate, (XtPointer)NULL },

	{XtNvendorInstance, XtCVendorInstance, XtRPointer, sizeof(Widget),
	  Offset(root.vendor_instance), XtRImmediate, (XtPointer)NULL },
};

externaldef(rootshellclassrec) RootShellClassRec rootShellClassRec = {
  {
    /* superclass           */  (WidgetClass) &overrideShellClassRec,
    /* class_name           */  "RootShell",
    /* size                 */  sizeof(RootShellRec),
    /* Class Initialize     */	RootClassInitialize,
    /* class_part_initialize*/	RootClassPartInitialize,
    /* Class init'ed ?      */	FALSE,
    /* initialize           */  RootInitialize,
    /* initialize_notify    */	NULL,		
    /* realize              */  NULL,
    /* actions              */  NULL,
    /* num_actions          */  0,
    /* resources            */  rootShellResources,
    /* resource_count       */	XtNumber(rootShellResources),
    /* xrm_class            */  NULLQUARK,
    /* compress_motion      */  FALSE,
    /* compress_exposure    */  FALSE,
    /* compress_enterleave  */ 	FALSE,
    /* visible_interest     */  FALSE,
    /* destroy              */  RootDestroy,
    /* resize               */  NULL,
    /* expose               */  NULL,
    /* set_values           */  RootSetValues,
    /* set_values_hook      */	NULL,			
    /* set_values_almost    */	NULL,  
    /* get_values_hook      */	RootGetValuesHook,			
    /* accept_focus         */  NULL,
    /* intrinsics version   */	XtVersion,
    /* callback offsets     */  NULL,
    /* tm_table		    */  NULL,
    /* query_geometry	    */  NULL,
    /* display_accelerator  */  NULL,
    /* extension	    */  NULL
  },
  { /* composite            */
    /* geometry_manager     */  GeometryManager,
    /* change_managed       */  XtInheritChangeManaged,
    /* insert_child	    */	XtInheritInsertChild,
    /* delete_child	    */	XtInheritDeleteChild,
    /* extension	    */  NULL
  },
  { /* shell                */
    /* extension	    */  (XtPointer)&rootShellClassExtRec,
  },
  { /* override shell       */
    /* extension	    */  NULL
  },
  { /* root shell           */
    /* appl_res		   */  NULL,
    /* num_appl_res	   */  0,
    /* add_shell           */  AddShell,
    /* delete_shell        */  DeleteShell,
    /* shell_registered    */  (RSShellRegisteredProc)NULL,

    /* add_root             */  AddRoot,
    /* delete_root          */  DeleteRoot,
    /* root_registered      */  RootShellOfScreen,

    /* prop_notify	    */  _rootPropChanged,
    /* prop_changed	    */  (RSPropChangedProc)NULL,

    /* root_shells          */  (Widget *)NULL,
    /* num_roots            */  (Cardinal)NULL,

    /* extension            */  (XtPointer)NULL
  }
};

externaldef(rootshellwidgetclass) WidgetClass rootShellWidgetClass = 
	                                (WidgetClass) (&rootShellClassRec);

/***************************************************************************
 *
 * DisplayShell class record
 *
 ***************************************************************************/

static void DisplayShellClassInitialize (void);

static void DisplayShellDestroy    ( Widget );

static Widget   DisplayShellOfScreen (WidgetClass, Screen *);

static void
DisplayShellInitialize   ( Widget, Widget, ArgList, Cardinal* );

static void
ResourceManagerPropUpdated ( Widget );

#define Offset(x) (XtOffsetOf(RootShellRec, x))

static XtResource displayShellResources[]=
{
	{ XtNrootProperty, XtCReadOnly, XtRAtom, sizeof(Atom),
	    Offset(root.root_property), XtRString,
	    (XtPointer)"RESOURCE_MANAGER" },

#undef  Offset
#define Offset(x) (XtOffsetOf(DisplayShellRec, x))
 
        { XtNshapeExtensionPresent, XtCReadOnly, XtRBoolean, sizeof(Boolean),
            Offset(display.shape_extension_present), XtRImmediate,
            (XtPointer)False },
};

/* 
 * WARNING THESE RESOURCES ARE NOT INHERITED IN THE SAME FASHION AS
 * *NORMAL* WIDGET INSTANCE RESOURCES.
 */

static XtResource displayShellAppResources[] = {
#undef	Offset
#define Offset(x) (XtOffsetOf(DisplayShellRec, x))

	{ XtNbeep, XtCBeep,
	  XtROlDefine, sizeof(OlDefine), Offset(display.beep),
	  XtRImmediate, (XtPointer)OL_BEEP_ALWAYS },

	{ XtNbeepVolume, XtCBeepVolume,
	  XtRInt, sizeof(int), Offset(display.beep_volume),
	  XtRImmediate, (XtPointer)0 },

	{ XtNbeepDuration, XtCBeepDuration,
	  XtRInt, sizeof(int), Offset(display.beep_duration),
	  XtRImmediate, (XtPointer)-1},

	{ XtNmultiClickTimeout, XtCMultiClickTimeout,
	  XtRCardinal, sizeof(Cardinal),
	  Offset(display.multi_click_timeout), XtRImmediate,
	  (XtPointer)OL_MULTI_CLICK_TIMEOUT },

	{ XtNmultiObjectCount, XtCMultiObjectCount,
	  XtRCardinal, sizeof(Cardinal),
	  Offset(display.multi_object_count), XtRImmediate,
	  (XtPointer)3 },

	{ XtNselectDoesPreview, XtCSelectDoesPreview,
	  XtRBoolean, sizeof(Boolean),
	  Offset(display.select_does_preview), XtRImmediate,
	  (XtPointer)True },

	{ XtNgrabPointer, XtCGrabPointer,
	  XtRBoolean, sizeof(Boolean), Offset(display.grab_pointer),
	  XtRImmediate, (XtPointer)True },

	{ XtNgrabServer, XtCGrabServer,
	  XtRBoolean, sizeof(Boolean), Offset(display.grab_server),
	  XtRImmediate, (XtPointer)False },

	{ XtNmenuAccelerators, XtCMenuAccelerators,
	  XtRBoolean, sizeof(Boolean), Offset(display.menu_accelerators),
	  XtRImmediate, (XtPointer)True },

	{ XtNinputFocusFeedback, XtCInputFocusFeedback,
          XtROlDefine, sizeof(OlDefine), Offset(display.input_focus_feedback),
          XtRImmediate, (XtPointer)OL_SUPERCARET },

	{ XtNmouseless, XtCMouseless,
	  XtRBoolean, sizeof(Boolean), Offset(display.mouseless),
	  XtRImmediate, (XtPointer)False },

	{ XtNmnemonicPrefix, XtCMnemonicPrefix,
	  XtRModifiers, sizeof(Modifiers),
	  Offset(display.mnemonic_modifiers), XtRImmediate,
	  (XtPointer)None },

	{ XtNshowMnemonics, XtCShowAccelerators,
	  XtROlDefine, sizeof(OlDefine),
	  Offset(display.show_mnemonics), XtRString,
	  (XtPointer)"underline" },

	{ XtNshowAccelerators, XtCShowAccelerators,
	  XtROlDefine, sizeof(OlDefine),
	  Offset(display.show_accelerators), XtRString,
	  (XtPointer)"display" },

	{ XtNshiftName, XtCShiftName,
	  XtRString, sizeof(String), Offset(display.shift_name),
	  XtRString, (XtPointer)"Shift" },

	{ XtNlockName, XtCLockName,
	  XtRString, sizeof(String), Offset(display.lock_name),
	  XtRString, (XtPointer)"Lock" },

	{ XtNcontrolName, XtCControlName,
	  XtRString, sizeof(String), Offset(display.control_name),
	  XtRString, (XtPointer)"Ctrl" },

	{ XtNmod1Name, XtCMod1Name,
	  XtRString, sizeof(String), Offset(display.mod1_name),
	  XtRImmediate, (XtPointer) NULL },

	{ XtNmod2Name, XtCMod2Name,
	  XtRString, sizeof(String), Offset(display.mod2_name),
	  XtRImmediate, (XtPointer) NULL },

	{ XtNmod3Name, XtCMod3Name,
	  XtRString, sizeof(String), Offset(display.mod3_name),
	  XtRImmediate, (XtPointer) NULL },

	{ XtNmod4Name, XtCMod4Name,
	  XtRString, sizeof(String), Offset(display.mod4_name),
	  XtRImmediate, (XtPointer) NULL },

	{ XtNmod5Name, XtCMod5Name,
	  XtRString, sizeof(String), Offset(display.mod5_name),
	  XtRImmediate, (XtPointer) NULL },

	{ XtNhelpModel, XtCHelpModel,
	  XtROlDefine, sizeof(OlDefine), Offset(display.help_model),
	  XtRImmediate, (XtPointer) OL_POINTER },

	{ XtNmouseStatus, XtCMouseStatus,
	  XtRBoolean, sizeof(Boolean), Offset(display.mouse_status),
	  XtRImmediate, (XtPointer) True },

	{ XtNolDefaultFont, XtCOlDefaultFont,
	  XtRString, sizeof(String), Offset(display.ol_default_font),
	  XtRImmediate, (XtPointer) OL_DEFAULT_FONT_NAME },

	{ XtNctrlAltMetaKey, XtCCtrlAltMetaKey, XtRBoolean,
	  sizeof(Boolean), Offset(display.ctrl_alt_meta_key),
	  XtRCallProc, (XtPointer) _OlGetMetaKey },

	{ XtNuseShortOLWinAttr, XtCUseShortOLWinAttr, XtRBoolean,
	  sizeof(Boolean), Offset(display.use_short_OlWinAttr),
	  XtRImmediate, (XtPointer) False },
};

externaldef(displayshellclassrec) DisplayShellClassRec displayShellClassRec = {
  {
    /* superclass           */  (WidgetClass) &rootShellClassRec,
    /* class_name           */  "DisplayShell",
    /* size                 */  sizeof(DisplayShellRec),
    /* Class Initialize     */	DisplayShellClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?      */	FALSE,
    /* initialize           */  DisplayShellInitialize,
    /* initialize_notify    */	NULL,		
    /* realize              */  NULL,
    /* actions              */  NULL,
    /* num_actions          */  0,
    /* resources            */  displayShellResources,
    /* resource_count       */	XtNumber(displayShellResources),
    /* xrm_class            */  NULLQUARK,
    /* compress_motion      */  FALSE,
    /* compress_exposure    */  FALSE,
    /* compress_enterleave  */ 	FALSE,
    /* visible_interest     */  FALSE,
    /* destroy              */  DisplayShellDestroy,
    /* resize               */  NULL,
    /* expose               */  NULL,
    /* set_values           */  NULL,
    /* set_values_hook      */	NULL,			
    /* set_values_almost    */	NULL,  
    /* get_values_hook      */	NULL,			
    /* accept_focus         */  NULL,
    /* intrinsics version   */	XtVersion,
    /* callback offsets     */  NULL,
    /* tm_table		    */  NULL,
    /* query_geometry	    */  NULL,
    /* display_accelerator  */  NULL,
    /* extension	    */  NULL
  },
  { /* composite            */
    /* geometry_manager     */  XtInheritGeometryManager,
    /* change_managed       */  XtInheritChangeManaged,
    /* insert_child	    */	XtInheritInsertChild,
    /* delete_child	    */	XtInheritDeleteChild,
    /* extension	    */  NULL
  },
  { /* shell                */
    /* extension	    */  (XtPointer)&rootShellClassExtRec,
  },
  { /* override shell       */
    /* extension	    */  NULL
  },
  { /* root shell          */
    /* appl res		   */  displayShellAppResources,
    /* num appl res	   */  XtNumber(displayShellAppResources),
    /* add_shell           */  XtInheritRSAddShell,
    /* delete_shell        */  XtInheritRSDeleteShell,
    /* shell_registered    */  (RSShellRegisteredProc)NULL,

    /* add_root            */  XtInheritRSAddRoot,
    /* delete_root         */  XtInheritRSDeleteRoot,
    /* root_registered     */  DisplayShellOfScreen,

    /* prop_notify	   */  XtInheritRSPropertyNotify,
    /* prop_changed        */  (RSPropChangedProc)ResourceManagerPropUpdated,

    /* root_shells         */  (Widget *)NULL,
    /* num_roots           */  (Cardinal)NULL,

    /* extension           */  (XtPointer)NULL
  },
  { /* display shell	   */
    /* extension	   */  (XtPointer)NULL
  }
};

externaldef(displayshellwidgetclass) WidgetClass displayShellWidgetClass = 
	                                (WidgetClass) (&displayShellClassRec);

/***************************************************************************
 *
 * ScreenShell class record
 *
 ***************************************************************************/

static void
ScreenShellClassPartInitialize ( WidgetClass );

static void
ScreenResourcesPropUpdated ( Widget );

static void
NotifyScreenOfRMUpdate ( Widget );

static void ScreenShellInitialize   ( Widget, Widget,
					       ArgList, Cardinal* );

static Boolean ScreenShellSetValues ( Widget, Widget, Widget, ArgList, 
				       	      Cardinal* );

static void
ScreenShellDestroy ( Widget );

#undef	Offset
#define Offset(x) (XtOffsetOf(RootShellRec, x))

static XtResource screenShellResources[]=
{
	{ XtNrootProperty, XtCReadOnly, XtRAtom, sizeof(Atom),
	    Offset(root.root_property), XtRString, (XtPointer) 
	    "SCREEN_RESOURCES" },

#undef	Offset
#define Offset(x) (XtOffsetOf(ScreenShellRec, x))

	{ XtNolCursorFontName, XtCOlCursorFontName,
	  XtRString, sizeof(String), Offset(screen.ol_cursor_font_name),
	  XtRImmediate,
	  (XtPointer) "*-%-*-sunolcursor-1" },

	/* in the initialize we substitute the % for the current scale */

	{ XtNolCursorFontID, XtCReadOnly,
	  XtRFont, sizeof(Font), Offset(screen.ol_cursor_font_id),
	  XtRImmediate, (XtPointer)((Font)NULL) },

	{ XtNolCursorFontData, XtCReadOnly,
	  XtRFontStruct, sizeof(XFontStruct *),
	  Offset(screen.ol_cursor_font_data),
	  XtRImmediate, (XtPointer) ((XFontStruct *)NULL) },

	{ XtNdoingDynamicResProcessing, XtCDoingDynamicResProcessing,
	  XtRBoolean, sizeof(Boolean),
	  Offset(screen.doing_dynamic_res_processing),
	  XtRImmediate, (XtPointer) False},
};

/* WARNING THESE RESOURCES ARE NOT INHERITED */

static XtResource screenShellAppResources[] = {
	{ XtNmouseDampingFactor, "MouseDampingFactor",
	  XtRCardinal, sizeof(Cardinal),
	  Offset(screen.mouse_damping_factor), XtRImmediate,
	  (XtPointer)8 },

	{ XtNthreeD, XtCThreeD,
	  XtRBoolean, sizeof(Boolean), Offset(screen.three_d),
	  XtRImmediate, (XtPointer)True },

	{ XtNscale, XtCScale,
	  XtROlScale, sizeof(int), Offset(screen.scale),
	  XtRImmediate, (XtPointer)OL_DEFAULT_POINT_SIZE },

	{ XtNscaleMap, XtCScaleMap,
	  XtRString, sizeof(String), Offset(screen.scale_map_file),
	  XtRString, (XtPointer)NULL },

	{ XtNdragRightDistance, XtCDragRightDistance, XtRDimension,
	  sizeof(Dimension), Offset(screen.drag_right_distance),
	  XtRImmediate, (XtPointer)OL_DRAG_RIGHT_DISTANCE },

	{ XtNmenuMarkRegion, XtCMenuMarkRegion, XtRDimension,
	  sizeof(Dimension), Offset(screen.menu_mark_region),
	  XtRImmediate, (XtPointer)10 },
};

externaldef(screenshellclassrec) ScreenShellClassRec screenShellClassRec = {
  {
    /* superclass           */  (WidgetClass) &rootShellClassRec,
    /* class_name           */  "ScreenShell",
    /* size                 */  sizeof(ScreenShellRec),
    /* Class Initialize     */	NULL,
    /* class_part_initialize*/	ScreenShellClassPartInitialize,
    /* Class init'ed ?      */	FALSE,
    /* initialize           */  ScreenShellInitialize,
    /* initialize_notify    */	NULL,		
    /* realize              */  NULL,
    /* actions              */  NULL,
    /* num_actions          */  0,
    /* resources            */  screenShellResources,
    /* resource_count       */	XtNumber(screenShellResources),
    /* xrm_class            */  NULLQUARK,
    /* compress_motion      */  FALSE,
    /* compress_exposure    */  FALSE,
    /* compress_enterleave  */ 	FALSE,
    /* visible_interest     */  FALSE,
    /* destroy              */  ScreenShellDestroy,
    /* resize               */  NULL,
    /* expose               */  NULL,
    /* set_values           */  ScreenShellSetValues,
    /* set_values_hook      */	NULL,			
    /* set_values_almost    */	NULL,  
    /* get_values_hook      */	NULL,			
    /* accept_focus         */  NULL,
    /* intrinsics version   */	XtVersion,
    /* callback offsets     */  NULL,
    /* tm_table		    */  NULL,
    /* query_geometry	    */  NULL,
    /* display_accelerator  */  NULL,
    /* extension	    */  NULL
  },
  { /* composite            */
    /* geometry_manager     */  XtInheritGeometryManager,
    /* change_managed       */  XtInheritChangeManaged,
    /* insert_child	    */	XtInheritInsertChild,
    /* delete_child	    */	XtInheritDeleteChild,
    /* extension	    */  NULL
  },
  { /* shell                */
    /* extension	    */  (XtPointer)&rootShellClassExtRec,
  },
  { /* override shell       */
    /* extension	    */  NULL
  },
  { /* root shell           */
    /* appl res		   */  screenShellAppResources,
    /* appl num res 	   */  XtNumber(screenShellAppResources),
    /* add_shell           */  XtInheritRSAddShell,
    /* delete_shell        */  XtInheritRSDeleteShell,
    /* shell_registered    */  (RSShellRegisteredProc)NULL,

    /* add_root             */  XtInheritRSAddRoot,
    /* delete_root          */  XtInheritRSDeleteRoot,
    /* root_registered      */  XtInheritRSRootRegistered,

    /* prop_notify	    */  XtInheritRSPropertyNotify,
    /* prop_changed	    */  ScreenResourcesPropUpdated,

    /* root_shells          */  (Widget *)NULL,
    /* num_roots            */  (Cardinal)NULL,

    /* extension            */  (XtPointer)NULL
  },
  { /* screen shell	   */
    /* rm_prop_changed	   */  NotifyScreenOfRMUpdate,
    /* extension	   */  (XtPointer)NULL
  }
};

externaldef(screenshellwidgetclass) WidgetClass screenShellWidgetClass = 
	                                (WidgetClass) (&screenShellClassRec);

/************* private resource initialisation ftns ************/

/***************************************************************
 *
 * _Depth
 *
 ***************************************************************/

/*FTNPROTOB*/
static
void _Depth (Widget widget, int closure, XrmValue *value)
      	        
   	         
        	        
/*FTNPROTOE*/
{
	static unsigned int	depth; /* uugh */

	depth = DefaultDepthOfScreen(widget->core.screen);
	value->addr = (XtPointer)&depth;
}

/***************************************************************
 *
 * _Colormap
 *
 ***************************************************************/

/*FTNPROTOB*/
static
void _Colormap (Widget widget, int closure, XrmValue *value)
      		       
   			        
          		      
/*FTNPROTOE*/
{
	static Colormap	colormap;

	colormap = DefaultColormapOfScreen(widget->core.screen);
	value->addr = (XtPointer)&colormap;
}

/***************************************************************
 *
 * _Visual
 *
 ***************************************************************/

/*FTNPROTOB*/
static
void _Visual (Widget widget, int closure, XrmValue *value)
      		       
   			        
          		      
/*FTNPROTOE*/
{
	ShellWidget	sh = (ShellWidget)widget;
	static Visual	*visual;

	visual = DefaultVisualOfScreen(sh->core.screen);
	value->addr = (XtPointer)&visual;
}

/***************************************************************
 *
 * _Width
 *
 ***************************************************************/

/*FTNPROTOB*/
static
void _Width (Widget widget, int closure, XrmValue *value)
      		       
   			        
          		      
/*FTNPROTOE*/
{
	static Dimension width;

	width = WidthOfScreen(widget->core.screen);
	value->addr = (XtPointer)&width;
}

/***************************************************************
 *
 * _Height
 *
 ***************************************************************/

/*FTNPROTOB*/
static
void _Height (Widget widget, int closure, XrmValue *value)
      		       
   			        
          		      
/*FTNPROTOE*/
{
	static Dimension height;

	height = HeightOfScreen(widget->core.screen);
	value->addr = (XtPointer)&height;
}

/************ private functions *********/

/***************************************************************
 *
 * GetRootShellPropString
 *
 ***************************************************************/

/*FTNPROTOB*/
static Boolean
_GetRootShellPropString (RootShellWidget rsw)
               		    
/*FTNPROTOE*/
{
	Widget			w = (Widget)rsw;
	Atom			actual_type;
	int			actual_format;
	unsigned long		nitems;
	unsigned long		leftover;
	char 			*string_db;
	Boolean			fail;

	if (fail = XGetWindowProperty(XtDisplay(w), XtWindow(w),
	           rsw->root.root_property, 0L, 100000000L, False, XA_STRING,
	           &actual_type, &actual_format, &nitems, &leftover,
	           (unsigned char **) &string_db) != Success)
	{
                        string_db = (char *) NULL;
	}
	else if ((actual_type != XA_STRING) || (actual_format != 8))
	{
		if (string_db != NULL) {
			XFree ( string_db );
                        string_db = (char *) NULL;
                }
		fail = True;
	} 
	
	if (rsw->root.property_string != (String)NULL)
		XFree((char *)rsw->root.property_string);

	rsw->root.property_string = string_db;

	return (!fail);
}

/***************************************************************
 *
 * _rootPropChanged
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
_rootPropChanged (Widget w, XtPointer clientd, XEvent *xevent, Boolean *continue_to_dispatch)
      			  
         		        
        			       
         		                     
/*FTNPROTOE*/
{
	RootShellWidget		rsw;
	Widget			*roots;
	RootShellWidgetClass	rswc;
	int			i;
	Cardinal		num;

	if (xevent->xany.type != PropertyNotify	||
	    xevent->xproperty.state != PropertyNewValue)
		return;
	
	roots = rootShellClassRec.root_shell.root_shells;
	num   = rootShellClassRec.root_shell.num_roots;

	for (i = 0; i < num; i++) {
		rsw = (RootShellWidget)roots[i];

		if (rsw->core.window != w->core.window ||
		    /* !XtIsSubclass(rsw, rootShellWidgetClass) || */
		    rsw->root.root_property != xevent->xproperty.atom) continue;

		rsw->root.last_update = xevent->xproperty.time;

		rswc = (RootShellWidgetClass)rsw->core.widget_class;

		if (rswc->root_shell.prop_changed != (RSPropChangedProc)NULL) {
			(*rswc->root_shell.prop_changed)((Widget)rsw);
		}
	}
}

/***************************************************************
 *
 * loadCursorFont
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
loadCursorFont (ScreenShellWidget ssw)
                 		    
/*FTNPROTOE*/
{
	char			*fn, *savefn, *p;
	int			n,
				l = 0,		/* only valid if p != NULL */
				tryagain = 2;


	if (ssw->screen.ol_cursor_font_name == (char *)NULL)
		return;

	n = strlen(ssw->screen.ol_cursor_font_name);

	p = strchr(ssw->screen.ol_cursor_font_name, '%');

	if (p != (char *)NULL) { /* we substitute scale into font name */
		int	scale = ssw->screen.scale,
			field = 2;
		char	b[5];

		while (scale >= 10) {
			scale /= 10;
			field++;
		}

		savefn = fn = XtCalloc(n + field + 1, sizeof(char));

		(void) snprintf(b, 5, "%*d", field, ssw->screen.scale * 10);

		l = p - ssw->screen.ol_cursor_font_name;
		strncpy(fn, ssw->screen.ol_cursor_font_name, l);
		strcat(fn, b);
		strcat(fn, p + 1);
	} else {
		fn = ssw->screen.ol_cursor_font_name;
	}

	while (tryagain--) {
		ssw->screen.ol_cursor_font_data =
			XLoadQueryFont(XtDisplay(ssw), fn);
		if (ssw->screen.ol_cursor_font_data != (XFontStruct *)NULL) {
			ssw->screen.ol_cursor_font_id =
				ssw->screen.ol_cursor_font_data->fid;
			tryagain = 0;
		} else if (p != (char *)NULL) { /* tried at particular scale */
			*(fn = savefn) = '\0';
			strncat(fn,  ssw->screen.ol_cursor_font_name, l);
			fn[l] = '*'; fn[l + 1] ='\0';
			strcat(fn, p + 1);
		} else
			tryagain = 0;
	}

	if (fn != ssw->screen.ol_cursor_font_name) XtFree(fn);
}

/***************************************************************
 *
 * DisplayShellClassInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
Check3D (ScreenShellWidget ssw)
{
	XVisualInfo	vinfo,
			*vi;
	int		nitems,
			i;
	VisualID	vid = XVisualIDFromVisual(ssw->shell.visual);

	vinfo.screen = XScreenNumberOfScreen(XtScreen((Widget)ssw));

	vi = XGetVisualInfo(XtDisplay((Widget)ssw), 
				      (long)VisualScreenMask,
				      &vinfo, &nitems);

	if (vi == (XVisualInfo *)NULL || nitems == 0) return;

	for (i = 0; i < nitems; i++) {
		if (vi[i].visualid == vid && vi[i].depth == 1) {
			ssw->screen.three_d = False;
			break;
		}
	}

	XFree((char *)vi);
}

/************ class methods ***********/

/***************************************************************
 *
 * DisplayShellClassInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
DisplayShellClassInitialize(void)
/*FTNPROTOE*/
{
	extern Boolean _OlStringToOlBtnDef (Display *,
						    XrmValue *,
						    Cardinal *,
					            XrmValue *,
						    XrmValue *,
						    XtPointer *);

	extern Boolean _OlStringToOlKeyDef (Display *,
						    XrmValue *,
						    Cardinal *,
					            XrmValue *,
						    XrmValue *,
						    XtPointer *);

	OlRegisterConverters();

	XtSetTypeConverter(XtRString, XtROlKeyDef, _OlStringToOlKeyDef,
			(XtConvertArgList)NULL, (Cardinal)0,
			XtCacheNone, (XtDestructor)0);

	XtSetTypeConverter(XtRString, XtROlBtnDef, _OlStringToOlBtnDef,
			(XtConvertArgList)NULL, (Cardinal)0,
			XtCacheNone, (XtDestructor)0);
 

	_OlAddOlDefineType("supercaret",       OL_SUPERCARET);
	_OlAddOlDefineType("inputfocuscolor",  OL_INPUT_FOCUS_COLOR);

	_OlAddOlDefineType("never",            OL_BEEP_NEVER);
	_OlAddOlDefineType("always",           OL_BEEP_ALWAYS);
	_OlAddOlDefineType("notices",          OL_BEEP_NOTICES);

	_OlAddOlDefineType("underline",        OL_UNDERLINE);
	_OlAddOlDefineType("highlight",        OL_HIGHLIGHT);
	_OlAddOlDefineType("none",             OL_NONE);
	_OlAddOlDefineType("inactive",         OL_INACTIVE);
	_OlAddOlDefineType("display",          OL_DISPLAY);

	_OlAddOlDefineType("pointer",          OL_POINTER);
	_OlAddOlDefineType("inputfocus",       OL_INPUTFOCUS);
}

/***************************************************************
 *
 * RootClassInitialize
 *
 ***************************************************************/

static	void InitializeWPSRewritingRules(void);

static	Boolean	RewriteRDBForWPS(RootShellWidget, XrmDatabase*,
				 XrmDatabase*, XtPointer);

/*FTNPROTOB*/
static void
RootClassInitialize(void)
/*FTNPROTOE*/
{
	InitializeWPSRewritingRules();
}

/***************************************************************
 *
 * RootClassPartInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
RootClassPartInitialize (WidgetClass wc)
           		   
/*FTNPROTOE*/
{
        XrmResourceList         super_res       = (XrmResourceList)NULL;
        Cardinal                super_num_res   = (Cardinal)0,
                                super_part_size = (Cardinal)0;
 
	RootShellWidgetClass	rswc = (RootShellWidgetClass)wc,
			        super = (RootShellWidgetClass)wc->core_class.superclass;

        extern void     _OlResourceDependencies(XtResourceList  *appl_res,
                                                Cardinal        *num_appl_res,
                                                XrmResourceList super_res,
                                                Cardinal        super_num_res,
                                                Cardinal        super_part_size)
;
 
        if (wc != rootShellWidgetClass) {
                super_res       = (XrmResourceList)super->root_shell.appl_res;
                super_num_res   = super->root_shell.num_appl_res;
                super_part_size = super->core_class.widget_size;
        }
 
        /*
         * inherit/compile superclass appl resources
         */
 
        _OlResourceDependencies(&(rswc->root_shell.appl_res),
                                &(rswc->root_shell.num_appl_res),
                                super_res,
                                super_num_res,
                                super_part_size);

#ifndef	__STDC__
#define	_INHERIT(field, Proc)					\
	if (rswc->root_shell.field == XtInherit/**/Proc)	\
		rswc->root_shell.field = super->root_shell.field
#else
#define	_INHERIT(field, Proc)					\
	if (rswc->root_shell.field == XtInherit##Proc)		\
		rswc->root_shell.field = super->root_shell.field
#endif

	_INHERIT(add_shell,RSAddShell);
	_INHERIT(delete_shell,RSDeleteShell);
	_INHERIT(shell_registered,RSShellRegistered);
	_INHERIT(add_root,RSAddRoot);
	_INHERIT(delete_root,RSDeleteRoot);
	_INHERIT(root_registered,RSRootRegistered);
	_INHERIT(prop_notify,RSPropertyNotify);

#undef	_INHERIT
}
/***************************************************************
 *
 * ScreenShellClassPartInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
ScreenShellClassPartInitialize (WidgetClass wc)
           		   
/*FTNPROTOE*/
{
	ScreenShellWidgetClass	sswc = (ScreenShellWidgetClass)wc,
			        super = (ScreenShellWidgetClass)wc->core_class.superclass;

#ifndef	__STDC__
#define	_INHERIT(field, Proc)					\
	if (sswc->screen_shell.field == XtInherit/**/Proc)	\
		sswc->screen_shell.field = super->screen_shell.field
#else
#define	_INHERIT(field, Proc)					\
	if (sswc->screen_shell.field == XtInherit##Proc)	\
		sswc->screen_shell.field = super->screen_shell.field
#endif

	_INHERIT(rm_prop_changed,RSPropChangedProc);
#undef	_INHERIT
}

/***************************************************************
 *
 * RootInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void 
RootInitialize (Widget req, Widget new, ArgList args, Cardinal *num_args)
      	     
      	     
       	      
        	           
/*FTNPROTOE*/
{
        Boolean			db_rewritten = (Boolean) False;
	RootShellWidget		rsw = (RootShellWidget) new;
	Screen			*scr = rsw->core.screen;
	RootShellWidgetClass	rswc = (RootShellWidgetClass)rsw->core.widget_class;

	if (rsw->core.parent != (Widget)NULL) {
		rsw->core.being_destroyed = True; /* we aint no popup dude! */
		return;
	}

	/*
	 * check the resource database on this screen and perhaps rewrite
	 * workspace properties.
	 */

	{ 
		XrmDatabase	sdb,
				trdb = (XrmDatabase)NULL;
		Display		*dpy    = XtDisplay(new);
		Screen		*screen = XtScreen(new);

		sdb = XtScreenDatabase(screen);

		db_rewritten = RewriteRDBForWPS(rsw, &sdb, &trdb,
						(XtPointer)NULL);

		if (db_rewritten) {
			XrmMergeDatabases(trdb, &sdb);

			_OlVendorRereadSomeResources(rsw->root.vendor_instance,
						     args, num_args);
			
			/*
			 * reload 'toolkit' resources 
			 */
			if (rswc->root_shell.appl_res != (XtResourceList)NULL &&
		            rswc->root_shell.num_appl_res > 0) {
				XtGetSubresources(new, (XtPointer)new,
						  (String)NULL, (String)NULL,
						  rswc->root_shell.appl_res,
						  rswc->root_shell.num_appl_res,
						  args, *num_args
				);
			}
		}
	}

	rsw->core.depth    = DefaultDepthOfScreen(scr);
	rsw->shell.visual  = DefaultVisualOfScreen(scr);
	rsw->core.colormap = DefaultColormapOfScreen(scr);

	rsw->core.x        = 0;
        rsw->core.y        = 0;
	rsw->core.width    = WidthOfScreen(scr);
	rsw->core.height   = HeightOfScreen(scr);

	rsw->core.border_width = 0;
	rsw->core.mapped_when_managed = False;

	rsw->shell.save_under = False;
	rsw->shell.override_redirect = False;
	rsw->shell.visual = DefaultVisualOfScreen(rsw->core.screen);

	rsw->core.window = RootWindowOfScreen(rsw->core.screen);

	rsw->shell.client_specified =
		( _XtShellPositionValid |
		  _XtShellNotReparented |
		  _XtShellPPositionOK   |
		  _XtShellGeometryParsed );

	/* only one root shell per screen gets an event handler! */

	_OlGetRootShellApplRes(rsw, args, *num_args);

	if (rswc->root_shell.prop_notify != (RSPropertyNotifyProc)NULL) {
		Widget	w;

		w = _OlRootShellOfScreen(new->core.screen,
					 new->core.widget_class);

		if (w == (Widget)NULL) {
			w = _OlRootShellOfScreen(new->core.screen,
						 rootShellWidgetClass);
		}

		if (w == (Widget)NULL || w->core.window != rsw->core.window) {
			_XtRegisterWindow(rsw->core.window, new);
			XtInsertEventHandler(new, PropertyChangeMask, False,
					     rswc->root_shell.prop_notify,
					     NULL, XtListHead);
		}
	}

	if (rootShellClassRec.root_shell.add_root != (RSAddRootProc)NULL)
		(*rootShellClassRec.root_shell.add_root)
				(rsw, &rootShellClassRec);

	if (rswc->root_shell.add_root != (RSAddRootProc)NULL)
		(*rswc->root_shell.add_root)(rsw, rswc);


	rsw->root.last_update = CurrentTime;
	rsw->root.property_string = (String)NULL;
	
	if (rswc->root_shell.appl_res != (XtResourceList)NULL &&
	    rswc->root_shell.num_appl_res > 0) {
		XtSetSubvalues((XtPointer)rsw,
			       rswc->root_shell.appl_res,
			       rswc->root_shell.num_appl_res,
			       args, *num_args);
	}
}

/***************************************************************
 *
 * DisplayShellInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void 
DisplayShellInitialize (Widget req, Widget new, ArgList args, Cardinal *num_args)
{
	DisplayShellWidget	dsw = (DisplayShellWidget)new;

	_OlGetModifierMapping(XtDisplay(dsw));

		/* Get the application's attributes */

	if (toplevelDisplay == (Display *)NULL) {
		toplevelDisplay = XtDisplay(new);
		InitializeOpenLook(toplevelDisplay);
	}


	_OlInitDynamicHandler((Widget)dsw);

	/* Initialize display drag state to False */
	dsw->display.doing_drag = FALSE;
	/*
	 * Initialize the list of im_related info recs
	 * to be created: one rec per VendorShell using
	 * this display.
	 */
	dsw->display.im_vs_info_list = (ImVSInfoList) NULL;

	dsw->display.shape_extension_present =
                XShapeQueryExtension(XtDisplay(new),
                                     &(dsw->display.shape_event_base),
                                     &(dsw->display.shape_error_base)
                );
	dsw->display.ol_default_font = XtNewString(dsw->display.ol_default_font);

	if (dsw->display.mnemonic_modifiers == (Modifiers)None)
		dsw->display.mnemonic_modifiers = _OlGetModifierBinding(XtDisplay(dsw), Meta);

	if (dsw->display.mod1_name == NULL)
		dsw->display.mod1_name = _OlGetModifierNames(XtDisplay(dsw), Mod1Mask);
	if (dsw->display.mod2_name == NULL)
		dsw->display.mod2_name = _OlGetModifierNames(XtDisplay(dsw), Mod2Mask);
	if (dsw->display.mod3_name == NULL)
		dsw->display.mod3_name = _OlGetModifierNames(XtDisplay(dsw), Mod3Mask);
	if (dsw->display.mod4_name == NULL)
		dsw->display.mod4_name = _OlGetModifierNames(XtDisplay(dsw), Mod4Mask);
	if (dsw->display.mod5_name == NULL)
		dsw->display.mod5_name = _OlGetModifierNames(XtDisplay(dsw), Mod5Mask);
		
}

/***************************************************************
 *
 * ScreenShellInitialize
 *
 ***************************************************************/

/*FTNPROTOB*/
static void 
ScreenShellInitialize (Widget req, Widget new, ArgList args, Cardinal *num_args)
      	     
      	     
       	      
        	           
/*FTNPROTOE*/
{
	extern	void 		_OlInitAttributes ( Widget );

	ScreenShellWidget	ssw = (ScreenShellWidget)new;

	ssw->screen.prev_app_attrs = (_OlAppAttributes *)NULL;
	ssw->screen.app_attrs      = (_OlAppAttributes *)NULL;
	ssw->screen.dyn_cbs        = (OlDynamicScreenCallbacks)NULL;
	ssw->screen.num_dyn_cbs    = (Cardinal)0;

	ssw->screen.app_attrs_need_update = (Boolean)False;

	loadCursorFont(ssw);


	_OlAddShellToDisplayList(new);

	Check3D(ssw);

	_OlInitAttributes(new);
}

/***************************************************************
 *
 * RootDestroy
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
RootDestroy (Widget wid)
      	     
/*FTNPROTOE*/
{
	RootShellWidget		rsw = (RootShellWidget)wid;
	RootShellWidgetClass	rswc = (RootShellWidgetClass)rsw->core.widget_class;
	Widget			*wp = rsw->root.shells;
	Window			root_window;


	root_window = RootWindowOfScreen(wid->core.screen);

	if (wp != (Widget *)NULL) {
		int			i;
		RSDeleteShellProc	rootp;

		rootp = rswc->root_shell.delete_shell;

		if (rootp != (RSDeleteShellProc)NULL) {
			for (i = 0; i < rsw->root.num_shells; i++)
				(*rootp)(rsw, *wp++);
		}

		XtFree((char *)rsw->root.shells);
	}

	if (rootShellClassRec.root_shell.delete_root != (RSDeleteRootProc)NULL)
		(*rootShellClassRec.root_shell.delete_root)
				(rsw, &rootShellClassRec);

	if (rswc->root_shell.delete_root !=
	    (RSDeleteRootProc)NULL)
		(*rswc->root_shell.delete_root)(rsw, rswc);

	if (rsw->root.property_string != (String)NULL)
		XFree((char *)rsw->root.property_string);

	if (XtWindowToWidget(XtDisplay(rsw), root_window) == wid) {
		Widget		w;
		XtEventHandler	evh;

		_XtUnregisterWindow(root_window, wid);

		evh = rswc->root_shell.prop_notify;

		XtRemoveEventHandler(wid, PropertyChangeMask, False,
				     evh, NULL);

		w = _OlRootShellOfScreen(rsw->core.screen,
					 wid->core.widget_class);

		if (w == (Widget)NULL) {
			w = _OlRootShellOfScreen(rsw->core.screen,
						 rootShellWidgetClass);
		}

		if (w != (Widget)NULL) {
			_XtRegisterWindow(root_window, w);

			rswc = (RootShellWidgetClass)w->core.widget_class;

			if (rswc->root_shell.prop_notify != 
			    (RSPropertyNotifyProc)NULL) 
				evh = rswc->root_shell.prop_notify;

			XtInsertEventHandler(w, PropertyChangeMask, False,
					     evh, NULL, XtListHead);
		}

	} 

	if (wid->core.window == root_window)
		wid->core.window = (Window)NULL;
}

/***************************************************************
 *
 * ScreenShellDestroy
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
ScreenShellDestroy (Widget w)
      	   
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw = (ScreenShellWidget)w;

	if (ssw->screen.prev_app_attrs != (_OlAppAttributes *)NULL) {
		XtFree((char *)ssw->screen.prev_app_attrs);
	}

	if (ssw->screen.app_attrs != (_OlAppAttributes *)NULL) {
		XtFree((char *)ssw->screen.app_attrs);
	}

	if (ssw->screen.dyn_cbs != (OlDynamicScreenCallbacks)NULL) {
		XtFree((char *)ssw->screen.dyn_cbs);
	}

	if (ssw->screen.ol_cursor_font_data != (XFontStruct *)NULL) {
		XFreeFont(XtDisplay(w), ssw->screen.ol_cursor_font_data);
	}

	_OlgxFreeDeviceData(ssw->core.screen);
	_OlDeleteShellFromDisplayList(w);
}

/***************************************************************
 *
 * DisplayShellDestroy
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
DisplayShellDestroy (Widget w)
      	   
/*FTNPROTOE*/
{
	extern	void	_OlFlush50PercentGreyCache(void);
	extern	void	_OlFlush75PercentGreyCache(void);

	_OlCleanupPDVEDB(XtDisplay(w));

	_OlFlush50PercentGreyCache();
	_OlFlush75PercentGreyCache();

	if (XtDisplay(w) == toplevelDisplay) 
		toplevelDisplay = (Display *)NULL;
}

/***************************************************************
 *
 * GeometryManager
 *
 ***************************************************************/

/*FTNPROTOB*/
static XtGeometryResult
GeometryManager (Widget gw, XtWidgetGeometry *request, XtWidgetGeometry *reply)
      	 	         
                 	          
                 	        
/*FTNPROTOB*/
{
    return XtGeometryYes;	/* let them be free! */
}

/***************************************************************
 *
 * RootGeometryManager
 *
 ***************************************************************/

/*FTNPROTOB*/
static XtGeometryResult
RootGeometryManager (Widget gw, XtWidgetGeometry *request, XtWidgetGeometry *reply)
      	 	         
                 	          
                 	        
/*FTNPROTOE*/
{
    return XtGeometryNo;	/* we cant - so dont! */
}

/***************************************************************
 *
 * RootSetValues
 *
 ***************************************************************/

/* ARGSUSED */
/*FTNPROTOB*/
static Boolean
RootSetValues (Widget old, Widget ref, Widget new, ArgList args, Cardinal *num_args)
      		     
      		     
      	        
       	        
         	          
/*FTNPROTOE*/
{
	RootShellWidget		nw = (RootShellWidget) new;
	RootShellWidget		ow = (RootShellWidget) old;
	RootShellWidgetClass	rswc;

	rswc = (RootShellWidgetClass)(nw->core.widget_class);

#define _READONLY(field)	nw->field = ow->field

	_READONLY(core.x);
	_READONLY(core.y);
	_READONLY(core.width);
	_READONLY(core.height);
	_READONLY(core.border_width);
	_READONLY(core.background_pixel);
	_READONLY(core.depth);
	_READONLY(core.colormap);
	_READONLY(core.screen);
	_READONLY(core.window);
	_READONLY(shell.visual);
	_READONLY(shell.allow_shell_resize);
	_READONLY(shell.client_specified);
	_READONLY(shell.override_redirect);
	_READONLY(root.root_property);
	_READONLY(root.property_string);
#undef	_READONLY

	if (ow->root.appl_shell == (Widget)NULL &&
	    nw->root.appl_shell != (Widget)NULL &&
	    XtIsSubclass(nw->root.appl_shell,
			 applicationShellWidgetClass))
		_OlGetRootShellApplRes(nw, args, *num_args);
	else
		nw->root.appl_shell = ow->root.appl_shell;

	if (rswc->root_shell.appl_res != (XtResourceList)NULL &&
	    rswc->root_shell.num_appl_res > 0) {
		XtSetSubvalues((XtPointer)nw,
			       rswc->root_shell.appl_res,
			       rswc->root_shell.num_appl_res,
			       args, *num_args);
	}
			
	return (FALSE);
}

/***************************************************************
 *
 * RootGetValuesHook
 *
 ***************************************************************/

/* ARGSUSED */
/*FTNPROTOB*/
static void
RootGetValuesHook (Widget w, ArgList args, Cardinal *num_args)
      			  
       			     
          		         
/*FTNPROTOE*/
{
	RootShellWidgetClass	rswc = (RootShellWidgetClass)(w->core.widget_class);

	if (rswc->root_shell.appl_res != (XtResourceList)NULL &&
	    rswc->root_shell.num_appl_res > 0) {
		XtGetSubvalues((XtPointer)w, 
			       rswc->root_shell.appl_res,
			       rswc->root_shell.num_appl_res,
			       args, *num_args);
	}
}
/***************************************************************
 *
 * ScreenShellSetValues
 *
 ***************************************************************/

/* ARGSUSED */
/*FTNPROTOB*/
static Boolean
ScreenShellSetValues (Widget old, Widget ref, Widget new, ArgList args, Cardinal *num_args)
      		     
      		     
      	        
       	        
         	          
/*FTNPROTOE*/
{
	ScreenShellWidget nw = (ScreenShellWidget) new;
	ScreenShellWidget ow = (ScreenShellWidget) old;

#define _READONLY(field)	nw->field = ow->field

	_READONLY(screen.ol_cursor_font_id);
	_READONLY(screen.ol_cursor_font_data);

	if (nw->screen.ol_cursor_font_name != ow->screen.ol_cursor_font_name)
		loadCursorFont(nw);
	if (nw->screen.three_d != ow->screen.three_d)
		Check3D(nw);

	nw->screen.app_attrs_need_update = (Boolean)False;
#undef	_READONLY
	return (Boolean)False;
}

/***************************************************************
 *
 * RootShellOfScreen
 *
 ***************************************************************/

/*FTNPROTOB*/
static Widget
RootShellOfScreen (WidgetClass wc, Screen *screen)
           		   
        			       
/*FTNPROTOE*/
{
	RootShellWidgetClass	rswc = (RootShellWidgetClass)wc;
	Widget			*roots = rswc->root_shell.root_shells;
	Cardinal		num    = rswc->root_shell.num_roots;
	Cardinal		i;

	if (roots != (Widget *)NULL) {
		for (i = 0; i < num; i++) {
			if (roots[i]->core.screen == screen /* ||
			    roots[i]->core.window == RootWindowOfScreen(screen) */) {
				return roots[i];
			}
		}
	
		return ((Widget) NULL);
	}

	return ((Widget) NULL);
}


/***************************************************************
 *
 * DisplayShellOfScreen
 *
 ***************************************************************/

/*FTNPROTOB*/
static Widget
DisplayShellOfScreen (WidgetClass wc, Screen *screen)
           		   
        			       
/*FTNPROTOE*/
{
	return RootShellOfScreen(wc, ScreenOfDisplay(DisplayOfScreen(screen), 0));
}

/***************************************************************
 *
 * _shelldied
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
_shelldied (Widget shell, caddr_t client_d, caddr_t call_d)
      			      
       			         
       			       
/*FTNPROTOE*/
{
	RootShellWidget 	root = (RootShellWidget)client_d;
	RSDeleteShellProc	rootp;

	rootp = 
	   ((RootShellWidgetClass)root->core.widget_class)->root_shell.delete_shell;

	if (rootp != (RSDeleteShellProc)NULL)
		(*rootp)(root, shell);
}

/***************************************************************
 *
 * AddShell
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
AddShell (RootShellWidget root, Widget shell)
               		     
      			      
/*FTNPROTOE*/
{
	register int	i;

	if (VECTORFULL(root->root.num_shells, VECTOR_INCR) ||
	    root->root.num_shells == 0) {
		root->root.shells = (Widget *)XtRealloc(
						(char *)root->root.shells,
						sizeof(Widget) * 
							(root->root.num_shells +
							VECTOR_INCR));
	}

	for (i = 0; i < root->root.num_shells; i++) {
		if (root->root.shells[i] == shell) {
			return; /* duplicates not allowed */
		}
	}

	root->root.shells[root->root.num_shells++] = shell;

	XtAddCallback(shell, XtNdestroyCallback,
		      (XtCallbackProc)_shelldied, (caddr_t)root);
}

/***************************************************************
 *
 * DeleteShell
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
DeleteShell (RootShellWidget root, Widget shell)
               		     
      			      
/*FTNPROTOE*/
{
	register int	i;

	if (root->root.num_shells == 0 || root->root.shells == (Widget *)NULL)
		return;

	for (i = 0; i < root->root.num_shells; i++) {
		if (root->root.shells[i] == shell) {
			break;
		}
	}

	if (i == root->root.num_shells) return; /* not found */

	XtRemoveCallback(shell, XtNdestroyCallback,
			 (XtCallbackProc)_shelldied, (caddr_t)root);

	root->root.num_shells--;

	if (i < root->root.num_shells) { /* not at end */
		memcpy((char *)(root->root.shells + i),
		       (char *)(root->root.shells + i + 1),
		       (root->root.num_shells - i) * sizeof(Widget));
	}

	root->root.shells[root->root.num_shells] = (Widget)NULL;

	if (root->root.num_shells == 0 && 
	    root->root.shells != (Widget *)NULL) {
		XtFree((char *)root->root.shells);
		root->root.shells = (Widget *)NULL;

		XtDestroyWidget((Widget)root);
	}
}

/***************************************************************
 *
 * AddRoot
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
AddRoot (RootShellWidget root, RootShellWidgetClass wc)
               			     
                    		   
/*FTNPROTOE*/
{
	register int		i;

	if (VECTORFULL(wc->root_shell.num_roots, VECTOR_INCR) ||
	    wc->root_shell.num_roots == 0) {
		wc->root_shell.root_shells =
				(Widget *)XtRealloc(
					(char *)wc->root_shell.root_shells,
					sizeof(Widget) *
					(wc->root_shell.num_roots + 
					 VECTOR_INCR));
	}

	for (i = 0; i < wc->root_shell.num_roots; i++) {
		if (wc->root_shell.root_shells[i] == (Widget)root)
			return;
	}

	wc->root_shell.root_shells[wc->root_shell.num_roots++] = (Widget)root;
}

/***************************************************************
 *
 * DeleteRoot
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
DeleteRoot (RootShellWidget root, RootShellWidgetClass wc)
               			     
                    		   
/*FTNPROTOE*/
{
	register int		i;

	if (wc->root_shell.num_roots == 0 ||
	    wc->root_shell.root_shells == (Widget *)NULL)
		return;

	for (i = 0; i < wc->root_shell.num_roots; i++) {
		if (wc->root_shell.root_shells[i] == (Widget)root)
			break;
	}

	if (i == wc->root_shell.num_roots) return; /* not found */

	wc->root_shell.num_roots--;

	if (i < wc->root_shell.num_roots) { /* not at end */
		memcpy((char *)(wc->root_shell.root_shells + i),
		       (char *)(wc->root_shell.root_shells + i + 1),
		       (wc->root_shell.num_roots - i) * sizeof(Widget));
	}

	wc->root_shell.root_shells[wc->root_shell.num_roots] = (Widget)NULL;

	if (wc->root_shell.num_roots == 0 &&
	    wc->root_shell.root_shells != (Widget *)NULL){
		XtFree((char *)wc->root_shell.root_shells);
		wc->root_shell.root_shells = (Widget *)NULL;
	}
}

/***************************************************************
 *
 * ResourceManagerPropUpdated
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
ResourceManagerPropUpdated (Widget w)
      		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw = (DisplayShellWidget)w;
	XrmDatabase		rdb,
				trdb;
	int			i;
	char 			*dpy_defaults;
	struct _XDisplay	*dpy = (struct _XDisplay *)XtDisplay(w);
	Boolean                 mouseless_orig;

	if (!_GetRootShellPropString((RootShellWidget)dsw)) return; /* oops */

	rdb = XrmGetStringDatabase(dsw->root.property_string);

	if (rdb != (XrmDatabase)NULL) {
		trdb = XrmGetDatabase(XtDisplay((Widget)dsw));
		XrmMergeDatabases(rdb, &trdb);
	}

	if ((dpy_defaults = XResourceManagerString(XtDisplay(w))) !=
	    (char *)NULL)
		XFree(dpy_defaults);

	dpy->xdefaults = XtCalloc(strlen(dsw->root.property_string) + 1,
				 sizeof(char));

	strcpy(dpy->xdefaults, dsw->root.property_string);

	/*
	 *  Oops need to rewrite rdb for WPS and reload Display shell
	 *  resources
	 */

	/* This routine updates all rootshell widget's resources as well as
	   displayshell widget's whenever those resources are changed in the
	   rootshell.  We do not want the mouseless resource to be dynamically
	   changed, so we need to restore its original value after it
	   gets updated.
	 */
	mouseless_orig = dsw->display.mouseless;
	_OlGetRootShellApplRes((RootShellWidget)w, (ArgList)NULL, (Cardinal)0);
	XtVaSetValues((Widget) dsw, XtNmouseless, mouseless_orig, NULL);

	if (XtHasCallbacks(w, XtNpropNotifyCallback) == XtCallbackHasSome)
		XtCallCallbacks(w,XtNpropNotifyCallback, (XtPointer)NULL);

	for (i = 0; i < dsw->root.num_shells; i++) {
		ScreenShellWidget	ssw;
		RSPropChangedProc	prop_changed;
		ScreenShellWidgetClass	sswc;

		w = dsw->root.shells[i];

		if (!XtIsSubclass(w, screenShellWidgetClass)) continue;

		ssw = (ScreenShellWidget)w;

		rdb = XrmGetStringDatabase(dsw->root.property_string);
					
		if (rdb != (XrmDatabase)NULL) {
			trdb = XtScreenDatabase(XtScreen((Widget)ssw));
			XrmMergeDatabases(rdb, &trdb);
			RewriteRDBForWPS((RootShellWidget)ssw,
					 &trdb, (XrmDatabase*)NULL,
					 (XtPointer)NULL
			);
	/*
	 *  Oops need to rewrite rdb for WPS and reload Screen shell
	 *  resources
	 */

		}

		sswc         = (ScreenShellWidgetClass)ssw->core.widget_class;
		prop_changed = sswc->screen_shell.rm_prop_changed;

		ssw->screen.doing_dynamic_res_processing = True;


		if (prop_changed != (RSPropChangedProc)NULL) {
			(*prop_changed)((Widget)ssw);
		}
		ssw->screen.doing_dynamic_res_processing = False;
	}
}

/***************************************************************
 *
 * NotifyScreenOfRMUpdate
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
NotifyScreenOfRMUpdate (Widget w)
      		  
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw = (ScreenShellWidget)w;

	_OlGetRootShellApplRes((RootShellWidget)w, (ArgList)NULL, (Cardinal)0);

	ssw->screen.app_attrs_need_update = (Boolean)True;

	Check3D(ssw);

	if (toplevelDisplay != (Display *)NULL &&
	    XtScreen(w) == DefaultScreenOfDisplay(toplevelDisplay))
		OlCallDynamicCallbacks(); /* backwards compatibility */

	DynamicHandler((Widget)ssw);
	OlCallDynamicScreenCBs(XtScreen(w));

	if (XtHasCallbacks(w, XtNpropNotifyCallback) == XtCallbackHasSome)
		XtCallCallbacks(w, XtNpropNotifyCallback, (XtPointer)NULL);
}

/***************************************************************
 *
 * ScreenResourcesPropUpdated
 *
 ***************************************************************/

/*FTNPROTOB*/
static void
ScreenResourcesPropUpdated (Widget w)
      		  
/*FTNPROTOE*/
{
        XrmDatabase                     sdb;
        ScreenShellWidget               ssw = (ScreenShellWidget)w;
        ScreenShellWidgetClass          sswc;
        Display                         *dpy    = XtDisplay(w);
        Screen                          *screen = XtScreen(w);
        char                            *scr_res;
 
        sswc    = (ScreenShellWidgetClass)w->core.widget_class;
        sdb     = XtScreenDatabase(screen);
        scr_res = XScreenResourceString(screen);
 
        if (scr_res != (char *)NULL) {
                XrmMergeDatabases(XrmGetStringDatabase(scr_res), &sdb);
                XFree(scr_res);
		RewriteRDBForWPS((RootShellWidget)ssw,
				 &sdb, (XrmDatabase*)NULL,
				 (XtPointer)NULL
		);
        }
 
        ssw->screen.doing_dynamic_res_processing = True;
        if (sswc->screen_shell.rm_prop_changed != (RSPropChangedProc)NULL)
                (*sswc->screen_shell.rm_prop_changed)(w);
        ssw->screen.doing_dynamic_res_processing = False;

}

/********************** user functions **********************/


/***************************************************************
 *
 * The following mechanism is used to take Workspace properties
 * and rewrite them for use in an Xt environment.
 *
 ***************************************************************/

static XrmQuark	OpenWindows     = NULLQUARK;

/* function proto for rewriting rule function */

typedef	Boolean	(*WPSRewritingRuleProc)(XrmDatabase		*database,
					RootShellWidget		rsw,
				        XrmQuark		target,
				        XrmBindingList		bindings,
				        XrmQuarkList		quarks,
				        Cardinal		num_quarks,
				        XrmRepresentation*	type,
				        XrmValue*		value,
				        XtPointer		closure);

/* the rewriting rules themselves ... */

static Boolean
_GenericRule(XrmDatabase*       rdb,
          RootShellWidget       rsw,
          XrmQuark              target,
          XrmBindingList        bindings,
          XrmQuarkList          quarks,
          Cardinal              num_quarks,
          XrmRepresentation*    type,
          XrmValue*             value,
          XtPointer             closure)
{
        if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);

        /*
         * store as *target:    <value>
         */

        bindings[0] = XrmBindLoosely;
        quarks[0]   = quarks[1];
        quarks[1]   = NULLQUARK;

        XrmQPutResource(rdb, bindings, quarks, *type, value);

        return ((Boolean)True);
}

static Boolean
_KeyboardCommandsRule(XrmDatabase*      rdb,
                 RootShellWidget        rsw,
                 XrmQuark               target,
                 XrmBindingList         bindings,
                 XrmQuarkList           quarks,
                 Cardinal               num_quarks,
                 XrmRepresentation*     type,
                 XrmValue*              value,
                 XtPointer              closure)
{
        if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);

        /*
         * store as *Mouseless: True/False
         *
         * KeyboardCommands             Mouseless
         * ----------------             ---------
         * Full                 ->      True
         * Basic                ->      False
         * SunView1             ->      False
         */

        if (value->addr != NULL) {
            if (!strcmp(value->addr, "Full")) {
                bindings[0] = XrmBindLoosely;
                quarks[0]   = XrmStringToQuark("Mouseless");
                quarks[1]   = NULLQUARK;
                value->addr = (XtPointer) "True";
                value->size = strlen("True") + 1;
                XrmQPutResource(rdb, bindings, quarks, *type, value);
                return ((Boolean)True);
            } else if (!strcmp(value->addr, "SunView1") ||
                       !strcmp(value->addr, "Basic")) {
                bindings[0] = XrmBindLoosely;
                quarks[0]   = XrmStringToQuark("Mouseless");
                quarks[1]   = NULLQUARK;
                value->addr = (XtPointer) "False";
                value->size = strlen("False") + 1;
                XrmQPutResource(rdb, bindings, quarks, *type, value);
                return ((Boolean)True);
            }
        }
 
        return ((Boolean)False);
}

static Boolean
_MultiClickTimeoutRule(XrmDatabase*		rdb,
		       RootShellWidget		rsw,
		       XrmQuark			target,
		       XrmBindingList		bindings,
		       XrmQuarkList   		quarks,
		       Cardinal			num_quarks,
		       XrmRepresentation*	type,
		       XrmValue*		value,
		       XtPointer		closure)
{
	if (num_quarks != 2 ||
	    !(quarks[0] == OpenWindows && quarks[1] == target))
		return ((Boolean)False);

	/* 
	 * store as *MultiClickTimeout:	<value>*100
	 */
	
	num_quarks--;
	bindings[0] = XrmBindLoosely;
	quarks[0]   = XrmStringToQuark("MultiClickTimeout");
	quarks[1]   = NULLQUARK;

	if (value->addr != NULL) {
	    int i = atoi((String) value->addr);

	    if (i > 0) {
			static char *val_str = NULL;
			
			if (!val_str)
				if (!(val_str = malloc(80)))
					return ((Boolean) False);
			i *= 100;
			snprintf(val_str, 80, "%d", i);
			value->addr = (XPointer) val_str;
			value->size = strlen(val_str) + 1;
			XrmQPutResource(rdb, bindings, quarks, *type, value);
			return ((Boolean) True);
	    }
	}
	return ((Boolean) False);
}

static Boolean   
_ScrollbarPlacementRule(XrmDatabase*            rdb,
                 RootShellWidget                rsw,
                 XrmQuark                       target,
                 XrmBindingList                 bindings,
                 XrmQuarkList                   quarks,
                 Cardinal                       num_quarks,
                 XrmRepresentation*             type,
                 XrmValue*                      value,
                 XtPointer                      closure)
{
        if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);

        /*
         * store as *AlignVertical:     <value>
         */

        bindings[0] = XrmBindLoosely;
        quarks[0]   = XrmStringToQuark("AlignVertical");
        quarks[1]   = NULLQUARK;
        XrmQPutResource(rdb, bindings, quarks, *type, value);

        return ((Boolean)True);
}

static Boolean
_SelectDisplaysMenuRule(XrmDatabase*    rdb,
                 RootShellWidget        rsw,
                 XrmQuark               target,
                 XrmBindingList         bindings,
                 XrmQuarkList           quarks,
                 Cardinal               num_quarks,
                 XrmRepresentation*     type,
                 XrmValue*              value,
                 XtPointer              closure)
{
        if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);

        /*
         * store as *SelectDoesPreview: True/False
         *
         * SelectDisplaysMenu           SelectDoesPreview
         * ------------------           -----------------
         * True                 ->      False
         * False                ->      True
         */

        if (value->addr != NULL) {
            if (!strcmp(value->addr, "True")) {
                bindings[0] = XrmBindLoosely;
                quarks[0]   = XrmStringToQuark("SelectDoesPreview");
                quarks[1]   = NULLQUARK;
                value->addr = (XtPointer) "False";
                value->size = strlen("False") + 1;
                XrmQPutResource(rdb, bindings, quarks, *type, value);
                return ((Boolean)True);
            } else if (!strcmp(value->addr, "False")) {
                bindings[0] = XrmBindLoosely;
                quarks[0]   = XrmStringToQuark("SelectDoesPreview");
                quarks[1]   = NULLQUARK;
                value->addr = (XtPointer) "True";
                value->size = strlen("True") + 1;
                XrmQPutResource(rdb, bindings, quarks, *type, value);
                return ((Boolean)True);
            }
        }

        return ((Boolean)False);
}

static Boolean
_WindowColorRule(XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	if (num_quarks != 2 ||
	    !(quarks[0] == OpenWindows && quarks[1] == target))
		return ((Boolean)False);

	/* 
	 * store as *Background:	<value>
	 */
	
	bindings[0] = XrmBindLoosely;
	quarks[0]   = XrmStringToQuark("Background");
	quarks[1]   = NULLQUARK;
	XrmQPutResource(rdb, bindings, quarks, *type, value);

	return ((Boolean)True);
}

static Boolean
_WindowForegroundRule(XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	if (num_quarks != 2 ||
	    !(quarks[0] == OpenWindows && quarks[1] == target))
		return ((Boolean)False);

	/* 
	 * store as *Foreground:	<value>
	 */
	
	bindings[0] = XrmBindLoosely;
	quarks[0]   = XrmStringToQuark("Foreground");
	quarks[1]   = NULLQUARK;
	XrmQPutResource(rdb, bindings, quarks, *type, value);
	
	/*
	 * store as *FontColor: <value>
	 */
	quarks[0]   = XrmStringToQuark("FontColor");
	XrmQPutResource(rdb, bindings, quarks, *type, value);

	return ((Boolean)True);
}

static Boolean
_DataBackgroundRule(XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	if (num_quarks != 2 ||
	    !(quarks[0] == OpenWindows && quarks[1] == target))
		return ((Boolean)False);

	/* 
	 * store as *TextBackground:	<value>
	 */
	
	bindings[0]   =	XrmBindLoosely;
	quarks[0]     = XrmStringToQuark("TextBackground");
	quarks[1]     = NULLQUARK;
	XrmQPutResource(rdb, bindings, quarks, *type, value);

	return ((Boolean)True);
}

static Boolean
_DataForegroundRule(XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	if (num_quarks != 2 ||
	    !(quarks[0] == OpenWindows && quarks[1] == target))
		return ((Boolean)False);

	/* 
	 * store as *TextFontColor:	<value>
	 */
	
	bindings[0]   =	XrmBindLoosely;
	quarks[0]   = XrmStringToQuark("TextFontColor");
	quarks[1]   = NULLQUARK;
	XrmQPutResource(rdb, bindings, quarks, *type, value);

	return ((Boolean)True);
}


static Boolean
_ScrollbarJumpCursorRule(
		 XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	XrmBinding	olbindings[2];
	XrmQuark	olquarks[3];

	if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);
	/*
	 * The name of the resource specified in work space
	 * props is Openwindows.ScrollbarJumpCursor.
	 */

	/*
	 * Store as *Scrollbar.PointerWarping: <value>
	 */
	olbindings[0]	= XrmBindLoosely;
	olbindings[1]	= XrmBindTightly;
	olquarks[0] 	= XrmStringToQuark("Scrollbar");
	olquarks[1]	= XrmStringToQuark("PointerWarping");
	olquarks[2]	= NULLQUARK;

	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	return((Boolean)TRUE);
}

static Boolean
_PopupJumpCursorRule(
		 XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	XrmBinding	olbindings[2];
	XrmQuark	olquarks[3];

	if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);
	/*
	 * The name of the resource specified in work space
	 * props is Openwindows.PopupJumpCursor.
	 */

	/*
	 * Store as *NoticeShell.PointerWarping: <value>
	 */

	olbindings[0]	= XrmBindLoosely;
	olbindings[1]	= XrmBindTightly;
	olquarks[0] 	= XrmStringToQuark("NoticeShell");
	olquarks[1]	= XrmStringToQuark("PointerWarping");
	olquarks[2]	= NULLQUARK;

	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	/*
	 * Store as *PopupWindowShell.PointerWarping: <value>
	 */
	olquarks[0] 	= XrmStringToQuark("PopupWindowShell");
	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	/*
	 * Store as *FileChooserShell.PointerWarping: <value>
	 */
	olquarks[0] 	= XrmStringToQuark("FileChooserShell");
	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	return((Boolean)TRUE);
}

static Boolean
_MonospaceFontRule(
		 XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	XrmBinding	olbindings[2];
	XrmQuark	olquarks[3];

	if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);
	/*
	 * The name of the resource specified in work space
	 * props is Openwindows.MonospaceFont.
	 */

	/*
	 * Store as *TextEdit*Font: <value>
	 */

	olbindings[0]	= XrmBindLoosely;
	olbindings[1]	= XrmBindLoosely;
	olquarks[0]	= XrmStringToQuark("TextEdit");
	olquarks[1]	= XrmStringToQuark("Font");
	olquarks[2]	= NULLQUARK;

	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	/*
	 * Store as *TextField*Font: <value>
	 */
	olquarks[0]     = XrmStringToQuark("TextField");
	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	/*
	 * Store as *TextLine*Font: <value>
	 */
	olquarks[0]     = XrmStringToQuark("TextLine");
	XrmQPutResource(rdb, olbindings, olquarks, *type, value);


	return((Boolean)TRUE);
}

static Boolean
_RegularFontRule(
		 XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);
	/*
	 * The name of the resource specified in work space
	 * props is Openwindows.RegularFont.
	 */

	/*
	 * Store as *OlDefaultFont: <value>
	 */

	bindings[0]	= XrmBindLoosely;
	quarks[0]	= XrmStringToQuark("OlDefaultFont");
	quarks[1]	= NULLQUARK;

	XrmQPutResource(rdb, bindings, quarks, *type, value);

	return((Boolean)TRUE);
}

static Boolean
_BoldFontRule(
		 XrmDatabase*		rdb,
		 RootShellWidget	rsw,
		 XrmQuark		target,
		 XrmBindingList		bindings,
		 XrmQuarkList   	quarks,
		 Cardinal		num_quarks,
		 XrmRepresentation*	type,
		 XrmValue*		value,
		 XtPointer		closure)
{
	XrmBinding	olbindings[2];
	XrmQuark	olquarks[3];

	if (num_quarks != 2 ||
            !(quarks[0] == OpenWindows && quarks[1] == target))
                return ((Boolean)False);
	/*
	 * The name of the resource specified in work space
	 * props is Openwindows.BoldFont
	 */

	/*
	 * Store as *Caption.Font: <value>
	 */

	olbindings[0]	= XrmBindLoosely;
	olbindings[1]	= XrmBindTightly;
	olquarks[0] 	= XrmStringToQuark("Caption");
	olquarks[1]	= XrmStringToQuark("Font");
	olquarks[2]	= NULLQUARK;

	XrmQPutResource(rdb, olbindings, olquarks, *type, value);

	olquarks[0] 	= XrmStringToQuark("CaptionFont");
	olquarks[1]	= NULLQUARK;
	XrmQPutResource(rdb, olbindings, olquarks, *type, value);


	return((Boolean)TRUE);
}


typedef	struct	_wps_rewriting_rule {
		union {
			String		resource_string;
			XrmQuark	resource_quark;
		} res;
		WPSRewritingRuleProc	rewriting_rule;
} WPSRewritingRule, *WPSRewritingRulePtr;

/*
 * this table defines the resources of the form
 * OpenWindows{.|*}<resource> that are to be rewritten
 * for re-inclusion into the resource database in a form
 * that can be applied to widget instance heirarchies.
 *
 * the rewriting rule is a function of type WPSRewritingRuleProc
 * which will be invoked to actually effect the rewriting of
 * the current WPS resource entry into the more appropriate
 * form, this will usually just take the form of a call to
 * XrmQPutResource.
 */

static WPSRewritingRule	rewriting_rule_table[] = {
	/* resource name		rewriting rule */
        { "Beep",                       _GenericRule            },
	{ "BeepDuration",		_GenericRule            },
        { "DragRightDistance",          _GenericRule            },
        { "KeyboardCommands",           _KeyboardCommandsRule   },
        { "MenuAccelerators",           _GenericRule            },
        { "MultiClickTimeout",          _MultiClickTimeoutRule  },
        { "ScrollbarPlacement",         _ScrollbarPlacementRule },
        { "SelectDisplaysMenu",         _SelectDisplaysMenuRule },
        { "WindowColor",                _WindowColorRule        },
	{ "WindowForeground",		_WindowForegroundRule	},
	{ "DataBackground",		_DataBackgroundRule	},
	{ "DataForeground",		_DataForegroundRule	},
	{ "ScrollbarJumpCursor",	_ScrollbarJumpCursorRule},
	{ "PopupJumpCursor",		_PopupJumpCursorRule	},
	{ "RegularFont",		_RegularFontRule	},
	{ "BoldFont",			_BoldFontRule		},
	{ "MonospaceFont",		_MonospaceFontRule	},
	{ "CtrlAltMetaKey",		_GenericRule		},
	{ "Scale",			_GenericRule		}

};

/*
 * since rewriting the contents of a resource database is prohibited
 * during an enumeration of that database, entries found during the
 * enumeration as candidates for rewriting must be recorded for
 * rewriting after the database has been enumerated. This data
 * structure following provides a function closure for entries
 * requiring the application of a rewriting rule.
 */

typedef	struct _wps_rr_proc_closure *WPSRRProcClosurePtr;

typedef	struct	_wps_rr_proc_closure {
	WPSRRProcClosurePtr	next;
	unsigned int		rr_table_index;
	XrmBindingList		bindings;
	XrmQuarkList		quarks;
	Cardinal		num_quarks;
	XrmRepresentation	type;
	XrmValue		value;
} WPSRRProcClosure;

static WPSRRProcClosurePtr	wps_rr_closure_list = (WPSRRProcClosurePtr)NULL;

#define	SizeOfWPSRRProcClosure(nq)	(sizeof(WPSRRProcClosure) +	 \
					 ((nq + 1) * sizeof(XrmQuark)) + \
					 ((nq) * sizeof(XrmBinding)))

#define	QuarksOfWPSRRProcClosurePtr(wpsrrpcp)				 \
	((XrmQuarkList)(((unsigned char *)(wpsrrpcp)) +			 \
			  sizeof(WPSRRProcClosure)))

#define	BindingsOfWPSRRProcClosurePtr(wpsrrpcp)				\
	((XrmBindingList)(((unsigned char *)(wpsrrpcp)) + 		\
					   sizeof(WPSRRProcClosure) +	\
			(((wpsrrpcp)->num_quarks + 1) * sizeof(XrmQuark))))

/*
 * rewriting rule table comparator .. used by qsort/bsearch
 */

static int
_ComparWPSRR(const void * i, const void * j)
{
	WPSRewritingRulePtr	ip = (WPSRewritingRulePtr)i,
				jp = (WPSRewritingRulePtr)j;

	return ((int)(ip->res.resource_quark) - (int)(jp->res.resource_quark));
}

/*
 * the initialiser for the rewriting rule mechanism.
 */

/*FTNPROTOB*/
static void
InitializeWPSRewritingRules(void)
/*FTNPROTOE*/
{
	unsigned int		i;
	WPSRewritingRulePtr	wpsrrp = rewriting_rule_table;

	if (OpenWindows != NULLQUARK) return; /* already init'd */

	/* quark WPS prefix ... */

	OpenWindows = XrmPermStringToQuark("OpenWindows");

	/* quark resource table entries */

	for (i = 0; i < XtNumber(rewriting_rule_table); i++) {
		wpsrrp[i].res.resource_quark =
			XrmPermStringToQuark(wpsrrp[i].res.resource_string);
	}

	/* ensure they are in ascending order ... */

	qsort((char *)rewriting_rule_table, XtNumber(rewriting_rule_table),
	      sizeof(WPSRewritingRule), _ComparWPSRR);
}

/*
 * this is the database enumeration function called from XrmEnumerateDatabase.
 */

static Bool
_DBRewriteEnumeration(XrmDatabase*	database,
		      XrmBindingList	bindings,
		      XrmQuarkList	quarks,
		      XrmRepresentation	*type,
		      XrmValue		*value,
		      XPointer		closure)
{
	WPSRewritingRulePtr	wpsrrp	     = (WPSRewritingRulePtr)NULL;
	Cardinal		num_quarks   = (Cardinal)0;
	int			i;

	for(; quarks[num_quarks] != NULLQUARK; num_quarks++)
	;

	
	wpsrrp = (WPSRewritingRulePtr)bsearch((char *)&(quarks[num_quarks - 1]),
					      (char *)rewriting_rule_table,
					      XtNumber(rewriting_rule_table),
					      sizeof(WPSRewritingRule),
					      _ComparWPSRR);

	/*
	 * we cant apply the rewriting rule while we are enumerating the
	 * database according to the spec, therefore we must build a
	 * proc closure to be executed after we have completed the
	 * enumeration.
	 */

	if (wpsrrp != (WPSRewritingRulePtr)NULL) {
		WPSRRProcClosurePtr	wpsrrpcp;

		wpsrrpcp = (WPSRRProcClosurePtr)
				XtCalloc(1, SizeOfWPSRRProcClosure(num_quarks));

		wpsrrpcp->rr_table_index = wpsrrp - rewriting_rule_table;
		wpsrrpcp->num_quarks     = num_quarks;
		wpsrrpcp->type		 = *type;
		wpsrrpcp->value.size	 = value->size;
		wpsrrpcp->value.addr	 = value->addr;

		wpsrrpcp->quarks = QuarksOfWPSRRProcClosurePtr(wpsrrpcp);

		memcpy((char *)QuarksOfWPSRRProcClosurePtr(wpsrrpcp),
		       (char *)quarks,
		       (num_quarks + 1) * sizeof(XrmQuark));/* inc NULLQUARK */

		wpsrrpcp->bindings = BindingsOfWPSRRProcClosurePtr(wpsrrpcp);

		memcpy((char *)BindingsOfWPSRRProcClosurePtr(wpsrrpcp),
		       (char *)bindings,
		       num_quarks * sizeof(XrmBinding));
		
		wpsrrpcp->next      = wps_rr_closure_list;
		wps_rr_closure_list = wpsrrpcp;
	}

	return((Boolean)False);
}

/*
 * rewrite a resource database, locating all the WorkSpace Properties
 * applicable and rewrite them in the database according the their
 * rewriting rules.
 *
 * returns True if any resources have been rewritten False otherwise;
 *
 */

static Boolean
RewriteRDBForWPS(RootShellWidget	rsw,
		 XrmDatabase*		rdb,
		 XrmDatabase*		trdb,
		 XtPointer		closure)
{
	XrmDatabase*		prdb;
	WPSRRProcClosurePtr	wpsrrpcp;
	WPSRRProcClosurePtr	temp;
	XrmQuark		names[2];
	Boolean			ret = (Boolean)False;

	names[0] = OpenWindows;
	names[1] = NULLQUARK;

	(void)XrmEnumerateDatabase(*rdb, names, names,
				   XrmEnumOneLevel, _DBRewriteEnumeration,
				   closure
	);

	if (wps_rr_closure_list == (WPSRRProcClosurePtr)NULL)
		return ((Boolean)False);

	prdb = ((trdb == (XrmDatabase*)NULL) ? rdb : trdb);

	for (wpsrrpcp =  wps_rr_closure_list;
	     wpsrrpcp != (WPSRRProcClosurePtr)NULL;) {
		unsigned int	idx = wpsrrpcp->rr_table_index;

		ret |= (*rewriting_rule_table[idx].rewriting_rule)(
				 prdb,
				 rsw,
				 rewriting_rule_table[idx].res.resource_quark,
				 BindingsOfWPSRRProcClosurePtr(wpsrrpcp),
				 QuarksOfWPSRRProcClosurePtr(wpsrrpcp),
				 wpsrrpcp->num_quarks,
				 &(wpsrrpcp->type),
				 &(wpsrrpcp->value),
				 closure
		);

		temp = wpsrrpcp->next;
		XtFree((char *)wpsrrpcp);
		wpsrrpcp = temp;
	}

	/*
	 * mark the database as being updated ... if it was .....
	 */

	if (ret) {
		XrmPutStringResource(prdb, "_OlRDBBeenRewritten", "True");
	}

	wps_rr_closure_list = (WPSRRProcClosurePtr)NULL;

	return (ret);
}

/***************************************************************/

/*FTNPROTOB*/
void
_SetDefaultAppCon (XtAppContext app_con)
             		        
/*FTNPROTOE*/
{
	ProcessContext	proc = _XtGetProcessContext();

	if (proc->defaultAppContext == (XtAppContext)NULL) {
		proc->defaultAppContext = app_con;
	}
}

/*FTNPROTOB*/
Widget
_OlRootShellOfScreen (Screen *screen, WidgetClass subclass)
        			       
           		         
/*FTNPROTOE*/
{
	register WidgetClass	wc;
	RSRootRegisteredProc	rootp;
	Widget			w = (Widget)NULL;

	for (wc = subclass;
	     wc != (WidgetClass)NULL && wc != rootShellWidgetClass;
	     wc = wc->core_class.superclass)
	;

	if (wc == (WidgetClass)NULL) return (Widget)NULL;

	rootp =
	  ((RootShellWidgetClass)subclass)->root_shell.root_registered;

	if (rootp != (RSRootRegisteredProc)NULL )
			w = (*rootp)(subclass, screen);

	return (w);
}


/*FTNPROTOB*/
void
_OlAddShellToRootList (Widget shell, WidgetClass subclass)
      			      
           		         
/*FTNPROTOE*/
{
	RootShellWidget		root;
	RSAddShellProc		rootp;
	RootShellWidgetClass	rswc;

	if (!XtIsSubclass(shell, shellWidgetClass))
		return;

	root = (RootShellWidget)_OlRootShellOfScreen(shell->core.screen,
						     subclass);

        if (root == (RootShellWidget)NULL) {
		return;
	}

	rswc = (RootShellWidgetClass)root->core.widget_class;
	rootp = rswc->root_shell.add_shell;

	if (rootp != (RSAddShellProc)NULL)
		(*rootp)(root, shell);
}

/*FTNPROTOB*/
void
_OlDeleteShellFromRootList (Widget shell, WidgetClass subclass)
      			      
           		         
/*FTNPROTOE*/
{
	RootShellWidget		root;
	RSAddShellProc		rootp;
	RootShellWidgetClass	rswc;

	if (!XtIsSubclass(shell, shellWidgetClass))
		return;

	root = (RootShellWidget)_OlRootShellOfScreen(shell->core.screen,
						    subclass);

        if (root == (RootShellWidget)NULL) {
		return;
	}

	rswc = (RootShellWidgetClass)root->core.widget_class;
	rootp = rswc->root_shell.delete_shell;

	if (rootp != (RSAddShellProc)NULL)
		(*rootp)(root, shell);
}

/*FTNPROTOB*/
void
_OlAddShellToDisplayList (Widget shell)
      		      
/*FTNPROTOE*/
{
	_OlAddShellToRootList(shell, displayShellWidgetClass);
}

/*FTNPROTOB*/
void
_OlDeleteShellFromDisplayList (Widget shell)
      		      
/*FTNPROTOE*/
{
	_OlDeleteShellFromRootList(shell, displayShellWidgetClass);
}

/*FTNPROTOB*/
void
_OlAddShellToScreenList (Widget shell)
      		      
/*FTNPROTOE*/
{
	_OlAddShellToRootList(shell, screenShellWidgetClass);
}

/*FTNPROTOB*/
void
_OlDeleteShellFromScreenList (Widget shell)
      		      
/*FTNPROTOE*/
{
	_OlDeleteShellFromRootList(shell, screenShellWidgetClass);
}

/*
 * a tad inefficient but it gets the job done .... since there is unlikely
 * to be many subclasses of root it doesnt really matter too much!
 *
 * the overhead is a redundant DB probe per resource per subclass overridden.
 */

static void
_FetchApplRes (register RootShellWidget rsw, register WidgetClass wc, register ArgList args, register Cardinal num_args)
{
	if (wc != rootShellWidgetClass)
		_FetchApplRes(rsw, wc->core_class.superclass, args, num_args);

	{ 
		Widget			appl = rsw->root.appl_shell;
		RootShellWidgetClass	rswc = (RootShellWidgetClass)wc;

		if (rswc->root_shell.appl_res != (XtResourceList)NULL &&
		    rswc->root_shell.num_appl_res > (Cardinal)NULL) {
			XtGetApplicationResources(appl, (XtPointer)rsw, 
			  	  		  rswc->root_shell.appl_res,
				  		  rswc->root_shell.num_appl_res,
						  (ArgList)NULL, 0);
		}

	}
}
/*FTNPROTOB*/
void
_OlGetRootShellApplRes (RootShellWidget rsw, ArgList args, Cardinal num_args)
               		    
       			     
        			         
/*FTNPROTOE*/
{
	if (!XtIsSubclass((Widget)rsw, rootShellWidgetClass)) return;

	if (rsw->root.appl_shell != (Widget)NULL)
		_FetchApplRes(rsw, rsw->core.widget_class, args, num_args);
}

/***************************** USER CALLS ********************************/


/*************************************************************************
 *
 *	_OlGetListOfDisplayShells()
 *
 * USE THIS LIST AS READ-ONLY!!!!!!
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
_OlGetListOfDisplayShells (Widget **list, Cardinal *num)
         		     
          		    
/*FTNPROTOE*/
{
	DisplayShellWidgetClass	dswc;

	dswc = (DisplayShellWidgetClass)displayShellWidgetClass;

	*list = dswc->root_shell.root_shells;
	*num  = dswc->root_shell.num_roots;
}

/*************************************************************************
 *
 *	_OlGetListOfScreenShells()
 *
 * USE THIS LIST AS READ-ONLY!!!!!!
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
_OlGetListOfScreenShells (Widget **list, Cardinal *num)
         		     
          		    
/*FTNPROTOE*/
{
	ScreenShellWidgetClass	sswc;

	sswc = (ScreenShellWidgetClass)screenShellWidgetClass;

	*list = sswc->root_shell.root_shells;
	*num  = sswc->root_shell.num_roots;
}


/*************************************************************************
 *
 *	OlInternAtom
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Atom
OlInternAtom (Display *dpy, const char *name)
/*FTNPROTOE*/
{
	Widget		dsw;
	Atom		ret;
	XrmValue	from,
			to;

	GetToken();
	dsw = _OlGetDisplayShellOfScreen(DefaultScreenOfDisplay(dpy));
	from.addr = (char *)name;
	from.size = strlen(name);

	to.size = sizeof(Atom);
	to.addr = (caddr_t)&ret;

	XtConvertAndStore(dsw, XtRString, &from, XtRAtom, &to);

	ReleaseToken();
	return (ret);
}

/*************************************************************************
 *
 *	OlCreateDisplayShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
OlCreateDisplayShell (Widget w, ArgList args, Cardinal num_args)
/*FTNPROTOE*/
{
	extern	Widget	OlApplicationWidget;

	Widget		 	shell = _OlGetShellOfWidget(w);
	DisplayShellWidget	dsw;
	ArgList		 	merge_args = args;
	Cardinal	 	merge_num  = num_args;
	Arg			largs[3];
	Cardinal		num = 0;
	String			name;
	Arg			*ash_arg = (Arg *)NULL,
				*scr_arg = (Arg *)NULL;

	if (w == (Widget)NULL || shell == (Widget)NULL) return (Widget)NULL;
	
	dsw = (DisplayShellWidget)_OlGetDisplayShellOfWidget(w);

	for (; num < num_args; num++) {
		if (ash_arg != (Arg *)NULL && 
		    (args[num].name == XtNapplShell ||
		     strcmp(args[num].name, XtNapplShell)))
			ash_arg = args + num;
		if (scr_arg != (Arg *)NULL &&
		    (args[num].name == XtNscreen ||
		     strcmp(args[num].name, XtNscreen)))
			scr_arg = args + num;
	}

	num = 0;

	if (dsw == (DisplayShellWidget)NULL ||
	    dsw->root.appl_shell == (Widget)NULL) {
		ApplicationShellWidget	ash;

		if (XtParent(shell) != (Widget)NULL)
			shell = _OlGetShellOfWidget(XtParent(shell));

		name = shell->core.name;
	
		if (XtIsSubclass(shell, applicationShellWidgetClass) &&
		    ash_arg == (Arg *)NULL) {
			ash = (ApplicationShellWidget)shell;

			XtSetArg(largs[num], XtNapplShell, (XtPointer)ash); 
			num++;
		}

		if (dsw != (DisplayShellWidget)NULL) {
			merge_args = XtMergeArgLists(args, num_args, largs, num);
							    
			XtSetValues((Widget)dsw, merge_args, num_args + num);

			XtFree((char *)merge_args);
		}
	}

	if (dsw == (DisplayShellWidget)NULL) {
		if (scr_arg != (Arg *)NULL) {
			scr_arg->value = (XtArgVal)
					 ScreenOfDisplay(XtDisplay(w), 0);
		} else {
			XtSetArg(largs[num], XtNscreen,
				 ScreenOfDisplay(XtDisplay(w), 0)); num++;

		}
		XtSetArg(largs[num], XtNvendorInstance, w); num++;


		merge_args = XtMergeArgLists(args, num_args,
					     largs , num);
		merge_num  += num;

		dsw = (DisplayShellWidget)
			      XtAppCreateShell(name,
					       (String)NULL,
					       displayShellWidgetClass,
					       XtDisplay(w),
					       merge_args, merge_num);

		if (OlApplicationWidget == (Widget)NULL &&
		    XtIsSubclass(w, applicationShellWidgetClass))
			OlApplicationWidget = w;

		XtFree((char *)merge_args);
	}

	return ((Widget)dsw);
}

/*************************************************************************
 *
 *	OlDestroyDisplayShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlDestroyDisplayShell (Widget widget, Boolean xpd)
/*FTNPROTOE*/
{
	int			i;
	DisplayShellWidget	dsw = (DisplayShellWidget)
					_OlGetDisplayShellOfWidget(widget);

	if (dsw == (DisplayShellWidget)NULL) return;

	if (xpd)
		 for (i = 0; i < dsw->root.num_shells; i++)
			XtDestroyWidget(dsw->root.shells[i]);

	if (dsw->root.num_shells == 0 || xpd) {
		_OlCleanupPDVEDB(dsw->core.screen->display);
		XtDestroyWidget((Widget)dsw);
	}
}

/*************************************************************************
 *
 *	OlCreateScreenShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
OlCreateScreenShell (Widget w, ArgList args, Cardinal num_args)
      		  
       		     
        		         
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;
	Widget			shell = _OlGetShellOfWidget(w);
	ApplicationShellWidget	ash;
	ArgList		 	merge_args = args;
	Cardinal	 	merge_num  = num_args;
	Arg			largs[3]; /* a small Scottish seaside town */
	Cardinal		num = 0;
	String			name;
	Arg			*ash_arg = (Arg *)NULL,
				*scr_arg = (Arg *)NULL;

	if (w == (Widget)NULL || shell == (Widget)NULL) return (Widget)NULL;
	
	ssw = (ScreenShellWidget)_OlGetScreenShellOfWidget(w);

	for (; num < num_args; num++) {
		if (ash_arg != (Arg *)NULL && 
		    (args[num].name == XtNapplShell ||
		     strcmp(args[num].name, XtNapplShell)))
			ash_arg = args + num;
		if (scr_arg != (Arg *)NULL &&
		    (args[num].name == XtNscreen ||
		     strcmp(args[num].name, XtNscreen)))
			scr_arg = args + num;
	}

	num = 0;

	if (ssw == (ScreenShellWidget)NULL ||
	    ssw->root.appl_shell == (Widget)NULL) {
		Widget			*list;
		Cardinal		nwidgets;
		DisplayShellWidget	dsw;

		if (XtParent(shell) != (Widget)NULL)
			shell = _OlGetShellOfWidget(XtParent(shell));

		name = shell->core.name;

		if (ash_arg == (Arg *)NULL) {
			if (XtIsSubclass(shell, applicationShellWidgetClass)) {
				ash = (ApplicationShellWidget)shell;
			} else {
				dsw = (DisplayShellWidget)
						_OlGetDisplayShellOfWidget(w);

				if (dsw != (DisplayShellWidget)NULL &&
				    dsw->root.appl_shell != (Widget)NULL)
					ash = (ApplicationShellWidget)
							(dsw->root.appl_shell);
				else
					return ((Widget)ssw);
				
			}

			XtSetArg(largs[num], XtNapplShell, (XtPointer)ash);
			num++;
		}

		_OlGetListOfScreenShells(&list, &nwidgets);

		merge_args = XtMergeArgLists(args, num_args, largs, num);

		while(nwidgets--) {
			if (XtDisplay(*list) == XtDisplay(ash))
				XtSetValues(*list, merge_args, num_args + num);
			list++;
		}

		XtFree((char *)merge_args);
	}

	if (ssw == (ScreenShellWidget)NULL) {
		if (scr_arg != (Arg *)NULL) {
			scr_arg->value = (XtArgVal)(w->core.screen);
		} else {
			XtSetArg(largs[num], XtNscreen, w->core.screen); num++;

		}
		XtSetArg(largs[num], XtNvendorInstance, w); num++;

		merge_args = XtMergeArgLists(args, num_args,
					     largs , num);
		merge_num  += num;

		ssw = (ScreenShellWidget)
				XtAppCreateShell(name,
						 (String)NULL,
						 screenShellWidgetClass,
						 XtDisplay(w),
						 merge_args,
						 merge_num);

		XtFree((char *)merge_args);
	}

	return ((Widget)ssw);
}

/*************************************************************************
 *
 *	OlDestroyScreenShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlDestroyScreenShell (Widget widget, Boolean xpd)
/*FTNPROTOE*/
{
	int			i;
	ScreenShellWidget	ssw = (ScreenShellWidget)
					_OlGetScreenShellOfWidget(widget);

	if (ssw == (ScreenShellWidget)NULL) return;

	if (xpd)
		 for (i = 0; i < ssw->root.num_shells; i++)
			XtDestroyWidget(ssw->root.shells[i]);

	if (ssw->root.num_shells == 0 || xpd) {
		XtDestroyWidget((Widget)ssw);
	}
}

/*************************************************************************
 *
 *	OlgIs3d
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean	OlgIs3d (Screen *screen)
        		       
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

	if (ssw == (ScreenShellWidget)NULL || !ssw->screen.three_d)
		return (Boolean)False;
	else
		return (Boolean)True;
}

/***************** Reading Workspace props for locale ********************/
#define NUM_LANG_WSP_NAMES	5
#define LANG_STR_LEN		1024

static char *lang_wsp_names[NUM_LANG_WSP_NAMES] = {
    "basicLocale",	/* LC_CTYPE, LC_MONETARY, LC_COLLATE */
    "displayLang",	/* LC_MESSAGES */
    "inputLang",	/* will use LC_CTYPE */
    "numericFormat",		/* LC_NUMERIC */
    "timeFormat"	/* LC_TIME */
};

static char *lang_wsp_classes[NUM_LANG_WSP_NAMES] = {
    "BasicLocale",
    "DisplayLang",
    "InputLang",
    "NumericFormat",
    "TimeFormat"
};

static String
_OlSetLocaleFromWSP(Display * dpy,
		    String    language)
{
    String		ret_val   = language;
    String		rm_string = XResourceManagerString(dpy);

    if (rm_string != (String) NULL) {
	register int	i;
	XrmName		name_list[3];
	XrmName		class_list[3];
	XrmRepresentation 	type;
	XrmValue		values[NUM_LANG_WSP_NAMES];
	Boolean		found[NUM_LANG_WSP_NAMES];
	XrmDatabase		rdb = XrmGetStringDatabase(rm_string);

	name_list[0] = class_list[0] = XrmPermStringToQuark("OpenWindows");
	name_list[2] = class_list[2] = NULLQUARK;

	name_list[1]  = XrmPermStringToQuark(lang_wsp_names[0]);
	class_list[1] = XrmPermStringToQuark(lang_wsp_classes[0]);

	found[0]      = XrmQGetResource(rdb, name_list, class_list,
					&type, &values[0]);
	if (found[0]) {
	    char *	ol_ctype = (String) values[0].addr;
	    Boolean	others	 = False;

	    for (i=1; i < NUM_LANG_WSP_NAMES; i++) {
		name_list[1]  = XrmPermStringToQuark(lang_wsp_names[i]);
		class_list[1] = XrmPermStringToQuark(lang_wsp_classes[i]);
		others |= (found[i] = XrmQGetResource(rdb, name_list,
					    class_list, &type, &values[i]));
	    }
	    if (others) {
		char * ol_input_lang;
		char * ol_messages = (found[1]) ? (String) values[1].addr:
							ol_ctype;
		char * ol_numeric  = (found[3]) ? (String) values[3].addr:
							ol_ctype;
		char * ol_time     = (found[4]) ? (String) values[4].addr:
							ol_ctype;

		if (found[2]) {
		    ol_input_lang = (String) values[2].addr;
		    if (strcmp(ol_ctype, ol_input_lang))
		      OlWarning(dgettext(OlMsgsDomain,
		       "*inputLang is not same as derived LC_CTYPE.\n"));
		}

		setlocale(LC_CTYPE,	ol_ctype);
		setlocale(LC_COLLATE,	ol_ctype);
		setlocale(LC_TIME,	ol_time);
		setlocale(LC_NUMERIC,	ol_numeric);
		setlocale(LC_MONETARY,	ol_ctype);
		setlocale(LC_MESSAGES,	ol_messages);
	    } else
		setlocale(LC_ALL,	ol_ctype);
	} else
		setlocale(LC_ALL, "");
    }
    return(setlocale(LC_ALL, NULL));
}

/*************************************************************************
 *
 *      _OlDefaultLanguageProc
 *
 ****************************procedure*header*****************************/
 
/*FTNPROTOB*/
static String
_OlDefaultLanguageProc(Display *    dpy,
		       String	    language,
		       XtPointer    closure)
{
    static Boolean	first_time   = (Boolean) True;
    XtLanguageProc	xt_lang_proc = (XtLanguageProc) closure;
    String		retval      = (String) NULL;
    char		*dirname;
    char		*domainpath;

    if (first_time) {
	first_time = False;
       /*
	* If xnlLanguage is found by Intrinsics, the value
	* of language will not be an empty string.
	* It is then and only then will the Workspace props for
	* locale will be checked.
	* So, precedence order (decreasing) now is:
	*	-xnlLanguage,
	*	*xnlLanguage,
	*	workspace props,
	*	LANG variable
	*/
	if (language == (String) NULL || *language == '\0') {
	    retval = _OlSetLocaleFromWSP(dpy, language);
	    if (!XSupportsLocale()) {
		OlWarning(dgettext(OlMsgsDomain,
		    "Workspace locale not supported by Xlib, set to C"));
		setlocale(LC_ALL, "C");
	    }
	    if (!XSetLocaleModifiers(""))
		OlWarning(dgettext(OlMsgsDomain,
		    "X locale modifiers not supported, using default"));
	} else
	    retval = xt_lang_proc(dpy, language, closure);

        if((dirname = getenv("OPENWINHOME")) == NULL){
			OlWarning(dgettext(OlMsgsDomain, 
				"OPENWINHOME not set: Cannot get localized messages ."));
			return retval;
        }
		if (domainpath = malloc(BUFSIZ)) {
			(void)snprintf(domainpath, BUFSIZ, "%s/lib/locale", dirname);
			(void)bindtextdomain(OlMsgsDomain,domainpath);
		}

    } else {   
       /*
	* In R5 intrinsics, language proc gets called during
	* XtDisplayInitialize, and is parameterised by xnlLanguage
	* resource of display.  If setlocale is called by language
	* proc (which is the intention of language procs) then 
	* application's locale could change anytime a display is
	* initialized, and is not predictable until damage is done.
	*   Solution:
	* First invocation: call intrinsics language proc to set locale
	* Subsequent invocations: return current locale setting
	*   The app's locale for its lifetime is the locale of the
	* first display it initializes (unless app calls setlocale
	* later ...but that's naughty!)
	*/

	retval = setlocale(LC_ALL, NULL);
    }
    return retval;
}
 
/*******************************************************************************
 *
 * OlToolkitInitialize() -  a convenience routine used by applications
 * to initialize the Xt(TM) and OLIT(TM) toolkits
 *
 *******************************************************************************/
/*FTNPROTOB*/
void
OlToolkitInitialize(XtPointer param)
/*FTNPROTOE*/
{
	XtLanguageProc		default_proc = NULL;

	_OlLoadVendorShell();
 
	/*
	 * This installs intrinsics' default langauge procedure,
	 * then gets a handle to it, and sets this up to be passed
	 * to OLIT's language procedure _OlDefaultLanguageProc so it
	 * can inherit intrinsics behaviour
	 */
        (void) XtSetLanguageProc(NULL, NULL, NULL);
        default_proc = XtSetLanguageProc(NULL, NULL, NULL);
        XtSetLanguageProc(NULL, _OlDefaultLanguageProc,
                        (XtPointer)default_proc);
}


/*************************************************************************
 *	_OlOnXtGrabList()
 *
 *	naughtiness abounds .... not really relevant to RootShell
 *	but in here becasue we include IntrinsicI.h!
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
_OlWidgetOnXtGrabList (Widget widget)
      		       
/*FTNPROTOE*/
{
	extern Boolean _XtOnGrabList ( Widget, XtGrabList );

	XtPerDisplay pd;
	XtPerDisplayInput pdi;
	XtGrabList  grabList;

	pd = _XtGetPerDisplay(XtDisplay(widget));
	pdi = _XtGetPerDisplayInput(XtDisplay(widget));
	grabList = *_XtGetGrabList(pdi);

	return _XtOnGrabList(widget, grabList);
}

/**************************************************************************
 *
 *	_OlRegisterShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
_OlRegisterShell (Widget w)
       	   
/*FTNPROTOE*/
{
	if (XtIsSubclass(w, shellWidgetClass)) {
		_OlAddShellToDisplayList(w);
		_OlAddShellToScreenList(w);
	}
}

/**************************************************************************
 *
 *	_OlUnregisterShell
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
_OlUnregisterShell (Widget w)
       	   
/*FTNPROTOE*/
{
	if (XtIsSubclass(w, shellWidgetClass)) {
		_OlDeleteShellFromDisplayList(w);
		_OlDeleteShellFromScreenList(w);
	}
}

/**************************************************************************
 *
 *	_OlGetDisplayShellOfScreen
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
_OlGetDisplayShellOfScreen (Screen *scr)
        		    
/*FTNPROTOE*/
{
	scr = ScreenOfDisplay(DisplayOfScreen(scr), 0);
	return _OlRootShellOfScreen(scr, displayShellWidgetClass);
}

/**************************************************************************
 *
 *	_OlGetDisplayShellOfWidget
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
_OlGetDisplayShellOfWidget (Widget w)
      		  
/*FTNPROTOE*/
{
	return _OlRootShellOfScreen(ScreenOfDisplay(XtDisplayOfObject(w), 0),
				     displayShellWidgetClass);
}

/**************************************************************************
 *
 *	_OlGetScreenShellOfWidget
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
_OlGetScreenShellOfWidget (Widget w)
      		  
/*FTNPROTOE*/
{
	return _OlRootShellOfScreen(XtScreenOfObject(w), screenShellWidgetClass);
}

/**************************************************************************
 *
 *	_OlGetScreenShellOfScreen
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Widget
_OlGetScreenShellOfScreen (Screen *scr)
        		    
/*FTNPROTOE*/
{
	return _OlRootShellOfScreen(scr, screenShellWidgetClass);
}

/**************************************************************************
 *
 *	_OlGetAppAtributesRef
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
static void
_LoadAppAttributes (_OlAppAttributes *app_attrs, DisplayShellWidget dsw,
		    ScreenShellWidget ssw)
/*FTNPROTOE*/
{
	if (app_attrs == (_OlAppAttributes *)NULL) return;

	if (dsw != (DisplayShellWidget)NULL) {
#define	Copy(field)	app_attrs->field = dsw->display.field
		Copy(multi_click_timeout);
		Copy(beep_volume);
		Copy(beep);
		Copy(beep_duration);
		Copy(select_does_preview);
		Copy(grab_pointer);
		Copy(grab_server);
		Copy(menu_accelerators);
		Copy(mouseless);
		Copy(multi_object_count);
		Copy(dont_care);
		Copy(mnemonic_modifiers);
		Copy(show_mnemonics);
		Copy(show_accelerators);
		Copy(shift_name);
		Copy(lock_name);
		Copy(control_name);
		Copy(mod1_name);
		Copy(mod2_name);
		Copy(mod3_name);
		Copy(mod4_name);
		Copy(mod5_name);

		Copy(help_model);
		Copy(mouse_status);

#ifdef  sun
		Copy(use_short_OlWinAttr);
#endif
#undef	Copy
	}
	if (ssw != (ScreenShellWidget)NULL) {
#define	Copy(field)	app_attrs->field = ssw->screen.field
		Copy(mouse_damping_factor);
		Copy(three_d); 
		Copy(scale_map_file);
		Copy(drag_right_distance);
		Copy(menu_mark_region);
#undef	Copy
	}
}

/*FTNPROTOB*/
_OlAppAttributes *
_OlGetAppAttributesRef (Widget widget)
       		       
/*FTNPROTOE*/
{
	_OlAppAttributes	*app_attrs = (_OlAppAttributes *)NULL;
	DisplayShellWidget	dsw;
	ScreenShellWidget	ssw;
	Screen			*screen = XtScreenOfObject(widget);

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(screen);
	ssw = (ScreenShellWidget) _OlGetScreenShellOfScreen(screen);

	if (ssw != (ScreenShellWidget)NULL && dsw != (DisplayShellWidget)NULL) {
		app_attrs = ssw->screen.app_attrs;
		if (app_attrs == (_OlAppAttributes *)NULL ||
		    ssw->screen.app_attrs_need_update) {
			UpdatePrevAppAttrs(ssw);
			app_attrs = ssw->screen.app_attrs =
				(_OlAppAttributes *)
					XtCalloc(1, sizeof(_OlAppAttributes));
			_LoadAppAttributes(app_attrs, dsw, ssw);
			ssw->screen.app_attrs_need_update = (Boolean)False;
		}

	}

	return (app_attrs);
}

/**************************************************************************
 *
 *	_OlGetPrevAppAttributesRef
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
_OlAppAttributes *
_OlGetPrevAppAttributesRef (Widget widget)
       		       
/*FTNPROTOE*/
{
	_OlAppAttributes	*app_attrs = (_OlAppAttributes *)NULL;
	ScreenShellWidget	ssw;
	Screen			*screen = XtScreenOfObject(widget);

	ssw = (ScreenShellWidget) _OlGetScreenShellOfScreen(screen);

	if (ssw != (ScreenShellWidget)NULL) {
		app_attrs = ssw->screen.prev_app_attrs;
		if (app_attrs == (_OlAppAttributes *)NULL)
			app_attrs = _OlGetAppAttributesRef(widget);
	}
	return (app_attrs);
}

/**************************************************************************
 *
 *	OlQueryMnemonicDisplay
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
OlDefine
OlQueryMnemonicDisplay (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.show_mnemonics);
}

/**************************************************************************
 *
 *	OlOKToGrabServer
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
OlOKToGrabServer (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.grab_server);
}

/**************************************************************************
 *
 *	OlOKToGrabPointer
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
OlOKToGrabPointer (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.grab_pointer);
}

/**************************************************************************
 *
 *	OlGetBeep
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
OlDefine
OlGetBeep (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.beep);
}

/**************************************************************************
 *
 *	OlGetBeepVolume
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
int
OlGetBeepVolume (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.beep_volume);
}

/**************************************************************************
 *
 *	OlGetBeepDuration
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
int
OlGetBeepDuration (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.beep_duration);
}

/**************************************************************************
 *
 *	OlQueryAcceleratorDisplay
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
OlDefine
OlQueryAcceleratorDisplay (Widget w)
       		  
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

	dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	return (dsw->display.show_accelerators);
}

/**************************************************************************
 *
 *	_OlCtrlAltMetaKey
 *
 ****************************procedure*header*****************************/

Boolean
_OlCtrlAltMetaKey (Display *dpy)
{
	DisplayShellWidget	dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(DefaultScreenOfDisplay(dpy));

       	return (dsw->display.ctrl_alt_meta_key); 
}

/**************************************************************************
 *
 *	_OlUseShortOLWinAttr
 *
 ****************************procedure*header*****************************/

#ifdef  sun
/*FTNPROTOB*/
Boolean
_OlUseShortOLWinAttr (Display *dpy)
          	     
/*FTNPROTOE*/
{
	DisplayShellWidget	dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(DefaultScreenOfDisplay(dpy));

        return (dsw->display.use_short_OlWinAttr);
}
#endif

/**************************************************************************
 *
 *	_OlSelectDoesPreview
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
_OlSelectDoesPreview (Widget w)
       	    
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

        return (dsw->display.select_does_preview);
}

/**************************************************************************
 *
 *	_OlMenuAccelerators
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
_OlMenuAccelerators (Widget w)
       	    
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

        return (dsw->display.menu_accelerators);
}

/**************************************************************************
 *
 *	_OlMouseless
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Boolean
_OlMouseless (Widget w)
       	    
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

        return (dsw->display.mouseless);
}

/**************************************************************************
 *
 *      _OlInputFocusFeedback
 *
 ****************************procedure*header*****************************/
 
/*FTNPROTOB*/
OlDefine
_OlInputFocusFeedback(Widget w)
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;
 
        dsw = (DisplayShellWidget)
                      _OlGetDisplayShellOfScreen(XtScreenOfObject(w));
 
        return (dsw->display.input_focus_feedback);
}

/**************************************************************************
 *
 *	_OlGetMouseDampingFactor
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Cardinal
_OlGetMouseDampingFactor (Widget w)
       	    
/*FTNPROTOE*/
{
        ScreenShellWidget      ssw;

        ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(XtScreenOfObject(w));

        return (ssw->screen.mouse_damping_factor);
}
/**************************************************************************
 *
 *	_OlGetMultiClickTimeout
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Cardinal
_OlGetMultiClickTimeout (Widget w)
       	    
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

        return (dsw->display.multi_click_timeout);
}
/**************************************************************************
 *
 *	_OlGetMultiObjectCount
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
Cardinal
_OlGetMultiObjectCount (Widget w)
       	    
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(XtScreenOfObject(w));

        return (dsw->display.multi_object_count);
}


/**************************************************************************
 *
 *	_OlGetOlDefaultFont()
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
String
_OlGetOlDefaultFont (Screen *screen)
         	         
/*FTNPROTOE*/
{
        DisplayShellWidget      dsw;

        dsw = (DisplayShellWidget)_OlGetDisplayShellOfScreen(screen);

        return (dsw->display.ol_default_font);
}

/**************************************************************************
 *
 *	_OlGetScaleMap
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
char *
_OlGetScaleMap (Screen *screen)
         	         
/*FTNPROTOE*/
{
        ScreenShellWidget      ssw;

        ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

        return (ssw->screen.scale_map_file);
}

/**************************************************************************
 *
 *      _OlGetScale
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
int
_OlGetScale (Screen *screen)
        	         
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

	return (ssw->screen.scale);
}

/**************************************************************************
 *
 *	OlGetApplicationValues
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlGetApplicationValues (Widget w, ArgList args, Cardinal num_args)
      		  
       		     
        		         
/*FTNPROTOE*/
{
	Widget			ssw,
				dsw;
	RootShellWidgetClass	rswc;

	if (w == (Widget)NULL) return;

	ssw = _OlGetScreenShellOfScreen(XtScreenOfObject(w));
	dsw = _OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	rswc = (RootShellWidgetClass)(dsw->core.widget_class);
	XtGetSubvalues((XtPointer)dsw,
		       rswc->root_shell.appl_res,
		       rswc->root_shell.num_appl_res,
		       args, num_args);

	rswc = (RootShellWidgetClass)(ssw->core.widget_class);
	XtGetSubvalues((XtPointer)ssw,
		       rswc->root_shell.appl_res,
		       rswc->root_shell.num_appl_res,
		       args, num_args);

}

/**************************************************************************
 *
 *	OlSetApplicationValues
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlSetApplicationValues (Widget w, ArgList args, Cardinal num_args)
      		  
       		     
        		         
/*FTNPROTOE*/
{
	Widget			ssw,
				dsw;
	RootShellWidgetClass	rswc;

	if (w == (Widget)NULL) return;

	ssw = _OlGetScreenShellOfScreen(XtScreenOfObject(w));
	dsw = _OlGetDisplayShellOfScreen(XtScreenOfObject(w));

	rswc = (RootShellWidgetClass)(dsw->core.widget_class);
	XtSetSubvalues((XtPointer)dsw,
		       rswc->root_shell.appl_res,
		       rswc->root_shell.num_appl_res,
		       args, num_args);

	rswc = (RootShellWidgetClass)(ssw->core.widget_class);
	XtSetSubvalues((XtPointer)ssw,
		       rswc->root_shell.appl_res,
		       rswc->root_shell.num_appl_res,
		       args, num_args);

}

/**************************************************************************
 *
 *	_OlInitAttributes
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
_OlInitAttributes (Widget w)
      		  
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;
	Screen			*screen = XtScreenOfObject(w);

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);
        OlgSetStyle3D (XtScreen(ssw), ssw->screen.three_d);
}

/**************************************************************************
 *
 *	_OlCreateImVSInfo
 *
 *             create the im related record for the vendor shell in
 *             its display shell
 *
 ****************************procedure*header*****************************/
void
_OlCreateImVSInfo(Widget vw)
{
    if (XtIsVendorShell(vw)) {
	ImVSInfo *          temp;
	DisplayShellWidget	dw =
	    (DisplayShellWidget) _OlGetDisplayShellOfWidget(vw);

	/* create and initialize an ImVSInfo record */
	temp = (ImVSInfo *) XtMalloc(sizeof(ImVSInfo));
	temp->vw                        = vw;
	temp->default_im_id             = (struct _InputMethodRec *) NULL;
	temp->default_im_open_attempted = FALSE;
	temp->ic_list                   = (struct _InputContextRec *) NULL;
	temp->num_ics                   = 0;

	/* Attach temp to list of ImVSInfo recs in DisplayShell */
	temp->next                  = dw->display.im_vs_info_list;
	dw->display.im_vs_info_list = temp;
    }
}

/**************************************************************************
 *
 *	_OlDestroyImVSInfo
 *
 *             destroys the im related record for the vendor shell in
 *             its display shell
 *
 ****************************procedure*header*****************************/

void
_OlDestroyImVSInfo(Widget vw)
{
    if (XtIsVendorShell(vw)) {
	DisplayShellWidget           dw     =
	    (DisplayShellWidget) _OlGetDisplayShellOfWidget(vw);
	DisplayShellPart *           idp    = &(dw->display);
	register ImVSInfo *          trav;
	register ImVSInfo *          prev   = (ImVSInfo *) NULL;
	
	for (trav = idp->im_vs_info_list;
	     trav->vw != vw && trav != (ImVSInfo *) NULL;
	     prev = trav, trav = trav->next)
	    ;
	if (trav == (ImVSInfo *) NULL)
	    OlError(dgettext(OlMsgsDomain,
		"OlIM: Internal error: ImVsInfoList is empty while destroying."));
	
	if (prev == (ImVSInfo *) NULL) {
	    /* At the head of the list */
	    idp->im_vs_info_list = trav->next;
	} else {
	    /* Somewhere in the middle of the list */
	    prev->next = trav->next;
	}
	XtFree((XtPointer) trav);
    }
}

/**************************************************************************
 *
 *	_OlGetImVSInfo
 *
 *             get the im related record for the vendor shell from
 *             its display shell
 *
 ****************************procedure*header*****************************/

ImVSInfo *
_OlGetImVSInfo(Widget w)
{
    Widget                       vw     = _OlFindVendorShell(w, TRUE);
    DisplayShellWidget           dw     =
	(DisplayShellWidget) _OlGetDisplayShellOfWidget(vw);
    DisplayShellPart *           idp    = &(dw->display);
    register ImVSInfo *          ivtrav;

    for (ivtrav = idp->im_vs_info_list;
	 ivtrav != (ImVSInfo *) NULL && ivtrav->vw != vw;
	 ivtrav = ivtrav->next)
	;
    if (ivtrav == (ImVSInfo *) NULL)
	OlWarning(dgettext(OlMsgsDomain,
		  "IM Management: Cannot find IM record for vendor shell in display shell."));
    else
	return(ivtrav);
}


/**************************************************************************
 *
 *	OlAddDynamicScreenCB
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlAddDynamicScreenCB (Screen *screen, OlDynamicScreenCallback proc, XtPointer closure)
        				       
                       		     
         			        
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;
	int			i;

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

	if (ssw == (ScreenShellWidget)NULL) return;

	for (i = 0; i < ssw->screen.num_dyn_cbs; i++) {
		if (ssw->screen.dyn_cbs[i].proc == proc &&
		    ssw->screen.dyn_cbs[i].closure == closure)
			return;	/* no duplicates */
	}

	if (VECTORFULL(ssw->screen.num_dyn_cbs, VECTOR_INCR) ||
	    ssw->screen.num_dyn_cbs == 0) {

		ssw->screen.dyn_cbs = (OlDynamicScreenCallbacks)
					XtRealloc((char *)ssw->screen.dyn_cbs,
						  (ssw->screen.num_dyn_cbs +
					           VECTOR_INCR) *
						   sizeof(OlDSCBRec));
	}

	ssw->screen.dyn_cbs[ssw->screen.num_dyn_cbs].proc      = proc;
	ssw->screen.dyn_cbs[ssw->screen.num_dyn_cbs++].closure = closure;
}

/**************************************************************************
 *
 *	OlRemoveDynamicScreenCB
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlRemoveDynamicScreenCB (Screen *screen, OlDynamicScreenCallback proc, XtPointer closure)
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;
	int			i;

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

	if (ssw == (ScreenShellWidget)NULL ||
	    ssw->screen.num_dyn_cbs == (Cardinal)0)
		return;

	for (i = 0; i < ssw->screen.num_dyn_cbs; i++)
		if (proc    == ssw->screen.dyn_cbs[i].proc &&
		    closure == ssw->screen.dyn_cbs[i].closure) break;

	if (i == ssw->screen.num_dyn_cbs) return; /* not found */

	ssw->screen.num_dyn_cbs--;

	if (i < ssw->screen.num_dyn_cbs) { /* not at end */
		memmove((char *)(ssw->screen.dyn_cbs + i),
			(char *)(ssw->screen.dyn_cbs + i + 1),
			(ssw->screen.num_dyn_cbs - i) * sizeof(OlDSCBRec));
	}

	if (ssw->screen.num_dyn_cbs == 0) {
		XtFree((char *)ssw->screen.dyn_cbs);
		ssw->screen.dyn_cbs = (OlDynamicScreenCallbacks)NULL;
	} else {
	/* 
	 * If the list has shrunk to a multiple of VECTOR_INCR then shrink
	 * the list back to that multiple
	 */
	    if (ssw->screen.num_dyn_cbs % VECTOR_INCR == 0)
		ssw->screen.dyn_cbs = (OlDynamicScreenCallbacks)
		    XtRealloc((char *)ssw->screen.dyn_cbs,
			      ssw->screen.num_dyn_cbs * sizeof(OlDSCBRec));
	}
}

/**************************************************************************
 *
 *	OlCallDynamicScreenCBs
 *
 ****************************procedure*header*****************************/

/*FTNPROTOB*/
void
OlCallDynamicScreenCBs (Screen *screen)
        		       
/*FTNPROTOE*/
{
	ScreenShellWidget	ssw;
	int			i;

	ssw = (ScreenShellWidget)_OlGetScreenShellOfScreen(screen);

	if (ssw == (ScreenShellWidget)NULL) return;

	for (i = 0; i < ssw->screen.num_dyn_cbs; i++)
		(*ssw->screen.dyn_cbs[i].proc)(screen,
					       ssw->screen.dyn_cbs[i].closure);
}

static void
_OlGetMetaKey(Widget w, int offset, XrmValue *value)
{
	Display *dpy = XtDisplay(w);
	static Boolean no_meta_key = False;

	if ((XKeysymToKeycode(dpy, XK_Meta_L) == 0) &&
		(XKeysymToKeycode(dpy, XK_Meta_R) == 0))
	/* META key not present in our keyboard ... Hence MUST use Ctrl-Alt to simulate it*/
			no_meta_key = True;

	value->addr = (caddr_t)&no_meta_key;
}

