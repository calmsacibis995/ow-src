#ident "@(#)abinfo.cc	1.17 93/02/15 Copyright 1992 Sun Microsystems, Inc."

#include <doc/abinfo.h>
#include <doc/pathname.h>
#include <doc/token_list.h>
#include <doc/utils.h>
#include "dvlocale.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>


// Valid attributes names in ABINFO records.
//
static const STRING	ATTR_ID("id");
static const STRING	ATTR_TITLE("title");
static const STRING	ATTR_VERSION("version");
static const STRING	ATTR_TOCPATH("tocpath");
static const STRING	ATTR_PSPATH("pspath");
static const STRING	ATTR_INDEXPATH("indexpath");


ABINFO::ABINFO()
{
	DbgFunc("ABINFO::ABINFO" << endl);

	objstate.MarkReady();
}

ABINFO::ABINFO(const ABINFO &info) :
	name		(info.name),
	title		(info.title),
	toc_path	(info.toc_path),
	ps_path		(info.ps_path),
	index_path	(info.index_path)
{
	DbgFunc("ABINFO::ABINFO(ABINFO&)" << endl);

	objstate.MarkReady();
}

ABINFO::~ABINFO()
{
	DbgFunc("ABINFO::~ABINFO" << endl);
}

// Is this ABINFO record valid?
//
BOOL
ABINFO::IsValid() const
{
	if (Name().IsValid() != BOOL_TRUE    ||
	    toc_path         == NULL_STRING  ||
	    ps_path          == NULL_STRING  ||
	    index_path       == NULL_STRING) {
		DbgFunc("ABINFO::IsValid: not!" << endl);
		return(BOOL_FALSE);
	} else {
		DbgFunc("ABINFO::IsValid" << endl);
		return(BOOL_TRUE);
	}
}

// Set Title for this AB
STATUS
ABINFO::SetTitleForAB (const STRING &new_title)
{
	title = new_title;
	return (STATUS_OK);
}

// Set TOC path for this AB
STATUS
ABINFO::SetTOCPathForAB (const STRING &tocpath)
{
	toc_path = tocpath;
	return (STATUS_OK);
}

// Set Index path for this AB
STATUS
ABINFO::SetIndexPathForAB (const STRING &indexpath)
{
	index_path = indexpath;
	return (STATUS_OK);
}

// Set PS path for this AB
STATUS
ABINFO::SetPSPathForAB (const STRING &pspath)
{
	ps_path = pspath;
	return (STATUS_OK);
}

// Returns the full path name of the TOC database for book 'id'
// in the specified language.
//
const STRING
ABINFO::TOCPath(const BOOKNAME &book_name) const
{
	STRING	path;

	assert(objstate.IsReady());

	if (toc_path == NULL_STRING) {
		path = NULL_STRING;

	} else if (book_name.BookLang() != "C") {
		path  = toc_path + "/locale/" + book_name.BookLang() +
		                   "/"        + book_name.BookId();
	} else {
		path = toc_path + "/" + book_name.BookId();
	}

	DbgFunc("ABINFO::TOCPath: " << path << endl);
	return(path);
}

// Returns the full path name of the PostScript directory
// for book 'id' in the specified language.
//
const STRING
ABINFO::PSPath(const BOOKNAME &book_name) const
{
	STRING	path;

	assert(objstate.IsReady());

	if (ps_path == NULL_STRING) {
		path = NULL_STRING;

	} else if (book_name.BookLang() != "C") {
		path  = ps_path + "/locale/" + book_name.BookLang() +
		                  "/"        + book_name.BookId();
	} else {
		path = ps_path + "/" + book_name.BookId();
	}

	DbgFunc("ABINFO::PSPath: " << path << endl);
	return(path);
}

