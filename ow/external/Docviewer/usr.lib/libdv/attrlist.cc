#ident "@(#)attrlist.cc	1.20 06/11/93 Copyright 1990 Sun Microsystems, Inc."



#include <doc/attrlist.h>
#include <rpc/rpc.h>


class	_ATTR {
	
    public:

	STRING		name;
	STRING		value;

	_ATTR(const STRING &n, const STRING &v)	{ name = n; value = v; }
	_ATTR(const STRING &n)			{ name = n; }

	const _ATTR	&operator = (const _ATTR &attr)
			{
				this->name = attr.name;
				this->value = attr.value;
				return(*this);
			}

	friend ostream	&operator << (ostream &ostr, const _ATTR &a)
		{ ostr << a.name << "=" << a.value; return(ostr); }
};


ATTRLIST::ATTRLIST()
{
	DbgFunc("ATTRLIST::ATTRLIST" << endl);
	objstate.MarkReady();
}

ATTRLIST::~ATTRLIST()
{
	DbgFunc("ATTRLIST::~ATTRLIST" << endl);

	Clear();	// delete all attributes in list
}

// Access attributes as if they were array elements.
//
STRING &
ATTRLIST::operator [] (const STRING &name)
{
	int	i;


	assert(name != NULL_STRING);
	assert(objstate.IsReady());


	// Search for attribute name in the (alphabetically sorted) list.
	//
	for (i = 0; i < attr_list.Count(); i++) {
		if (attr_list[i]->name >= name)
			break;
	}

	if (i >= attr_list.Count()) {

		// Attribute is not in list, and belongs at the
		// end of the (alphabetically sorted) list.
		//
		attr_list.Add(new _ATTR(name));

	} else if (attr_list[i]->name != name) {

		// Attribute is not in list, and belongs at
		// position 'i' in the (alphabetically sorted) list.
		//
		assert(attr_list[i]->name > name);
		attr_list.Insert(new _ATTR(name), i);
	}


	DbgFunc("ATTRLIST [" << attr_list[i]->name  << "]="
			     << attr_list[i]->value << endl);


	// Return (reference to) attribute's value.
	//
	return(attr_list[i]->value);
}

// Convert ATTRLIST to XDR stream.
// See XDR section in "Network Programming" manual for details.
//
int
ATTRLIST::XdrEncode(XDR *xdrs)
{
	int	count = attr_list.Count();
	int	i;


	assert(objstate.IsReady());
	DbgFunc("ATTRLIST::XdrEncode" << endl);
	
	assert(xdrs->x_op == XDR_ENCODE);
	

	if ( ! xdr_int(xdrs, &count))
		return(FALSE);

	for (i = 0; i < count; i++) {

		if ( ! xdr_STRING(xdrs, &attr_list[i]->name))
			return(FALSE);
		if ( ! xdr_STRING(xdrs, &attr_list[i]->value))
			return(FALSE);

		DbgLow("ATTRLIST::XdrEncode: " << attr_list[i] << endl);
	}

	return(TRUE);
}


// Convert XDR stream to ATTRLIST.
// See XDR section in "Network Programming" manual for details.
//
int
ATTRLIST::XdrDecode(XDR *xdrs)
{
	STRING	name, value;
	int	count;
	int	i;

	
	assert(objstate.IsReady());
	assert(xdrs->x_op == XDR_DECODE);
	DbgFunc("ATTRLIST::XdrDecode" << endl);
	
		
	// Clear out list of any existing attributes.
	//
	Clear();

	// Get attribute count.
	//
	if ( ! xdr_int(xdrs, &count))
		return(FALSE);

	// For each attribute, read in name and value,
	// and add attribute to list.
	//
	for (i = 0; i < count; i++) {

		if (!xdr_STRING(xdrs, &name)  ||  !xdr_STRING(xdrs, &value)) {
			Clear();
			return(FALSE);
		}

		attr_list.Add(new _ATTR(name, value));
		DbgLow("ATTRLIST::XdrDecode: " << attr_list[i] <<endl);

		// Make sure attr names are in alphabetical order.
		//
		if (i > 0) {
			assert(name > attr_list[i-1]->name);
		}
	}


	return(TRUE);
}

// XDR filter for ATTRLISTs
//
int
xdr_ATTRLIST(XDR *xdrs, ATTRLIST *attr_list)
{
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		return(attr_list->XdrEncode(xdrs));

	case XDR_DECODE:
		return(attr_list->XdrDecode(xdrs));

	case XDR_FREE:
		break;
	}

	return(TRUE);
}

// Delete all attributes in list.
//
void
ATTRLIST::Clear()
{
	int	i;


	assert(objstate.IsReady());
	DbgFunc("ATTRLIST::Clear" << endl);


	// Delete each attribute in list.
	//
	for (i = 0; i < attr_list.Count(); i++)
		delete(attr_list[i]);

	// Clear list.
	//
	attr_list.Clear();
}

// Print all attributes in list.
//
ostream	&operator << (ostream &out, ATTRLIST &a)
{
	int	i;

	for (i = 0; i < a.attr_list.Count(); i++)
		out << a.attr_list[i] << endl;
	
	return(out);
}
