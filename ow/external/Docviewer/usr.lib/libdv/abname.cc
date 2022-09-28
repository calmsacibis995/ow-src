#ident "@(#)abname.cc	1.9 06/11/93 Copyright 1992 Sun Microsystems, Inc."



#include <doc/abname.h>
#include <doc/token_list.h>
#include <ctype.h>


const STRING	DNATTR_AB_ID		("bs");
const STRING	DNATTR_AB_VERSION	("vr");
const STRING	DNATTR_BOOK_ID		("bk");
const STRING	DNATTR_BOOK_LANG	("ln");
const STRING	DNATTR_DOC_ID		("dc");
const STRING	DNATTR_DOC_OFFSET	("of");


ABNAME::ABNAME()
{
	strict = BOOL_TRUE;
	objstate.MarkReady();
	DbgFunc("ABNAME::ABNAME()" << endl);
}

ABNAME::ABNAME(const ABNAME &abname)
{
	ab_id      = abname.ABId();
	ab_version = abname.ABVersion();
	strict     = BOOL_TRUE;
	objstate.MarkReady();

	DbgFunc("ABNAME::ABNAME(ABNAME &)" << *this << endl);
}

// Construct ABNAME from an answerbook name STRING.
//
ABNAME::ABNAME(const STRING &abstr)
{
	Init(abstr);
	DbgFunc("ABNAME::ABNAME(STRING &)" << *this << endl);
}

// Assign ABNAME to ABNAME.
//
ABNAME &
ABNAME::operator=(const ABNAME &abname)
{
	assert(objstate.IsReady());

	ab_id      = abname.ABId();
	ab_version = abname.ABVersion();

	DbgFunc("ABNAME::operator=(ABNAME &)" << *this << endl);
	return(*this);
}

// Are two ABNAMEs equal?
//
BOOL
ABNAME::operator==(const ABNAME &abname) const
{
	assert(objstate.IsReady());

	if (ABId() == abname.ABId()  &&  ABVersion() == abname.ABVersion()) {

		DbgFunc("ABNAME::operator==: "<<*this<<" == "<<abname<<endl);
		return(BOOL_TRUE);

	} else {

		DbgFunc("ABNAME::operator==: "<<*this<<" != "<<abname<<endl);
		return(BOOL_FALSE);
	}
}

// Is this ABNAME valid?
// i.e., are all its required attributes set?
//
BOOL
ABNAME::IsValid() const
{
	assert(objstate.IsReady());

	if (ABId() != NULL_STRING) {
		DbgFunc("ABNAME::IsValid: " << *this << endl);
		return(BOOL_TRUE);
	} else {
		DbgFunc("ABNAME::IsValid: " << *this << " (NOT!)" << endl);
		return(BOOL_FALSE);
	}
}

STATUS
ABNAME::SetABId(const STRING &ab_id_arg)
{
	assert(objstate.IsReady());

	ab_id = STRING::CleanUp(ab_id_arg);

	if ( ! IsValidId(ab_id, strict)) {
		ab_id = NULL_STRING;
		return(STATUS_FAILED);
	}

	DbgFunc("ABNAME::SetABId: " << *this << endl);
	return(STATUS_OK);
}

STATUS
ABNAME::SetABVersion(const STRING &ab_version_arg)
{
	assert(objstate.IsReady());

	ab_version = STRING::CleanUp(ab_version_arg);
	DbgFunc("ABNAME::SetABVersion: " << *this << endl);
	return(STATUS_OK);
}


// Internal form of Name Resolver: called by BOOKNAME::Resolve().
//
void
ABNAME::DoResolve(const ABNAME &context)
{
	assert(objstate.IsReady());
	DbgFunc("ABNAME::DoResolve" << endl);

	if (ab_id == NULL_STRING)
		ab_id = context.ab_id;
	if (ab_version == NULL_STRING)
		ab_version = context.ab_version;
}

// Convert ABNAME to STRING.
//
const STRING 
ABNAME::NameToString() const
{
	STRING	namestr;

	assert(objstate.IsReady());

	namestr = "<"	+ DNATTR_AB_ID		+ "=" + ABId()		+ ";"
			+ DNATTR_AB_VERSION	+ "=" + ABVersion()	+ ">";

	return(namestr);
}

// Assign STRING to ABNAME.
// Parse name string, extract ABNAME attributes & values.
//
STATUS
ABNAME::Init(const STRING &namestr)
{
	LIST<STRING>	attrs;

	assert(objstate.IsReady());

	if (ParseNameString(namestr, attrs)  !=  STATUS_OK)
		return(STATUS_FAILED);

	return(Init(attrs));
}

