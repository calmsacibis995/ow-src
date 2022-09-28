#ident "@(#)bookmark.cc	1.6 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/bookmark.h>


void
BOOKMARK::SetTitle(const STRING &t)
{
	int	newline;

	// Make sure title does not contain newline character.
	//
	switch (newline = t.Index('\n')) {

	case -1:		// no newline in title
		title = t;
		break;
	case 0:			// title == "\n"
		title = NULL_STRING;
		break;
	default:		// truncate at first newline in title
		title = t.SubString(0, newline-1);
		break;
	}

	DbgFunc("BOOKMARK::SetTitle: " << title << endl);
}

void
BOOKMARK::SetAnnotation(const STRING &annot)
{
	int	len = annot.Length();

	// Truncate terminating newline.
	//
	if (len > 0  &&  annot[len-1] == '\n')
		annotation = annot.SubString(0, len-2);
	else
		annotation = annot;

	DbgFunc("BOOKMARK::SetAnnotation: " << annotation << endl);
}

void
BOOKMARK::SetAnswerBookTitle(const STRING &abtitle_arg)
{
	abtitle = abtitle_arg;

	DbgFunc("BOOKMARK::SetAnswerBookTitle: " << abtitle << endl);
}

ostream &
operator << (ostream &ostr, const BOOKMARK *bookmark)
{
	assert(bookmark != NULL);

	ostr << "name    = "	<< bookmark->DocName()		<< endl;
	ostr << "title   = "	<< bookmark->Title()		<< endl;
	ostr << "annot   = "	<< bookmark->Annotation()	<< endl;
	ostr << "abtitle = "	<< bookmark->AnswerBookTitle()	<< endl;

	return(ostr);
}
