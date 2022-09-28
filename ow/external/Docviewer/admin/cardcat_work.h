#ident "@(#)cardcat_work.h	1.9 06/22/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/abinfo.h>
#include <doc/cardcat.h>
#include <doc/cardcats.h>
#include <doc/listx.h>
#include <doc/book.h>
#include <doc/abgroup.h>
#include <doc/abclient.h>
#include <doc/query.h>

// This class contains all the methods that do the real work
// of interfacing with a Card Catalog and performing administrative
// operations on it
// It's a separate object mainly because all the code it has was
// cluttering up cardcatadmin. Not surprisingly, its sole client
// is cardcatadmin

class CARDCAT_WORK {
private:
	STRING cc_name;
	STRING AB_id;
	STRING AB_version;
	STRING AB_tocpath;
	STRING AB_indexpath;
	STRING AB_pspath;
	STRING AB_title;
	

	OBJECT_STATE objstate;
	
	CARDCAT_WORK();
	
	// Test if the current cardcat_work settings (AB_id, AB_version...)
	// already exist in the given Card Catalog ABINFO list
	// returns the index of the matched item (if there is a match)	
	// in index
	BOOL TestMatch (LISTX<ABINFO*> &info_list, int& index);
	
	// make an ABINFO from AB_id, AB_version, AB_tocpath...
	STATUS MakeABINFO (ABINFO &info, ERRSTK &err);
	

public:
	~CARDCAT_WORK() {}
	
	static CARDCAT_WORK *Init();
	
	void SetCardCatName (const STRING &in_cc_name);
	void SetABId (const STRING &in_AB_id);
	void SetABVersion (const STRING &in_AB_version);
	void SetABTOCPath (const STRING &in_AB_tocpath);
	void SetABIndexPath (const STRING &in_AB_indexpath);
	void SetABPSPath (const STRING &in_AB_pspath);
	void SetABTitle (const STRING &in_title);
	

	// Is this file a Card Catalog
	BOOL IsACardCat (FILE *fp);

	// list all the Card Catalogs you can see 
	STATUS ListPaths (ERRSTK &err);
	
	// Get an ABINFO list from a Card Catalog
	STATUS GetInfoListFromCardCat (const STRING &input_file_name,
				       LISTX<ABINFO*> &info_list,
				       ERRSTK &err);
	// Get an ABINFO list from a Bookinfo file
	STATUS GetInfoListFromBookinfo (const STRING &input_file_name,
				       LISTX<ABINFO*> &info_list,
				       ERRSTK &err);
		

	// Add an info_list to a Card Catalog
	STATUS AddInfoListToCardCat (LISTX<ABINFO*> &info_list, 
				     ERRSTK &err);
	
	
	// add AB's from another Card Catalog file
	STATUS DoAddFromFile (const STRING &file, ERRSTK &err);

	// Add a new AB entry from args - set via appropriate set call
	STATUS DoAddFromArgs (ERRSTK &err);
	
	// Remove an AB entry
	STATUS DoRemove (ERRSTK &err);
	
	// Modify an AB entry
	STATUS DoModify (ERRSTK &err);
	
	// Match an AB entry
	STATUS DoMatch (ERRSTK &err);
	
	// List all AB entries in this CC
	STATUS DoList (ERRSTK &err);

       // print ABName. 

       void printABName(ABNAME* aName);

      // replace colons in title field with a blank:

      STRING* deleteColonsInTitle(STRING& nameStr);

      STATUS ConductSearch(ABINFO* abInfo, const STRING& cardcatname);
      STATUS VerifyBookExists(ABINFO* abInfo, const STRING& cardcat);
      STATUS VerifyPaths(ABINFO* abInfo);
      STATUS Verify(const STRING& idStr, const STRING& versionStr, const STRING& cardcatname, ERRSTK &err);

	
};


		
		
