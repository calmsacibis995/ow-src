#ident "@(#)main.cc	1.93 94/01/14 Copyright 1990-1992 Sun Microsystems, Inc."

#include "navigator.h"
#include "xview.h"
#include <doc/bookshelf.h>
#include <doc/cardcats.h>
#include <doc/utils.h>
#include <locale.h>
#include <sys/time.h>		// needed for set/getrlimit()
#include <sys/resource.h>	// needed for set/getrlimit()


// Global variables.
//
NAVIGATOR	*navigator = NULL;
BOOKSHELF	*bookshelf = NULL;
STRING		bookshelf_path;
int		debug = 0;		// debugging level


// Text domain name for localization of error messages, etc.
// Do not change this name - it is registered with the text domain name
// registry (textdomain@Sun.COM).
//
static const STRING	DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_NAVIGATOR");


static STRING	pgmname;
static void	Usage();
static void	CleanUp();


main(int argc, char **argv)
{
	STRING		language;
	STRING		cclist;		// list of card catalogs to use
	ERRSTK		err;
	extern char	*optarg;
	extern int	optind;
	int		c;
	int		slash;
	STRING		filename;
	void		set_new_handler(void (*)());


	// Get name of this program.
	//
	pgmname = argv[0];
	if ((slash = pgmname.RightIndex('/'))  >=  0)
		pgmname = pgmname.SubString(slash+1, END_OF_STRING);


	// Set up for localization.
  	//
	InitTextDomain(DOMAINNAME);


	// Handle "out of memory" condition.
	//
	set_new_handler(OutOfMemory);


	// Initialize this Navigator session.
	//
	navigator = new NAVIGATOR();

	if (navigator->Init(&argc, argv, err)  !=  STATUS_OK) {
		cerr << err;
		return(1);
	}


	// Process command line arguments.
	//
	while ((c = getopt(argc, argv, "b:c:l:f:x:")) != EOF) {

		switch (c) {

		case 'l':
			language = optarg;
			break;

		case 'b':
			bookshelf_path = optarg;
			break;

		case 'c':
			cclist = optarg;
			break;

		case 'f':
			filename = optarg;
			break;

#ifdef	DEBUG
		case 'x':
			debug = atoi(optarg);
			break;
#endif	DEBUG
		default:
			Usage();
			return(1);
		}
	}

	if (optind != argc) {
		Usage();
		return(1);
	}


	// If prefererd language was not specified on the command line,
	// check the locale.
	//
	if (language == NULL_STRING) {
		language = getenv("LANG");
		if (language == NULL_STRING)
			language = setlocale(LC_MESSAGES, NULL);
	}

	navigator->SetPreferredLanguage(language);


	// If additional card catalogs were specified on the command line,
	// pass them on.  Also set up to use the default card catalogs.
	//
	CARDCATS &cardcats = navigator->GetCardCatalogs();
	if (cclist != NULL_STRING) {
		if (cardcats.Append(cclist, err) != STATUS_OK)
			cerr << err;
	}

	if (cardcats.AppendDefaults(err) != STATUS_OK)
		cerr << err;


	// Bookshelf file defaults to "$HOME/.ab_library".
	//
	if (bookshelf_path == NULL_STRING)
		BOOKSHELF::GetDefaultPath(bookshelf_path);


	// XXX - we've had problems running out of file descriptors.
	// XXX   This is a hack to work around the problem for now.
	//
	struct rlimit	rlbuf;
	rlim_t		nofile;

	if (getrlimit(RLIMIT_NOFILE, &rlbuf)  !=  0) {
		perror("getrlimit");
	} else {
		nofile = (rlbuf.rlim_max < 256  ?  rlbuf.rlim_max  :  256);
		if (rlbuf.rlim_cur < nofile) {
			rlbuf.rlim_cur = nofile;
			if (setrlimit(RLIMIT_NOFILE, &rlbuf)  !=  0) {
				perror("setrlimit");
			}
		}
	}
	// XXX end of hack

	// If we were started with the -f option (helios) then
	// start the viewer;

	if (filename != NULL_STRING) 
	   navigator->ViewFile (filename, err);

	// Enter event loop.
	//
	navigator->EnterEventLoop();


	CleanUp();
	return(0);
}

void
CleanUp()
{
	STRING 	tmp_cardcat_file;
	char	cmdbuf[255];

	if ((tmp_cardcat_file = getenv("AB_CCTMP")) != NULL_STRING)
		unlink(~tmp_cardcat_file);
}



void
Usage()
{
	fprintf(stderr,
		gettext("usage: %s [-b library-file] [-c card-catalog]\n"), ~pgmname);
}
