#ident "@(#)psdoc.cc	1.28 93/12/20 Copyright 1989, 1990 Sun Microsystems, Inc."

#include <doc/common.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <ctype.h>
#include <string.h>
#include <doc/psdoc.h>
#include <doc/pathlist.h>
#include "dvlocale.h"

#define	NEWLINE			'\n'
#define	SPACE			' '

static struct ps_comment	comments[] = {

	// EPSF comment conventions
	"BeginSetup",	0,	_EPSF_BEGINSETUP,
	"EndProlog",	0,	_EPSF_ENDPROLOG,
	"EndSetup",	0,	_EPSF_ENDSETUP,
	"Pages:",	0,	_EPSF_NUMPAGES,
	"Page: ",	0,	_EPSF_PAGE,
	"PageTrailer",	0,	_EPSF_PGTRAILER,
	"Title:",	0,	_EPSF_TITLE,
	"Trailer",	0,	_EPSF_TRAILER,

	// DNF comment conventions
	"DisplayBox",	0,	_DNF_VUBOX,
	"Link",		0,	_DNF_LINK,
	"NumLinks",	0,	_DNF_NUMLINKS,
	"ObjectId",	0,	_DNF_OBJECTID,
};

static int	num_comments = sizeof(comments) / sizeof(ps_comment);
static char	commbuf[MAX_COMMENT];

/*
 * PSDOC is the class that understands the format of PostScript documents.
 *
 * Currently supports only EPSF (Encapsulated PostScript Format) documents.
 * EPSF commenting convention goes something like this:
 * 
 * %%EndProlog
 * %%BeginSetup
 * %%EndSetup
 * %%Page: 1 1
 * %%PageTrailer	(optional)
 * %%Page: 2 2
 * %%PageTrailer	(optional)
 *   .
 *   .
 *   .
 * %%Trailer
 */


inline BOOL strneq(const char *s1, const char *s2, const int n)
{
	return((strncmp(s1, s2, n) == 0) ? BOOL_TRUE : BOOL_FALSE);
}

/*
 * Filter mechanism.
 */
static BOOL	IsCompressed(const STRING &, char *);
static STATUS	Decompress(const STRING &, STRING &);
static BOOL	IsFrame2dot0(const STRING &, char *);
static STATUS	FixFrame2dot0(const STRING &, STRING &);

typedef struct {
	BOOL	(*IsType)(const STRING &name, char *bytes);
	STATUS	(*Apply)(const STRING &name, STRING &newname);
	char	*name;
} FILTER;

FILTER	FilterTable[] = {
	{ IsCompressed,		Decompress,	"Decompress"	},
	{ IsFrame2dot0,		FixFrame2dot0,	"Fix Frame 2.0" },
};

static int	num_filters = sizeof(FilterTable) / sizeof(FILTER);

/*
 * XXX - Since documents (files) are accessed via 'mmap' vs. 'read',
 * we must be careful about accesses past EOF, since it can cause
 * segmentation faults.  There's lots of code in this module that doesn't
 * check for this and needs to be cleaned up someday.
 */


PSDOC::PSDOC(const STRING &name, const STRING &paths)
{
	register int	cntr;

	DbgFunc("PSDOC::PSDOC: entered" << endl);

	docfd		= -1;
	docname		= name;
	pathlist	= paths;
	docTitle	= name;
	firstbyte	= lastbyte = NULL;
	numbytes	= numpages = -1;
	pgTable		= NULL;
	is_valid	= BOOL_FALSE;
	displayBox.ll_x	= 0;
	displayBox.ll_y	= 0;
	displayBox.ur_x	= 612;
	displayBox.ur_y	= 792;

	// Init PS comment table.
	for (cntr = 0; cntr < num_comments; cntr++)
		comments[cntr].len = strlen(comments[cntr].string);
}

