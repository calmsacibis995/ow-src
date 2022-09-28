#ident "@(#)array.cc	1.4 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/array.h>


template <class T>
ARRAY<T>::ARRAY(void)
{
	DbgFunc("ARRAY<T>::ARRAY" << endl);

	array = NULL;
	count = 0;

	objstate.MarkReady();
}

template <class T>
ARRAY<T>::~ARRAY(void)
{
	DbgFunc("ARRAY<T>::~ARRAY" << endl);

	if (array != NULL)
		delete [] array;
}

// Access "n"th element of this array.
//
template <class T>
T &
ARRAY<T>::operator [] (int n)
{
	assert(objstate.IsReady());
	DbgFunc("ARRAY<T>::operator[" << n << "]" << endl);

	if (OutOfBounds(n))
		Grow(n);

	return(array[n]);
}

// Is array element 'n' within the current array bounds?
//
template <class T>
BOOL
ARRAY<T>::OutOfBounds(int n)
{
	assert(objstate.IsReady());
	assert(n >= 0);
	DbgFunc("ARRAY<T>::OutOfBounds" << endl);

	return(n >= count ? BOOL_TRUE : BOOL_FALSE);
}

// The element being referenced is outside the current
// array bounds - we need to grow the array.
//
template <class T>
void
ARRAY<T>::Grow(int n)
{
	T	*new_array;
	int	new_count;
	int	i;


	assert(objstate.IsReady());
	assert(n >= 0);
	assert(n < count + 100);
	DbgFunc("ARRAY<T>::Grow: " << n << endl);


	// Grow the array in in powers of 2, starting with 8 elements.
	//
	new_count = (count == 0  ?  8  :  count);

	while (n >= new_count)
		new_count *= 2;


	// Allocate elements for new array.
	//
	new_array = new T[new_count];


	// Copy the existing array elements into the new array.
	//
	for (i = 0; i < count; i++)
		new_array[i] = array[i];


	// Clean up after ourselves.
	//
	if (array != NULL)
		delete [] array;

	array = new_array;
	count = new_count;
}
