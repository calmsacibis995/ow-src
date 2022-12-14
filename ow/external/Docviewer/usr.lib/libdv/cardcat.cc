#ident "@(#)cardcat.cc	1.5 95/06/27 Copyright 1992 Sun Microsystems, Inc."


#include <doc/abinfo.h>
#include <doc/cardcat.h>
#include <doc/utils.h>
#include "dvlocale.h"

// Card Catalog file header, version number.
//
static const STRING	CC_HEADER_MAGIC	("#<Card Catalog>");
static const STRING	CC_HEADER_FMT	("#<Card Catalog> version %d\n");
static const int	CC_HEADER_VERSION = 1;


CARDCAT::CARDCAT()
{

	DbgFunc("CARDCAT::CARDCAT: " << endl);

	objstate.MarkReady();
}

CARDCAT::~CARDCAT()
{
	DbgFunc("CARDCAT::~CARDCAT" << endl);

}

// return a Card Catalog magic #
const STRING &
CARDCAT::GetHeaderMagic ()
{
	return (CC_HEADER_MAGIC);
}

// Open the specified card catalog.
// "mode" is the same as for "fopen(3)".
//
CARDCAT *
CARDCAT::Open(const STRING &path, const STRING &mode, ERRSTK &err)
{
        char            lead_char;
	CARDCAT		*ret_cardcat;

	DbgFunc("CARDCAT::Open: " << path << " (" << mode << ")" << endl);
	lead_char = path [0];
	
	switch (lead_char) {
		
        case '+':
	
		ret_cardcat = NULL;

	default:
		ret_cardcat = CARDCAT_NFS::Open_NFS (path, mode, err);
		

	}
	
	return (ret_cardcat);
	
}

CARDCAT_NFS::CARDCAT_NFS(const STRING &path, FILE *fp) :
        ccpath  (path),
        ccfp    (fp)
{
        assert(ccfp   != NULL);
        assert(ccpath != NULL_STRING);
        DbgFunc("CARDCAT_NFS::CARDCAT_NFS: " << ccpath<< endl);

        objstate.MarkReady();
}

CARDCAT_NFS::~CARDCAT_NFS()
{
        DbgFunc("CARDCAT_NFS::~CARDCAT_NFS" << endl);

        if (ccfp)
                fclose(ccfp);
}

// Open for NFS Card Catalogs
CARDCAT *
CARDCAT_NFS::Open_NFS(const STRING &path, const STRING &mode, ERRSTK &err)
{
		
	
	FILE	*ccfp;

	DbgFunc("CARDCAT_NFS::Open_NFS: " << path << " (" << mode << ")" << endl);

	if ((ccfp = fopen(path, mode))  ==  NULL) {
		err.Init(DGetText("Can't open card catalog \"%s\": %s"),
			~path, SysErrMsg(errno));
		return(NULL);
	}

	return((CARDCAT *) new CARDCAT_NFS(path, ccfp));
}
	
// Retrieve the ABINFO record matching "abname".
//
STATUS
CARDCAT_NFS::GetMatch(const ABNAME &abname, ABINFO &info, ERRSTK &err)
{
	STATUS	status;


	assert(objstate.IsReady());
	DbgFunc("CARDCAT_NFS::GetMatch" << endl);


	for (status =  GetFirst(info, err);
	     status == STATUS_OK;
	     status =  GetNext(info, err)) {

		if (info.Name() == abname)
			return(STATUS_OK);
	}

	return(STATUS_FAILED);
}

// Retrieve the first ABINFO record in this card catalog.
//
STATUS
CARDCAT_NFS::GetFirst(ABINFO &info, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(ccfp != NULL);
	DbgFunc("CARDCAT_NFS::GetFirst" << endl);

	rewind(ccfp);

	return(GetNext(info, err));
}

// Retrieve the next ABINFO record in this card catalog.
//
STATUS
CARDCAT_NFS::GetNext(ABINFO &info, ERRSTK &err)
{
	STRING	infostr;


	assert(objstate.IsReady());
	assert(ccfp != NULL);
	DbgFunc("CARDCAT_NFS::GetNext" << endl);


	// Return next valid info record (line) from file.
	//
	while (GetLine(ccfp, infostr)  !=  NULL_STRING) {
		if (ABINFO::ParseInfoString(infostr, info, err)  ==  STATUS_OK)
			return(STATUS_OK);
	}

	return(STATUS_FAILED);
}

// Retrieve all ABINFO records in this card catalog.
// Records in list are all new ABINFOs (caller is responsible
// for deallocation),
//
STATUS
CARDCAT_NFS::GetAll(LIST<ABINFO*> &info_list, ERRSTK &err)
{
	ABINFO	info;
	STATUS	status;

	assert(objstate.IsReady());
	DbgFunc("CARDCAT_NFS::GetAll" << endl);


	for (status =  GetFirst(info, err);
	     status == STATUS_OK;
	     status =  GetNext(info, err)) {

		info_list.Add(new ABINFO(info));
	}


	// XXX can't distinguish between last record and error condition.
	//
	return(STATUS_OK);
}

// Rewrite this card catalog to contain the ABINFO records in
// the list provided.
//
STATUS
CARDCAT_NFS::Rewrite(LIST<ABINFO*> &info_list, ERRSTK &err)
{
	int	i;

	assert(objstate.IsReady());
	assert(ccfp != NULL);
	DbgFunc("CARDCAT_NFS::Rewrite" << endl);


	// Truncate card catalog file, then add each record in the
	// list, one at a time.
	//
	if (ftruncate(fileno(ccfp), 0)  !=  0) {
		err.Init(DGetText("Can't truncate card catalog \"%s\": %s"),
			~ccpath, SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	rewind (ccfp);
	
	// Write Card Catalog header.
	//
	fprintf(ccfp, CC_HEADER_FMT, CC_HEADER_VERSION);
	fprintf(ccfp, "#\n");
	fprintf(ccfp, "# This file was generated by cardcatadmin.\n");
	fprintf(ccfp, "#\n");
	fflush (ccfp);
	
	for (i = 0; i < info_list.Count(); i++) {
		if (Add(*info_list[i], err)  !=  STATUS_OK)
			return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

// Write out an abinfo record to this card catalog.
//
STATUS
CARDCAT_NFS::Add(const ABINFO &info, ERRSTK &err)
{
	STRING	infostr;


	assert(objstate.IsReady());
	assert(ccfp != NULL);
	DbgFunc("CARDCAT_NFS::Add" << info.Name() << endl);


	// Seek to end of card catalog file.
	//
	if (fseek(ccfp, 0, SEEK_END)  !=  0)  {
		err.Init(DGetText(
			"Can't seek to end of card catalog \"%s\": %s"),
			~ccpath, SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	// Convert record into string, then write it out
	// to the card catalog file.
	//
	if (ABINFO::MakeInfoString(info, infostr, err) 	!=  STATUS_OK)
		return(STATUS_FAILED);

	if (fputs(infostr, ccfp)  !=  infostr.Length()) {
		err.Init(DGetText("Can't write to card catalog \"%s\": %s"),
			~ccpath, SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}