// Return number of pages in this document.
int
PSDOC::CalcNumPages()
{
	register COMMTYPE	ctype;
	register char	       *sptr1;
	register char	       *sptr2;

	DbgFunc("PSDOC::CalcNumPages: entered" << endl);

	// If numpages is already known, return it
	if (numpages > 0) {
		DbgMed("PSDOC::CalcNumPages: already know numpages:\t"
		       << numpages << endl);
		return (numpages);
	}

	DbgLow("PSDOC::CalcNumPages: do not know numpages yet ... "
	       << "have to calculate it" << endl);

	/*
	 * Start from the end of the file (the "%%Pages: " comment is usually
	 * towards the end of the file)
	 */

	// Get the last line in the file
	if (*(sptr1 = lastbyte) == NEWLINE) {
		if ((sptr1 = lastbyte - 2) < firstbyte) {
			sptr1 = firstbyte;
		}
	}

	// GetPrevComment expects newline
	while(sptr1 >= firstbyte && *sptr1 != NEWLINE)
		sptr1--;

	// Search for "%%Pages: " comment
	while ((ctype = GetPrevComment(&sptr1, &sptr2)) != _COMM_EOF) {

		if (ctype == _EPSF_NUMPAGES) {

			if (sscanf(commbuf, " %d", &numpages) != 1 ||
			    sscanf(commbuf, "%d", &numpages)  != 1 ) {
				numpages = -1;
			}
			else {
				break;
			}
		}
		sptr1 = sptr2;
	}

	// If numpages is valid, return it ...
	if (numpages > 0) {
		DbgLow("PSDOC::CalcNumPages: numpages\t= " << numpages
		       << endl);
		return (numpages);
	}

	// ... otherwise count the number of pages and hope for the best
	sptr1 = firstbyte;
	numpages = 0;

	// GetNextComment expects newline
	for (; *sptr1 != NEWLINE && sptr1 <= lastbyte; sptr1++);


	while ((ctype = GetNextComment(&sptr1, &sptr2)) != _COMM_EOF) {
		if (ctype == _EPSF_PAGE) {
			numpages++;
		}
		sptr1 = sptr2;
	}

	// If numpages is still zero, set numpages to 1
	if (numpages <= 0) {
		DbgHigh("PSDOC::CalcNumPages: Could not find \"%%Pages: \" "
			"or any \"%%Page:\" comment(s) ... "
			<< "This may not be a conformant PostScript file\n"
			<< "PSDOC::CalcNumPages: setting numpages to 1"
			<< endl);
		numpages = 1;
	}

	DbgLow("PSDOC::CalcNumPages: numpages\t= " << numpages << endl);
	return (numpages);
}

/*
 * Starting at location 's', get the *next* non-blank line
 * from the document.
 *
 * Returns pointer to beginning of line, or NULL if we hit EOF.
 */
COMMTYPE
PSDOC::GetNextComment(char **start, char **next)
{
	register char	*s = *start;
	register int	nbytes;
	int		commlen;
	COMMTYPE	ctype;

	DbgFunc("PSDOC::GetNextComment: entered" << endl);

	assert(s != NULL  &&  lastbyte != NULL);
	assert(*s == NEWLINE);

	nbytes = lastbyte - s - 1;

	// Look for lines that begin with '%%'.
	for ( ; nbytes > 0; nbytes--, s++) {

		if (*s != NEWLINE)
			continue;

		// See if first two characters are '%%'.
		if (s[1] != '%'  ||  s[2] != '%')
			continue;

		/*
		 * We found a line beginning with the requisite '%%'.
		 * See if we recognize the comment.
		 */
		if ((ctype = ParseComment(s+1, &commlen))  ==  _COMM_UNKNOWN)
			continue;

		/*
		 * We do recognize the comment.
		 * Note the start of the comment line (sans initial newline)
		 * and the start of the next line.
		 */
		*start = s + 1;
		*next = *start + commlen;
		assert(**next == NEWLINE);

		return(ctype);
	}

	return(_COMM_EOF);	// end of file - no more comments
}

/*
 * Starting at location sptr, get the *previous* EPSF or DNF comment line
 * from the document.
 *
 */
