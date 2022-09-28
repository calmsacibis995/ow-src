#ident "@(#)pgrange.cc	1.7 93/04/14 Copyright 1990 Sun Microsystems, Inc."


#include <doc/pgrange.h>


PGRANGE::PGRANGE()
{
	first = last = -1;
	is_valid = BOOL_FALSE;

	DbgFunc("PGRANGE::PGRANGE: " << *this << endl);
}

PGRANGE::PGRANGE(int first_page, int last_page)
{
	first = first_page;
	last  = last_page;

	if (first >= 0  &&  last >= 0  &&  first <= last) {
		is_valid = BOOL_TRUE;
	} else {
		first = -1;
		last  = -1;
		is_valid = BOOL_FALSE;
	}

	DbgFunc("PGRANGE::PGRANGE: " << *this << endl);
}

PGRANGE::PGRANGE(const PGRANGE &range)
{
	first    = range.first;
	last     = range.last;
	is_valid = range.is_valid;

	DbgFunc("PGRANGE::PGRANGE: " << *this << endl);
}

// Parse page range string.
//
const PGRANGE &
PGRANGE::operator = (const PGRANGE &range)
{
	first    = range.first;
	last     = range.last;
	is_valid = range.is_valid;

	DbgFunc("PGRANGE::Assign: " << *this << endl);

	return(*this);
}

// Does page lie within page range?
//
BOOL
PGRANGE::Includes(int page) const
{
	assert(IsValid());

	if (page >= first  &&  page <= last) {
		DbgFunc("PGRANGE::Includes: " << page
			<< " (" << *this << ") (yes)" << endl);
		return(BOOL_TRUE);
	} else {
		DbgFunc("PGRANGE::Includes: " << page
			<< " (" << *this << ") (no)" << endl);
		return(BOOL_FALSE);
	}
}

// How many pages in this page range?
//
int
PGRANGE::NumPages() const
{
	assert(IsValid());
	DbgFunc("PGRANGE::NumPages: " << *this << endl);

	return(last - first + 1);

}

// Are two page ranges the same?
//
BOOL
operator == (const PGRANGE &r1, const PGRANGE &r2)
{
	DbgFunc("PGRANGE::operator==: " << r1 << " ==? " << r2 << endl);

	// Note that if both page ranges are "invalid",
	// they're considered equivalent.
	//
	if (r1.first == r2.first  &&  r1.last == r2.last)
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

ostream &
operator << (ostream &ostr, const PGRANGE &rng)
{
	assert(rng.IsValid());
	ostr << rng.first << " - " << rng.last;
	return( ostr );
}
