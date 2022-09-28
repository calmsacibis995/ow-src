#ifndef	_AB_STRING_H
#define	_AB_STRING_H

#ident	"@(#)string.h	1.49 93/02/12 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <string.h>

// Forward references.
//
class	XDR;


// Generic STRING object.
// 
// Implementation notes:
//
//	o The only direct access to the internal representation
//	  (char *) of a STRING is via the (const char *) casting
//	  operator.  This was done intentionally to prevent modification
//	  of the internal representation outside of the STRING package.
//
//	o STRING assignment operators return a reference to the lhs STRING
//	  object.  This is consistent with the behavior of assignment
//	  operators for builtin data types, and allows constructs such as:
//
//		string3 = string2 = string1 = "hello, world";
//
//	o All other (non-assignment) operators and methods that
//	  return STRINGs (e.g., "+", "SubString()", "CleanUp()")
//	  return an actual STRING object, rather than a reference
//	  to a STRING object.  This causes the compiler to generate
//	  a unique temporary STRING object for each invocation of
//	  the respective operator or method.  The scope of that
//	  temporary is expression in which it is used.  The compiler
//	  automatically deletes the temporary STRING object once it
//	  leaves the scope of the expression.  This allows constructs such as:
//
//		my_string = STRING::CleanUp(foo_string.SubString(1,5)) +
//			    STRING::CleanUp(foo_string.SubString(6,10)) +
//			    STRING::CleanUp(bar_string + zot_string);
//

extern "C" {
	// Why isn't this declared in /usr/include/string.h?
	int	strcasecmp(const char *s1, const char *s2);
};


// Polite implementation of "strcmp(3)".
// Won't dump core if either or both arguments are NULL.
//
inline int
StrCmp(const char *a, const char *b)
{
	DbgNit("StrCmp: s1='" << a << "' s2='" << b << "'" << endl);

	if (!a && !b)
		return(0);
	else if (!a)
		return(-1);
	else if (!b)
		return(1);
	else
		return(strcmp(a, b));
}

// Polite implementation of "strcasecmp(3)".
// Won't dump core if either or both arguments are NULL.
//
inline int
StrCaseCmp(const char *a, const char *b)
{
	DbgNit("StrCaseCmp: s1='" << a << "' s2='" << b << "'" << endl);

	if (!a && !b)
		return(0);
	else if (!a)
		return(-1);
	else if (!b)
		return(1);
	else
		return(strcasecmp(a, b));
}

// Polite implementation of "strcpy(3)".
// Won't dump core if either or both arguments are NULL.
//
inline char *
StrCpy(char *dst, const char *src)
{
	DbgNit("StrCpy: src='" << src << "'" << endl);

	if (!dst)
		return(dst);
	else if (!src)
		return(strcpy(dst, ""));
	else
		return(strcpy(dst, src));
}

// Polite implementation of "strlen(3)".
// Won't dump core if either or both arguments are NULL.
//
inline int	StrLen(const char *s)
{
	DbgNit("StrLen: '" << s << "'" << endl);
	return((s) ? strlen(s) : 0);
}

// Polite implementation of "strstr(3)"
// Won't dump core if either or both arguments are NULL
//

inline const char *
StrStr(const char *str, const char *substr)
{
	DbgNit("StrStr: str='" << str << "' substr='" << substr << "'" <<
	       endl);

	if (!str)
		return(NULL);
	else if (!substr)
		return(str);
	else
		return(strstr(str, substr));
}


class	STRING_REP 
{
	char		*string;
	int		refcount;

	STRING_REP(const char *);
	STRING_REP(const char *, const char *);
	~STRING_REP();

#ifdef	DEBUG
	OBJECT_STATE	objstate;
#endif	DEBUG

	friend class	STRING;
};


class	STRING {

    private:

	STRING_REP	*r;			// internal representation

	// XDR encode/decode routines.
	//
	int		XdrEncode(XDR *);
	int		XdrDecode(XDR *);

	// Private constructor used by "+" operator.
	//
	STRING(const char *, const char *);

#ifdef	DEBUG
	OBJECT_STATE	objstate;
#endif	DEBUG
	BOOL		IsReady() const
			{
				assert(objstate.IsReady());
				assert(r != NULL);
				assert(r->objstate.IsReady());
				return(BOOL_TRUE);
			}


