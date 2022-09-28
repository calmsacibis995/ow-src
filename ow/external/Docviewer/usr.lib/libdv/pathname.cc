#ident "@(#)pathname.cc	1.8 06/11/93 Copyright 1991 Sun Microsystems, Inc."


#include <doc/pathname.h>
#include <doc/token_list.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>		/* MAXPATHLEN */
#include <string.h>
#ifndef SVR4
#include <strings.h>
#endif SVR4
#include <ctype.h>
#include <pwd.h>


const STRING	ROOT_PATHNAME("/");
const STRING	DOT_PATHNAME(".");
const STRING	DOTDOT_PATHNAME("..");
const STRING	TMPMNT_PATHNAME("tmp_mnt");
const STRING	NULL_PATHNAME;


// Get path's tail (last component of pathname).
//
const STRING &
PATHNAME::BaseName(const STRING &path, STRING &base)
{
	int		slash;

	if (path == DOT_PATHNAME  ||  path == DOTDOT_PATHNAME)
		base = NULL_STRING;
	else if ((slash = path.RightIndex('/'))  <  0)
		base = path;
	else
		base = path.SubString(slash+1, END_OF_STRING);

	DbgFunc("PATHNAME::BaseName: " << base << endl);
	return(base);
}

// Get path's head (everything but the tail).
//
const STRING &
PATHNAME::DirName(const STRING &path, STRING &dir)
{
	int		slash = path.RightIndex('/');

	if (path == DOT_PATHNAME  ||  path == DOTDOT_PATHNAME)
		dir = path;
	else if (slash < 0)
		dir = DOT_PATHNAME;
	else if (slash == 0)
		dir = ROOT_PATHNAME;
	else
		dir = path.SubString(0, slash-1);

	DbgFunc("PATHNAME::DirName: " << dir << endl);
	return(dir);
}

// Extract the suffix from a file name into "suffix".
// Suffix does NOT include the ".".
// Returns reference to "suffix", or NULL_STRING if there is no suffix.
//
const STRING &
PATHNAME::Suffix(const STRING &path, STRING &suffix)
{
	int	dot;
	int	slash;
	int	len;
	STRING	tmppath;


	if ((slash = path.RightIndex('/'))  >=  0)
		tmppath = path.SubString(slash+1, END_OF_STRING);
	else
		tmppath = path;

	dot   = tmppath.RightIndex('.');
	len   = tmppath.Length();

	if (dot > 0  && dot < len-1)
		suffix = tmppath.SubString(dot+1, END_OF_STRING);
	else
		suffix = NULL_STRING;


	DbgFunc("PATHNAME::Suffix: " << path << " = " << suffix << endl);
	return(suffix);
}

// Determine whether path is absolute (starts with "/") or
// relative.  "dot relative" means relative to current or parent
// directory ("./" or "../").
//
BOOL
PATHNAME::IsRelative(const STRING &path)
{
	if (IsAbsolute(path))
		return(BOOL_FALSE);
	else
		return(BOOL_TRUE);
}

BOOL
PATHNAME::IsAbsolute(const STRING &path)
{
	return(path[0] == '/'  ?  BOOL_TRUE  :  BOOL_FALSE);
}

// Expand ~'s, environment variables, etc., in a path.
// Returns a reference to the second argument, 
// which contains the result.
//
const STRING &
PATHNAME::Expand(const STRING &path, STRING &expanded)
{
	STRING		tilde;
	STRING		dollar;
	STRING		abs;
	STRING		tmppath(path);
	STRING		&pathref = tmppath;

	if (pathref[0] == '~') {
		ExpandTilde(pathref, tilde);
		pathref = tilde;
	}

	if (pathref.Index('$') >= 0) {
		ExpandDollar(pathref, dollar);
		pathref = dollar;
	}

	if (IsRelative(pathref)) {
		GetAbsolute(pathref, abs);
		pathref = abs;
	}

	return(CleanUp(pathref, expanded));
}

