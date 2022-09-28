#ident "@(#)hist_stack.h	1.9 93/03/09 Copyright 1990-1992 Sun Microsystems, Inc."

#ifndef	_HIST_STACK_H_
#define	_HIST_STACK_H_

#include "common.h"
#include <doc/list.h>
#include <doc/docname.h>


// Forward references.
//
class	HIST_ITEM;


class	HIST_STACK {

    private:

	// Stack of history items.
	//
	LIST<HIST_ITEM*>	stack;

	// Max depth of this stack.
	//
	int			max_depth;

	// Current state of this object.
	//
	OBJECT_STATE		objstate;


    public:

	HIST_STACK(int max_depth);
	~HIST_STACK();

	// Push history item onto stack.
	//
	void		Push(const DOCNAME &doc, const STRING &path, int page);

	// Pop history item off stack.
	//
	void		Pop(DOCNAME &doc, STRING &path, int &page);

	// Clear the stack.
	//
	void		Clear();

	// Get current stack depth.
	//
	int		Depth()		{ return(stack.Count()); }

	// Is stack empty?
	//
	BOOL		IsEmpty()	{ return((BOOL)(Depth()==0)); }
};

#endif /* _HIST_STACK_H_ */
