#ifndef	_ABNAME_H
#define	_ABNAME_H

#ident "@(#)abname.h	1.6 06/11/93 Copyright 1989 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/list.h>


class	ABNAME {

    private:

	STRING		ab_id;		// AnswerBook id
	STRING		ab_version;	// AnswerBook version


    protected:

	BOOL		strict;		// do strict syntax checking
	OBJECT_STATE	objstate;	// current state of this object

	// Internal form of initialization routine.
	// See also 'Init(const STRING &)'.
	//
	STATUS		Init(LIST<STRING> &attrs);

	// Internal form of Name Resolver: called by BOOKNAME::Resolve().
	//
	void		DoResolve(const ABNAME &name);


    public:

	// ABNAME constructors, destructor.
	//
	ABNAME();
	ABNAME(const ABNAME &);
	ABNAME(const STRING &);
	~ABNAME()				{ }

	// Assign another ABNAME to this one.
	//
	ABNAME		&operator=(const ABNAME &);

	// Are two ABNAMEs (not) equivalent?
	//
	BOOL		operator==(const ABNAME &) const;
	BOOL		operator!=(const ABNAME &) const;

	// Initialize ABNAME from a STRING.
	// Returns STATUS_OK if 'abname' is valid,
	// otherwise returns STATUS_FAILED.
	//
	STATUS		Init(const STRING &abname);

	// Is this ABNAME valid?
	// i.e., are all its required attributes set?
	//
	BOOL		IsValid() const;

	// Get/set individual ABNAME attributes.
	//
	const STRING	&ABId() const		{ return(ab_id); }
	const STRING	&ABVersion() const	{ return(ab_version); }
	STATUS		SetABId(const STRING &ab_id);
	STATUS		SetABVersion(const STRING &ab_version);

	// Do strict syntax checking on the contents of names.
	//
	void		Strict(BOOL s)		{ strict = s; }

	// Various conversions of ABNAME to STRING, char *, etc.
	// Provided for convenience.
	//
	const STRING	NameToString() const;
	operator	const STRING () const	{ return( NameToString()); }
	operator	const char * () const	{ return( NameToString()); }
	const char	*operator ~  () const	{ return(~NameToString()); }

	// Print ABNAME.
	//
	friend ostream	&operator << (ostream &ostr, const ABNAME &name)
			{ return(ostr << ~name); }
};


// ABNAME, BOOKNAME, and DOCNAME attribute names.
//
extern const STRING	DNATTR_AB_ID;		// AnswerBook id
extern const STRING	DNATTR_AB_VERSION;	// AnswerBook version
extern const STRING	DNATTR_BOOK_ID;		// book id
extern const STRING	DNATTR_BOOK_LANG;	// book language
extern const STRING	DNATTR_DOC_ID;		// document id
extern const STRING	DNATTR_DOC_OFFSET;	// offset into document

// Parse object name string (ABNAME, BOOKNAME, DOCNAME)
// into its component attributes.
//
STATUS	ParseNameString(const STRING &namestr, LIST<STRING> &attrs);

// Determine whether STRING 'id' is a valid ABNAME, BOOKNAME, or DOCNAME id;
// i.e., contains only alphanumerics, underscores, or periods.
//
BOOL	IsValidId(const STRING &id, BOOL strict);

#endif	_ABNAME_H
