#ifndef	_QUERY_H
#define	_QUERY_H

#ident "@(#)query.h	1.10 11/15/96 Copyright 1990-1992 Sun Microsystems, Inc."

// This header file defines the interface for the QUERY class.
// The QUERY class is used to store and maintain an active user query
// within an application.
//


#include <doc/common.h>
#include <doc/list.h>


// Relevance ranking algorithms supported by
// the Ful/Text search engine.
//
enum RANKING {
	RANK_TOTAL_OCCURRENCES	= 1,
	RANK_TOTAL_TERMS	= 2,
	RANK_FREQUENCY		= 3,
	RANK_TF_IDF		= 4
};

// Various hitlist sorting orders.
//
enum SORT_SEARCH_RESULTS {
	SORT_BY_RANK,
	SORT_BY_BOOK
};


// Forward references.
//
class	SCOPEKEY;
class	ZONEWGHT;


// Search query object.
// Maintains the query parameters (e.g., scoping, ranking algorithm)
// as well as the text of the query.
//
class	QUERY {

    private:

	// Actual text for the current query,
	// as typed in by the user.
	//
	STRING	text;

	// Search only the titles of documents,
	// vs. searching the entire document (titles *and body text).
	//
	BOOL	titles_only;

	// Ful/Text ranking algorithm (default=RANK_TF_IDF).
	//
	RANKING	ranking;

	// Max number of documents to retrieve.
	//
	int	max_docs;

	// Automatically display first doc in hitlist.
	//
	BOOL	show_first;

	// Enable parsing of query operators: "", (), *.
	//
	BOOL	operators;

	// Enable scoping for this search - use current scoping keys.
	//
	BOOL	scoped;

	// Strip accents from query text before performing search.
	//
	BOOL	strip_accents;

	// Sorting order of search results list.
	//
	SORT_SEARCH_RESULTS	sort_order;


    public:

	QUERY();
	~QUERY();

	// Get the query formatted as a Ful/Text query,
	// suitable for passing directly to the Ful/Text search engine.
	//
	void		ComposeQuery(	LIST<SCOPEKEY*> &keys,
					LIST<ZONEWGHT*> &weights,
					STRING &query_string) const;

	// Query property manipulation interfaces for
	// getting/setting query parameters prior to search operations.
	//
	const STRING	&Text() const		{ return(text); };
	void		Text(const STRING &t)	{ text = t; }

	int		MaxDocuments() const	{ return(max_docs); }
	void		MaxDocuments(int n)	{ max_docs = n; }

	RANKING		Ranking() const		{ return(ranking); }
	void		Ranking(RANKING r)	{ ranking = r; }

	BOOL		TitlesOnly() const	{ return(titles_only); }
	void		TitlesOnly(BOOL t)	{ titles_only = t; }

	BOOL		FirstDoc() const	{ return(show_first); }
	void		FirstDoc(BOOL s)	{ show_first = s; }

	BOOL		Operators() const	{ return(operators); }
	void		Operators(BOOL o)	{ operators = o; }

	BOOL		Scoped() const		{ return(scoped); }
	void		Scoped(BOOL s)		{ scoped = s; }

	BOOL		StripAccents() const	{ return(strip_accents); }
	void		StripAccents(BOOL s)	{ strip_accents = s; }

	SORT_SEARCH_RESULTS SortOrder() const	{ return(sort_order); }
	void		SortOrder(SORT_SEARCH_RESULTS s)  { sort_order = s; }

	// The ostream operator provides a detailed output of the
	// query textual data and the search engine parameters.
	// It is primarily used in debugging statements when there
	// has been an error returned from the low level
	// search engine interfaces.
	//
        friend          ostream & operator <<  (ostream &, const QUERY &);
};


// Parse query, converting query operators [(), "", -, *]
// into the equivalent Ful/Text query operators.
// 'outquery' has the resulting parsed query string.
//
STATUS	ParseQuery(const STRING &inquery, STRING &outquery);

// Map ISO Latin-1 characters in a string to Ful/Text character set
// and vice-versa.
// All accents (and other non-ascii stuff) are preserved in the translation.
//
void	MapISOStringToFTCS(const STRING &iso_string,  STRING &ftcs_string);
void	MapFTCSStringToISO(const STRING &ftcs_string, STRING &iso_string);

#endif	_QUERY_H
