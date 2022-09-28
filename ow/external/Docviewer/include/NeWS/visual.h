/*
 * #ident "@(#)visual.h	1.2 06/11/93 NEWS SMI"
 */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _visual_h
#define _visual_h

#include <cscript/cs_visual.h>


/*
** Container object for NeWS visual.
*/


/*
** Visual container type.
*/

struct visual_data {
    ENTITY_FIELDS				/* header and ref info */
    VISUAL		pVisual;		/* ptr to actual visual info */
};

typedef struct visual_data *Visual_datap;

#define visual_of(ref)		((Visual_datap)entity_addr(ref))
#define visualptr_of(ref)	(visual_of(ref)->pVisual)


#endif /* _visual_h */