// Returns the full path name (sans file name extensions such as ".cfg")
// of the search index for this AnswerBook.
// There are several situations we must accomodate here for purposes
// of backward compatibility.  Here's what we do:
//
//	o If <index_path>.cfg is a regular file, return <index_path>
//	o Else if <index_path> is a directory, then:
//		- If <index_path>/index.cfg is a regular file,
//		  return <index_path>/index
//		- Else if <index_path>/<ab_id>.cfg is a regular file,
//		  return <index_path>/<ab_id>
//
// XXX Need to return status here - there's currently no way to communicate
// XXX whether the index exists or not.  Further, the code in
// XXX ABCLIENT::GetSearcher() that calls us *assumes* that the value
// we return is non-null.
//
const STRING
ABINFO::IndexPath(const STRING &lang) const
{
	STRING	dirname;	// directory containing index files
	STRING	basename;	// basename of index files
	STRING	fullname;
	STRING	tmp1, tmp2;
	STRING	ab_id = this->name.ABId();	// AnswerBook id
	BOOL	found_it = BOOL_FALSE;


	assert(objstate.IsReady());
	assert(ab_id != NULL_STRING);


	if (index_path == NULL_STRING)
		return(NULL_STRING);


	if (access(index_path + ".cfg", R_OK) == 0) {

		PATHNAME::DirName(index_path,  dirname);
		PATHNAME::BaseName(index_path, basename);
		found_it = BOOL_TRUE;

	} else if (access(index_path, R_OK) != 0) {

		// Search index directory does not exist

	} else {
		tmp1 = index_path + "/" + ab_id + ".cfg";
		tmp2 = index_path + "/index.cfg";

		if (access(tmp1, R_OK) == 0) {
			dirname  = index_path;
			basename = ab_id;
			found_it = BOOL_TRUE;
		} else if (access(tmp2, R_OK) == 0) {
			dirname  = index_path;
			basename = "index";
			found_it = BOOL_TRUE;
		}
	}


	// If this is a foreign language index, it will should be located in
	//
	// 	<dirname>/locale/<lang>
	//
	if (dirname != NULL_STRING  &&  lang != NULL_STRING  &&  lang != "C") {

		dirname += "/locale/" + lang;

		tmp1 = dirname + "/" + basename + ".cfg";
		if (access(tmp1, R_OK) == 0) {
			found_it = BOOL_TRUE;
		}
	}


	if ( ! found_it) {
		//XXX should return bad status, NULL_STRING here
		//XXX
		DbgFunc("ABINFO::IndexPath: NO INDEX: " << index_path << endl);
		return(index_path);
	} else {
		fullname = dirname + "/" + basename;
		DbgFunc("ABINFO::IndexPath: " << fullname << endl);
		return(fullname);
	}
}

// Returns list of foreign languages available on this AnswerBook.
// Each list item is one of the standard ISO 639 language/territory
// abbreviations (e.g., de (German), fr (French)).
//
// Returns new LIST of STRINGs.  Caller is responsible for deallocation.
//
STATUS
ABINFO::GetListOfLangs(LIST<STRING> &lang_list, ERRSTK &err) const
{
	STRING		locale_path;	// locale-specific AB data directory
	DIR		*locale_dirp;
	dirent		*locale_dirent;
	STRING		tmp_path;
	struct stat	statbuf;

	assert(objstate.IsReady());
	DbgFunc("ABINFO::GetListOfLangs" << endl);


	lang_list.Clear();
	lang_list.Add("C");		// must have default locale

	// A language is 'available' on an AnswerBook if there is
	// a toc database directory for that language.
	// So we just look in the toc database locale directory for a
	// list of language-specific toc database directories.
	// We assume that the name of each language-specific
	// toc database directory is the standard ISO 639
	// language/territory abbreviation for the corresponding language.
	// XXX This seems like a bit of a hack, but...
	//
	locale_path = toc_path + "/locale";
	if ((locale_dirp = opendir(locale_path))  ==  NULL) {

		switch (errno) {
		case ENOTDIR:	// directory doesn't exist: that's ok
		case ENOENT:
			return(STATUS_OK);
		default:
			err.Init(DGetText(
			    "can't open AnswerBook locale directory '%s': %s"),
			    ~locale_path, SysErrMsg(errno));
			return(STATUS_FAILED);
		}
	}

	// Get the name of each entry in the locale directory.
	// Each entry represents an available language.
	//
	while ((locale_dirent = readdir(locale_dirp))  !=  NULL) {

		// Skip all dot files.
		// Also skip "C" locale (it's already in the list).
		//
		if (locale_dirent->d_name[0] == '.'	||
		    locale_dirent->d_name[0] == '\0'	||
		    locale_dirent->d_name    == "C")
			continue;

		// Make sure it's a directory.
		//
		tmp_path  = locale_path + "/";
		tmp_path += locale_dirent->d_name;
		if (stat(tmp_path, &statbuf) != 0) {
			cerr	<< DGetText("can't stat file/directory: ")
				<< tmp_path << endl;
			continue;
		}
		if ( ! S_ISDIR(statbuf.st_mode))
			continue;

		// Add directory name to language list.
		//
		lang_list.Add(locale_dirent->d_name);
	}

	return(STATUS_OK);
}

