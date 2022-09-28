#ident	"@(#)environ.h	1.8	93/06/08 SMI"

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc.
 */

/*
 *      Sun design patents pending in the U.S. and foreign countries. See
 *      LEGAL_NOTICE file for terms of the license.
 */

#ifndef _OLWM_ENVIRON_H
#define _OLWM_ENVIRON_H

extern	char	**MakeEnviron(	Display		*dpy,
				int		screen,
				OLLCItem	*lc);

extern	char	**UpdateEnviron(Display		*dpy,
				int		screen,
				char		**environ,
				OLLCItem	*lc);

#endif /* _OLWM_ENVIRON_H */
