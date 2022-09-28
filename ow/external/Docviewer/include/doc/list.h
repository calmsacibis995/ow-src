#ifndef	__LIST_H__
#define	__LIST_H__

#ident "@(#)list.h	1.26 11/15/96 Copyright 1992 Sun Microsystems, Inc."


#include <doc/array.h>


// Parameterized list class
//
template <class T>
class	LIST : private ARRAY<T> {

    private:

	// Index of next available list item slot.
	//
	int	end;


    public:

	// LIST constructor, destructor.
	//
	LIST()			: end(0)	{ }
	virtual		~LIST()			{ }

	// Append a new item to this LIST.
	//
	virtual void	Add(const T &item);

	// Insert a new item to the "n"th slot in this LIST.
	// Assumes list contains at least n-1 items.
	//
	virtual void	Insert(const T &item, int n);

	// Delete "n"th item from this LIST.
	// Assumes list contains at least n-1 items.
	//
	virtual void	Delete(int n);

	// Remove all items from list.
	//
	virtual void	Clear();

	// Is this LIST empty?
	//
	BOOL		IsEmpty() const		{ return((BOOL)(end == 0)); }

	// Get number of items currently in this LIST.
	//
	int		Count() const;

	// Access "n"th element of this LIST.
	// Assumes list contains at least n-1 items.
	//
	T		&operator [] (int n)
			{
				assert(n >= 0  &&  n < Count());
				return(array[n]);
			}

	void		Destroy () 
	{ 
		DestroyArray ();
	} 
	// List sorting routine.
	//
	friend void	SortList(LIST<T> &list_to_sort,
				 int (*compar)(const void *,const void *));
};

template <class T> void SortList(LIST<T> &list_to_sort,
				 int (*compar)(const void *,const void *));

#endif	__LIST_H__
