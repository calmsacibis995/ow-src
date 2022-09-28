#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)xv_slots.h 1.2 93/07/02";
#endif
#endif

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL NOTICE
 *      file for terms of the license.
 */

#ifndef slotsh
#define slotsh 0

unsigned int 	xv_slots_bytes	(unsigned int num_elements);
unsigned int 	xv_slots_init	(void * ptr, unsigned int size);
unsigned int * 	xv_slots_get	(void * ptr, unsigned int key);
unsigned int * 	xv_slots_find	(void * ptr, unsigned int key);
unsigned int 	xv_slots_delete	(void * ptr, unsigned int key);

#endif slotsh


