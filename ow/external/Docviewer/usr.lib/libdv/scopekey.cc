#ident "@(#)scopekey.cc	1.7 93/02/15 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/scopekey.h>
#include <doc/utils.h>
#include <ctype.h>
#include "dvlocale.h"


STATUS
ReadKeysFile(const STRING &keypath, LIST<SCOPEKEY*> &keys, ERRSTK &err)
{
	FILE		*keyfp;			// Keys file pointer
	STRING		key, title;		// key, title from Keys file
	STRING		linestr;
	int		i, len;


	keys.Clear();


	// Open Keys file.
	//
	if ((keyfp = fopen(keypath, "r"))  ==  NULL) {
		err.Init(DGetText("Can't open Search Key file '%s': %s"),
			~keypath, SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	// Read each <key, title> pair.
	//
	while (GetLine(keyfp, linestr)  !=  NULL_STRING) {

		// Ignore "Collection Keywords"
		//
		if (strncmp(linestr, "Collection Keywords", 19) == 0)
			continue;

		// Look for white space separator
		//
		len = linestr.Length();
		for (i = 0; i < len  &&  ! isspace(linestr[i]); i++)
			;
		if (i == 0  ||  i == len)
			continue;

		key   = linestr.SubString(0, i-1);
		title = linestr.SubString(i, END_OF_STRING);
		title = STRING::CleanUp(title);

		if (key == NULL_STRING  ||  title == NULL_STRING)
			continue;

		keys.Add(new SCOPEKEY(key, title));
	}

	fclose(keyfp);

	if (keys.Count() == 0) {
		err.Init(DGetText("No keys in Search Key file '%s'."),~keypath);
		return(STATUS_FAILED);
	} else {
		return(STATUS_OK);
	}
}