COMMTYPE
PSDOC::GetPrevComment(char **start, char **next)
{
	register char  *sptr = *start;
	int		commlen;
	COMMTYPE	ctype;

	DbgFunc("PSDOC::GetPrevComment: entered" << endl);

	assert(*sptr != NULL && firstbyte != NULL);
	assert(*sptr == NEWLINE || sptr == firstbyte);


	// Look for lines that begin with '%%'.
	for (; sptr >= firstbyte; sptr--) {

		if (*sptr != NEWLINE)
			continue;

		// See if first two characters are '%%'.
		if (sptr[1] != '%' || sptr[2] != '%')
			continue;

		/*
		 * We found a line beginning with the requisite '%%'. See if
		 * we recognize the comment.
		 */
		ctype = ParseComment(sptr + 1, &commlen);

		if (ctype != _COMM_UNKNOWN) {

			/*
			 * We do recognize the comment. Note the start of the
			 * comment line (sans initial newline) and the start
			 * of the next line.
			 */
			*start = sptr + 1;
			while (--sptr > firstbyte && *sptr != NEWLINE);

			*next = sptr;
			assert(**next == NEWLINE || *next >= firstbyte);

			return (ctype);
		}
	}

	return (_COMM_EOF);	// end of file - no more comments
}

/*
 * Get page n from document.
 * Page numbers start from 1.
 */
STATUS
PSDOC::GetPage(int pgnum, caddr_t *pgstart, int *pgbytes)
{
	PSpage	       *page;


	DbgFunc("PSDOC::GetPage: " << pgnum << endl);

	assert(IsValid());		// assume valid document
	assert(pgnum > 0);		// assume page numbers start with 1


	if (pgnum > numpages) {
		DbgHigh("PSDOC::GetPage: not that many pages:\t"
			<< pgnum << endl);
		return(STATUS_FAILED);
	}


	page = &pgTable[pgnum];

	if (page->start == NULL)
		if (FindPageStart(pgnum)  !=  STATUS_OK)
			return(STATUS_FAILED);

	if (page->end == NULL)
		if (FindPageEnd(pgnum)  !=  STATUS_OK)
			return(STATUS_FAILED);


	*pgstart = page->start;
	*pgbytes = page->end - page->start + 1;

	DbgLow("PSDOC::GetPage: "	<< pgnum
		<< ": offset = " 	<< (*pgstart - firstbyte)
		<< ", size = "		<< *pgbytes
		<< endl);

	return(STATUS_OK);
}

/*
 * Determine where page 'pgnum' starts in this document.
 */
STATUS
PSDOC::FindPageStart(int pgnum)
{
	register BOOL	done;
	int		nbytes;
	char	       *next;
	char	       *sptr;		// first byte in page

	DbgFunc("PSDOC::FindPageStart:\t" << pgnum << endl);

	assert(IsValid());		// assume valid document
	assert(pgnum > 0);		// assume page numbers start with 1
	assert(pgnum <= numpages);


	if (pgTable[pgnum].start != NULL)
		return(STATUS_OK);

	assert(pgnum > 1);	// we found page 1 back in ParseProlog()

	// Find the last page for which we know the location.
	for (int page = 1; page < pgnum; page++) {

		if (GetPage(page, &sptr, &nbytes)  !=  STATUS_OK) {
			DbgHigh("PSDOC::GetPage: cannot find page:\t"
				<< page << endl);
			return(STATUS_FAILED);
		}
	}

	assert(pgTable[pgnum-1].end != NULL);

	sptr = pgTable[pgnum-1].end;

	for (done = BOOL_FALSE; !done; sptr = next) {

		switch (GetNextComment(&sptr, &next)) {

		case _COMM_EOF:
			/*
			 * We've hit document EOF without finding
			 * another '%%' comment line.
			 * Looks like page we're looking for doesn't exist.
			 */
//XXX			numpages = pgnum - 1;
			DbgMed("PSDOC::FindPageStart: hit EOF" << endl);
			return(STATUS_FAILED);

		case _EPSF_PAGE:
			pgTable[pgnum].start = sptr;
			done = BOOL_TRUE;
			DbgLow("PSDOC::FindPageStart: offset:\t"
			       << (sptr - firstbyte) << endl);
			break;

		default:
			break;
		}
	}

	return(STATUS_OK);
}

/*
 * Determine where page 'pgnum' ends.
 */
