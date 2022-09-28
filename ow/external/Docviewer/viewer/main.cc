#ident "@(#)main.cc	1.103 26 May 1994 Copyright 1989-1992 Sun Microsystems, Inc."

#include <locale.h>
#include <new.h>
#include <stdlib.h>
#include <sys/param.h>
#include <doc/abgroup.h>
#include <doc/abclient.h>
#include <doc/token_list.h>
#include <doc/utils.h>
#include <xview/xview.h>
#include "psviewer.h"
#include "common.h"


// Global variables
ABGROUP		*abgroup = NULL;
CONSOLE		console;		// how to print error messages
NOTIFY		*notify;
STRING		pgmname;
int		debug = 0;
STRING		docviewer_version("3.6 FCS");

// Text domain names for localization of error messages, etc.
// Do not change these names - they are registered with the text domain name
// registry (textdomain@Sun.COM).
//
static const STRING	HELP_DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_HELPVIEWER");
static const STRING	VIEW_DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_DOCVIEWER");

// Functions called
void	Init(char		**argv,
	     ViewerType		*vtype);

void	Usage(ViewerType	vtype);


main(int argc, char **argv)
{
	ViewerType	vtype;
	ERRSTK		err;
	VIEWER		*viewer;
	STATUS		status = STATUS_OK;

	// Initialize some globals
	Init(argv, &vtype);

	// Create viewer and initialize it
	viewer = new PSVIEWER(vtype);

	status = viewer->Init(&argc, argv, err);

	// Returned from the notifier loop clean up after ourselves
	// and go away
	//

	if (abgroup != NULL) {
	   abgroup->DestroyCardCatalogs ();
	   delete abgroup;
	   }

	return(status == STATUS_OK ? 0 : 1);
}

void
Init(char **argv, ViewerType *vtype)
{
	STRING		domainname;
	int		slash;
	extern void	NewHandler();


	// Initialize new handler
	set_new_handler(NewHandler);

	pgmname = argv[0];

	// Get the basename of argv[0]
	//
	if ((slash = pgmname.RightIndex('/')) >= 0) {
		pgmname = pgmname.SubString(slash + 1, END_OF_STRING);
	}

	// Decide whether this is a HelpViewer or a DocViewer based on
	// the name of our executable
	//
	if (pgmname == "helpviewer") {
		*vtype = HELPVIEWER;
		domainname = HELP_DOMAINNAME;
	} else {
		*vtype = DOCVIEWER;
		domainname = VIEW_DOMAINNAME;
	}


	// Set up for localization.
  	//
	InitTextDomain(domainname);


	console.Init(pgmname);
}

void
Usage()
{
	if (pgmname == "helpviewer") {
		fprintf(stderr,
			gettext("usage: %s -f help-handbook-file\n"), 
			~pgmname);
	}
	else {
		fprintf(stderr,  
                        gettext("usage: %s -d document-name [-p tooltalk-procid] [-c card-catalog-file]\n"),  ~pgmname);
	}
}


char*
make_help_message(char* s)
{
	int	length;
	char	*str;

	if (pgmname == "helpviewer") {
		length = strlen(~pgmname) + strlen(s) + 2;
		str = new char [length];
		sprintf(str, "%s:%s", ~pgmname, s);
	}
	else {
		length = strlen("viewer") + strlen(s) + 2;
		str = new char [length];
		sprintf(str, "%s:%s", "viewer", s);
	}
	return str;
}
