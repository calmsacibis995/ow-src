#ifndef	_SCOPEKEY_H
#define	_SCOPEKEY_H

#ident "@(#)scopekey.h	1.7 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/list.h>


// SCOPEKEY implements the a search scoping key object.
//
class	SCOPEKEY {

    private:

	STRING	key;
	STRING	title;
	BOOL	enabled;

    public:

	SCOPEKEY(const STRING &k, const STRING &t) :
			key(k),
			title(t),
			enabled(BOOL_FALSE)	{ }
	~SCOPEKEY()				{ }

	const STRING	&Key() const		{ return(key); }
	const STRING	&Title() const		{ return(title); }
	BOOL		IsEnabled() const	{ return(enabled); }
	void		Enable()		{ enabled = BOOL_TRUE; }
	void		Disable()		{ enabled = BOOL_FALSE; }
};


// Read the specified scoping keys file.
// Format of the file is one key entry per line as follows:
//
//	# Lines beginning with '#' are comments - they're ignored
//	# Otherwise, each line is a <key, title> pair.  The key is
//	# a single token composed of alphanumeric characters and underscores.
//	# The key and it's title are separated by one or more white space
//	# characters.  Titles may contain arbitrary text, including
//	# ISO-Latin1 characters.
//	# Here's an example:
//	USER			Sun User Documentation
//	ADMINISTRATION		Sun Administration Documentation
//	DEVELOPMENT		Sun Software Development Documentation
//	MANPAGES		SunOS Reference Manual
//	HARDWARE		Sun Hardware Documentation
//
// Note also that in AnswerBook 1.1, the first line of the Keys file
// was *always* ignored, so we'll ignore it, too.
// That line began "Collection Keywords ..."
//
STATUS	ReadKeysFile(	const STRING &keypath,
			LIST<SCOPEKEY*> &keys,
			ERRSTK &);

#endif	_SCOPEKEY_H