// Assign STRING to ABNAME.
// Parse name string, extract ABNAME attributes & values.
//
STATUS
ABNAME::Init(LIST<STRING> &attrs)
{
	STRING		attr;
	STRING		attr_name, attr_value;
	int		equals;
	int		i;

	assert(objstate.IsReady());

	ab_id      = NULL_STRING;
	ab_version = NULL_STRING;


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

		if (attr_name == DNATTR_AB_ID) {
			if (SetABId(attr_value) != STATUS_OK)
				return(STATUS_FAILED);
		} else if (attr_name == DNATTR_AB_VERSION) {
			if (SetABVersion(attr_value) != STATUS_OK)
				return(STATUS_FAILED);
		}
	}

	DbgFunc("ABNAME::Init: " << *this << endl);
	return(STATUS_OK);
}

// Parse name string, extract ABNAME, BOOKNAME, and DOCNAME
// attributes & values.
//
STATUS
ParseNameString(const STRING &namestr, LIST<STRING> &attr_list)
{
	int		open_bracket;
	int		close_bracket;
	TOKEN_LIST	attrs(';');
	int		num_attrs = 0;
	STRING		doc_id;
	int		i;


	DbgFunc("ParseNameString: " << namestr << endl);


	// Extract the attributes within the angle brackets, e.g.:
	//	<id=FooAB;vers=1.0.1;book=FOOBAR;lang=C>1001
	//
	if ((open_bracket  = namestr.Index('<'))  !=  0  ||
	    (close_bracket = namestr.Index('>'))  <   2) {
		return(STATUS_FAILED);
	}

	attrs = namestr.SubString(open_bracket+1, close_bracket-1);


	// The document id portion of a document name may either
	// be part of the attribute list (i.e., within the brackets)
	// or may follow the attribute list.  Check for the latter case here.
	//
	doc_id = namestr.SubString(close_bracket+1, END_OF_STRING);
	doc_id = STRING::CleanUp(doc_id);


	// Now parse the attributes in the abname's attribute list.
	//
	if (attrs.Count() < 1) {

		// No attributes in list: syntax error.
		//
		return(STATUS_FAILED);

	} else if (attrs.Count() == 1  &&  attrs[0].Index('=') < 0) {

		// A single attribute *without* an equals sign.
		// Assume this is an old style 'document id',
		// e.g., "<FOOBOOK>1006".
		// In addition, if the docid portion is null
		// (e.g., "<FOOBOOK>"), assume this is the root name
		// of the book and set the docid portion of the docname
		// to be the bookid (e.g., "<FOOBOOK>FOOBOOK"
		//
		attr_list.Add(DNATTR_BOOK_ID + "=" + attrs[0]);
		if (doc_id == NULL_STRING)
			attr_list.Add(DNATTR_DOC_ID + "=" + attrs[0]);
		else
			attr_list.Add(DNATTR_DOC_ID + "=" + doc_id);

	} else for (i = 0; i < attrs.Count(); i++) {

		// Add each attribute to the attribute list
		// after first verifying its basic syntax.
		//
		if (attrs[i].Index('=')  <  0)
			return(STATUS_FAILED);

		attr_list.Add(attrs[i]);
	}


	return(STATUS_OK);
}

// Determine whether STRING 'id' is a valid ABNAME, BOOKNAME, or DOCNAME id.
// The following criteria must be met:
//
//	o Contains only alphanumeric characters, underscores, or periods
//	o Contains at least one alphanumeric character
//
// XXX	"strict" flag is no longer used - there is now only one valid
// XXX	character set for document ids.
//
BOOL
IsValidId(const STRING &id, BOOL /*strict*/)
{
	int			len;
	int			alphanumeric = 0;
	int			i;
//XXX	static const STRING	ID_CHARS_STRICT("._");
//XXX	static const STRING	ID_CHARS_LAX   ("._;-");
	static const STRING	ID_CHARS       ("._-+");


	// Check for zero-length.
	//
	if ((len = id.Length()) == 0)
		return(BOOL_FALSE);


	for (i = 0; i < len; i++) {

		if (isalnum(id[i])) {
			++alphanumeric;
		} else if (strchr(ID_CHARS, id[i])  !=  NULL) {
			// nada...
		} else {
			return(BOOL_FALSE);
		}
	}


	// Check for at least one alphanumeric.
	//
	if (alphanumeric == 0)
		return(BOOL_FALSE);

	return(BOOL_TRUE);
}
