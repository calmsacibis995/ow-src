#ident "@(#)viewprint.cc	1.8 94/04/11 Copyright (c) 1990-1993 Sun Microsystems, Inc."

#include "viewprint.h"
#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/cardcats.h>
#include <doc/console.h>
#include <doc/document.h>
#include <doc/dvlink.h>
#include <doc/utils.h>
#include <doc/list.h>
#include <errno.h>
#include <new.h>
#include <ctype.h>
#include <locale.h>


static int
Min(int a, int b)
{
	if (a < b)
		return(a);
	else
		return(b);
}

static int
Max(int a, int b)
{
	if (a > b)
		return(a);
	else
		return(b);
}

// Convert integer to string.
// XXX this belongs in string.h.
//
const STRING &
int2string(int n, STRING &str)
{
	char	buf[20];
	sprintf(buf, "%d", n);
	return(str = buf);
}

// Global variables
CARDCATS	cardcats;
ABGROUP		abgroup(cardcats);
CONSOLE		console;
STRING		progName;
int		debug = 0;

// Text domain name for localization of error messages, etc.
// Do not change this name - it is registered with the text domain name
// registry (textdomain@Sun.COM).
//
static const STRING	DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_VIEWPRINT");


void
NewHandler()
{
	fprintf(stderr, gettext("out of memory"));
	exit(ENOMEM);
}

STATUS
PrintDoc(PSOUT		&psout,
	 const DOCUMENT	*doc,
	 BOOL		revPages,
	 int		offset,
	 STRING		&pageRange,
	 ERRSTK		&err)
{
	LIST<STRING>	files_to_print;
	STRING		pspath;
	STRING		pfpath;
	DVLINK		print_method;
	ABCLIENT	*answerbook;
	FILE		*printfp;
	STRING		printfile;


	assert(doc != NULL);
	DbgFunc("PrintDoc: " << doc->Title() << endl);


	// Get document's print method.
	// It's either a list of files to print (with optional page ranges),
	// or its the name of a file that contains such a list.
	// In either case, all file names are interpreted relative
	// to our document's PostScript directory (pspath).
	//
	print_method.SetCookie(doc->PrintMethod());
	if (print_method.LinkType() != DVLINK_PRINT      &&
	    print_method.LinkType() != DVLINK_PRINTFILE) {
		err.Init(gettext("invalid print method for document '%s'"),
			~doc->Title());
		return(STATUS_FAILED);
	}

	DbgMed("PrintDoc: print method: " << print_method << endl);


	// All file names are interpreted relative
	// to our document's PostScript directory (pspath).
	// We get this information from our document's AnswerBook.
	//
	answerbook = abgroup.GetAnswerBook(doc->Name());
	if (answerbook == NULL) {
		// XXX this shouldn't happen
		err.Init(gettext("Can't find AnswerBook for document '%s'"),
			~doc->Title());
		return(STATUS_FAILED);
	}
	pspath = answerbook->PSPath(doc->Name());
	if (pspath == NULL_STRING) {
		// XXX this shouldn't happen either
		err.Init(gettext(
			"Can't find PostScript directory for document '%s'"),
			~doc->Title());
		return(STATUS_FAILED);
	}


	// Get the list of files to print.
	//
	if (print_method.LinkType() == DVLINK_PRINTFILE) {

		// The print method specifies a file containing the list
		// of files to print.  Get the list from that file.
		//
		print_method.PrintFile(pfpath);
		if (pfpath == NULL_STRING) {
			err.Init(gettext(
				"invalid print method for document '%s'"),
				~doc->Title());
			return(STATUS_FAILED);
		}

		pfpath = pspath + "/" + pfpath;
		if ((printfp = fopen(pfpath, "r"))  ==  NULL) {
			err.Init(gettext("can't open file '%s': %s"),
				~pfpath, SysErrMsg(errno));
			return(STATUS_FAILED);
		}

		while (GetLine(printfp, printfile)  !=  NULL_STRING) {
			files_to_print.Add(printfile);
		}

		(void) fclose(printfp);

	} else {

		// The print method specifies the list of files to print.
		// Get the list from the print method;
		//
		print_method.Print(files_to_print);
	}


	return(PrintList(files_to_print, pspath, offset, revPages,
				pageRange, psout, err));
}

