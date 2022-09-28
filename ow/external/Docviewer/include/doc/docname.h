#ifndef	_DOCNAME_H
#define	_DOCNAME_H

#ident "@(#)docname.h	1.9 06/11/93 Copyright 1989 Sun Microsystems, Inc."


#include <doc/bookname.h>


class	DOCNAME : public BOOKNAME {

    private:

	STRING		doc_id;		// document id
	int		offset;		// offset into document

	// Internal form of initialization routine.
	// See also 'Init(const STRING &)'.
	//
	STATUS		Init(LIST<STRING> &attrs);


    public:

	// DOCNAME constructors, destructor.
	//
	DOCNAME();
	DOCNAME(const DOCNAME &);
	DOCNAME(const STRING &);
	~DOCNAME()				{ }

	// Assign another DOCNAME to this one.
	//
	DOCNAME		&operator=(const DOCNAME &);

	// Are two DOCNAMEs (not) equivalent?
	//
	BOOL		operator==(const DOCNAME &) const;
	BOOL		operator!=(const DOCNAME &) const;

	// Initialize DOCNAME from a STRING.
	// Returns STATUS_OK if 'docname' is valid,
	// otherwise returns STATUS_FAILED.
	//
	STATUS		Init(const STRING &namestr);

	// Is this DOCNAME valid?
	// i.e., are all its required attributes set?
	//
	BOOL		IsValid() const;

	// Is this the root document of a particular book?
	//
	BOOL		IsRoot() const;

	// Get/Set DOCNAME attributes.
	//
	const STRING	&DocId() const		{ return(doc_id); }
	STATUS		SetDocId(const STRING &doc_id);
	int		Offset() const		{ return(offset); }
	STATUS		SetOffset(int offset_arg);

	// If a docname is not fully qualified (i.e., some of its attributes
	// are missing), try to resolve it in the context of the
	// BOOKNAME 'context'.
	// In other words, use the attributes in 'context' to fill in
	// those missing in this DOCNAME.
	//
	void		Resolve(const BOOKNAME &context);

	// Get BOOKNAME component of DOCNAME.
	//
	const BOOKNAME	&BookName() const	{ return(*this); }

	// Get old-style "short name" version of DOCNAME,
	// e.g. "<FOOBAR>1006".
	//
	const STRING	ShortName() const;

	// Various conversions of DOCNAME to STRING, char *, etc.
	// Provided for convenience.
	//
	const STRING	NameToString() const;
	operator	const STRING () const	{ return( NameToString()); }
	operator	const char * () const	{ return( NameToString()); }
	const char	*operator ~  () const	{ return(~NameToString()); }

	// Print DOCNAME.
	//
	friend ostream	&operator << (ostream &ostr, const DOCNAME &name)
			{ return(ostr << ~name); }
};

// Construct the name of the root document for the given BOOKNAME.
// The resulting "rootname" can then be passed to ABGROUP::LookUpDoc()
// to find the actual root document database record for that book.
// Assumes "bookname" is valid.
//
void	MakeBookRootDocName(const BOOKNAME &bookname, DOCNAME &rootname);

// Ditto, but for AnswerBooks rather than books.
//
void	MakeAnswerBookRootDocName(const ABNAME &abname, DOCNAME &rootname);

#endif	_DOCNAME_H
