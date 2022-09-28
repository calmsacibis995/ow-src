#ifndef	_ATTRLIST_H
#define	_ATTRLIST_H

#ident "@(#)attrlist.h	1.17 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/common.h>	//XXX needed to prevent illegal forward refs to "ARRAY"
#include <doc/list.h>


// Forward references.
//
class	XDR;
class	_ATTR;

// Class ATTRLIST manages variable-length lists of <name,value> string pairs.
//
class	ATTRLIST {

    private:

	LIST<_ATTR*>	attr_list;		// list of attributes
	OBJECT_STATE	objstate;		// current state of this object

	// XDR encode/decode routines.
	//
	int		XdrEncode(XDR *);
	int		XdrDecode(XDR *);
	

    public:

	ATTRLIST(void);		// construct empty attrlist
	~ATTRLIST(void);

	// Treat attribute list as an associative array XXX.
	// Allow array-type operations using attribute name as index
	// into the array, and attribute value as array element.
	//
	// XXX Describe semantics:
	//	alist["foo"] = "bar";
	//	bar = alist["foo"];
	//
	STRING		&operator [] (const STRING &);

	// Delete all attributes in this list.
	//
	void		Clear();

	// XDR conversion routine.
	//
	friend int	xdr_ATTRLIST(XDR *, ATTRLIST *);

	// Print attribute list to output stream.
	//
	friend ostream	&operator << (ostream &, ATTRLIST &);
};

#endif	_ATTRLIST_H