// Print the specified PostScript file.
// "prfile" looks like:
// 
//	filename:12 - 14	(print pages 12 thru 14 of filename)
// or
//	filename:7		(print page 7 of filename)
// or
//	filename		(print all pages in filename)
//
STATUS
PrintFile(const STRING	&prfile,
	  const STRING	&pspath,
	  int		offset,
	  BOOL		rev,
	  const STRING	&pageRange,
	  PSOUT		&psout,
	  ERRSTK	&err)
{
	int		begPage;
	int		endPage;
	int		numPages;
	STRING		filename;
	STRING		pagestr;
	int		colon;
	PGRANGE		pgrange;
	PSDOC	       *psdoc;
	STATUS		status = STATUS_OK;


	DbgFunc("PrintFile: "<< (pspath + "/" + prfile) << endl);


	// Extract file name and page range info.
	//
	if ((colon = prfile.Index(':'))  <  0) {
		filename = prfile;
		pagestr  = NULL_STRING;
	} else {
		filename = prfile.SubString(0, colon-1);
		pagestr  = prfile.SubString(colon+1, END_OF_STRING);
	}
	filename = STRING::CleanUp(filename);
	if (pageRange != NULL_STRING)
		pagestr = pageRange;
	else
		pagestr = STRING::CleanUp(pagestr);


	// Create and open new PostScript document object.
	//
	psdoc = new PSDOC(filename, pspath);
	if (psdoc->Open(err) != STATUS_OK) {
		delete(psdoc);
		return(STATUS_FAILED);
	}


	// Interpret the page range information.
	//
	numPages = psdoc->NumPages();
	if (pagestr == NULL_STRING) {

		// No page range specified - print the whole file.
		//
		begPage	= 1;
		endPage	= numPages;
	} else {

		// 
		if (sscanf(pagestr, "%d - %d", &begPage, &endPage)  !=  2) {
			begPage = atoi(pagestr);
			endPage = begPage;
		}
	}

	// Make sure begin/end pages are sane.
	//
	begPage = Min(Max(1,        begPage), numPages);
	endPage = Max(Min(numPages, endPage), begPage);


	// Print the pages.
	//
	status = PrintPages(*psdoc, begPage, endPage, rev, psout, err);

	delete( psdoc );

	return( status );
}

