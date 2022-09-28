#ident "@(#)bookname.cc	1.8 06/11/93 Copyright 1992 Sun Microsystems, Inc."



#include <doc/bookname.h>


// BOOKNAME constructor: construct BOOKNAME from another BOOKNAME.
//
BOOKNAME::BOOKNAME(const BOOKNAME &bookname) : ABNAME(bookname)
{
	book_id   = bookname.BookId();
	book_lang = bookname.BookLang();

	DbgFunc("BOOKNAME::BOOKNAME(BOOKNAME &)" << *this << endl);
}

// Construct BOOKNAME from a book name STRING.
//
BOOKNAME::BOOKNAME(const STRING &bookstr)
{
	Init(bookstr);
	DbgFunc("BOOKNAME::BOOKNAME(STRING &)" << *this << endl);
}

// Assign BOOKNAME to BOOKNAME.
//
BOOKNAME &
BOOKNAME::operator=(const BOOKNAME &bookname)
{
	assert(objstate.IsReady());

	this->ABNAME::operator=(bookname);
	book_id   = bookname.BookId();
	book_lang = bookname.BookLang();

	DbgFunc("BOOKNAME::operator=(BOOKNAME &)" << *this << endl);
	return(*this);
}

// Are two BOOKNAMEs equal?
//
BOOL
BOOKNAME::operator==(const BOOKNAME &bookname) const
{
	assert(objstate.IsReady());

	if (BookId()   == bookname.BookId()   &&
	    BookLang() == bookname.BookLang() &&
	    ABName()   == bookname.ABName())  {

		DbgFunc("BOOKNAME::op==: "<<*this<<" == "<<bookname<<endl);
		return(BOOL_TRUE);

	} else {

		DbgFunc("BOOKNAME::op==: "<<*this<<" != "<<bookname<<endl);
		return(BOOL_FALSE);
	}
}

// Is this BOOKNAME valid?
// i.e., are all its required attributes set?
//
BOOL
BOOKNAME::IsValid() const
{
	assert(objstate.IsReady());

	if (BookId() != NULL_STRING  &&  ABNAME::IsValid()) {
		DbgFunc("BOOKNAME::IsValid: " << *this << endl);
		return(BOOL_TRUE);
	} else {
		DbgFunc("BOOKNAME::IsValid: " << *this << " (NOT!)" << endl);
		return(BOOL_FALSE);
	}
}

STATUS
BOOKNAME::SetBookId(const STRING &book_id_arg)
{
	assert(objstate.IsReady());

	book_id = STRING::CleanUp(book_id_arg);

	if (! IsValidId(book_id, strict)) {
		book_id = NULL_STRING;
		return(STATUS_FAILED);
	}

	DbgFunc("BOOKNAME::SetBookId: " << *this << endl);
	return(STATUS_OK);
}

STATUS
BOOKNAME::SetBookLang(const STRING &book_lang_arg)
{
	assert(objstate.IsReady());

	book_lang = STRING::CleanUp(book_lang_arg);
	if (book_lang == NULL_STRING)
		book_lang = "C";
	DbgFunc("BOOKNAME::SetBookLang: " << *this << endl);
	return(STATUS_OK);
}

// Convert BOOKNAME to STRING.
//
const STRING 
BOOKNAME::NameToString() const
{
	STRING	namestr;

	assert(objstate.IsReady());

	namestr ="<"	+ DNATTR_AB_ID		+ "=" + ABId()		+ ";"
			+ DNATTR_AB_VERSION	+ "=" + ABVersion()	+ ";"
			+ DNATTR_BOOK_ID	+ "=" + BookId()	+ ";"
			+ DNATTR_BOOK_LANG	+ "=" + BookLang()	+ ">";

	return(namestr);
}

// If a bookname is not fully qualified (i.e., some of its attributes
// are missing), try to resolve it in the given context.
// In other words, use the attributes in the context to fill in
// those missing in the bookname.
//
void
BOOKNAME::Resolve(const ABNAME &context)
{
	assert(objstate.IsReady());

	ABNAME::DoResolve(context);
	DbgFunc("BOOKNAME::Resolve: "
		<< *this << " (context=" << context << ")" << endl);
}

// Internal form of Name Resolver: called by DOCNAME::Resolve().
//
void
BOOKNAME::DoResolve(const BOOKNAME &context)
{
	assert(objstate.IsReady());
	DbgFunc("BOOKNAME::DoResolve" << endl);

	ABNAME::DoResolve(context);
	if (book_id == NULL_STRING)
		book_id = context.book_id;
	if (book_lang == "C")
		book_lang = context.book_lang;
}

// Assign STRING to BOOKNAME.
// Parse name string, extract BOOKNAME attributes & values.
//
STATUS
BOOKNAME::Init(const STRING &namestr)
{
	LIST<STRING>	attrs;

	assert(objstate.IsReady());

	// Parse the DOCNAME string into a list of attribues.
	//
	if (ParseNameString(namestr, attrs)  !=  STATUS_OK)
		return(STATUS_FAILED);

	return(Init(attrs));
}

// Assign STRING to BOOKNAME.
// Parse name string, extract BOOKNAME attributes & values.
//
STATUS
BOOKNAME::Init(LIST<STRING> &attrs)
{
	STRING		attr;
	STRING		attr_name, attr_value;
	int		equals;
	int		i;

	assert(objstate.IsReady());


	// Reset BOOKNAME attributes.
	//
	book_id   = NULL_STRING;
	book_lang = "C";


	// Pass attribute list on to this BOOKNAME's ABNAME component
	// so it can initialize itself.
	//
	if (ABNAME::Init(attrs)  !=  STATUS_OK)
		return(STATUS_FAILED);


	for (i = 0; i < attrs.Count(); i++) {

		// Extract name, value from attribute.
		// Note that the attribute value will undergo
		// cleanup, syntax checking in the individual
		// "Set*()" routine.
		//
		attr = attrs[i];
		equals = attr.Index('=');
		assert(equals >= 0);

		attr_name  = STRING::CleanUp(attr.SubString(0, equals-1));
		attr_value = attr.SubString(equals+1, END_OF_STRING);

		if (attr_name == DNATTR_BOOK_ID) {
			if (SetBookId(attr_value) != STATUS_OK)
				return(STATUS_FAILED);
		} else if (attr_name == DNATTR_BOOK_LANG) {
			if (SetBookLang(attr_value) != STATUS_OK)
				return(STATUS_FAILED);
		}
	}

	DbgFunc("BOOKNAME::Init: " << *this << endl);
	return(STATUS_OK);
}
