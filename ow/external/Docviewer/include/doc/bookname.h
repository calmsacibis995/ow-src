#ifndef	_BOOKNAME_H
#define	_BOOKNAME_H

#ident "@(#)bookname.h	1.7 06/11/93 Copyright 1989 Sun Microsystems, Inc."


#include <doc/abname.h>


class	BOOKNAME : public ABNAME {

    private:

	STRING		book_id;	// book id
	STRING		book_lang;	// book language


    protected:

	// Internal form of initialization routine.
	// See also 'Init(const STRING &)'.
	//
	STATUS		Init(LIST<STRING> &attrs);

	// Internal form of Name Resolver: called by DOCNAME::Resolve().
	//
	void		DoResolve(const BOOKNAME &name);


    public:

	// BOOKNAME constructors, destructor.
	//
	BOOKNAME()				{ book_lang = "C"; }
	BOOKNAME(const BOOKNAME &);
	BOOKNAME(const STRING &);
	~BOOKNAME()				{ }

	// Assign another BOOKNAME to this one.
	//
	BOOKNAME	&operator=(const BOOKNAME &);

	// Are two BOOKNAMEs (not) equivalent?
	//
	BOOL		operator==(const BOOKNAME &) const;
	BOOL		operator!=(const BOOKNAME &) const;

	// Initialize BOOKNAME from a STRING.
	// Returns STATUS_OK if 'bookname' is valid,
	// otherwise returns STATUS_FAILED.
	//
	STATUS		Init(const STRING &namestr);

	// Is this BOOKNAME valid?
	// i.e., are all its required attributes set?
	//
	BOOL		IsValid() const;

	// Get/Set BOOKNAME attributes.
	//
	const STRING	&BookId() const		{ return(book_id); }
	const STRING	&BookLang() const	{ return(book_lang); }
	STATUS		SetBookId(const STRING &book_id);
	STATUS		SetBookLang(const STRING &book_lang);

	// If a bookname is not fully qualified (i.e., some of its attributes
	// are missing), try to resolve it in the context of the
	// ABNAME 'context'.
	// In other words, use the attributes in 'context' to fill in
	// those missing in this BOOKNAME.
	//
	void		Resolve(const ABNAME &context);

	// Get ABNAME component of BOOKNAME.
	//
	const ABNAME	&ABName() const		{ return(*this); }

	// Various conversions of BOOKNAME to STRING, char *, etc.
	// Provided for convenience.
	//
	const STRING	NameToString() const;
	operator	const STRING () const	{ return( NameToString()); }
	operator	const char * () const	{ return( NameToString()); }
	const char	*operator ~  () const	{ return(~NameToString()); }

	// Print BOOKNAME.
	//
	friend ostream	&operator << (ostream &ostr, const BOOKNAME &name)
			{ return(ostr << ~name); }
};

#endif	_BOOKNAME_H
