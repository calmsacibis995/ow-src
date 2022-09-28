#ident "@(#)searcher.cc	1.12 95/02/08 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/searcher.h>
#include <doc/scopekey.h>
#include <doc/zonewght.h>
#include <doc/query.h>
#include "dvlocale.h"

// Create stubs for the index and extended filter mechanisms of the s&r engine.
extern "C" {
#define   FTADSMSG
#define   FTADSIOPN
#define   FTADSIUPD
#define   FTADSINDX
#include  <ft/ftads.h>
}


static const int	FIELD_OBJECTID(65);
static const int	FIELD_TITLE(66);


FTAPIH	SEARCHER::api_handle = NULL;
int	SEARCHER::api_refcount = 0;


// Convenience routine for formatting Ful/Text error messages.
//
static const char	*FTErrMsg(int error);


SEARCHER::SEARCHER(const STRING &index_path_arg) :
	index_path	(index_path_arg),
	coll_handle	(NULL),
	cat_handle	(NULL)
{
	assert(index_path != NULL_STRING  &&  index_path != "");
	DbgFunc("SEARCHER::SEARCHER: " << index_path << endl);
}

SEARCHER::~SEARCHER()
{
	DbgFunc("SEARCHER::~SEARCHER" << endl);

	if (cat_handle)
		ftcclose(cat_handle);
	if (coll_handle)
		ftbclose(coll_handle);
	if (api_handle != NULL) {
		--api_refcount;
		assert(api_refcount >= 0);
		if (api_refcount == 0) {
			ftapitrm(api_handle);
			api_handle = NULL;
		}
	}
}

STATUS
SEARCHER::Init(ERRSTK &err)
{
	DbgFunc("SEARCHER::Init: " << index_path << endl);

	objstate.MarkGettingReady();


	// Initialize the Ful/Text search engine.
	//
	if (InitFT(err) != STATUS_OK) {
		objstate.MarkUnusable();
		return(STATUS_FAILED);
	}


	// Open collection's configuration file.
	//
	if (ftufgfnd(index_path, &config)  !=  0) {
		err.Init(DGetText("Can't open index configuration file: %s"),
			FTErrMsg(fterrno));
		err.Push(DGetText("Can't initialize search index '%s'"),
			~index_path);
		objstate.MarkUnusable();
		return(STATUS_FAILED);

	}


	// Open collection.
	//
	if ((coll_handle = ftbopen(&config, "re", "r"))  ==  NULL) {

		switch (fterrno) {

		case FTETOOMNY:
		    err.Init(DGetText(
			"Too many search indices included in merged index"));
		    break;

		case FTEUNDEF:
		    err.Init(DGetText(
			"Missing entry(s) in configuration file '%s'"),
			config.fgfuln);
		    break;

		case FTEOCATI:
		    err.Init(DGetText(
			"Verify entries in configuration file '%s'"),
			config.fgfuln);
		    err.Push(DGetText("Can't open catalog file(s)"));
		    break;

		case FTEOCATO:
		    err.Init(DGetText("Too many open files"));
		    break;

		case FTENOFILE:
		    err.Init(DGetText(
			"Verify entries in configuration file '%s'"),
			config.fgfuln);
		    err.Push(DGetText("Can't open index file(s)"));
		    break;

		case FTEACCESS:
		    err.Init(DGetText(
			"Check index file permissions in directory '%s'"),
			config.fgpath);
		    err.Push(DGetText("Can't access index files.\n"));
		    break;

		case FTENOTDB:
		    err.Init(
			DGetText("can't find component of merged index: %s"),
			config.fgfuln);
		    break;

		default:
		    err.Init(DGetText("other (error code %d)"), fterrno);
		    break;
		}

		err.Push(DGetText("Can't open search index '%s'"), ~index_path);
		objstate.MarkUnusable();
		return(STATUS_FAILED);

	}


	// Open catalog for this collection.
	//
	if ((cat_handle = ftcopen(coll_handle, 0, 0, FTCRDONLY))  ==  NULL) {

		switch (fterrno) {

		case FTENOMEM:
			err.Init(DGetText("Out of memory"));
			break;

		default:
			err.Init(DGetText("(Ful/Text error code %d)"), fterrno);
			break;
		}

		err.Push(DGetText("Can't open search index '%s'"), ~index_path);
		objstate.MarkUnusable();
		return(STATUS_FAILED);
	}


	// Initialize Scoping Keys and Zone Weights.
	// Don't worry if these fail - they're not critical.
	//
	(void) InitKeys(err);
	(void) InitWeights(err);


	// We're ready to roll...
	//
	objstate.MarkReady();
	return(STATUS_OK);
}

