#ifndef	_DVLOCALE_H
#define	_DVLOCALE_H

#ident "@(#)dvlocale.h	1.3 93/02/15 Copyright 1993 Sun Microsystems, Inc."


#include <doc/common.h>
#include <locale.h>


// Text domain name for localization of error messages, etc.
// Do not change this name - it is registered with the text domain name
// registry (textdomain@Sun.COM).
//
#define	LIBDV_DOMAIN_NAME	"SUNW_DESKSET_ANSWERBOOK_LIBDV"

// libdv-specific version of "dgettext()".
// Note that this must be implemented via "#define" for the benefit
// of "xgettext()".  Kinda stupid, eh?
//
#define	DGetText(msgid)		dgettext(LIBDV_DOMAIN_NAME, msgid)

#endif	_DVLOCALE_H
