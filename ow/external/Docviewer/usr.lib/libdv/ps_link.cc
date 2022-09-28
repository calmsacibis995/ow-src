#ident "@(#)ps_link.cc	1.4 02/15/93 Copyright 1991 Sun Microsystems, Inc."

#include <doc/ps_link.h>
#include <doc/token_list.h>
#include "dvlocale.h"
#include <values.h>

// Miscellaneous defines
#define		COLON		':'

/*
 * Given a string, parse it, extract the link information and fill out a
 * the data strutures
 */
STATUS
PSLINK::Init(const STRING &str, ERRSTK  &err)
{
	const int	minNumFields	= 4;
	TOKEN_LIST	toklist(str, COLON);
	STATUS		status		= STATUS_OK;

	DbgFunc("PSLINK::Init: entered" << endl);

	if (toklist.Count() < minNumFields) {
		status = STATUS_FAILED;
		DbgHigh("PSLINK::SetLink: badly formed link \""
			<< link << "\"" << endl);
	}
	else {
		STRING		cookie_str = str;
		register int	cntr;
		int		index;

		// Find the link cookie - the char after the fourth ":"
		//
		for (cntr = 0; cntr < minNumFields; cntr++) {
			index = cookie_str.Index(COLON) + 1;
			cookie_str =
				cookie_str.SubString(index, END_OF_STRING);
		}

		// Set the cookie
		//
		if ((status = SetCookie(cookie_str)) == STATUS_OK) {

			// Set the link attributes & BBox
			status = SetAttrs(toklist[1], err);

			if (status == STATUS_OK)
				status = SetBBox(toklist[2], err);
		}
		else {
			err.Init(DGetText("Could not parse link cookie \"%s\""),
				 ~str.SubString(index+1, END_OF_STRING));
		}
	}

	if (status != STATUS_OK)
		err.Push(DGetText("Could not parse link comment \"%s\""), ~str);

	return (status);
}

STATUS
PSLINK::SetAttrs(const STRING &str, ERRSTK &err)
{
	STATUS status = STATUS_OK;

	DbgFunc("PSLINK::SetAttrs: entered\n");

	if (sscanf(str, "%x", &attrs) != 1) {
		err.Init(DGetText("Could not parse link attribute \"%s\""),
			 ~str);
		status = STATUS_FAILED;
	}
	else if (attrs == 2)
		attrs = DOUBLE_CLICK;

	return(status);
}

STATUS
PSLINK::SetBBox(const STRING &str, ERRSTK &err)
{
	int	ll_x;
	int	ll_y;
	int	ur_x;
	int	ur_y;
	STATUS	status = STATUS_OK;

	DbgFunc("PSLINK::SetBBox: \"" << str << "\"" << endl);

	// Get the vubox into a vubox structure
	if (sscanf(str, "%d %d %d %d", &ll_x, &ll_y, &ur_x, &ur_y)  !=  4) {
		DbgHigh("PSLINK::SetBBox: bad bbox comment" << endl);
		err.Init(DGetText("Could not parse display box field"));
		status = STATUS_FAILED;
	}
	else {
		DbgLow("PSLINK::SetBBox:"
		       << " ll_x = " << ll_x
		       << " ll_y = " << ll_y
		       << " ur_x = " << ur_x
		       << " ur_y = " << ur_y
		       << endl);


		// Fill in the PostScript BoundingBox
		//
		bbox.ll_x = ll_x;
		bbox.ll_y = ll_y;
		bbox.ur_x = ur_x;
		bbox.ur_y = ur_y;
	}

	return(status);
}

#ifdef	DEBUG
ostream &
operator << (ostream &ostr, const PSLINK &link)
{
	return(ostr
	       << "attrs=" << (unsigned) link.GetAttrs() << " "
	       << "bbox="
	       << link.bbox.ll_x << " "
	       << link.bbox.ll_y << " "
	       << link.bbox.ur_x << " "
	       << link.bbox.ur_y << " "
	       << "cookie=\"" << link.GetCookie() << "\"");
}	
#endif	/* DEBUG */
