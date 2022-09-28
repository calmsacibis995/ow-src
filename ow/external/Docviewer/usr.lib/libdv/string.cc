#ident "@(#)string.cc	1.46 93/02/05 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/string.h>
#include <rpc/rpc.h>
#include <ctype.h>


// Handy debugging print macro for STRINGs
//
#define	DbgString(s)	DbgNit(s << ": " << this->r->string \
				 << " (" << this->r->refcount << ")" << endl)


const STRING	NULL_STRING;	// generically useful


// STRING_REP constructor.
//
STRING_REP::STRING_REP(const char *s)
{
	if (s == NULL) {
		string = NULL;
	} else {
		string = new char[strlen(s) + 1];
		strcpy(string, s);
	}
	
	refcount = 1;

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

// STRING_REP constructor (two strings).
//
STRING_REP::STRING_REP(const char *s1, const char *s2)
{
	if (s1 == NULL  &&  s2 == NULL) {
		string = NULL;

	} else if (s1 == NULL) {
		string = new char[strlen(s2) + 1];
		strcpy(string, s2);

	} else if (s2 == NULL) {
		string = new char[strlen(s1) + 1];
		strcpy(string, s1);

	} else {
		string = new char[strlen(s1) + strlen(s2) + 1];
		strcpy(string, s1);
		strcat(string, s2);
	}
	
	refcount = 1;

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

// STRING_REP destructor.
//
STRING_REP::~STRING_REP()
{
	assert(refcount == 0);

	if (string != NULL)
		delete(string);
}

STRING::STRING()
{
	r =  new STRING_REP(NULL);
	DbgString("STRING::STRING()");

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

STRING::STRING(const char *s)
{
	r = new(STRING_REP)(s);
	DbgString("STRING::STRING(char*)");

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

STRING::STRING(const char *s1, const char *s2)
{
	r = new(STRING_REP)(s1, s2);
	DbgString("STRING::STRING(char*,char*)");

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

STRING::STRING(const STRING &s)
{
	assert(s.IsReady());

	r = s.r;
	r->refcount++;
	DbgString("STRING::STRING(STRING &)");

#ifdef	DEBUG
	objstate.MarkReady();
#endif	DEBUG
}

STRING::~STRING()
{
	assert(IsReady());
	DbgString("STRING::~STRING");

	if (--r->refcount == 0)
			delete(r);
}

// STRING assignment operator: STRING = STRING
// Returns reference to lhs STRING.
//
STRING &
STRING::operator =(const STRING &s)
{
	assert(IsReady());
	assert(s.IsReady());

	if (--r->refcount == 0)
		delete(r);
	r = s.r;
	r->refcount++;

	DbgString("STRING operator = STRING &");

	return(*this);
}

// STRING assignment operator: STRING = char*
// Returns reference to lhs STRING.
//
STRING &
STRING::operator =(const char *s)
{
	assert(IsReady());

	if (--r->refcount == 0)
		delete(r);

	r = new(STRING_REP)(s);

	DbgString("STRING operator = char*");

	return(*this);
}

// STRING assignment operator: STRING += STRING
// Returns reference to lhs STRING.
//
STRING &
STRING::operator +=(const STRING &s)
{
	STRING_REP	*new_r;
	
	assert(IsReady());
	assert(s.IsReady());

	// Create new string_rep that is is a concatenation of the two strings
	//
	new_r = new(STRING_REP)(r->string, s.r->string);

	// Replace our old string_rep with the new one,
	// deleting the old one if we're the last reference to it.
	//
	if (--r->refcount == 0)
		delete(r);
	r = new_r;

	DbgString("STRING operator += STRING &");

	return(*this);
}

// STRING assignment operator: STRING += char*
// Returns reference to lhs STRING.
//
STRING &
STRING::operator +=(const char *s)
{
	STRING_REP	*new_r;
	
	assert(IsReady());

	// Create new string_rep that is is a concatenation of the two strings
	//
	new_r = new(STRING_REP)(r->string, s);

	// Replace our old string_rep with the new one,
	// deleting the old one if we're the last reference to it.
	//
	if (--r->refcount == 0)
		delete(r);
	r = new_r;

	DbgString("STRING operator += char*");

	return(*this);
}

// Find the first occurence of a character in a STRING.
// Returns index of character, or -1 if not character not in string.
// This routine serves the same function as index()/strchr().
//
int
STRING::Index(char c) const
{
	char	*s = this->r->string;	// internal string representation
	char	*cp;
	int	n;
	
	assert(IsReady());

	if (s == NULL  ||  (cp = strchr(s, c))  ==  NULL)
		n = -1;
	else
		n = (int)(cp - s);

	DbgNit("STRING::Index: " << *this << "[" << c << "]==" << n << endl);
	return(n);
}

// Find the last occurence of a character in a STRING.
// Returns index of character, or -1 if not character not in string.
// This routine serves the same function as rindex()/strrchr().
//
int
STRING::RightIndex(char c) const
{
	char	*s = this->r->string;	// internal string representation
	char	*cp;
	int	n;
	
	assert(IsReady());

	if (s == NULL  ||  (cp = strrchr(s, c))  ==  NULL)
		n = -1;
	else
		n = (int)(cp - s);

	DbgNit("STRING::RightIndex: " << *this << "[" << c << "]==" <<n<<endl);
	return(n);
}

// Return the substring defined by the range <first_char, last_char>.
// If last_char exceeds the string length, substring just extends
// to the end of the string.
//
const STRING
STRING::SubString(int first_char, int last_char) const
{
	char	*s = this->r->string;	// internal string rep.
	int	len = Length();		// length of internal string
	int	substring_len;		// substring length
	STRING	substring;		// substring itself
	
	assert(IsReady());

	// Handle boundary conditions.
	//
	if (first_char >= len	||
	    first_char <  0	||
	    first_char >  last_char) {
		DbgString("STRING::SubString: null substring");
		substring = NULL_STRING;
		return(substring);
	}

	if (last_char > len-1)
		last_char = len-1;
	substring_len = last_char - first_char + 1;


	// Create substring.
	//
	substring = &s[first_char];
	substring.r->string[substring_len] = '\0';

	DbgNit("STRING::SubString: "	<< *this
			<< "[" << first_char << "," << last_char << "]"
			<< "=='" << substring << "'"
			<< endl);
	return(substring);
}

// Convert STRING to XDR stream.
// See XDR section in "Network Programming" manual for details.
//
bool_t
STRING::XdrEncode(XDR *xdrs)
{
	int	len;
	
	assert(IsReady());
	assert(xdrs->x_op == XDR_ENCODE);
	DbgString("STRING::XdrEncode");
	
	len = (r->string  ?  Length() + 1  :  0);

	if ( ! xdr_int(xdrs, &len))
		return(FALSE);
	if (len > 0  &&  !xdr_string(xdrs, &r->string, len))
		return(FALSE);

	return(TRUE);
}


// Convert XDR stream to STRING.
// See XDR section in "Network Programming" manual for details.
//
bool_t
STRING::XdrDecode(XDR *xdrs)
{
	STRING_REP	*new_r;
	int		len;
	
	assert(IsReady());
	assert(xdrs->x_op == XDR_DECODE);
		
	// Create new (empty) string_rep.
	// Replace the old string_rep with the new one,
	// deleting the old one if we're the last reference to it.
	//
	if ((new_r = new STRING_REP(NULL))  ==  NULL)
		return(FALSE);
	if (--r->refcount == 0)
		delete(r);
	r = new_r;

	// Get the string length from the xdr stream.
	// if non-zero (string is not NULL), allocate
	// memory for the new string and get it from
	// the xdr stream.
	//
	if ( ! xdr_int(xdrs, &len))
		return(FALSE);

	if (len > 0) {
		if ((r->string = new char[len])  ==  NULL)
			return(FALSE);
		if ( ! xdr_string(xdrs, &r->string, len))
			return(FALSE);
	}


	DbgString("STRING::XdrDecode");
	return(TRUE);
}

// XDR filter for STRINGs
//
bool_t
xdr_STRING(XDR *xdrs, STRING *s)
{
	switch (xdrs->x_op) {

	case XDR_ENCODE:
		return(s->XdrEncode(xdrs));

	case XDR_DECODE:
		return(s->XdrDecode(xdrs));

	case XDR_FREE:
		break;
	}

	return(TRUE);
}

BOOL
STRING::IsBlank() const
{
	int	len = Length();
	int	i;

	assert(IsReady());

	for (i = 0; i < len; i++)
		if ( ! isspace((u_char)((*this)[i]))) {
			DbgString("STRING::IsBlank (no)");
			return(BOOL_FALSE);
		}

	DbgString("STRING::IsBlank (yes)");
	return(BOOL_TRUE);
}

// Strip leading and trailing blanks from string.
//
const STRING
STRING::CleanUp(const STRING &dirty)
{
	int	len;
	int	start, end;

	assert(dirty.IsReady());
	DbgNit("STRING::CleanUp: '" << dirty << "'" << endl);

	if ((len = dirty.Length()) == 0)
		return(NULL_STRING);

	start = 0;
	while (dirty[start] != '\0'  &&  isspace((u_char)dirty[start]))
		++start;

	if (start >= len)
		return(NULL_STRING);

	end = len - 1;
	while (end >= 0  &&  isspace((u_char)dirty[end]))
		--end;

	assert(end >= start);

	return(dirty.SubString(start, end));
}