// Expand path beginning with '~'.
//
const STRING &
PATHNAME::ExpandTilde(const STRING &path, STRING &expanded)
{
	STRING		home;		// home directory path
	int		slash;		// index of '/' character within path
	STRING		user;		// user name: for resolving "~user"
	STRING		the_rest;	// remainder of path after "~user"
	struct passwd	*pw;		/* Password file entry */


	assert(path != NULL_STRING);
	DbgFunc("PATHNAME::Expand: " << path << endl);


	if (path[0] != '~')
		return(expanded = path);


	// Separate path into "~user" and the rest.
	//
	if ((slash = path.Index('/'))  >= 0) {
		user     = path.SubString(1, slash-1);
		the_rest = path.SubString(slash+1, END_OF_STRING);
	} else {
		user     = path.SubString(1, END_OF_STRING);
	}


	// Resolve tilde syntax.
	//
	if (user.Length() == 0) {

		// My home directory.
		// XXX $HOME isn't always set correctly -
		// XXX should get pwname for current uid
		//
		home = getenv("HOME");

	} else {

		// Somebody else's home directory.
		//
		if ((pw = getpwnam(user))  !=  NULL)
			home = pw->pw_dir;
		else
			home = "~" + user;
	}


	if (the_rest != NULL_STRING)
		expanded = home + "/" + the_rest;
	else
		expanded = home;

	return(expanded);
}

// Expand path containing embedded environment variables.
//
const STRING &
PATHNAME::ExpandDollar(const STRING &path, STRING &expanded)
{
	char		*varname;		// name of env variable
	STRING		value;			// value of env variable
	char		*expbuf;
	const char	*pathp = path;
	int		len;
	int		bn = 0;
	int		vn = 0;
#define	isvarchar(c)	(isalnum(c) || (c) == '_')


	assert(path != NULL_STRING);
	DbgFunc("PATHNAME::ExpandDollar: " << path << endl);


	len = path.Length();
	expbuf  = new char [len+1];
	varname = new char [len+1];

	expanded = NULL_STRING;


	while (*pathp) {

		if (*pathp != '$') {
			expbuf[bn++] = *pathp++;

		} else {

			for (pathp++; *pathp; pathp++) {
				if (isvarchar(*pathp))
					varname[vn++] = *pathp;
				else
					break;
			}

			expbuf[bn]  = '\0';
			varname[vn] = '\0';

			expanded += expbuf;

			if ((value = getenv(varname))  !=  NULL_STRING) {
				expanded += value;
			} else {
				expanded += "$";
				expanded += varname;
			}

			bn = vn = 0;
		}
	}

	expbuf[bn] = '\0';
	expanded  += expbuf;
		
	delete(expbuf);
	delete(varname);

	DbgFunc("PATHNAME::ExpandDollar: " << expanded << endl);

	return(expanded);
}

// Resolve a relative path into an absolute path.
//
const STRING &
PATHNAME::GetAbsolute(const STRING &path, STRING &abs)
{
	STRING		cwd;		// current working directory
	char		cwdbuf[MAXPATHLEN+1];

	assert(path != NULL_STRING);
	assert(IsRelative(path));
	DbgFunc("PATHNAME::GetAbsolute: " << path << endl);

	cwd = getcwd(cwdbuf, MAXPATHLEN);

	if (path[0] == '~'  ||  cwd == NULL_STRING)
		return(abs = path);

	return(abs = cwd + "/" + path);
}

const STRING &
PATHNAME::CleanUp(const STRING &path, STRING &clean)
{
	TOKEN_LIST	parts(path, '/');	// individual path components
	int		i;


	assert(path != NULL_STRING);
	DbgFunc("PATHNAME::CleanUp: " << path << endl);


	// Get rid of tacky "/tmp_mnt" prefix on automounted paths.
	//
	if (parts.Count() > 0  &&  parts[0] == TMPMNT_PATHNAME)
		parts.Delete(0);


	// Go through path components tossing out redundant
	// "." and ".." entries.
	//
	for (i = 0; i < parts.Count(); i++) {

		if (parts[i] == "."  ||  parts[i] == NULL_STRING) {
			parts.Delete(i--);

		} else if (parts[i] == "..") {
			parts.Delete(i--);
			if (i >= 0)
				parts.Delete(i--);
		}
	}


	// Reassemble path.
	//
	if (IsAbsolute(path))
		clean = "/";
	else
		clean = NULL_STRING;

	for (i = 0; i < parts.Count(); i++) {
		if (i > 0)
			clean += "/";
		clean += parts[i];
	}

	return(clean);
}
