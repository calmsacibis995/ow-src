#ident "@(#)utils.cc	1.6 93/02/24 Copyright 1992-1993 Sun Microsystems, Inc."


#include <doc/pathname.h>
#include <doc/utils.h>
#include "dvlocale.h"


// Read a "logical line" from a file into "linestr".
// Handles arbitrarily long lines.
// A line beginning with "#" is treated as a comment and ignored.
// A line ending with "\" is concatenated with the subsequent line.
//
// Returns reference to "linestr", or NULL if no more lines.
//
const STRING &
GetLine(FILE *fp, STRING &linestr)
{
#define	LINEBUFSIZE	200
	char		linebuf[LINEBUFSIZE];
	int		len;
	BOOL		gotit = BOOL_FALSE;


	assert(fp != NULL);

	linestr = NULL_STRING;


	// Assemble logical line, skipping comments, etc.
	//
	while ( ! gotit) {

		// First, fetch a complete 'physical' line.
		// If line is longer than LINEBUFSIZE, this will
		// take multiple iterations through this loop.
		//
		while (fgets(linebuf, LINEBUFSIZE, fp)  !=  NULL) {

			// Add this chunk to the line.
			//
			linestr += linebuf;

			// If we just read a newline, we're done.
			// Chuck the newline.
			//
			len = linestr.Length();
			if (linestr[len-1] == '\n') {
				linestr = linestr.SubString(0, len-2);
				break;
			}
		}


		// Check for read errors, end of file.
		//
		if (ferror(fp)) {
			//XXX
			return(linestr=NULL_STRING);

		} else if (feof(fp)) {
			//XXX
			return(linestr=NULL_STRING);
		}


		len = linestr.Length();

		if (len == 0  ||  linestr[0] == '#') {

			// Empty line or comment.
			// Skip it and go through the loop again.
			//
			linestr = NULL_STRING;

		} else if (linestr[len-1] == '\\') {

			// Line ends with a backslash ('\'),
			// so it's continued on the next line.
			// Delete the backslash and go through the loop again.
			//
			linestr = linestr.SubString(0, len-2);

		} else {

			// We found what we sought.
			//
			gotit = BOOL_TRUE;
		}
	}


	DbgFunc("GetLine: " << linestr << endl);
	return(linestr);
}

void
InitTextDomain(const STRING &domain_name)
{
	STRING	bindDir;

	assert(domain_name != NULL_STRING);
	DbgFunc("InitTextDomain: " << domain_name << endl);

	PATHNAME::Expand("$OPENWINHOME/lib/locale", bindDir);
	bindtextdomain(domain_name, bindDir);
	bindtextdomain(LIBDV_DOMAIN_NAME, bindDir);
  	textdomain(domain_name);
}
