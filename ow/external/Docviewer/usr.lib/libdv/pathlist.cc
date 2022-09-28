#ident "@(#)pathlist.cc	1.10 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/pathlist.h>


const STRING
PATHLIST::Find(const STRING &file, int mode)
{
	STRING	path;
	int	i;


	if (file == NULL_STRING)
		return(NULL_STRING);


	// If file is an "absolute" path name (begins with /, ./, or ../),
	// we don't need to do any searching.
	//
	if ((file[0] == '/')						||
	    (file[0] == '.'  &&  file[1] == '/')			||
	    (file[0] == '.'  &&  file[1] == '.'  &&  file[2] == '/')) {

		DbgFunc("PATHLIST::Find: absolute path: " << file << endl);
		if (access(file, mode)  ==  0)
			return(path = file);
		else
			return(NULL_STRING);
	}


	// Look for file in each directory in path list.
	//
	for (i = 0; i < Count(); i++) {

		if ((path = (*this)[i])  ==  NULL_STRING)
			continue;

		path += "/" + file;
		DbgFunc("PATHLIST::Find: checking: " << path << endl);
		if (access(path, mode)  ==  0)
			return(path);
	}


	// If we didn't find the file,
	// look for it relative to the current directory.
	// XXX - is this the proper semantics?
	//	We're effectively adding "." to the path automatically -
	//	should we just give up instead?
	//
	DbgFunc("PATHLIST::Find: not in path, current dir?: " << file << endl);
	if (access(file, mode)  ==  0)
		return(file);
	else
		return(NULL_STRING);
}