// Get ids of all books in this AnswerBook that are
// available in the specified language ("C"=default=English).
//
STATUS
ABINFO::GetListOfBooks(	LIST<BOOKNAME>	&books,
			const STRING	&lang,
			ERRSTK		&err)
{
	BOOKNAME	bookname;
	STRING		bookid;
	STRING		dirpath;	// TOC directory path
	DIR		*toc_dirp;
	dirent		*toc_dirent;
	STRING		filename;
	STRING		suffix;		// file name suffix (e.g., ".dir")
	int		len;


	assert(objstate.IsReady());
	DbgFunc("ABINFO::GetListOfBooks" << endl);


	books.Clear();

	// The most reliable way to obtain a list of books in
	// an AnswerBook is to list the book databases in the
	// AnswerBook's TOC directory.  Each TOC corresponds to a book.
	// The book id is just the database file name minus its suffix
	// (e.g., ".dir", or ".rec").
	//
	if (lang == "C")
		dirpath = toc_path;
	else
		dirpath = toc_path + "/locale/" + lang;

	if ((toc_dirp = opendir(dirpath))  ==  NULL) {

		switch (errno) {
		case ENOTDIR:	// directory doesn't exist: that's ok
		case ENOENT:
			if (lang != "C")
				return(STATUS_OK);
		default:
			err.Init(DGetText(
			    "can't open AnswerBook directory '%s': %s"),
			    ~dirpath, SysErrMsg(errno));
			return(STATUS_FAILED);
		}
	}


	// Initialize the book name.
	//
	bookname.Resolve(Name());
	bookname.SetBookLang(lang);


	// Get the name of each entry in the locale directory.
	// Each entry represents an available language.
	//
	while ((toc_dirent = readdir(toc_dirp))  !=  NULL) {

		// Skip "." and ".." entries.
		//
		filename = toc_dirent->d_name;
		if (filename == "."         ||
		    filename == ".."        ||
		    filename == ""          ||
		    filename == NULL_STRING)
			continue;

		// If file is a NetISAM or ndbm database (its suffix
		// is ".dir" or ".rec"), it represents a book - strip off
		// its suffix and add it to the list.
		// XXX NOTE that we're making assumptions about database
		// suffix names here.
		// XXX NOTE too that it's possible to have two different
		// databases (isam and dbm) for the same book in the same
		// directory.  If this occurs, we'll get duplicate entries
		// for that book in the book list.
		//
		PATHNAME::Suffix(filename, suffix);
		if (suffix == "dir"  ||  suffix == "rec") {
			len    = filename.Length();
			bookid = filename.SubString(0, len-5);
			if (bookname.SetBookId(bookid) != STATUS_OK)
				continue;
			books.Add(bookname);
		}
	}

	return(STATUS_OK);
}

// Parse an ABINFO string, use it to initialize the ABINFO record.
// Each record consists of a list of colon-separated attribute/value pairs.
// Each pair is of the format "attribute_name=attribute_value".
// Valid attribute names are:
//
//	o title		AnswerBook title
//	o id		AnswerBook id
//	o version	AnswerBook version number
//	o tocpath	full path of Table of Contents database directory
//	o pspath	full path of PostScript directory
//	o indexpath	full path of search index directory
//
STATUS
ABINFO::ParseInfoString(const STRING &infostr, ABINFO &info, ERRSTK &err)
{
	TOKEN_LIST	attrlist(infostr, ':');	// tokenized attribute list
	STRING		attr;
	STRING		name, value;
	ABNAME		blankname;
	int		equals;
	int		i;


	DbgFunc("ABINFO::ParseInfoString: " << infostr << endl);


	// First, clear out residual stuff in "info".
	//
	info.title      = NULL_STRING;
	info.toc_path   = NULL_STRING;
	info.ps_path    = NULL_STRING;
	info.index_path = NULL_STRING;
	info.name       = blankname;


	for (i = 0; i < attrlist.Count(); i++) {

		attr = attrlist[i];

		if ((equals = attr.Index('='))  <  0)
			continue;

		name  = attr.SubString(0, equals-1);
		value = attr.SubString(equals+1, END_OF_STRING);

		name  = STRING::CleanUp(name);	// strip leading/trailing space
		value = STRING::CleanUp(value);

		if (name == ATTR_ID) {
			info.name.SetABId(value);
		} else if (name == ATTR_VERSION) {
			info.name.SetABVersion(value);
		} else if (name == ATTR_TITLE) {
			info.title = value;
		} else if (name == ATTR_TOCPATH) {
			info.toc_path = value;
		} else if (name == ATTR_PSPATH) {
			info.ps_path = value;
		} else if (name == ATTR_INDEXPATH) {
			info.index_path = value;
		} else {
			//XXX
			cerr << DGetText("Invalid attribute: ") << attr << endl;
		}
	}

	// Make sure the necessary attributes were present in the
	// ABINFO record.
	//
	if (info.IsValid()) {
		return(STATUS_OK);
	} else {
		err.Init(DGetText(
		    "Invalid AnswerBook configuration info for \"%s\""),
		    ~info.Title());
		return(STATUS_FAILED);
	}
}

STATUS
ABINFO::MakeInfoString(const ABINFO &info, STRING &infostr, ERRSTK &err)
{
	// Validate ABINFO record.
	//
	if ( ! info.IsValid()) {
		err.Init(DGetText(
		    "Invalid AnswerBook configuration info for \"%s\""),
		    ~info.Title());
		return(STATUS_FAILED);
	}


	// Convert ABINFO record to string.
	//

	infostr = ":" + ATTR_ID        + "=" + info.name.ABId() + ": \\\n";
	infostr += ":" + ATTR_VERSION   + "=" + info.name.ABVersion() +": \\\n";
	infostr += ":" + ATTR_TITLE     + "=" + info.Title()     + ": \\\n";
	infostr += ":" + ATTR_TOCPATH   + "=" + info.toc_path    + ": \\\n";
	infostr += ":" + ATTR_PSPATH    + "=" + info.ps_path     + ": \\\n";
	infostr += ":" + ATTR_INDEXPATH + "=" + info.index_path  + ":\n";


	DbgFunc("ABINFO::MakeInfoString: " << infostr << endl);
	return(STATUS_OK);
}