STATUS
PSDOC::FindPageEnd(int pgnum)
{
	register BOOL	done;
	ERRSTK		err;
	PSLINK	       *link;
	char	       *next;
	PSpage	       *page;
	char	       *sptr;

	DbgFunc("PSDOC::FindPageEnd: " << pgnum << endl);

	page = &pgTable[pgnum];

	assert(IsValid());		// assume valid document
	assert(pgnum > 0);		// assume page numbers start with 1
	assert(pgnum <= numpages);
	assert(page->start != NULL);


	if (page->end != NULL)
		return(STATUS_OK);

	/*
	 * We don't have the end of page 'pgnum',
	 * so look for one of the following page delimiters:
	 *	%%PageTrailer	(end of this page)
	 *	%%Page:		(beginning of next page)
	 *	%%Trailer	(end of document),
	 *	EOF		(end of file)
	 */

	// GetNextComment() expects newline
	for (sptr = page->start; *sptr != NEWLINE; sptr++)
		;

	for (done = BOOL_FALSE; !done; sptr = next) {

		switch (GetNextComment(&sptr, &next)) {

		case _COMM_EOF:
			/*
			 * We've hit document EOF without finding
			 * any of the delimiters we were looking for.
			 * This page must extend to the end of the
			 * document.
			 */
			page->end = lastbyte;
//XXX			numpages = pgnum;    // now we know page count
			DbgMed("PSDOC::FindPageEnd: hit EOF" << endl);
			done = BOOL_TRUE;
			break;

		case _EPSF_PGTRAILER:

			/*
			 * We found the trailer marking the end
			 * of this page.
			 * 'end' is the end of the current line.
			 */
			page->end = next;
			done = BOOL_TRUE;
			break;

		case _DNF_LINK:
			/*
			 * We found a hypertext link. Add the link to the
			 * linked-list of links :-).
			 */
			link = new PSLINK();

			if (link->Init(commbuf, err) != STATUS_OK) {
				delete(link);
			}
			else {
				page->linkList.Add(link);
			}
			break;

		case _EPSF_PAGE:

			/*
			 * We found the beginning of the next page.
			 * 'end' is the end of the previous line.
			 */
			page->end = sptr - 1;
			done = BOOL_TRUE;
			break;

		case _EPSF_TRAILER:

			/*
			 * We found the trailer marking the end
			 * of the document.
			 * 'end' is the end of the previous line.
			 */
			page->end = sptr - 1;
			done = BOOL_TRUE;
//XXX			numpages = pgnum;     // now we know page count
			break;

		default:
			break;
		}
	}

	return(STATUS_OK);
}

#ifdef	UNUSED
/*
 * Starting at location 's', get the *prev* non-blank line
 * from the document.
 *
 * Returns pointer to beginning of line
 */
char *
PSDOC::GetPrevLine(register char *fptr)
{
	register char  *save = fptr;

	// Find the end of the previous line
	while ((fptr > firstbyte) && *fptr-- != NEWLINE)
		;

	// Skip past any empty lines
	while ((fptr > firstbyte) && *fptr-- == NEWLINE)
		;

	if (fptr > firstbyte) {
		while ((fptr > firstbyte) && *fptr-- != NEWLINE)
			;
	}

	return((*(fptr + 1) == NEWLINE) ? (fptr + 2) : (char*) 0);
}
#endif	/* UNUSED */

// Get document prolog section.
STATUS
PSDOC::GetProlog(char **prostart, int *probytes)
{
	PSpage	       *prolog = &pgTable[0];	// prolog is first entry

	DbgFunc("PSDOC::GetProlog: entered" << endl);

	assert(IsValid());		// assume valid document

	// Check to see if there is a prolog first
	if (prolog->start == prolog->end) {
		return(STATUS_FAILED);
	}
	else {
		*prostart = prolog->start;
		*probytes = prolog->end - prolog->start + 1;

		DbgLow("PSDOC::GetProlog: start:\t" << *prostart - firstbyte
		       << " nbytes:\t" << *probytes << endl);
	}

	return(STATUS_OK);
}