STATUS
PrintList(LIST<STRING>		&files_to_print,
	  const STRING		&pspath,
	  int			offset,
	  BOOL			rev,
	  const STRING		&pageRange,
	  PSOUT		&psout,
	  ERRSTK		&err)
{
	STRING	prfile;
	int	nfiles;
	int	i;
	STATUS	status;


	DbgFunc("PrintList" << endl);


	// Make sure the first line is "%!"
	//
	psout.WriteString("%!PS-Adobe-2.1\n", err);
	psout.WriteString("%%Creator: SunSoft viewprint\n", err);


	// Print each file in list.
	// If "rev", print files in reverse order.
	//
	nfiles = files_to_print.Count();
	for (i = 0; i < nfiles; i++) {

		if (rev)
			prfile = files_to_print[nfiles-i-1];
		else
			prfile = files_to_print[i];

		status = PrintFile(prfile, pspath, offset, rev,
					pageRange, psout, err);
		if (status != STATUS_OK)
			return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

STATUS
PrintPages(PSDOC	&psdoc,
	   int		begPage,
	   int		endPage,
	   BOOL		reverse,
	   PSOUT	&psout,
	   ERRSTK	&err)
{
	int	numPages = endPage - begPage + 1;
	int	page;
	int	i;


	assert(numPages > 0);
	assert(numPages <= psdoc.NumPages());
	DbgFunc("PrintPages:\t" << begPage << " -> " << endPage << endl);


	// Surround the document with a save restore pair. Some printers run
	// out of memory if this not done.
	//
	SendSave(psout);


	// Send the prolog down the wire
	//
	if (SendPage(psdoc, 0, psout, err)  !=  STATUS_OK)
		return(STATUS_FAILED);


	// Print out each page of the document.
	//
	for (i = 0; i < numPages; i++) {

		if (reverse)
			page = endPage - i;
		else
			page = begPage + i;

		if (SendPage(psdoc, page, psout, err)  !=  STATUS_OK)
			return(STATUS_FAILED);

	}

	// Append PS trailer.
	//
	psout.WriteString("%%Trailer\nend %PROLOGUE\n", err);


	SendRestore(psout);


	return(STATUS_OK);
}

STATUS
SendDoc(PSDOC &psdoc, PSOUT &psout, ERRSTK &err)
{
	caddr_t	docBeg;
	int	nb;


	DbgFunc("SendDoc" << endl);


	// Surround the document with a save restore pair. Some printers run
	// out of memory if this not done.
	//
	SendSave(psout);


	if (psdoc.GetDocSize(&docBeg, &nb)  !=  STATUS_OK) {
		err.Init(gettext("error - could not get document length"));
		return(STATUS_FAILED);
	}

	// Write it out
	//
	if (psout.WriteBytes(docBeg, (size_t)nb, err)  !=  STATUS_OK) {
		return(STATUS_FAILED);
	}


	SendRestore(psout);

	return(STATUS_OK);
}

STATUS
SendPage(PSDOC	&psdoc,
	 int	page,
	 PSOUT	&psout,
	 ERRSTK	&err)
{
	caddr_t	begOfPage = (caddr_t) 0;
	int	pagelen;


	assert(page >= 0);
	DbgFunc("SendPage: " << page << endl);


	if (page > 0) {
		if (psdoc.GetPage(page, &begOfPage, &pagelen)  !=  STATUS_OK) {
			err.Init(gettext("Can't read PostScript"));
			return(STATUS_OK);
		}
	} else {
		if (psdoc.GetProlog(&begOfPage, &pagelen)  !=  STATUS_OK) {
			err.Init(gettext("Can't read PostScript"));
			return(STATUS_OK);
		}
	}
	
	assert(begOfPage != NULL);
	assert(pagelen >= 0);


	if (psout.WriteBytes(begOfPage, pagelen, err)  !=  STATUS_OK) {
		err.Init(gettext("error writing output: %s"),
			 SysErrMsg(errno));
		return(STATUS_OK);
	}


	return(STATUS_OK);
}

// Surround the document with a save restore pair.
// Some printers run out of memory if this not done.
//
void
SendSave(PSOUT &psout)
{
	ERRSTK	err;

	DbgFunc("SendSave" << endl);

	psout.WriteString("%\n", err);
	psout.WriteString("% some printers run out of memory without\n", err);
	psout.WriteString("% save/restore pairs around each file save\n", err);
	psout.WriteString("%\n", err);
	psout.WriteString("save\n\n", err);
}

void
SendRestore(PSOUT &psout)
{
	ERRSTK	err;

	DbgFunc("SendRestore" << endl);

	psout.WriteString("%\n", err);
	psout.WriteString("% some printers run out of memory without\n", err);
	psout.WriteString("% save/restore pairs around each file save\n", err);
	psout.WriteString("%\n", err);
	psout.WriteString("restore\n\n", err);
}

void
Usage()
{
	STRING	usage("[-P<printer> | -f<file>] -#<copies> [-R] <docname>");

	fprintf(stderr, gettext("usage: %s %s\n"), ~progName, ~usage);
}


main(int argc, char **argv)
{
	DOCUMENT       *doc = NULL;
	DOCNAME		docname;
	ERRSTK		err;
	int		offset		= 0;
	int		ncopies		= 1;
	char		*slash;
	STRING		docnameStr;
	STRING		outfilename;
	STRING		printer;
	STRING		pageRange;
	PSOUT	*out		= NULL;
	BOOL		revPages	= BOOL_FALSE;
	STATUS		status		= STATUS_OK;
	int		c;

	extern char    *optarg;
	extern int	optind;


	// Set the error handler for the "new" operator
	//
	set_new_handler(NewHandler);


	// Set up for localization.
  	//
  	setlocale(LC_ALL, "");
	InitTextDomain(DOMAINNAME);


	// What's the name of this binary?
	//
	if ((slash = strrchr(argv[0], '/')) != NULL)
		progName = slash + 1;
	else
		progName = argv[0];

	console.Init(progName);

	while ((c = getopt(argc, argv, "c:f:p:#:P:Rx:")) != EOF) {

		switch (c) {

		case '#':
			ncopies = atoi(optarg);
			ncopies = Max(ncopies, 1);
			ncopies = Min(ncopies, 100);
			break;

		case 'P':
			if (outfilename != NULL_STRING) {
				console.Message(
				    gettext("use either '-P' or '-f'"));
				Usage();
				return(1);
			}
			printer = optarg;
			break;

		case 'R':
			revPages = BOOL_TRUE;
			break;

		case 'c':
			cardcats.Append(optarg, err);
			break;

		case 'f':
			if (printer != NULL_STRING) {
				console.Message(
				    gettext("use either '-P' or '-f'"));
				Usage();
				return(1);
			}

			outfilename = optarg;
			break;

		case 'p':
			pageRange = optarg;
			break;
#ifdef	DEBUG
		case 'x':
			debug = atoi(optarg);
			break;
#endif	DEBUG
		default:
			Usage();
			break;
		}
	}

	// If there are no docids print usage and exit.
	//
	if (optind >= argc) {
		Usage();
		return(1);
	}

	docnameStr = argv[optind];

	cardcats.AppendDefaults(err);

	if (docname.Init(docnameStr) != STATUS_OK) {
		cerr	<< progName << ": "
			<< gettext("invalid document name: ")
			<< docnameStr
			<< endl;
		return(2);
	}

	offset = docname.Offset();

	// Get document
	//
	if ((doc = abgroup.LookUpDoc(docname, LU_AUTO_ADD, err))  ==  NULL) {
		cerr	<< progName << ": " << err;
		return(2);
	}

	if (outfilename != NULL_STRING) {

		out = PSOUTFILE::Open(outfilename, err);
		if (out == NULL) {
			cerr << err;
			return(3);
		}

	} else {

		out = PSOUTLP::Open(printer, ncopies, doc->Title(), err);
		if (out == NULL) {
			cerr << err;
			return(3);
		}
	}

	status = PrintDoc(*out, doc, revPages, offset, pageRange, err);
	if (status != STATUS_OK) {
		err.Push(gettext("Can't print document"));
		cerr << err;
	}

	(void) out->Close();

	delete out;
	delete doc;


	return(status == STATUS_OK ? 0 : 4);
}

PSOUT::PSOUT(FILE *fp) :
	psfp		(fp)
{
	assert(psfp != NULL);
	DbgFunc("PSOUT::PSOUT" << endl);

	objstate.MarkReady();
}

PSOUT::~PSOUT()
{
	DbgFunc("PSOUT::~PSOUT" << endl);
}

STATUS
PSOUT::WriteBytes(const char *start, size_t nbytes, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(psfp != NULL);
	DbgFunc("PSOUT::WriteBytes: " << nbytes << endl);


	if (fwrite((const void *)start, 1, nbytes, psfp)  !=  nbytes) {
		err.Init(gettext("Can't write PostScript: %s"),
			 SysErrMsg(errno));
		return(STATUS_FAILED);
	}
	

	fflush(psfp);
	return(STATUS_OK);
}

STATUS
PSOUT::WriteString(const char *string, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(psfp != NULL);
	DbgFunc("PSOUT::WriteString: " << string << endl);


	if (fputs(string, psfp)  ==  EOF) {
		err.Init(gettext("Can't write PostScript: %s"),
			 SysErrMsg(errno));
		return(STATUS_FAILED);
	}
	

	fflush(psfp);
	return(STATUS_OK);
}

PSOUTFILE::PSOUTFILE(const STRING &path, FILE *fp) :
	PSOUT		(fp),
	filename	(path)
{
	assert(filename != NULL_STRING);
	DbgFunc("PSOUTFILE::PSOUTFILE:" << filename << endl);
}

PSOUTFILE::~PSOUTFILE()
{
	DbgFunc("PSOUTFILE::~PSOUTFILE:" << endl);
	Close();
}

PSOUTFILE *
PSOUTFILE::Open(const STRING &path, ERRSTK &err)
{
	FILE		*psfp;


	assert(path != NULL_STRING);
	DbgFunc("PSOUTFILE::Open: " << path << endl);


	// Open up the output file.
	//
	if ((psfp = fopen(path, "w"))  ==  NULL) {
		err.Init(gettext("Can't open file '%s': %s"),
			~path, SysErrMsg(errno));
		return(NULL);
	}

	return(new PSOUTFILE(path, psfp));
}

STATUS
PSOUTFILE::Close()
{
	assert(objstate.IsReady());
	DbgFunc("PSOUTFILE::Close" << endl);

	if (psfp) {
		(void) fflush(psfp);
		(void) fclose(psfp);
		psfp = NULL;
	}

	return(STATUS_OK);
}

PSOUTLP::PSOUTLP(const STRING &printerArg, FILE *fp) :
	PSOUT		(fp),
	printer		(printerArg)
{
	DbgFunc("PSOUTLP::PSOUTLP: " << printer << endl);


	// Save the signal status
	//
	saveSig = signal(SIGPIPE, SIG_IGN);
}

PSOUTLP::~PSOUTLP()
{
	DbgFunc("PSOUTLP::PSOUTLP:" << endl);

	(void) Close();
	(void) signal(SIGPIPE, saveSig);
}

PSOUTLP *
PSOUTLP::Open(	const STRING	&printer,
		int		ncopies,
		const STRING	&jobTitle,
		ERRSTK		&err)
{
	FILE	*psfp;
	STRING	lprcmd;
	STRING	tmpstr;


	DbgFunc("PSOUTLP::Open: " << printer << "(" << ncopies << ")" << endl);


	// Build the command line
	//
#ifdef	SVR4
	lprcmd = "sed 's/null\ SS/pop pop % modified by viewprint/g'|lp";

	if (printer != NULL_STRING) {
		lprcmd += " -d " + printer;
	}

	if (ncopies > 1)
		lprcmd += " -n " + int2string(ncopies, tmpstr);

	if (jobTitle != NULL_STRING)
		lprcmd += " -t \"" + jobTitle + "\"";
#else
	lprcmd = "lpr";

	if (printer != NULL_STRING)
		lprcmd += " -P" + printer;

	if (ncopies > 1)
		lprcmd += " -#" + int2string(ncopies, tmpstr);

	if (jobTitle != NULL_STRING)
		lprcmd += " -J\"" + jobTitle + "\"";
#endif


	DbgFunc("PSOUTLP::Open: " << lprcmd << endl);


	// Open the pipe to lpr/lp
	//
	if ((psfp = popen(lprcmd, "w"))  ==  NULL) {
		err.Init(gettext("Can't communicate with printer '%s': %s"),
			 ~printer, SysErrMsg(errno));
		return(NULL);
	}


	return(new PSOUTLP(printer, psfp));
}

STATUS
PSOUTLP::Close()
{
	STATUS	return_status = STATUS_OK;
	int	status;

	if (psfp) {
		(void) fflush(psfp);
		status = pclose(psfp);

		if (status != 0) {
			return_status = STATUS_FAILED;
		}

		psfp = NULL;
	}

	return( return_status );
}
