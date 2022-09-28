/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)at_sel_atoms.c	1.1	91/07/10 SMI"

#include <stdio.h>
#include <string.h>

#include <xview/xview.h>

#include "atool_panel.h"
#include "atool_sel_impl.h"

/* takes are atom list and fills in all the Atom's by interning 
 * on the names.
 */
init_atoms(Xv_server srvr, register AtomListEntry *alist)
{
	for (;alist && alist->index; alist++)
	    alist->atom = (Atom)xv_get(srvr, SERVER_ATOM, alist->name);
}

/* looks up an atom by name in our table */
AtomListEntry *atom_lookup_name(register AtomListEntry *alist, char *name)
{
	for ( ; alist && alist->index; alist++) {
		if (!strcmp(alist->name, name))
		    return alist;
	}
	return NULL;
}

/* looks up an atom by Atom id in our table */
AtomListEntry *atom_lookup_atom(register AtomListEntry *alist, Atom atom)
{
	for ( ; alist && alist->index; alist++) {
		if (alist->atom == atom)
		    return alist;
	}
	return NULL;
}

/* retuns the atom for a given index  */
Atom atom_get_atom(register AtomListEntry *alist, AtomIndex ind)
{
	for ( ; alist && alist->index; alist++) {
		if (alist->index == ind)
		    return alist->atom;
	}
	return NULL;
}

/* returns the atom name for a given index */
char *atom_get_name(register AtomListEntry *alist, AtomIndex ind)
{
	for ( ; alist && alist->index; alist++) {
		if (alist->index == ind)
		    return alist->name;
	}
	return NULL;
}

/* return True if the given atom is in the given list of atoms.  */
is_atom_in_list(register Atom *alist, Atom atom)
{
	for(;alist && *alist; alist++)
	    if (*alist == atom)
		return (True);
	return (False);
}

