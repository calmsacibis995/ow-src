#ifndef	_PATHLIST_H
#define	_PATHLIST_H

#ident "@(#)pathlist.h	1.11 06/11/93 Copyright 1990 Sun Microsystems, Inc."

#include <doc/token_list.h>
#include <unistd.h>	// for access(s) "mode" definitions (F_OK, etc.)


// Class PATHLIST manages filename search paths
// (colon-separated lists of filenames).
//
class	PATHLIST : public TOKEN_LIST {

    public:

	PATHLIST(const STRING &pathlist) : TOKEN_LIST(pathlist, ':')	{}
	PATHLIST()                       : TOKEN_LIST(':')		{}

	// Search path list for file.
	// Uses 'access' to determine whether file exists.
	// 'mode' is the same as for 'access(2)', e.g, R_OK, W_OK, X_OK.
	// 'mode' defaults to 'F_OK' (file exists) if not specified.
	//
	const STRING	Find(const STRING &file, int mode=F_OK);
};

#endif	_PATHLIST_H
