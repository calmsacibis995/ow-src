#ifndef _DPS_DPSEXTERNS_H
#define _DPS_DPSEXTERNS_H
#include <DPS/dpsclient.h>
#include <DPS/dpsops.h>

#ident "@(#)dpsexterns.h	1.3 12/20/93 Copyright 1993 Sun Microsystems, Inc."

typedef void (*XDPSStatusProc)();
#ifdef _NO_PROTO /* extern declarations */

extern DPSContext XDPSCreateContext( /* Display *dpy, Drawable drawable,
				  	GC gc, int x, int y,
					unsigned int eventmask,	
					XStandardColormap *grayramp,
					XStandardColormap *ccube,
					int actual,	
					DPSTextProc testProc,
					DPSErrorProc errorProc,
					DPSSpace space */ );

extern void DPSDefaultTextBackstop( /* DPSContext ctxt, char* buf,
				       long unsigned int count */ );

extern Status XDPSCreateStandardColormaps();
extern void XDPSSetStatusMask();

extern XDPSStatusProc XDPSRegisterStatusProc();
#else

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern DPSContext XDPSCreateContext(Display *dpy, Drawable drawable,
				    GC gc, int x, int y,
                                    unsigned int eventmask,
                                    XStandardColormap *grayramp,
                                    XStandardColormap *ccube,
                                    int actual,
				    DPSTextProc testProc,
				    DPSErrorProc errorProc,
				    DPSSpace space );

extern Status XDPSCreateStandardColormaps(
    Display *dpy,
    Drawable drawable,
    Visual *visual,
    int reds, int greens, int blues, int grays,
    XStandardColormap *colorCube, XStandardColormap *grayRamp,
    Bool retain);

extern void DPSDefaultTextBackstop( DPSContext ctxt, char* buf,
				    long unsigned int count );

extern XDPSStatusProc XDPSRegisterStatusProc(DPSContext ctxt,
					     XDPSStatusProc proc);

extern void XDPSSetStatusMask(DPSContext ctxt,
			      unsigned long enableMask,
			      unsigned long disableMask,
			      unsigned long nextMask);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* extern declarations */



/* 
** The following definitions are from <DPS/XDPS.h>
** This file cannot be included because of redefinition of BOOL --etc.
*/

#define PSSTATUSERROR		0
#define PSRUNNING		1
#define PSNEEDSINPUT		2
#define PSZOMBIE		3
#define PSFROZEN		4
int frozen = 0;


unsigned long enable_mask, disable_mask, next_mask;

#define PSRUNNINGMASK           0x0001
#define PSNEEDSINPUTMASK        0x0002
#define PSZOMBIEMASK            0x0004
#define PSFROZENMASK            0x0008

#undef dps_err_invalidAccess
#define dps_err_invalidAccess	2000
#define dps_err_encodingCheck	2001
#define dps_err_closedDisplay	2002
#define dps_err_deadContext	2003
#define dps_err_warning		2004
#define dps_err_fatal		2005
#define dps_err_recursiveWait	2006

#endif _DPS_DPSEXTERNS_H
