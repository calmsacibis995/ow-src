#ident "@(#)list.cc	1.5 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/list.h>


// Append a new item to this LIST.
//
template <class T>
void
LIST<T>::Add(const T &item)
{
	assert(objstate.IsReady());
	DbgFunc("LIST<T>::Add" << endl);

	// Grow array if necessary.
	//
	if (OutOfBounds(end))
		Grow(end);

	// Add item to end of array.
	//
	array[end++] = item;
}

// Insert a new item to the "n"th slot in this LIST.
// Assumes list contains at least n items.
//
template <class T>
void
LIST<T>::Insert(const T &item, int n)
{
	assert(objstate.IsReady());
	assert(n >= 0  &&  n <= end);
	DbgFunc("LIST<T>::Insert: " << n << endl);

	// Grow array if necessary.
	//
	if (OutOfBounds(end))
		Grow(end);

	// Shift over elements starting with 'n'th
	// to make room for new element.
	//
	for (int i = end; i > n; i--)
		array[i] = array[i-1];

	// Insert element in the 'n'th slot.
	//
	array[n] = item;
	++end;
}

// Delete "n"th item from this LIST.
// Assumes list contains at least n-1 items.
//
template <class T>
void
LIST<T>::Delete(int n)
{
	assert(n >= 0  &&  n < end);
	assert(objstate.IsReady());
	DbgFunc("LIST<T>::Delete: " << n << endl);

	--end;

	// Shift over elements starting with 'n+1'
	// to cover up slot vacated by deleting 'n'th element.
	//
	for (int i = n; i < end; i++)
		array[i] = array[i+1];
}

// Clear this LIST.
//
template <class T>
void
LIST<T>::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("LIST<T>::Clear" << endl);

	end = 0;
}

// Get number of items currently in this LIST.
//
template <class T>
int
LIST<T>::Count() const
{
	assert(objstate.IsReady());
	DbgFunc("LIST<T>::Count: " << end << endl);

	return(end);
}

// List sorting routine.
//
template <class T>
void
SortList(LIST<T> &list_to_sort, int (*compar)(const void *,const void *))
{
	assert(list_to_sort.objstate.IsReady());
	DbgFunc("SortList" << endl);

	qsort(	(void *)list_to_sort.array,
		(size_t) list_to_sort.Count(),
		sizeof(T),
		compar);
}
