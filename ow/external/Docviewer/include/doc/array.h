#ifndef	__ARRAY_H__
#define	__ARRAY_H__

#ident "@(#)array.h	1.5 12/20/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>


// Parameterized dynamic array.
// this array grows automatically as elements past the end of the array
// are accessed.
//
template <class T>
class	ARRAY {

    private:

	int		count;		// number of elements in this array


    protected:

	T		*array;		// the array itself
	OBJECT_STATE	objstate;	// current state of this object

	// Is array element 'n' within the current array bounds?
	//
	BOOL		OutOfBounds(int n);

	// The element being referenced is outside the current
	// array bounds - we need to grow the array.
	//
	void		Grow(int n);


    public:

	// Array constructor, destructor.
	//
	ARRAY(void);
	~ARRAY(void);

	// Access "n"th element of this array.
	//
	T		&operator [] (int n);
	void		DestroyArray ()
	{
		if (array != NULL)
		   	delete array;
	}
};

#endif	__ARRAY_H__
