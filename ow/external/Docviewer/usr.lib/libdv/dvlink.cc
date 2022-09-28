#ident "@(#)dvlink.cc	1.12 06/10/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/dvlink.h>
#include <doc/token_list.h>


DVLINK::DVLINK() :
	link_type(DVLINK_INVALID)
{
	DbgFunc("DVLINK::DVLINK" << endl);

	objstate.MarkReady();
}

STATUS
DVLINK::SetCookie(const STRING &linkcookie)
{
	assert(objstate.IsReady());
	DbgFunc("DVLINK::SetCookie: " << linkcookie << endl);


	cookie = STRING::CleanUp(linkcookie);

	if (cookie == NULL_STRING  ||  ParseIt() != STATUS_OK) {
		link_type = DVLINK_INVALID;
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

// Is this a valid link?
//
BOOL
DVLINK::IsValid() const
{
	assert(objstate.IsReady());
	DbgFunc("DVLINK::IsValid" << endl);

	if (link_type == DVLINK_INVALID)
		return(BOOL_FALSE);
	else
		return(BOOL_TRUE);
}

STATUS
DVLINK::ParseIt()
{
	STRING		typestr;
	STRING		pgstr;
	int		colon;
	DOCNAME		docname;


	assert(objstate.IsReady());
	assert(cookie != NULL_STRING);
	DbgFunc("DVLINK::ParseIt: " << cookie << endl);


	if ((colon = cookie.Index(':'))  <  0) {
		typestr = "view";
		link_body = cookie;
	} else {
		typestr = cookie.SubString(0, colon-1);
		link_body = cookie.SubString(colon+1, END_OF_STRING);
	}


	if (typestr == "view")
		link_type = DVLINK_VIEWFILE;
	else if (typestr == "system")
		link_type = DVLINK_SYSTEM;
	else if (typestr == "print")
		link_type = DVLINK_PRINT;
	else if (typestr == "printfile")
		link_type = DVLINK_PRINTFILE;
	else {
		link_type = DVLINK_VIEWFILE;
		link_body = cookie;
	}


	// "view" links take some additional processing.
	//
	if (link_type == DVLINK_VIEWFILE) {

		if ((colon = link_body.Index(':'))  <  0) {
			view_page = VIEWPAGE_FIRST;
		} else {
			pgstr = link_body.SubString(colon+1, END_OF_STRING);
			pgstr = STRING::CleanUp(pgstr);
			link_body = link_body.SubString(0, colon-1);

			if (pgstr == "first")
				view_page = VIEWPAGE_FIRST;
			else if (pgstr == "last")
				view_page = VIEWPAGE_LAST;
			else if (pgstr == "next")
				view_page = VIEWPAGE_NEXT;
			else if (pgstr == "prev")
				view_page = VIEWPAGE_PREV;
			else if (pgstr == NULL_STRING)
				view_page = 1;
			else {
				if ((view_page = atoi(pgstr))  <  1)
					view_page = 1;
			}
		}

		link_body = STRING::CleanUp(link_body);


		// If link body is a DOCNAME, this is actually a
		// "view document" link.  Make sure the docname is valid.
		//
		if (link_body.Length() > 0  &&  link_body[0] == '<') {
			link_type = DVLINK_VIEWDOCUMENT;
			if (docname.Init(link_body) != STATUS_OK)
				return(STATUS_FAILED);
			int offset;
			if ((offset = docname.Offset()) != 0)
				view_page = offset;
		}
	}


	return(STATUS_OK);
}

// Get "view file" parameters (file and page number).
// Assumes this is a valid DVLINK_VIEWFILE link.
//
void
DVLINK::ViewFile(STRING &file, int &page) const
{
	assert(objstate.IsReady());
	assert(link_type == DVLINK_VIEWFILE);

	file = link_body;
	page = view_page;

	DbgFunc("DVLINK::ViewFile: " << file << "(" << page << ")" << endl);
}

// Get "view document" parameters (document name and page number).
// Assumes this is a valid DVLINK_VIEWDOCUMENT link.
//
void
DVLINK::ViewDocument(DOCNAME &docname, int &page) const
{
	assert(objstate.IsReady());
	assert(link_type == DVLINK_VIEWDOCUMENT);

	if (docname.Init(link_body) != STATUS_OK) {
		assert(0);
	}
	page = view_page;

	DbgFunc("DVLINK::ViewDocument: " << docname << "(" << page<<")"<<endl);
}

// Get "print" parameters (file or list of files to print).
// Link body is a comma-separated list of files and page ranges, e.g.:
//
//	file1:1-20,file2:1-10:file3,file4
//
// Assumes this is a valid DVLINK_PRINT link.
//
void
DVLINK::Print(LIST<STRING> &files_to_print) const
{
	TOKEN_LIST	files(link_body, ',');
	int		i;

	assert(objstate.IsReady());
	assert(link_type == DVLINK_PRINT);
	DbgFunc("DVLINK::Print: " << link_body << endl);

	files_to_print.Clear();
	for (i = 0; i < files.Count(); i++)
		files_to_print.Add(files[i]);
}

// Get "print file" parameters (name of file containing list of
// files to print).
// Assumes this is a valid DVLINK_PRINTFILE link.
//
void
DVLINK::PrintFile(STRING &printfile) const
{
	assert(objstate.IsReady());
	assert(link_type == DVLINK_PRINTFILE);

	printfile = link_body;

	DbgFunc("DVLINK::PrintFile: " << printfile << endl);
}

// Get shell command parameters.
// Assumes this is a valid DVLINK_SYSTEM link.
//
void
DVLINK::SystemCmd(STRING &syscmd) const
{
	assert(objstate.IsReady());
	assert(link_type == DVLINK_SYSTEM);

	syscmd = link_body;

	DbgFunc("DVLINK::SystemCmd: " << syscmd << endl);
}

const STRING
MakeViewDocumentLink(const DOCNAME &docname, int page)
{
	if ( ! docname.IsValid())
		return(NULL_STRING);
	else
		return(MakeViewFileLink(docname, page));
}

const STRING
MakeViewFileLink(const STRING &file, int page)
{
	STRING	link;
	char	buf[20];


	link = "view:" + file;


	switch (page) {
	case VIEWPAGE_FIRST:
		link += ":first";
		break;
	case VIEWPAGE_LAST:
		link += ":last";
		break;
	case VIEWPAGE_NEXT:
		link += ":next";
		break;
	case VIEWPAGE_PREV:
		link += ":prev";
		break;
	default:
		if (page < 1)
			page = 1;
		sprintf(buf, ":%d", page);
		link += buf;
		break;
	}


	DbgFunc("MakeViewFileLink: " << link << endl);
	return(link);
}

const STRING
MakeSystemLink(const STRING &command)
{
	STRING	link;

	DbgFunc("MakeSystemLink: " << command << endl);
	return(link = "system:" + command);
}
