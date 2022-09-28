#ifndef	_XOL_FC_IMPL_I_H
#define	_XOL_FC_IMPL_I_H

#pragma	ident	"@(#)FCImplI.h	1.5	93/02/24 include/Xol SMI"	/* OLIT */

#include <Xol/FontDB.h>

#define	NUM_TYPEFACE_FIELDS	4
#define NUM_STYLE_FIELDS	2
#define NUM_SIZE_FIELDS		1

#define STYLE_FIELD_SIZE	256
#define TYPEFACE_FIELD_SIZE	256
#define SIZE_FIELD_SIZE		256

typedef struct _OlFontComponentRec {
    FontField   fields[FC_XLFD_LAST];
} OlFontComponent; 

typedef struct _OlFontDescRec {
    String              font_name;
    OlFont              font;
    OlFontComponent *   components;
    Cardinal            num_components;
} OlFontDesc;

extern void	InitializeFC(FontChooserWidget	fcw,
			     String		initial_font_name,
			     XtCallbackProc	apply_CB,
			     XtCallbackProc	revert_CB,
			     XtCallbackProc	cancel_CB);

extern void	CleanupFC(FontChooserWidget	fcw);

extern void	RevertToInitialFont(FontChooserWidget	fcw);

extern void	PreviewFont(FontChooserWidget	fcw);

#endif	/* _XOL_FC_IMPL_I_H */

