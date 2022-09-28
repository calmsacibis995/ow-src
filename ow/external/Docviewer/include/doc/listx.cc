#ident "@(#)listx.cc	1.3 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/listx.h>


// Delete "n"th item from this LISTX.
// Assumes list contains at least n-1 items.
//
template <class T>
void
LISTX<T>::Delete(int n)
{
	assert(n >= 0  &&  n < Count());
//XXX	assert(objstate.IsReady());
	DbgFunc("LISTX<T>::Delete: " << n << endl);

	// Delete the specified element,
	// then invoke our parent's method to finish up the job.
	//
	delete (*this)[n];
	LIST<T>::Delete(n);
}

// Remove all items from list.
//
template <class T>
void
LISTX<T>::Clear()
{
	int	i;

//XXX	assert(objstate.IsReady());
	DbgFunc("LISTX<T>::Clear" << endl);

	// Delete all elements in list,
	// then invoke our parent's method to finish up the job.
	//
	for (i = 0; i < Count(); i++) {
		delete (*this)[i];
	}

	LIST<T>::Clear();
}
