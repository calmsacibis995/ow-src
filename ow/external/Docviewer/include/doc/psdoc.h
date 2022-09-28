#ifndef	_PSDOC_H_
#define	_PSDOC_H_

#ident "@(#)psdoc.h	1.15 11/15/96 Copyright 1989 Sun Microsystems, Inc."

#include "ps_link.h"
#include <doc/list.h>

class PSpage {

    public:

	char	       *start;
	char	       *end;
	LIST<PSLINK*>	linkList;

	PSpage() : start(NULL), end(NULL)	{ }
	~PSpage()				{ }
};


/*
 * XXX - ORDER MATTERS!! COMMTYPE should be in the same order as the
 * ps_comment struct below
 */

typedef enum {
	// EPSF comment conventions
	_EPSF_BEGINSETUP,
	_EPSF_ENDPROLOG,
	_EPSF_ENDSETUP,
	_EPSF_NUMPAGES,
	_EPSF_PAGE,
	_EPSF_PGTRAILER,
	_EPSF_TITLE,
	_EPSF_TRAILER,

	// DNF comment conventions
	_DNF_VUBOX,
	_DNF_LINK,
	_DNF_NUMLINKS,
	_DNF_OBJECTID,

	// No more comments
	_COMM_EOF,

	// Unrecognized comment
	_COMM_UNKNOWN
} COMMTYPE;

struct ps_comment {
	char		*string;
	int		len;
	COMMTYPE	ctype;
};

#define	MAX_COMMENT	1024

/*
 * PostScript document class.
 *
 * This class understands all about PostScript document formats.
 * It opens, reads, parses, caches, and otherwise manages PS documents.
 */
class	PSDOC {
	
    private:

	int		docfd;			// document file descriptor
	STRING		docname;		// document name
	STRING		pathlist;		// list of dirs to search
	STRING		tmpname;		// tmp file name (if any)
	STRING		docTitle;		// Title of document
	char	       *firstbyte;		// first byte of document
	char	       *lastbyte;		// last byte of document
	off_t		numbytes;		// number of bytes in this file
	int		numpages;		// document page count
	PSpage	       *pgTable;		// document page table
	BBox		displayBox;		// document DisplayBox
	BOOL		is_valid;		// document is valid
	
	// Private Functions
	PSLINK	       *AddLink(char *);
	int		CalcNumPages();
	STATUS		FindPageStart(int);
	STATUS		FindPageEnd(int);
	COMMTYPE	GetNextComment(char **start, char **next);
	COMMTYPE	GetPrevComment(char **start, char **prev);
	COMMTYPE	ParseComment(char *, int *);
	STATUS		ParseProlog();
	
    public:
	
	PSDOC(const STRING &name, const STRING &paths);
	~PSDOC();

	STATUS		GetDocSize(caddr_t *beg, int *sz)
	{
		if (is_valid == BOOL_TRUE) {
			*beg	= firstbyte;
			*sz	= (int) numbytes;
			return(STATUS_OK);
		}
		else
			return(STATUS_FAILED);
	}

	LIST<PSLINK*>	&LinkList(int pagenum)
	{
		assert(pagenum > 0 && pagenum <= numpages);
		return(pgTable[pagenum].linkList);
	}
	STATUS		GetPage(int, caddr_t *, int *);
	STATUS		GetProlog(caddr_t *, int*);
	const STRING	&GetDocTitle() const		{ return(docTitle); }
	const BBox	&DisplayBox() const		{ return(displayBox); }
	BOOL		IsValid() const			{ return(is_valid); }
	STATUS		Open(ERRSTK &);
	const STRING    &Name() const			{ return(docname); }
	int		NumPages()
	{
		return((numpages > 0) ? numpages : CalcNumPages());
	}
};

#endif	_PSDOC_H_
