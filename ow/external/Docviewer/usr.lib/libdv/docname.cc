#ident "@(#)docname.cc	1.13 06/11/93 Copyright 1992 Sun Microsystems, Inc."



#include <doc/docname.h>

// Convert integer to string.
// XXX this belongs in string.h.
//
const STRING
int2string(int n)
{
	char	buf[20];
	sprintf(buf, "%d", n);
	return(buf);
}


// Basic DOCNAME constructor.
//
DOCNAME::DOCNAME() :
	offset		(0)
{
	DbgFunc("DOCNAME::DOCNAME" << endl);
}

// Construct DOCNAME from another DOCNAME.
//
DOCNAME::DOCNAME(const DOCNAME &docname) :
	BOOKNAME	(docname),
	doc_id		(docname.DocId()),
	offset		(docname.Offset())
{
	DbgFunc("DOCNAME::DOCNAME(DOCNAME &)" << *this << endl);
}

// Construct DOCNAME from a doc name STRING.
//
DOCNAME::DOCNAME(const STRING &docstr) :
	offset		(0)
{
	Init(docstr);
	DbgFunc("DOCNAME::DOCNAME(STRING &)" << *this << endl);
}

// Assign DOCNAME to DOCNAME.
//
DOCNAME &
DOCNAME::operator=(const DOCNAME &docname)
{
	assert(objstate.IsReady());

	this->BOOKNAME::operator=(docname);
	doc_id = docname.DocId();
	offset = docname.Offset();

	DbgFunc("DOCNAME::operator=(DOCNAME &)" << *this << endl);
	return(*this);
}

// Are two DOCNAMEs equal?
//
BOOL
DOCNAME::operator==(const DOCNAME &docname) const
{
	assert(objstate.IsReady());

	if (DocId()    == docname.DocId()     &&
	    Offset()   == docname.Offset()    &&
	    BookName() == docname.BookName()) {

		DbgFunc("DOCNAME::operator==: "<<*this<<" == "<<docname<<endl);
		return(BOOL_TRUE);

	} else {

		DbgFunc("DOCNAME::operator==: "<<*this<<" != "<<docname<<endl);
		return(BOOL_FALSE);
	}
}

// Is this DOCNAME valid?
// i.e., are all its required attributes set?
//
BOOL
DOCNAME::IsValid() const
{
	assert(objstate.IsReady());

	if (DocId() != NULL_STRING  &&  BOOKNAME::IsValid()) {
		DbgFunc("DOCNAME::IsValid: " << *this << endl);
		return(BOOL_TRUE);
	} else {
		DbgFunc("DOCNAME::IsValid: " << *this << " (NOT!)" << endl);
		return(BOOL_FALSE);
	}
}

// Is this the root document of a particular book?
//
BOOL
DOCNAME::IsRoot() const
{
	assert(objstate.IsReady());

	if (DocId() != NULL_STRING  &&  DocId() == BookId()) {
		DbgFunc("DOCNAME::IsRoot: " << *this << endl);
		return(BOOL_TRUE);
	} else {
		DbgFunc("DOCNAME::IsRoot: " << *this << " (NOT!)" << endl);
		return(BOOL_FALSE);
	}
}

STATUS
DOCNAME::SetDocId(const STRING &doc_id_arg)
{
	assert(objstate.IsReady());

	doc_id = STRING::CleanUp(doc_id_arg);

	if ( ! IsValidId(doc_id, strict)) {
		doc_id = NULL_STRING;
		return(STATUS_FAILED);
	}

	DbgFunc("DOCNAME::SetDocId: " << *this << endl);
	return(STATUS_OK);
}

STATUS
DOCNAME::SetOffset(int offset_arg)
{
	assert(offset_arg >= 0);
	assert(objstate.IsReady());

	offset = offset_arg;

	DbgFunc("DOCNAME::SetOffset: " << *this << endl);
	return(STATUS_OK);
}

