#ident "@(#)cardcats.cc	1.14 93/12/20 Copyright 1992 Sun Microsystems, Inc."


#include <doc/abinfo.h>
#include <doc/cardcat.h>
#include <doc/cardcats.h>
#include <doc/pathname.h>
#include <doc/token_list.h>
#include <doc/utils.h>
#include "dvlocale.h"


// Comparison routines for sorting card catalog entries.
//
static int	SortEntriesById(const void *, const void *);
int		SortEntriesByTitle(const void *, const void *);


CARDCATS::CARDCATS()
{
	DbgFunc("CARDCATS::CARDCATS: " << endl);

	objstate.MarkReady();
}

CARDCATS::~CARDCATS()
{
	DbgFunc("CARDCATS::~CARDCATS" << endl);

	// Note that "cclist" is an auto-delete list.
	// The card catalog objects in cclist will delete themselves. 
	// (I don't think so... 12/16/93 joew)

	cclist.Clear();
}

// Append the default card catalogs to this list, i.e.:
//
//	$AB_CARDCATALOG
//	~/.ab_cardcatalog
//
STATUS
CARDCATS::AppendDefaults(ERRSTK &err)
{
	STRING	defaults;	// list of default card catalogs

	assert(objstate.IsReady());

	defaults  = getenv("AB_CARDCATALOG");
	defaults += ":";
	defaults += "~/.ab_cardcatalog";

	DbgFunc("CARDCATS::AppendDefaults: " << defaults << endl);

	return(Append(defaults, err));
}

