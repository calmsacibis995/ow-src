#ifndef	__STACK_H__
#define	__STACK_H__

#ident "@(#)stack.h	1.5 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/array.h>


// Parameterized stack class
//
template <class T>
class	STACK : private ARRAY<T> {

    private:

	int	top;		// top of stack index


    public:

	// Stack constructor, destructor.
	//
	STACK()			{ top = 0; }
	~STACK()		{ }

	// Push a new item onto this stack.
	//
	void	Push(const T &t) { (*this)[top++] = t; }

	// Pop top item off this stack.
	// Assumes this stack is non-empty.
	//
	T	&Pop()		{ assert(!IsEmpty()); return((*this)[--top]); }

	// Peek at nth item on this stack without popping it.
	// Assumes stack contains at least n-1 items.
	//
	T	&Peek(int n)	{ assert(n < Depth()); 	return((*this)[n]); }

	// Is this stack empty?
	//
	BOOL	IsEmpty() const	{ return(top==0 ? BOOL_TRUE : BOOL_FALSE); }

	// Get number of items currently on this stack.
	//
	int	Depth() const	{ return(top); }

	// Remove all items from stack.
	// Doesn't actually "delete()" those items - just resets
	// 'top of stack' pointer.
	//
	void	Clear()		{ top = 0; }
};

#endif	__STACK_H__
