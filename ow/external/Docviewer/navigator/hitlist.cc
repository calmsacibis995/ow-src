#ident "@(#)hitlist.cc	1.76 93/02/17 Copyright 1990-1992 Sun Microsystems, Inc."


// This module implements the HITLIST user interface class.

#include "hitlist.h"
#include <doc/searchdoc.h>
#include <doc/abgroup.h>
#include <doc/document.h>

// Private HIT class for HITLIST.
//
class	HIT : public SEARCHDOC {
    private:
	STRING		doctitle;
	STRING		booktitle;
	STRING		abtitle;
	BOOL		is_valid;
	BOOL		is_header;
	OBJECT_STATE	objstate;
	static ABGROUP	*abgroup;
	void		Init();
    public:
	HIT(const DOCNAME &name);
	HIT(const SEARCHDOC &searchdoc);
	~HIT()		{}
	BOOL		IsValid() const		{ return(is_valid); }
	BOOL		IsHeader() const	{ return(is_header); }
	void		MarkHeader()		{ is_header=BOOL_TRUE;}
	const STRING	&DocTitle() const	{ return(doctitle); }
	const STRING	&BookTitle() const	{ return(booktitle); }
	const STRING	&ABTitle() const	{ return(abtitle); }
	static void	SetABGroup(ABGROUP *abg){ abgroup = abg; }
};

ABGROUP	*HIT::abgroup = NULL;


// The following structure defines the bits to be used for
// the glyphs within the relevance ranked search hit list.
// Glyph_bits contains the 16x16x1 bit patterns and
// image contains the actual server image handles.
//

#define	NUMBER_OF_RANGES	(5)
static u_short glyph_bits[NUMBER_OF_RANGES][16] = {
{
#include <images/square_0.pr>
},
{
#include <images/square_25.pr>
},
{
#include <images/square_50.pr>
},
{
#include <images/square_75.pr>
},
{
#include <images/square_black.pr>
}
};

static short		blank_bits[16] = {
#include <images/blank.pr>
};

static Server_image	glyphs[NUMBER_OF_RANGES];
static Server_image	blank_glyph = XV_NULL;
static Server_image	GetGlyphForWeight(int);


HITLIST::HITLIST(Xv_opaque panel, int x, int y, ABGROUP *abgroup_arg) :
	WINLIST		(panel, x, y),
	abgroup		(abgroup_arg)
{ 
	int	i;


	assert(panel != NULL);
	assert(abgroup != NULL);
	DbgFunc("HITLIST::HITLIST" << endl);


	// Create server images for each of the relevance ranges.
	//
	for (i = 0; i < NUMBER_OF_RANGES; i++) {

		glyphs[i] = xv_create(NULL, SERVER_IMAGE,
				XV_WIDTH,		16,
				XV_HEIGHT,		16,
				SERVER_IMAGE_BITS,	glyph_bits[i],
				NULL);

		if (glyphs[i] == NULL)
			OutOfMemory();
	}

	blank_glyph = xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH,		16,
			XV_HEIGHT,		16,
			SERVER_IMAGE_BITS,	blank_bits,
			NULL);

	if (blank_glyph == XV_NULL)
		OutOfMemory();


	xv_set(win_list, XV_HELP_DATA, SEARCH_HITLIST_HELP, NULL);


	HIT::SetABGroup(abgroup);


	// We're ready to roll...
	//
	objstate.MarkReady();
}

HITLIST::~HITLIST()
{ 
	DbgFunc("HITLIST::~HITLIST" << endl);
}

// Clear this Hit List.
//
void
HITLIST::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("HITLIST::Clear" << endl);

	hitlist.Clear();
	WINLIST::Clear();
}

// Display the HITLIST from the Query.
//
STATUS
HITLIST::Display(LIST<SEARCHDOC*>	&results,
		 SORT_SEARCH_RESULTS	sort_order,
		 ERRSTK &)
{
	HIT		*hit;
	Server_image	glyph;
	STRING		title;
	int		i;


//XXX	assert(objstate.IsReady());
	assert(abgroup != NULL);
	DbgFunc("HITLIST::Display" << endl);


	// Clear the current hitlist, then copy the hits from the
	// results list into the hitlist.
	//
	HITLIST::Clear();
	for (i = 0; i < results.Count(); i++) {
		hit = new HIT(*results[i]);
		if ( ! hit->IsValid()) {
			delete hit;
			continue;
		}
		hitlist.Add(hit);
	}


	// Sort hitlist.
	//
	SortHitList(hitlist, sort_order);


	WINLIST::BeginBatch();


	// Display title & glyph for each hit in hitlist.
	//
	for (i = 0; i < hitlist.Count(); i++) {

		hit = hitlist[i];

		switch (sort_order) {

		case SORT_BY_RANK:
			title  = hit->DocTitle();
			title += "   (" + hit->BookTitle() + ")";
			glyph  = GetGlyphForWeight(hit->Weight());
			AppendEntry(title, glyph);
			break;

		case SORT_BY_BOOK:
			if (hit->IsHeader()) {
			    title  = hit->BookTitle();
			    title += "   (" + hit->ABTitle() + ")";
			    AppendHeaderEntry( title );
			} else {
			    title  = "     " + hit->DocTitle();
			    glyph  = GetGlyphForWeight(hit->Weight());
			    AppendEntry(title, glyph);
			}
			break;
		}
	}


	WINLIST::EndBatch();
	assert(hitlist.Count() == WINLIST::NumEntries());

	return(STATUS_OK);
}