// Initialize the Ful/Text API.
//
STATUS
SEARCHER::InitFT(ERRSTK &err)
{
	long	initial_values[FTANLONG];
	int	i;

	assert(objstate.IsReady());
	assert((api_handle == NULL && api_refcount == 0)  ||
	       (api_handle != NULL && api_refcount >  0));
	DbgFunc("SEARCHER::InitFT" << endl);


	if (api_handle != NULL) {
		++api_refcount;		// bump reference count.
		return(STATUS_OK);
	}


	// Disable Ful/Text file locking by setting the $FTNLOCK
	// environment variable (the value doesn't matter).
	// We're assuming here that all AnswerBook indices are
	// static, read-only entities.
	// We do this to circumvent the ongoing problems with the SunOS
	// file locking implementation.
	// Note that "putenv" expects the its argument to stick around -
	// "envstr" cannot point to local memory.
	//
	char *envstr = new char[20];
	strcpy(envstr, "FTNLOCK=true");
	putenv(envstr);


	// Initialize the Ful/Text API.
	//
	for (i = 0; i < FTANLONG; i++)
		initial_values[i] = -1;

//XXX	initial_values[FTALSHIS] = 65536 ;
	initial_values[FTALSHIS] = 1024 * 1024;
	initial_values[FTALNOSV] = FTASVINDX;

	if (ftapiini(NULL, 0, initial_values, FTANLONG, &api_handle)  !=  0) {
		err.Init(DGetText("Can't initialize searcher: %s"),
				FTErrMsg(fterrno));
		api_handle = NULL;
		return(STATUS_FAILED);
	}


	// Bump reference count.
	//
	++api_refcount;

	return(STATUS_OK);
}

