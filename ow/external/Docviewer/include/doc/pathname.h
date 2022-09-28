#ifndef	_PATHNAME_H
#define	_PATHNAME_H

#ident "@(#)pathname.h	1.6 06/11/93 Copyright 1991-1992 Sun Microsystems, Inc."


#include <doc/common.h>


// PATHNAME is not so much an object as a collection of pathname-related
// utilities.  Most of the time it's most convenient to represent pathnames
// as strings rather than as explicit pathname objects.
//
class	PATHNAME {

    private:

	// Expand ~'s in a path.
	// Returns a reference to the second argument, 
	// which contains the result.
	//
	static const STRING	&ExpandTilde(	const STRING	&path,
						STRING		&expanded);

	// Expand environment variables imbedded in a path.
	// Returns a reference to the second argument, 
	// which contains the result.
	//
	static const STRING	&ExpandDollar(	const STRING	&path,
						STRING		&expanded);

	// Resolve relative path into absolute path.
	// Returns a reference to the second argument, 
	// which contains the result.
	//
	static const STRING	&GetAbsolute(const STRING &path, STRING &abs);

	// Clean up an absolute path - remove redundant ".", "..",
	// and "/" entries.
	// Returns a reference to the second argument, 
	// which contains the result.
	//
	static const STRING	&CleanUp(const STRING &path, STRING &clean);


    public:

	// Extract path components.
	// BaseName() and DirName() are equivalent to the shell 'basename'
	// and 'dirname' commands.  DirName() returns all but the
	// last component of a path name.  BaseName() delivers the rest.
	// Suffix() gets the suffix of a path (e.g., ".c")
	// (suffix does NOT include the ".").
	//
	// Here's what they do:
	//
	//	pathname	DirName()	BaseName()
	//	========	==========	=========
	//	/foo/bar/zot	/foo/bar	zot
	//	/foo/bar	/foo		bar
	//	/foo		/		foo
	//	/		/		<empty>
	//	foo/bar		foo		bar
	//	foo		.		foo
	//	<empty>		.		<empty>
	//	. (dot)		.		.
	//	.. (dotdot)	.		..
	//
	// All three methods return a reference to the second argument,
	// which contains the result.
	//
	static const STRING	&DirName(const STRING &path, STRING &dir);
	static const STRING	&BaseName(const STRING &path, STRING &base);
	static const STRING	&Suffix(const STRING &path, STRING &suffix);

	// Determine whether path is absolute (starts with "/") or relative.
	//
	static BOOL		IsRelative(const STRING &path);
	static BOOL		IsAbsolute(const STRING &path);

	// Expand ~'s, environment variables, etc., in a path.
	// Returns a reference to the second argument, 
	// which contains the result.
	//
	static const STRING	&Expand(const STRING &path, STRING &expanded);
};

#endif	_PATHNAME_H