    public:

	STRING();
	STRING(const char *);
	STRING(const STRING &);
	~STRING();


	// String length.
	//
	int		Length() const		{ return(StrLen(r->string)); }


	// "assignment" operators of various types
	//
	STRING		&operator  = (const STRING &);	// STRING  = STRING;
	STRING		&operator  = (const char *);	// STRING  = char *;

	STRING		&operator += (const STRING &);	// STRING += STRING;
	STRING		&operator += (const char *);	// STRING += char *;


	// Cast STRING to "const char *".
	//
	operator	const char *() const	{ return(r->string); }


	// Handy shortcut for getting the "const char *" value of a STRING.
	// Returns STRING value, or "(null)" if value is NULL.
	// Particularly useful for passing STRING as argument to routines
	// such as printf() that choke when passed a NULL pointer:
	//	STRING foo;
	//	printf("value of foo is %s", ~foo);
	// And yes, I know that overloading the "~" operator like this
	// is a hack, but what the hey.
	//
	const char	*operator ~ () const
				{ return(r->string ? r->string : ""); }


	// "plus" (concatenate) operator with various operand combinations
	//
	friend const STRING
	operator + (const STRING &s1, const STRING &s2)	// STRING + STRING
		{ STRING tmp(s1, s2); return(tmp); }

	friend const STRING
	operator + (const STRING &s1, const char *s2)	// STRING + char*
		{ STRING tmp(s1, s2); return(tmp); }

	friend const STRING
	operator + (const char *s1, const STRING &s2)	// char* + STRING
		{ STRING tmp(s1, s2); return(tmp); }


	// "less than" operator with various operand combinations
	//
	friend int
	operator <  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  <  0); }

	friend int
	operator <  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  <  0); }

	friend int
	operator <  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  <  0); }



	// "greater than" operator with various operand combinations
	//
	friend int
	operator >  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  >  0); }

	friend int
	operator >  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  >  0); }

	friend int
	operator >  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  >  0); }



	// "greater than or equal to" operator with various operand combos

	friend int
	operator >=  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  >=  0); }

	friend int
	operator >=  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  >=  0); }

	friend int
	operator >=  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  >=  0); }



	// "less than or equal to" operator with various operand combos
	//
	friend int
	operator <=  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  <=  0); }

	friend int
	operator <=  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  <=  0); }

	friend int
	operator <=  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  <=  0); }



	// "equal to" operator with various operand combos
	//
	friend int
	operator ==  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  ==  0); }

	friend int
	operator ==  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  ==  0); }

	friend int
	operator ==  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  ==  0); }


	// "not equal to" operator with various operand combos
	//
	friend int
	operator !=  (const STRING &s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  !=  0); }

	friend int
	operator !=  (const char *s1, const STRING &s2)
		{ return(StrCmp(s1, s2)  !=  0); }

	friend int
	operator !=  (const STRING &s1, const char *s2)
		{ return(StrCmp(s1, s2)  !=  0); }


	// Find the first/last occurence of a character in a STRING.
	// Returns index of character, or -1 if not character not in string.
	// These routines serve the same function as index()/rindex() and
	// strchr()/strrchr().
	//
	int		Index(char) const;
	int		RightIndex(char) const;


	// Return the substring defined by the range <first_char, last_char>.
	// If last_char exceeds the string length, substring just extends
	// to the end of the string.
	//
	const STRING	SubString(int first_char, int last_char) const;
#define	END_OF_STRING	0x7fffffff	// should use MAXINT


	// Get character from string.
	//
	char		operator [] (int index) const
			{
				assert(r->string != NULL);
				assert(index >= 0  &&  index <= Length());
				return(r->string[index]);
			}

	// Is string composed soley of space characters?
	//
	BOOL		IsBlank() const;


	// Strip leading and trailing blanks from string.
	//
	static const STRING	CleanUp(const STRING &dirty);


	// XDR- conversion
	//
	friend int	xdr_STRING(XDR *, STRING *);


	// Print string value.
	//
	friend ostream	&operator <<  (ostream &ostr, const STRING &s)
			{ return(ostr << (const char *) s); }
};


// NULL string - generically useful (and usable) by anyone
// as return value, etc.
//
extern const STRING	NULL_STRING;

#endif	_AB_STRING_H