// Open document docname.
STATUS
PSDOC::Open(ERRSTK &err)
{
	PATHLIST	plist(pathlist);
	STRING		fullpath;
	STRING		docnameZ = docname + ".Z";
	struct stat	stbuf;

	DbgFunc("PSDOC::Open:\t\"" << docname << "\"" << endl);


	/*
	 * Search list of directories for document.
	 * XXX We also search for document with ".Z" suffix -
	 * the compressed version of this document.  It'd be nice if
	 * we could keep all knowledge of filter-specific things in
	 * the filter code, but alas, we can't in this case.
	 */
	if ((fullpath = plist.Find(docname))  ==  NULL_STRING  &&
	    (fullpath = plist.Find(docnameZ))  ==  NULL_STRING) {
		err.Init(DGetText("Can't find document \"%s\""), ~docname);
		return(STATUS_FAILED);
	}

	docname = fullpath;


	/*
	 * If the document is of a certain type that needs filtering,
	 * run it through the appropriate filter first, then open the
	 * filtered version of the document.
	 */
	for (int i = 0; i < num_filters; i++) {

		// Check file type
		if ( ! FilterTable[i].IsType(docname, NULL))
			continue;

		// Create unique name for filter temp file.
		if ((tmpname = tempnam("/tmp", "hvtmp"))  ==  NULL_STRING) {
			err.Init(DGetText("Can't create temporary file: %s"),
				SysErrMsg(errno));
			return(STATUS_FAILED);
		}

		// Apply type-specific filter to this file
		if (FilterTable[i].Apply(docname, tmpname)  !=  STATUS_OK) {
			err.Init(DGetText("Can't decompress file \"%s\""),
				~docname);
			tmpname = NULL;
			return(STATUS_FAILED);
		}

		DbgMed("PSDOC::Open:"
			<< " applied "		<< FilterTable[i].name
			<< " to "		<< docname
			<< ", results in "	<< tmpname
			<< endl);
		break;		// all done
	}


	/*
	 * Open (filtered?) document.
	 */
	if (tmpname != NULL_STRING)
		docfd = open(tmpname, O_RDONLY);
	else
		docfd = open(docname, O_RDONLY);

	if (docfd < 0) {
		err.Init(DGetText("Can't open document \"%s\": %s"),
			~docname, SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	/*
	 * Find out how big it is,
	 * then mmap it.
	 */
	if (fstat(docfd, &stbuf) < 0) {
		(void) close(docfd);
		err.Init(DGetText("Can't stat document \"%s\": %s"),
			~docname, SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	if ((numbytes = stbuf.st_size) <= 0) {
		(void) close(docfd);
		err.Init(DGetText("Document is empty \"%s\""), ~docname);
		return(STATUS_FAILED);
	}

	if ((firstbyte = mmap((caddr_t) 0, (size_t) numbytes, PROT_READ,
			      MAP_SHARED, docfd, (off_t) 0)) == (caddr_t) -1) {
		(void) close(docfd);
		err.Init(DGetText("Can't read document \"%s\": %s"),
			~docname, SysErrMsg(errno));
		return(STATUS_FAILED);
	}
	
	lastbyte = firstbyte + numbytes - 1;

	// Create the page table; +1 for the prolog
	pgTable = new PSpage[CalcNumPages()+1];

	// Do some initial parsing of document
	if (ParseProlog() != STATUS_OK) {
		err.Init(DGetText(
			"Invalid or corrupt PostScript document: \"%s\""),
			~docname);
		(void) close(docfd);
		(void) munmap(firstbyte, (int) numbytes);
		delete(pgTable);
		return(STATUS_FAILED);
	}

	is_valid = BOOL_TRUE;		// this document is ok

	return(STATUS_OK);
}

/*
 * Do some initial parsing of the document to find prolog & setup sections,
 * first page, etc.
 */
STATUS
PSDOC::ParseProlog()
{
	char	       *endprolog	= NULL;
	PSpage	       *firstpage	= &pgTable[1];
	register char  *line;
	char	       *next;
	PSpage	       *prolog		= &pgTable[0];
	char	       *startpage	= NULL;
	BOOL		done		= BOOL_FALSE;

	DbgFunc("PSDOC::ParseProlog: entered" << endl);

	// Check for the basics ...
	if (numbytes < 2  ||  ! strneq(firstbyte, "%!", 2)) {
		DbgHigh("PSDOC::ParseProlog: not a PostScript file!" << endl);
		return(STATUS_FAILED);
	}

	/*
	 * XXX - can have problems here peeking past EOF
	 * while looking for comments.  May cause segmentation faults.
	 */
	// GetNextComment expects newline
	for (line = firstbyte; line <= lastbyte && *line != NEWLINE; line++)
		;

	for ( ; done == BOOL_FALSE; line = next) {

		switch (GetNextComment(&line, &next)) {

		case _COMM_EOF:
			/*
			 * We've hit document EOF without finding
			 * another '%%' comment line.
			 */
			done = BOOL_TRUE;
			break;

		case _EPSF_ENDPROLOG:
			endprolog = next;
			DbgLow("PSDOC::ParseProlog: EndProlog" << endl);
			break;

		case _EPSF_ENDSETUP:
			endprolog = next;
			DbgLow("PSDOC::ParseProlog: EndSetup" << endl);
			break;

		case _EPSF_PAGE:
			startpage = line;
			DbgLow("PSDOC::ParseProlog: Page" << endl);
			done = BOOL_TRUE;	// finished parsing
			break;

		case _EPSF_TITLE:
			DbgLow("PSDOC::ParseProlog: Title: "
			       << commbuf << endl);
			// If there's a space, get rid of it
			if (commbuf[0] == SPACE) {
				docTitle = &commbuf[1];
			}
			else {
				docTitle = commbuf;
			}
			break;
			
		case _EPSF_TRAILER:
			DbgLow("PSDOC::ParseProlog: Trailer" << endl);
			done = BOOL_TRUE;	// finished parsing
			break;

		case _EPSF_PGTRAILER:
			DbgLow("PSDOC::ParseProlog: PageTrailer" << endl);
			done = BOOL_TRUE;	// finished parsing
			break;

		case _DNF_VUBOX:
			DbgLow("PSDOC::ParseProlog: DisplayBox: "
				<< commbuf << endl);
			if (sscanf(commbuf,
				   ":%d %d %d %d", &displayBox.ll_x,
				   &displayBox.ll_y,
				   &displayBox.ur_x, &displayBox.ur_y) != 4 &&
			    sscanf(commbuf,
				   ": %d %d %d %d", &displayBox.ll_x, &displayBox.ll_y,
				   &displayBox.ur_x, &displayBox.ur_y) != 4) {
				DbgHigh("PSDOC::ParseProlog: bad bbox comment"
					<< endl);
			}

			break;

		default:
			break;
		}
	}


	if (endprolog && startpage) {

		/*
		 * We found comments indicating the extent of the document
		 * prolog, and the beginning of the first page.
		 */
		prolog->start = firstbyte;
		prolog->end   = endprolog;

		firstpage->start = startpage;
		firstpage->end   = NULL;	// don't know this yet

	}
	else if (endprolog && startpage == NULL) {

		/*
		 * We found the prolog, but no page delimiters.
		 * Assume this is a 'one page' document, and that
		 * the first page is everything following the prolog.
		 */
		prolog->start = firstbyte;
		prolog->end   = endprolog;

		firstpage->start = endprolog + 1;
		firstpage->end   = lastbyte;

//XXX		numpages = 1;

	}
	else if (endprolog == NULL  &&  startpage) {

		/*
		 * Hmmmm ...
		 * No prolog, but we found a page delimiter.
		 * Assume this means the document has no prolog. (XXX)
		 */
		prolog->start = NULL;
		prolog->end   = NULL;

		firstpage->start = startpage;
		firstpage->end   = NULL;

	}
	else {	// (endprolog == NULL  &&  startpage == NULL)

		/*
		 * No prolog, no page delimiters.
		 * Assume entire document is just one long page.
		 */
		prolog->start = NULL;
		prolog->end   = NULL;

		firstpage->start = firstbyte;
		firstpage->end   = lastbyte;

//XXX		numpages = 1;
	}

	return(STATUS_OK);
}

/*
 * Returns comment type and sets 'commlen' to be length of line
 * containing the comment, not including the newline.
 */
COMMTYPE
PSDOC::ParseComment(register char *s, int *commlen)
{
	register struct ps_comment	*comm = comments;
	int				maxbytes;
	register int			i;

	DbgFunc("PSDOC::ParseComment: entered" << endl);

	assert(s[0] == '%'  &&  s[1] == '%');

	s += 2;		// skip over leading '%%'

	maxbytes = lastbyte - s + 1;

	for (i = 0; i < num_comments; i++, comm++) {

		/*
		 * Since this file is memory mapped,
		 * be extremely cautious about peeking past EOF
		 * since that could cause a segmentation violation.
		 */
		if (comm->len + 1 > maxbytes)	// length of comment + newline
			continue;

		if (strneq(comm->string, s, comm->len)) {

			register char	*bufp = commbuf;

			// Copy body of comment into global buffer.
			s += comm->len;
			while (*s != NEWLINE)
				*bufp++ = *s++;
			*bufp = '\0';

			assert(*s == NEWLINE);

			/*
			 * Set comment length.
			 * Include the initial '%%' (+ 2).
			 */
			*commlen = comm->len + strlen(commbuf) + 2;


			DbgLow("%%" << comm->string << commbuf
			       << " (" << *commlen << ")" << endl);
			return(comm->ctype);
		}
	}


	return(_COMM_UNKNOWN);
}

PSDOC::~PSDOC()
{
	int	cntr;
	int	i;

	DbgFunc("PSDOC::~PSDOC: entered" << endl);

	if (!IsValid()) {
		return;
	}

	// Delete the links associated with this document
	int npages = NumPages () + 1;
	for (cntr = 0; cntr < npages; cntr++) {
		// Delete links for this page
		int ncount = pgTable[cntr].linkList.Count();
		if (ncount > 0) {
			for (i = 0; i < pgTable[cntr].linkList.Count(); i++)
				delete(pgTable[cntr].linkList[i]);
			pgTable[cntr].linkList.Destroy();
			}
	}

	// free PSpages
	if (pgTable)
		delete(pgTable);

	(void) munmap(firstbyte, (int) numbytes);

	(void) close(docfd);

	// If we used temp file, unlink it.
	if (tmpname != NULL_STRING)
		unlink(tmpname);
}

/*
 * See if file is compressed.
 * Check if suffix is ".Z" or if the file "filename.Z" exists.
 * Should also check magic number, but not for now.
 */
BOOL
IsCompressed(const STRING &filename, char *)
{
	STRING	dot;

	if ((dot = strrchr(filename, '.'))  !=  NULL_STRING  &&  dot == ".Z")
		return(BOOL_TRUE);
	else if (access(filename+".Z", F_OK) == 0)
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

/*
 * Decompress a compressed file.
 * Store decompressed file in 'output'.
 */
STATUS
Decompress(const STRING &filename, STRING &output)
{
	char	cmdbuf[255];
	STRING	outputZ = output + ".Z";
//XXX	STATUS	System(const STRING &, const STRING &);


	assert(IsCompressed(filename, NULL));
	DbgFunc("Decompress: " << filename << endl);


	/*
	 * Create a symbolic link to output.Z.
	 * This gives us better error output from 'uncompress'.
	 */
	if (symlink(filename, outputZ) != 0) {
		DbgHigh("Decompress: can't create symbolic link: " <<
			outputZ << endl);
		return(STATUS_FAILED);
	}


	/*
	 * Use 'uncompress' to decompress the file.
	 * XXX - using 'system()' is a big XView no-no, but it appears
	 * to work in this case.  Be very suspicious of this code.
	 */
	sprintf(cmdbuf, "uncompress %s", ~output);
	if (system(cmdbuf) != 0) {
//XXX	if (System(cmdbuf, NULL_STRING)  !=  STATUS_OK) {
		DbgHigh("Decompress: can't execute shell command: " <<
			cmdbuf << endl);
		(void) unlink(outputZ);
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

/*
 * See if this is a Frame 2.0 PostScript file.
 */
BOOL
#ifdef	DEBUG
IsFrame2dot0(const STRING &filename, char *)
#else
IsFrame2dot0(const STRING &, char *)
#endif	/* DEBUG */
{
	DbgFunc("IsFrame2dot0: " << filename << endl);
	return(BOOL_FALSE);	//XXX
}

/*
 * Fix PostScript problems with Frame 2.0 PostScript file.
 * Store fixed file in 'output'.
 */
STATUS
#ifdef	DEBUG
FixFrame2dot0(const STRING &filename, STRING &)
#else
FixFrame2dot0(const STRING &, STRING &)
#endif	/* DEBUG */
{
	DbgFunc("FixFrame2dot0: " << filename << endl);
	return(STATUS_FAILED);	//XXX
}