// Get number of actual hits (not counting book titles)
//
int
HITLIST::NumHits()
{
	int		numhits = 0;
	HIT		*hit;
	int		i;


//XXX	assert(objstate.IsReady());
	assert(abgroup != NULL);
	DbgFunc("HITLIST::NumHits" << endl);


	for (i = 0; i < hitlist.Count(); i++) {

		hit = hitlist[i];

		if ( !hit->IsHeader() ) {
			numhits++;
		}

	}

	return( numhits );
}

void
HITLIST::SortHitList(LISTX<HIT*> &hitlist, SORT_SEARCH_RESULTS order)
{
	HIT		*hit;
	DOCNAME		bookname;
	BOOKNAME	bkname1;
	BOOKNAME	bkname2;
	int		i;

//XXX	assert(objstate.IsReady());
	DbgFunc("HITLIST::SortHitList" << endl);


	switch (order) {

	case SORT_BY_RANK:	// Sort hit list by relevance rank.
		SortList(hitlist, SortHitListByRank);
		break;

	case SORT_BY_BOOK:
		SortList(hitlist, SortHitListByBook);

		for (i = 0; i < hitlist.Count(); i++) {

			if (i > 0)
				bkname1 = hitlist[i-1]->DocName().BookName();
			bkname2 = hitlist[i]->DocName().BookName();

			if (StrCmp(bkname1.BookId(), bkname2.BookId()) != 0) {
				MakeBookRootDocName(bkname2, bookname);
				hit = new HIT(bookname);
				hit->MarkHeader();
				hitlist.Insert(hit, i);
				++i;
			}
		}
		break;

	default:
		assert(0);
	}
}

// Comparison routine for sorting search hitlist.
// "arg1" and "arg2" are pointers to list elements, i.e., type "HIT**".
//
int
HITLIST::SortHitListByRank(const void *arg1, const void *arg2)
{
	HIT	*hit1 = *((HIT **) arg1);
	HIT	*hit2 = *((HIT **) arg2);

	assert(hit1 != NULL  &&  hit2 != NULL);

	return(hit2->Weight() - hit1->Weight());
}

// Comparison routine for sorting search hitlist.
// "arg1" and "arg2" are pointers to list elements, i.e., type "HIT**".
//
int
HITLIST::SortHitListByBook(const void *arg1, const void *arg2)
{
	HIT	*hit1 = *((HIT **) arg1);
	HIT	*hit2 = *((HIT **) arg2);
	int	n;

	assert(hit1 != NULL  &&  hit2 != NULL);

	if ((n = StrCmp(hit1->ABTitle(), hit2->ABTitle()))  !=  0)
		return(n);

	if ((n = StrCmp(hit1->BookTitle(), hit2->BookTitle()))  !=  0)
		return(n);

	return(hit2->Weight() - hit1->Weight());
}

Server_image
GetGlyphForWeight(int weight)
{
	int	index;

	DbgFunc("GetGlyphForWeight" << endl);

	// Calculate the relevance range.
	//		 0-19 %	= "images/blank.pr"
	//		20-39 % = "images/square_25.pr"
	//		40-59 % = "images/square_50.pr"
	//		60-79 % = "images/square_75.pr"
	//		80-100% = "images/square_black.pr"

	index = (weight-1)*NUMBER_OF_RANGES/1000;
	if (index >= NUMBER_OF_RANGES)
		index = NUMBER_OF_RANGES -1 ;

	return(glyphs[index]);
}

HIT::HIT(const DOCNAME &name) :
	SEARCHDOC	(name, 0),
	is_valid	(BOOL_FALSE),
	is_header	(BOOL_FALSE)
{
	DbgFunc("HIT::HIT: " << this << endl);
	Init();
}

HIT::HIT(const SEARCHDOC &searchdoc) :
	SEARCHDOC	(searchdoc),
	is_valid	(BOOL_FALSE),
	is_header	(BOOL_FALSE)
{
	DbgFunc("HIT::HIT: " << this << endl);
	Init();
}

void
HIT::Init()
{
	DOCUMENT	*doc;
	DOCUMENT	*book;
	DOCUMENT	*ab;
	DOCNAME		bookname;
	DOCNAME		abname;
	ERRSTK		err;


	assert(abgroup != NULL);
	DbgFunc("HIT::Init" << endl);


	// Look up document.
	//
	if ((doc = abgroup->LookUpDoc(DocName(), 0, err))  ==  NULL) {
		DbgHigh("HIT::Init: can't find doc: " << DocName() << endl);
		return;
	}


	// Skip documents marked as "No show"
	// (but what are they doing showing up here?).
	//
	if (doc->IsNoShow()) {
		DbgHigh("HIT::Init: doc is nodisplay: " << DocName() << endl);
		delete doc;
		return;
	}


	// Get document's title.
	//
	if (doc->Title() != NULL_STRING)
		doctitle = doc->Title();
	else
		doctitle = gettext("[No Title]");
	delete doc;


	// Get titles of document's book and AnswerBook.
	//
	MakeBookRootDocName(DocName(), bookname);
	book = abgroup->LookUpDoc(bookname, 0, err);

	if (book != NULL)
		booktitle = book->Title();
	if (booktitle == NULL_STRING)
		booktitle = gettext("[No Title]");
	delete book;

	MakeAnswerBookRootDocName(DocName(), abname);
	ab = abgroup->LookUpDoc(abname, 0, err);
	if (ab != NULL)
		abtitle = ab->Title();
	if (abtitle == NULL_STRING)
		abtitle = gettext("[No Title]");
	delete ab;


	is_valid = BOOL_TRUE;
	objstate.MarkReady();
}

// Get the DOCNAME of the document corresponding to
// the specified entry in this Location list.
//
const DOCNAME &
HITLIST::GetDocName(int n)
{
	assert(n < hitlist.Count());
	assert(hitlist[n]->IsValid());

	return(hitlist[n]->DocName());
}
