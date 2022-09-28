#ifndef	_CARDCAT_H
#define	_CARDCAT_H

#ident "@(#)cardcat.h	1.4 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/abname.h>
#include <doc/list.h>

// The CARDCAT object encapsulates the AnswerBook configuration mechanism
// for the 2.0 version of DocViewer.  This CARDCAT mechanism supercedes
// the BOOKINFO mechanism used in the 1.x DocViewer releases.
// CARDCAT simplifies BOOKINFO's AnswerBook component location mechanism
// and extends it to support networks containing multiple AnswerBooks.
//
// CARDCAT provides the following major functions:
//
//	o List all AnswerBooks available on network and/or local system
//	o Find a particular AnswerBook based on its id and version number.
//	o Get attributes of a particular AnswerBook needed for browsing,
//	  grouping, etc. operations
//	o Get location information for components of a particular AnswerBook
//
//
// CARDCAT Databases
//
// The CARDCAT mechanism is driven by CARDCAT databases - files or NIS+ maps
// containing configuration information for each AnswerBook in a network
// and/or on a local system.  An CARDCAT database is an ascii database
// containing one record (line) per AnswerBook.  Each AnswerBook's record
// contains the following information:
//
//	o id:		Unique id for this AnswerBook
//	o title:	AnswerBook name
//	o version:	AnswerBook version #
//	o languages:	Foreign languages supported by this AnswerBook
//	o tocpath:	Pathname of directory containing AB's toc databases.
//			(see "Directory Layout" below)
//	o pspath:		Pathname of directory containing AB's PostScript
//			directories (see "Directory Layout" below)
//	o indexpath:	Pathname of directory containing AB's search index
//			(see "Directory Layout" below)
//	o psproto:	network-absolute version of pspath - used for
//			generating automount tables for this AB
//	o tocproto:	ditto for TOC path
//	o indexproto:	ditto for index path
//OR MAYBE:
//	o autopath	For automount installations
//	o ps_abspath
//	o toc_abspath
//	o index_abspath
//AND
//	o pspath		For non-automount installations
//	o tocpath
//	o indexpath
//
//
// 
// 	cardcat.info	The NIS+ map
// 
// 


// Forward references.
//
class	ABINFO;


class	CARDCAT {

    private:

	// Current state of this object.
	//
	OBJECT_STATE	objstate;




    public:

	// Card catalog constructor.
	// It should be private because we want "Open()" to be the only
	// avenue for creating new card catalog objects. However the
	// compiler complains because the derived classes cannot 
	// implicitly call it as a (private) constructor 
	//
	// Note that as this is an abstract class the constructor can never be
	// called explicitly only implicitly during the creation of derived 
	// class objects.
	//
	CARDCAT();

	// Card catalog destructor.
	//
	// Declare the destructor as virtual so that the correct delete 
	// operator for the derived class object is called. 
	//
	virtual ~CARDCAT();

	// Open the specified card catalog.
	// "mode" is the same as for "fopen(3)".
	//
	// A path name beginning with "+" is taken as specifying 
	// an NIS+ map.  Otherwise the name is interpreted as the pathname of
	// a text file database.  If the specified database does not exist an 
	// attempt is made to create it. Caller is responsible for deallocation.
	//
	static CARDCAT	*Open(const STRING &path, const STRING &mode, ERRSTK&);
	

	// Retrieve the ABINFO record for the specified AnswerBook.
	//
	virtual STATUS		GetMatch(const ABNAME &abname, ABINFO &info, 
					 ERRSTK &) = 0;
	

	// Retrieve the first/next ABINFO record in this card catalog.
	//
	virtual STATUS		GetFirst(ABINFO &info, ERRSTK &) = 0;
	
	virtual STATUS		GetNext(ABINFO &info, ERRSTK &) = 0;
	

	// Retrieve all ABINFO records in this card catalog.
	// Records in list are all new ABINFOs (caller is responsible
	// for deallocation),
	//
	virtual STATUS		GetAll(LIST<ABINFO*> &info_list, ERRSTK &) = 0;
	

	// Add an ABINFO entry to this card catalog.
	//
	virtual STATUS		Add(const ABINFO &info, ERRSTK &) = 0;
	