// Convert DOCNAME to STRING.
//
const STRING 
DOCNAME::NameToString() const
{
	LIST<STRING>	namelist;
	STRING		namestr;
	int		i = 0;


	assert(objstate.IsReady());


	if (ABId() != NULL_STRING)
		namelist.Add(DNATTR_AB_ID      + "=" + ABId());

	if (ABVersion() != NULL_STRING)
		namelist.Add(DNATTR_AB_VERSION + "=" + ABVersion());

	if (BookId() != NULL_STRING)
		namelist.Add(DNATTR_BOOK_ID    + "=" + BookId());

	if (BookLang() != NULL_STRING  &&  BookLang() != "C")
		namelist.Add(DNATTR_BOOK_LANG  + "=" + BookLang());

	if (DocId() != NULL_STRING)
		namelist.Add(DNATTR_DOC_ID     + "=" + DocId());

	if (Offset() > 0)
		namelist.Add(DNATTR_DOC_OFFSET + "=" + int2string(Offset()));

	namestr = "<";
	for (i = 0; i < namelist.Count(); i++) {
		if (i > 0)
			namestr += ";";
		namestr += namelist[i];
	}
	namestr += ">";
	

	return(namestr);
}

// Convert DOCNAME to old-style "short name":
//	<book_id>doc_id
// Note that in doing so, we lose the offset info, if any.
//
const STRING 
DOCNAME::ShortName() const
{
	STRING	shortname;

	assert(objstate.IsReady());

	shortname = "<" + BookId() + ">";
	if ( ! IsRoot())
		shortname += DocId();

	return(shortname);
}

// If a DOCNAME is not fully qualified (i.e., some of its attributes
// are missing), try to resolve it in the given context.
// In other words, use the attributes in the context to fill in
// those missing in the DOCNAME.
//
void
DOCNAME::Resolve(const BOOKNAME &context)
{
	assert(objstate.IsReady());

	BOOKNAME::DoResolve(context);
	DbgFunc("DOCNAME::Resolve: "
		<< *this << " (context=" << context << ")" << endl);
}

// Assign STRING to DOCNAME.
// Parse name string, extract DOCNAME attributes & values.
//
STATUS
DOCNAME::Init(const STRING &namestr)
{
	LIST<STRING>	attrs;
	STRING		attr;
	STRING		attr_name, attr_value;
	int		equals;
	int		i;

	assert(objstate.IsReady());

	doc_id = NULL_STRING;
	offset = 0;


	// Parse the DOCNAME string into a list of attribues.
	//
	if (ParseNameString(namestr, attrs)  !=  STATUS_OK)
		return(STATUS_FAILED);


	// Pass attribute list on to this DOCNAME's BOOKNAME component
	// so it can initialize itself.
	//
	if (BOOKNAME::Init(attrs)  !=  STATUS_OK)
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

		if (attr_name == DNATTR_DOC_ID) {
			if (SetDocId(attr_value) != STATUS_OK)
				return(STATUS_FAILED);
		} else if (attr_name == DNATTR_DOC_OFFSET) {
			if (SetOffset(atoi(attr_value)) != STATUS_OK)
				return(STATUS_FAILED);
		}
	}

	DbgFunc("DOCNAME::Init: " << *this << endl);
	return(STATUS_OK);
}

// Construct the name of the root document for the given BOOKNAME.
//
void
MakeBookRootDocName(const BOOKNAME &bookname, DOCNAME &rootname)
{
	assert(bookname.IsValid());
	DbgFunc("MakeBookRootDocName: " << bookname << endl);

	rootname.SetABId(bookname.ABId());
	rootname.SetABVersion(bookname.ABVersion());
	rootname.SetBookId(bookname.BookId());
	rootname.SetBookLang(bookname.BookLang());
	rootname.SetDocId(bookname.BookId());
	rootname.SetOffset(0);

	assert(rootname.IsValid());
}

// Ditto, but for AnswerBooks rather than books.
//
void
MakeAnswerBookRootDocName(const ABNAME &abname, DOCNAME &rootname)
{
	assert(abname.IsValid());
	DbgFunc("MakeBookRootDocName: " << abname << endl);

	rootname.SetABId(abname.ABId());
	rootname.SetABVersion(abname.ABVersion());
	rootname.SetBookId(abname.ABId());
	rootname.SetDocId(abname.ABId());
	rootname.SetOffset(0);

	assert(rootname.IsValid());
}