// Append the specified list of card catalogs to our list.
// "cclist" is a colon-separated list of card catalog paths.
// Note that each card catalog path name in "cclist" is resolved
// to a full path name.
// Returns STATUS_OK if at least one card catalog in the list
// is successfully added to the list.
//
STATUS
CARDCATS::Append(const STRING &ccpaths, ERRSTK &err)
{
	TOKEN_LIST	tklist(ccpaths, ':');
	int		where;
	int		i;

	assert(objstate.IsReady());
	DbgFunc("CARDCATS::Append: " << ccpaths << endl);

	where = cclist.Count();

	for (i = 0; i < tklist.Count(); i++) {
		Insert(tklist[i], where, err);
	}

	if (cclist.Count() == 0) {
		err.Init(DGetText("No card catalogs"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

// Ditto, but prepend instead of append.
//
STATUS
CARDCATS::Prepend(const STRING &ccpaths, ERRSTK &err)
{
	TOKEN_LIST	tklist(ccpaths, ':');
	int		where;
	int		i;

	assert(objstate.IsReady());
	DbgFunc("CARDCATS::Prepend: " << ccpaths << endl);

	where = 0;

	for (i = 0; i < tklist.Count(); i++) {
		Insert(tklist[i], where, err);
	}

	if (cclist.Count() == 0) {
		err.Init(DGetText("No card catalogs"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

// Insert the specified card catalog into our list.
// Note that "ccpath" is resolved to a full path name.
//
STATUS
CARDCATS::Insert(const STRING &ccpath, int &n, ERRSTK &err)
{
	CARDCAT	*cardcat;
	STRING	fullpath;
	int	i;


	assert(objstate.IsReady());
	DbgFunc("CARDCATS::Insert: " << ccpath << endl);


	// Resolve "~", relative paths, embedded environment variables, etc.,
	// in card catalog path name.  This is particularly important since
	// these path names tend to get passed among processes whose
	// environments (e.g., current directory) may differ.
	//
	PATHNAME::Expand(ccpath, fullpath);


	// If card catalog is already in list,
	// don't add it a second time.
	//
	for (i = 0; i < cclist.Count(); i++) {
		if (cclist[i]->GetPath() == fullpath)
			return(STATUS_OK);
	}


	// Open the card catalog; add it to our list.
	//
	if ((cardcat = CARDCAT::Open(fullpath, "r", err))  ==  NULL)
		return(STATUS_FAILED);

	cclist.Insert(cardcat, n);
	n++;


	return(STATUS_OK);
}

// Clear out this list of card catalogs.
//
void
CARDCATS::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("CARDCATS::Clear" << endl);

	cclist.Clear();
}

// Retrieve the ABINFO record for the specified AnswerBook.
// Search each of the card catalogs in our list in order.
//
STATUS
CARDCATS::GetMatch(const ABNAME &abname, ABINFO &info, ERRSTK &err)
{
	int	i;


	assert(objstate.IsReady());
	DbgFunc("CARDCATS::GetMatch: " << abname << endl);


	for (i = 0; i < cclist.Count(); i++) {
		if (cclist[i]->GetMatch(abname, info, err)  ==  STATUS_OK)
			return(STATUS_OK);
	}


	// No luck.
	//
	if (cclist.Count() == 0) {
		err.Init(DGetText(
		"Can't find AnswerBook configuration info: no card catalogs"));
	} else {
		err.Init(DGetText(
			"Can't find configuration info for AnswerBook \"%s\""),
			~abname);
	}

	return(STATUS_FAILED);
}

// Retrieve the first/next ABINFO record in this card catalog.
//
STATUS
CARDCATS::GetFirst(ABINFO &/*info*/, ERRSTK &/*err*/)
{
	assert(objstate.IsReady());
	DbgFunc("CARDCATS::GetFirst" << endl);

	//XXX not yet implemented
	assert(0);
	return(STATUS_FAILED);
}

STATUS
CARDCATS::GetNext(ABINFO &/*info*/, ERRSTK &/*err*/)
{
	assert(objstate.IsReady());
	DbgFunc("CARDCATS::GetNext" << endl);

	//XXX not yet implemented
	assert(0);
	return(STATUS_FAILED);
}

// Retrieve all ABINFO records in every card catalog in this list.
// Records in list are all new ABINFOs (caller is responsible
// for deallocation),
//
STATUS
CARDCATS::GetAll(LISTX<ABINFO*> &info_list, ERRSTK &err)
{
	int	i;


	assert(objstate.IsReady());
	DbgFunc("CARDCATS::GetAll" << endl);


	info_list.Clear();


	if (cclist.Count() == 0) {
		err.Init(DGetText("No card catalogs"));
		return(STATUS_FAILED);
	}

	for (i = 0; i < cclist.Count(); i++)
		cclist[i]->GetAll(info_list, err);


	// Sort card catalog entries by AnswerBook ID,
	// then get rid of duplicate entries.
	//
	SortList(info_list, SortEntriesById);

	for (i = 0; i < info_list.Count()-1; i++) {
		if (info_list[i]->Name() == info_list[i+1]->Name()) {
			info_list.Delete(i--);
		}
	}


	return(STATUS_OK);
}

// Get the path names for the card catalogs in this list.
//
void
CARDCATS::GetPaths(LIST<STRING> &paths)
{
	int	i;

	assert(objstate.IsReady());
	DbgFunc("CARDCATS::&GetPaths(LIST<STRING>&)" << endl);

	paths.Clear();

	for (i = 0; i < cclist.Count(); i++)
		paths.Add(cclist[i]->GetPath());
}

// Get the path names for the card catalogs in this list.
//
void
CARDCATS::GetPaths(STRING &paths)
{
	int	i;

	assert(objstate.IsReady());
	DbgFunc("CARDCATS::&GetPaths(STRING&)" << endl);

	paths = NULL_STRING;

	for (i = 0; i < cclist.Count(); i++) {
		if (i > 0)
			paths += ":";
		paths += cclist[i]->GetPath();
	}
}

// Comparison routine for sorting card catalog entries.
// "arg1" and "arg2" are pointers to pointers to ABINFO objects.
//
int
SortEntriesById(const void *arg1, const void *arg2)
{
	ABINFO	*info1   = *((ABINFO **) arg1);
	ABINFO	*info2   = *((ABINFO **) arg2);
	STRING	id1      = info1->Name().ABId();
	STRING	id2      = info2->Name().ABId();
	STRING	version1 = info1->Name().ABVersion();
	STRING	version2 = info2->Name().ABVersion();

	assert(info1 != NULL  &&  info2 != NULL);

	if (id1 == id2)
		return(StrCmp(version1, version2));
	else
		return(StrCmp(id1, id2));
}		

// Comparison routine for sorting card catalog entries.
// "arg1" and "arg2" are pointers to pointers to ABINFO objects.
//
int
SortEntriesByTitle(const void *arg1, const void *arg2)
{
	ABINFO	*info1   = *((ABINFO **) arg1);
	ABINFO	*info2   = *((ABINFO **) arg2);
	STRING	title1	 = info1->Title();
	STRING	title2	 = info2->Title();

	assert(info1 != NULL  &&  info2 != NULL);

	if (title1 == title2)
		return (SortEntriesById(arg1, arg2));
	else {
		if (! title1 && !title2)
			return(0);
		else if(!title1)
			return(-1);
		else if(!title2)
			return(1);
		else
			return(strcoll(title1, title2));
	}
}		
