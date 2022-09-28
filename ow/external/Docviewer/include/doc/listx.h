#ifndef	__LISTX_H__
#define	__LISTX_H__

#ident "@(#)listx.h	1.4 11/15/96 Copyright 1992 Sun Microsystems, Inc."


#include <doc/list.h>


// Parameterized list class with "auto delete" feature.
//
template <class T>
class	LISTX : public LIST<T> {

    public:

	// LISTX constructor, destructor.
	//
	LISTX()				{ ; }
	~LISTX()			{ Clear(); }

	// Delete "n"th item from this LISTX,
	// and perform a "delete()" on that item.
	// Assumes list contains at least n-1 items.
	//
	void	Delete(int n);

	// Remove all items from list, and "delete()" those items.
	//
	void	Clear();
};

#endif	__LISTX_H__