	// Rewrite this card catalog to contain the ABINFO records in
	// the list provided.
	//
	virtual STATUS		Rewrite(LIST<ABINFO*> &info_list, ERRSTK &) = 0;
	

	// Get the path name for this card catalog.
	//
	virtual const STRING	&GetPath() const = 0;

	// get the magic # for a card catalog
	static const STRING 	&GetHeaderMagic();
	

};

// This is the NFS implementation of the CARDCAT object - derived from
// CARDCAT

class	CARDCAT_NFS: public CARDCAT {

    private:

	// Path name for this card catalog.
	//
	STRING		ccpath;

	// File handle for this card catalog.
	//
	FILE		*ccfp;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Card catalog constructor.
	// It's private because we want "Open()" to be the only
	// avenue for creating new card catalog objects.
	//
	CARDCAT_NFS(const STRING &path, FILE *fp);


    public:

	// Card catalog destructor.
	//
	~CARDCAT_NFS();

	static CARDCAT	*Open_NFS(const STRING &path, 
				  const STRING &mode, ERRSTK&);

	// Retrieve the ABINFO record for the specified AnswerBook.
	//
	STATUS		GetMatch(const ABNAME &abname, ABINFO &info, ERRSTK &);

	// Retrieve the first/next ABINFO record in this card catalog.
	//
	STATUS		GetFirst(ABINFO &info, ERRSTK &);
	STATUS		GetNext(ABINFO &info, ERRSTK &);

	// Retrieve all ABINFO records in this card catalog.
	// Records in list are all new ABINFOs (caller is responsible
	// for deallocation),
	//
	STATUS		GetAll(LIST<ABINFO*> &info_list, ERRSTK &);

	// Add an ABINFO entry to this card catalog.
	//
	STATUS		Add(const ABINFO &info, ERRSTK &);

	// Rewrite this card catalog to contain the ABINFO records in
	// the list provided.
	//
	STATUS		Rewrite(LIST<ABINFO*> &info_list, ERRSTK &);

	// Get the path name for this card catalog.
	//
	const STRING	&GetPath() const	{ return(ccpath); }


};

// This is the NIS+ implementation of the CARDCAT object - it is derived
// from CARDCAT
// XXX - NOT IMPLEMENTED YET!!! DON'T BE SURPRISED IF IT LOOKS JUST
// LIKE THE NFS IMPLEMENTATION - BECAUSE IT IS!!


// class	CARDCAT_NIS : public CARDCAT {
//
//    private:
//
//	// Path name for this card catalog.
//	//
//	STRING		ccpath;
//
//	// File handle for this card catalog.
//	//
//	FILE		*ccfp;
//
//	// Current state of this object.
//	//
//	OBJECT_STATE	objstate;
//
//	// Card catalog constructor.
//	// It's private because we want "Open()" to be the only
//	// avenue for creating new card catalog objects.
//	//
//	CARDCAT_NIS(const STRING &path, FILE *fp);
//
//
//    public:
//
//	// Card catalog destructor.
//	//
//	~CARDCAT_NIS();
//
//
//	// Retrieve the ABINFO record for the specified AnswerBook.
//	//
//	STATUS		GetMatch(const ABNAME &abname, ABINFO &info, ERRSTK &);
//
//	// Retrieve the first/next ABINFO record in this card catalog.
//	//
//	STATUS		GetFirst(ABINFO &info, ERRSTK &);
//	STATUS		GetNext(ABINFO &info, ERRSTK &);
//
//	// Retrieve all ABINFO records in this card catalog.
//	// Records in list are all new ABINFOs (caller is responsible
//	// for deallocation),
//	//
//	STATUS		GetAll(LIST<ABINFO*> &info_list, ERRSTK &);
//
//	// Add an ABINFO entry to this card catalog.
//	//
//	STATUS		Add(const ABINFO &info, ERRSTK &);
//
//	// Rewrite this card catalog to contain the ABINFO records in
//	// the list provided.
//	//
//	STATUS		Rewrite(LIST<ABINFO*> &info_list, ERRSTK &);
//
//	// Get the path name for this card catalog.
//	//
//	const STRING	&GetPath()	{ return(ccpath); }
//};
#endif	_CARDCAT_H
