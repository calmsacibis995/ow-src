/* RE_SID: @(%)/tmp_mnt/net/ltfu/opt4/S1093/external/Docviewer/include/doc/SCCS/s.abinfo.h 1.9 93/01/05 15:22:19 SMI */
#ifndef	_ABINFO_H
#define	_ABINFO_H

#ident "@(#)abinfo.h	1.11 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/abname.h>
#include <doc/bookname.h>
#include <doc/list.h>

// An ABINFO object encapsulates all the configuration information for
// a particular AnswerBook.  This information includes the following:
//
//	o Title		AnswerBook title
//	o Name		AnswerBook object name (ABNAME)
//	o tocpath	full path of Table of Contents db directory
//	o pspath	full path of PostScript directory
//	o indexpath	full path of search index directory
//
// So the proper way to locate any AnswerBook component is through that
// AnswerBook's ABINFO record.
//
// ABINFO records are stored in card catalogs and accessed and manipulated
// through the CARDCAT interface (see <doc/cardcat.h>).
//
class	ABINFO {

    private:

	ABNAME		name;		// AnswerBook's object name
	STRING		title;		// AnswerBook's title
	STRING		toc_path;	// TOC database directory
	STRING		ps_path;	// PostScript directory
	STRING		index_path;	// S&R index directory

	OBJECT_STATE	objstate;


    public:

	ABINFO();
	ABINFO(const ABINFO &);
	~ABINFO();

	// Is this a valid ABINFO record?
	//
	BOOL		IsValid() const;

	// Returns object name for this AnswerBook.
	//
	const ABNAME   &Name() const		{ return(name); }

	// Returns this AnswerBook's title.
	//
	const STRING   &Title() const		{ return(title); }

	// Return TOC Path for this AB
	const STRING	TOCPathForAB () const {
		return (toc_path);
	}

	// Return Index Path for this AB
	const STRING	IndexPathForAB () const {
		return (index_path);
	}
	
	// Return PS Path for this AB
	const STRING	PSPathForAB () const {
		return (ps_path);
	}

	// Set the Title for this AB
	STATUS  SetTitleForAB (const STRING &new_title);
	
	// Set the TOC Path for this AB
	STATUS 	SetTOCPathForAB (const STRING &tocpath);

	// Set the Index Path for this AB
	STATUS 	SetIndexPathForAB (const STRING &indexpath);
	
	// Set the TOC Path for this AB
	STATUS 	SetPSPathForAB (const STRING &pspath);
	
	// Returns the full path name of the TOC database for book 'id'
	// in the specified language.
	//
	const STRING	TOCPath(const BOOKNAME &book_name) const;

	// Returns the full path name of the directory containing the
	// PostScript files for book 'id' in the specified language.
	//
	const STRING	PSPath(const BOOKNAME &book_name) const;

	// Returns the full path name of the directory containing the
	// search index for this AnswerBook.
	//
	const STRING	IndexPath(const STRING &lang) const;

	// Get list of foreign languages available on this AnswerBook.
	// Each list item is one of the standard ISO 639 language/territory
	// abbreviations (e.g., de (German), fr (French)).
	//
	STATUS		GetListOfLangs(LIST<STRING> &langs, ERRSTK &) const;

	// Get ids of all books in this AnswerBook that are
	// available in the specified language ("C"=default=English).
	//
	STATUS		GetListOfBooks(	LIST<BOOKNAME>	&books,
					const STRING	&lang,
					ERRSTK		&err);

	// Parse an ABINFO string, use it to initialize the ABINFO record.
	// Each record consists of a list of colon-separated attribute/value
	// pairs.  Each pair is of the format "attribute_name=attribute_value".
	// Valid attribute names are:
	//
	//	o title		AnswerBook title
	//	o id		AnswerBook id
	//	o version	AnswerBook version number
	//	o tocpath	full path of Table of Contents db directory
	//	o pspath	full path of PostScript directory
	//	o indexpath	full path of search index directory
	//
	static STATUS	ParseInfoString(const STRING	&infostr,
					ABINFO		&info,
					ERRSTK		&err);

        // Returns the string version of an ABINFO record object suitable for
        // inclusion in an ABINFO text file database
        //
	static STATUS	MakeInfoString(	const ABINFO	&info,
					STRING		&infostr,
					ERRSTK		&err);
};

#endif	_ABINFO_H