// Initialize scope keys list.
//
STATUS
SEARCHER::InitKeys(ERRSTK &err)
{
	STRING		keys_path;	// path of collections 'Keys' file
	char		cfgbuf[FTGFIGSIZ];


	assert(objstate.IsReady());


	// Get rid of old keys, if any.
	//
	keys.Clear();


	// The Ful/Text configuration file for this collection
	// contains the information we need to find this collection's
	// Keys file.  Read the configuration file into 'cfgbuf'.
	//
	if (ftufgset(&config, cfgbuf)  !=  0) {
		err.Init(DGetText(
			"Can't read collection configuration file '%s'"),
			config.fgfuln);
		return(STATUS_FAILED);
	}


	// Read Keys file.
	// It's name is specified in the 'KEY' entry in the
	// collection configuration file.
	// For backwards compatibility with old AnswerBooks,
	// we'll also check for a file called "Keys" residing
	// in the same directory as the configuration file.
	//
	keys_path = ftufgval("KEY", cfgbuf);
	if (keys_path == NULL_STRING  ||  keys_path == "") {
		keys_path  = config.fgpath;
		keys_path += "Keys";
	}

	if (access(keys_path, R_OK) != 0) {
		err.Init(DGetText("Can't open Scoping Keys file '%s': %s"),
			~keys_path, SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	if (ReadKeysFile(keys_path, keys, err) !=  STATUS_OK) {
		err.Push(DGetText("Can't read Scoping Keys file"));
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

// Initialize zone weights list.
//
STATUS
SEARCHER::InitWeights(ERRSTK &err)
{
	STRING		weights_path;	// path of collections 'Weights' file
	char		cfgbuf[FTGFIGSIZ];


	assert(objstate.IsReady());


	// Get rid of old weights, if any.
	//
	weights.Clear();


	// The Ful/Text configuration file for this collection
	// contains the information we need to find this collection's
	// Weights file.  Read the configuration file into 'cfgbuf'.
	// The name of the Weights file is specified in the 'ZNW' entry
	// in the collection configuration file.
	//
	if (ftufgset(&config, cfgbuf)  ==  0) {
		weights_path = ftufgval("ZNW", cfgbuf);
		weights_path = STRING::CleanUp(weights_path);
	}


	// Read Weights file.
	//
	if (weights_path == NULL_STRING  ||
	    ReadWeightsFile(weights_path, weights, err)  != STATUS_OK) {

		// For backwards compatibility with old AnswerBooks,
		// if no Zone Weights file is specified, we'll use a
		// default set of weights.
		//
		weights.Add(new(ZONEWGHT)(32,  1, "Body"));
		weights.Add(new(ZONEWGHT)(33, 50, "Headers"));
		weights.Add(new(ZONEWGHT)(34, 20, "Notes"));
		weights.Add(new(ZONEWGHT)(35, 10, "Captions"));
		weights.Add(new(ZONEWGHT)(36,  5, "Index"));
	}


	return(STATUS_OK);
}

// Perform full-text query on all open collections.
//
STATUS
SEARCHER::Search(const QUERY		&query,
		int			max_docs,
		LISTX<SEARCHDOC*>	&results,
		ERRSTK			&err)
{
	STRING		query_string;	// composed Ful/Text query string
	FTSH		search;		// search result handle


	assert(objstate.IsReady());
	DbgFunc("SEARCHER::Search: " << query << endl);


	query.ComposeQuery(keys, weights, query_string);

	if (DoSearch(query_string, max_docs, search, err)  !=  STATUS_OK) {
		err.Push(DGetText("Search failed"));
		return(STATUS_FAILED);
	}

	if (GetResults(search, results, err)  !=  STATUS_OK) {
		err.Push(DGetText(
			"Search failed: could not get search results"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

// Perform the first half of the search operation.
//
STATUS
SEARCHER::DoSearch(	const STRING		&query,
			int			max_docs,
			FTSH			&search,
			ERRSTK			&err)
{
	FTSDATA		search_data;
	int		status;
	int		morestatus;
	STRING		fultemp;


	assert(objstate.IsReady());
	DbgFunc("SEARCHER::DoSearch" << query << endl);


	// Determine where temp files will be put.
	//
	if ((fultemp = getenv("FULTEMP"))  ==  NULL_STRING)
		fultemp = P_tmpdir;	// defined in <stdio.h>


	// Start the search engine with the query string and and query
	// parameters from the query object. Use all of the currently
	// opened collections in this search interface instance.
	//
	status = ftshstrt(api_handle, &coll_handle, 1, (FTCID) max_docs,
				FTSQSTRN, query, NULL, -1, &search);

	if (status != 0) {

		switch (fterrno) {
		case FTENOSPC:
		case FTEXFSZ:
		case FTEWRITEF:
			err.Init(DGetText(
			"Check permissions, available space in directory '%s'"),
				~fultemp);
			err.Push(DGetText("Can't write search results."));
			break;
		default:
			err.Init(DGetText("(Error code %d)"), fterrno);
			break;
		}

		return(STATUS_FAILED);
	}

	// If the collection is being searched via a network filter we
	// can display intermediate results at periodic intervals.
	// For now assume that the collection is 'local' and the user 
	// wants the search operation to run to completion. 
	//
	while ((status = ftshresm(api_handle, search, 0, 0, &search_data))  ==  0)
		 ;

	// If ftshresm status is < 0, search failed for some reason.
	// Check fterrno.  Unless fterrno is FTESEARCH, in which case
	// we check fterrsub (seems kinda odd - I wonder why?).
	//
	if (status < 0) {

		switch (fterrno == FTESEARCH ? fterrsub : fterrno) {

		case FTENOQRY:		// no query terms found in dictionary
		case FTENODOC:		// no docs meet search criteria
			ftsclose(search, 0);
			return(STATUS_OK);
		case FTEXFSZ:		// can't write search result (why?)
		case FTEWRITEF:		// ditto (why?)
		case FTENOSPC:		// ditto (out of space in FULTEMP?)
			err.Init(DGetText(
			"Check permissions, available space in directory '%s'"),
				~fultemp);
			err.Push(DGetText("Can't write search results."));
			break;
		case FTENOMEM:		// not enough memory to process query
			err.Init(DGetText("Out of memory"));
			break;
		case FTESYSTEM:		// other
		default:
			err.Init(DGetText("(Error code %d)"), fterrno);
			break;
		}

		ftsclose(search, 0);
		return(STATUS_FAILED);
	}

	// Make sure that the search completed successfully.
	//
	if (ftsstat(search, FTSRC, (char *)&status)  !=  0) {
		err.Init(DGetText("Can't determine status (error code %d)."),
			fterrno);
		ftsclose(search, 0);
		return(STATUS_FAILED);
	}


	// Check search engine status.
	// See 'ftsstat()' in Ful/Text API Reference Guide for details.
	//
	switch (status) {

	case 0:			// everything's ok
		break;

	case FTESEARCH:		// recoverable search error
		ftsstat(search, FTSNDOCS, (char *)&morestatus);

		switch (morestatus) {
		case FTEWRITEF:
			err.Init(DGetText(
			"Check permissions, available space in directory '%s'"),
				~fultemp);
			err.Push(DGetText("Can't write search results."));
			break;
		case FTENOMEM:
			err.Init(DGetText("Out of memory"));
			break;
		default:
			err.Init(DGetText("(Error code %d)"), morestatus);
			break;
		}

		ftsclose(search, 0);
		return(STATUS_FAILED);

	case FTEINTSSB:		// non-recoverable search error
		err.Init(DGetText("Internal error in searcher."));
		ftsclose(search, 0);
		return(STATUS_FAILED);

	default:		// we weren't expecting this
		err.Init(DGetText("(Unexpected error: code %d)"), fterrno);
		ftsclose(search, 0);
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

// Perform second half of search operation.
//
STATUS
SEARCHER::GetResults(	FTSH			search,
			LISTX<SEARCHDOC*>	&results,
			ERRSTK			&err)
{
	int		ndocs;
	int		nres;
	FTCID		cid;
	FTCINFO		cinfo;
	DOCNAME		docname;
	long		weight;
	int		coll_index;
	int		i, j;


	assert(objstate.IsReady());
	DbgFunc("SEARCHER::GetResults" << endl);


	// Clear result list.
	// XXX delete its contents?
	//
//XXX	results.Clear();


	// Get number of documents found.
	//
	if (ftsstat(search, FTSNDOCS, (char *)&ndocs)  !=  0) {
		err.Init(DGetText(
		   "Unexpected error getting document count (error code %d)."),
		   fterrno);
		ftsclose(search, 0);
		return(STATUS_FAILED);
	}


	// Go through result list one by one, retrieve the catalog entry
	// for each result, extract the desired information from the
	// catalog entry (document id, title), and add to the result list.
	//
	for (i = 0, nres = 0; i < ndocs; i++) {

		// Get the next entry in the search results ...
		//
		if ((cid = ftsgetc(search, i, &coll_index))  ==  0) {

			switch (fterrno) {
			case FTEOPENI:
				err.Init(DGetText("Can't open result file."));
				break;
			case FTEBADARG:
			default:
				err.Init(DGetText("(ftsgetc err=%d)."),fterrno);
				break;
			}

			ftsclose(search, 0);
			return(STATUS_FAILED);

		}

		assert(coll_index == 0);


		// Save the weight (for this search entry), the 
		// document name, and the rest of the relevant info from 
		// the catalog record.
		//
		if (GetCatalogEntry(cid, &cinfo, err)  !=  STATUS_OK)
			continue;
		if (ftsgwght(search, &weight)  !=  0)
			continue;

		for (j = 0; j < cinfo.cinflds; j++) {

			switch(cinfo.cicflds[j].cffldid){

			case FIELD_OBJECTID :
				docname.Init(cinfo.cicflds[j].cfbuf);
				break;

			default:
				break;
		      }
	      	}

		results.Add(new(SEARCHDOC)(docname, (int)weight));
	}


	// After successfully extracting all the relevant document 
	// information from the catalog record, close the search 
	// handle for future reuse.
	//
	ftsclose(search,0);
	return(STATUS_OK);
}

ostream	& 
operator <<  (ostream &ostr, SEARCHER *s)
{ 
	if (s != NULL)
		ostr << s->index_path;

	return(ostr);
}

// Read record from collection's catalog.
// Returns static FTCINFO that is overwritten on subsequent calls.
//
STATUS
SEARCHER::GetCatalogEntry(FTCID cid, FTCINFO *cinfo, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(cinfo != NULL);
	DbgFunc("SEARCHER::GetCatalogEntry: " << cid << endl);


	if (cid == FTCIDINF) {
		err.Init(DGetText("Invalid catalog id"));
		return(STATUS_FAILED);
	}

	cinfo->cicid = cid;

	if (ftcread(cat_handle, cinfo, FTCSEEK) != 0) {
		err.Init(DGetText(
		    "Can't read catalog entry '%d' in search index '%s': %s"),
		    (int)cid, index_path, FTErrMsg(fterrno));
		return(STATUS_FAILED);
	} else {
		return(STATUS_OK);
	}
}

#define	FT_ERRMAX	FTENODOC		// last fterrno in list
static STRING	ft_errtab[FT_ERRMAX+1];

// Convenience routine for formatting Ful/Text error messages.
//
const char *
FTErrMsg(int error)
{
	static STRING	errstring;
	static BOOL	inited = BOOL_FALSE;
	char		errbuf[100];

	if ( ! inited) {

		inited = BOOL_TRUE;

#define	ErrTab(err, msg)	ft_errtab[err] = DGetText(msg)
	ErrTab(FTEDBXIST,  "ftbcreate usage error (FTEDBXIST)");
	ErrTab(FTESYSTEM,  "Internal error (FTESYSTEM)");
	ErrTab(FTEFLXIST,  "Unwanted file exists (FTEFLXIST)");
	ErrTab(FTEOPENO,   "Can't open file for output (FTEOPENO)");
	ErrTab(FTEOPENI,   "Can't open file for input (FTEOPENI)");
	ErrTab(FTEREADF,   "Can't read file (FTEREADF)");
	ErrTab(FTEWRITEF,  "Can't write file (FTEWRITEF)");
	ErrTab(FTEBADARG,  "Bad argument (FTEBADARG)");
	ErrTab(FTENOTDB,   "Can't find index configuration file (FTENOTDB)");
	ErrTab(FTENOFILE,  "File does not exist (FTENOFILE)");
	ErrTab(FTEUNDEF,   "Undefined configuration file keyword (FTEUNDEF)");
	ErrTab(FTESYNTAX,  "Syntax error (FTESYNTAX)");
	ErrTab(FTETOOBIG,  "Buffer overflow (FTETOOBIG)");
	ErrTab(FTENOMEM,   "Out of memory (FTENOMEM)");
	ErrTab(FTEACCESS,  "Permission denied.  Check index file permissions (FTEACCESS)");
	ErrTab(FTETOOMNY,  "Internal limit exceeded (FTETOOMNY)");
	ErrTab(FTEOCATI,   "Can't read catalog and map files (FTEOCATI)");
	ErrTab(FTEOCATO,   "Can't write catalog and map files (FTEOCATO)");
	ErrTab(FTEONDXI,   "Can't read index files (FTEONDXI)");
	ErrTab(FTEONDXO,   "Can't write index files (FTEONDXO)");
	ErrTab(FTEDBBUSY,  "Can't close active collection (FTEDBBUSY)");
	ErrTab(FTEOF,      "End-of-file encountered (FTEOF)");
	ErrTab(FTELOCKED,  "Collection is locked (FTELOCKED)");
	ErrTab(FTENOLOCK,  "Can't lock collection (FTENOLOCK)");
	ErrTab(FTEIONO,    "I/O error (FTEIONO)");
	ErrTab(FTEXFSZ,    "file size limit exceeded (FTEXFSZ)");
	ErrTab(FTENOSRCH,  "Collection has not been indexed (FTENOSRCH)");
	ErrTab(FTEBADCAT,  "collection catalog is corrupt (FTEBADCAT)");
	ErrTab(FTESTREAM,  "Invalid seek on catalog record (FTESTREAM)");
	ErrTab(FTESEEKF,   "Seek operation failed (FTESEEKF)");
	ErrTab(FTECXDEL,   "Can't recover deleted catalog entry (FTECXDEL)");
	ErrTab(FTECATOVFL, "Can't write catalog record (FTECATOVFL)");
	ErrTab(FTECANCEL,  "Remote server terminated by signal (FTECANCEL)");
	ErrTab(FTECHILD,   "Remote server failed (FTECHILD)");
	ErrTab(FTEBADMSG,  "Remote server message garbled (FTEBADMSG)");
	ErrTab(FTENOTCAT,  "Invalid catalog (FTENOTCAT)");
	ErrTab(FTENOTCIX,  "Invalid map operation (FTENOTCIX)");
	ErrTab(FTENOSPC,   "File system is full (FTENOSPC)");
	ErrTab(FTEFLTYPE,  "Bad file type (FTEFLTYPE)");
	ErrTab(FTECATEND,  "Catalog identifier out of range (FTECATEND)");
	ErrTab(FTEOR,      "End-of-record in message file (FTEOR)");
	ErrTab(FTESEARCH,  "Recoverable search error (FTESEARCH)");
	ErrTab(FTEEXEC,    "Can't execute program (FTEEXEC)");
	ErrTab(FTEUPDNDX,  "Indexing operation failed (FTEUPDNDX)");
	ErrTab(FTEBADVER,  "Incompatible remote server software (FTEBADVER)");
	ErrTab(FTEUNLINK,  "Can't unlink file (FTEUNLINK)");
	ErrTab(FTEDEADLK,  "Can't read from busy catalog record (FTEDEADLK)");
	ErrTab(FTEBADNDX,  "Dictionary and/or reference file is corrupt (FTEBADNDX)");
	ErrTab(FTEINTNDX,  "Internal data inconsistency (FTEINTNDX)");
	ErrTab(FTEBADSR,   "Inconsistent search result (FTEBADSR)");
	ErrTab(FTENOSTR,   "Invalid conversion of catalog flags (FTENOSTR)");
	ErrTab(FTEOLDHAT,  "Stop file is outdated (FTEOLDHAT)");
	ErrTab(FTELEMMA,   "Reserved value (FTELEMMA)");
	ErrTab(FTEINTSSB,  "Internal data inconsistency (FTEINTSSB)");
	ErrTab(FTEBADFR,   "Catalog is corrupt (FTEBADFR)");
	ErrTab(FTEWINDOWS, "Call to a service function failed (FTEWINDOWS)");
	ErrTab(FTENOSUP,   "Operation not supported (FTENOSUP)");
	ErrTab(FTENOINIT,  "API not initialized (FTENOINIT)");
	ErrTab(FTENFSLK,   "Record locking (FTENFSLK)");
	ErrTab(FTENETIO,   "Network operation failed (FTENETIO)");
	ErrTab(FTEBADLK,   "Reserved (FTEBADLK)");
	ErrTab(FTEHANDLE,  "Can't convert handle (FTEHANDLE)");
	ErrTab(FTENOMARK,  "Corrupt or missing context map (FTENOMARK)");
	ErrTab(FTEBADDYX,  "Corrupt or missing differential file (FTEBADDYX)");
	ErrTab(FTEBADFID,  "Invalid field identifier (FTEBADFID)");
	ErrTab(FTECATFMT,  "Bad data in the catalog write stream (FTECATFMT)");
	ErrTab(FTEOSTRM,   "Invalid catalog operation (FTEOSTRM)");
	ErrTab(FTENOWGHT,  "Document weight not available (FTENOWGHT)");
	ErrTab(FTECATOLD,  "Catalog not compatible (FTECATOLD)");
	ErrTab(FTECATNEW,  "Catalog not compatible (FTECATNEW)");
	ErrTab(FTENOSRV,   "Function not supported (FTENOSRV)");
	ErrTab(FTEBADMF,   "Wrong version of minifile (FTEBADMF)");
	ErrTab(FTENSMETA,  "Invalid operation on meta-collection (FTENSMETA)");
	ErrTab(FTEFILTL,   "Filter not supported (FTEFILTL)");
	ErrTab(FTENOQRY,   "Empty query (FTENOQRY)");
	ErrTab(FTENODOC,   "No documents match search criteria (FTENODOC)");
	}

	if (error > FT_ERRMAX  ||  ft_errtab[error] == NULL_STRING) {
		sprintf(errbuf, "(F/T error code %d)", error);
		errstring = errbuf;
		return(errstring);
	} else {
		return(ft_errtab[error]);
	}
}
