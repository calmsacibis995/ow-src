#ifndef	_PGRANGE_H
#define	_PGRANGE_H

#ident "@(#)pgrange.h	1.6 93/04/14 Copyright 1990 Sun Microsystems, Inc."

#include <doc/common.h>


// Class PGRANGE - Document Page Range
//
// This class handles the document page range field for class 'DOCUMENT'.
// A page range is the logical (ordinal) range of pages covered by
// a logical document.
//
// PGRANGE is responsible for validating the page range in a DOCUMENT
// object, and performing comparision operations on that page range
// (e.g., "page lies within page range").
//

class	PGRANGE {

    private:

	BOOL	is_valid;		// is this a valid page range?
	int	first, last;		// first, last page in range


    public:

	// Page Range constructors.
	//
	PGRANGE();
	PGRANGE(int first_page, int last_page);
	PGRANGE(const PGRANGE &);

	// Is page range valid?
	//
	BOOL		IsValid() const		{ return(is_valid); }

	// Assign one page range to another.
	//
	const PGRANGE	&operator = (const PGRANGE &);

	// Compare two page ranges.
	//
	friend BOOL	operator == (const PGRANGE &, const PGRANGE &);

	// Does page range include specified page?
	//
	BOOL		Includes(int page) const;

	// Get first, last page of page range.
	//
	int		FirstPage() const	{ return(first); }
	int		LastPage() const	{ return(last); }

	// Get number of pages in page range.
	//
	int		NumPages() const;

	// Print page range to specified output stream.
	//
	friend ostream	&operator << (ostream &ostr, const PGRANGE &rng);
};

#endif	_PGRANGE_H
