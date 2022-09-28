#ident "@(#)dbmbook.cc	1.11 93/02/15 Copyright 1989-1991 Sun Microsystems, Inc."

#include <doc/dbmbook.h>
#include <doc/dbmdoc.h>
#include <doc/xxdoc.h>
#include <doc/attrlist.h>
#include <rpc/rpc.h>
#include <fcntl.h>
#include "dvlocale.h"

static const STRING	DOC_ID("ID");
static const STRING	BOOK_VERSION("1.0");
static const STRING	BOOK_INFO_KEY("_CDB_INFO_");
static const STRING	INFO_BOOKID("CID");
static const STRING	INFO_VERSION("VER");

// Database key/record conversion routines.
//
static STATUS	CvtStringToDatum(const STRING &s, datum *d, ERRSTK &);
static STATUS	CvtAttrListToDatum(ATTRLIST &attrs, datum *d, ERRSTK &);
static STATUS	CvtDatumToAttrList(datum *d, ATTRLIST &attrs, ERRSTK &);



DBMBOOK::DBMBOOK(const BOOKNAME &name, const STRING &path, DBM *handle)
			: BOOK(name, path)
{
	assert(handle != NULL);
	DbgFunc("DBMBOOK::DBMBOOK" << endl);

	dbhandle = handle;
}

DBMBOOK::~DBMBOOK()
{
	DbgFunc("DBMBOOK::~DBMBOOK" << endl);

	if (objstate.IsReady()) {
		assert(dbhandle != NULL);
		dbm_close(dbhandle);
	}
}

BOOL
DBMBOOK::Exists(const STRING &path)
{
	DbgFunc("BOOK::Exists: " << path << endl);

	if (access(path + ".dir", F_OK) == 0  &&
	    access(path + ".pag", F_OK) == 0) {
		return(BOOL_TRUE);
	} else {
		return(BOOL_FALSE);
	}
}

STATUS
DBMBOOK::Remove(const STRING &path, ERRSTK &err)
{
	DbgFunc("DBMBOOK::Remove: " << path << endl);

	if (unlink(path + ".dir") == 0  &&
	    unlink(path + ".pag") == 0) {
		return(STATUS_OK);
	} else {
		err.Init(DGetText("can't remove book database %s: %s"),
			~path, SysErrMsg(errno));
		return(STATUS_FAILED);
	}
}

DBMBOOK *
DBMBOOK::Open(	const BOOKNAME &name,
		const STRING &path,
		int flags,
		int mode,
		ERRSTK &err)
{
	DBMBOOK		*newbook;		// new book
	DBM		*handle;		// handle for dbm database
	ERRSTK		notused;


	DbgFunc("DBMBOOK::Open " << name
		<< ": path="	<< path
		<< ", flags="	<< flags
		<< ", mode="	<< mode
		<< endl);


	// Require that id and pathname be non-null.
	//
	if (path == NULL_STRING  ||  ! name.IsValid()) {
		err.Init(DGetText("can't open book '%s' (%s): %s"),
			~name, ~path, SysErrMsg(EINVAL));
		return(NULL);
	}


	// Create new database.
	//
	if (flags & O_CREAT) {

		// Make sure database doesn't already exist.
		//
		if (DBMBOOK::Exists(path)) {
			err.Init(DGetText(
				"can't create new book '%s' (%s): %s"),
				~name, ~path, DGetText("book already exists"));
			return(NULL);
		}

		if ((handle = dbm_open(path, flags, mode))  ==  NULL) {
			err.Init(DGetText(
				"can't create new book '%s' (%s): %s"),
				~name, ~path, SysErrMsg(errno));
			return(NULL);	
		}

		newbook = new DBMBOOK(name, path, handle);
		if (newbook == NULL) {
			err.Init(DGetText(
				"can't create new book '%s' (%s): %s"),
				~name, ~path, SysErrMsg(ENOMEM));
			dbm_close(handle);
			DBMBOOK::Remove(path, notused);
			return(NULL);
		}

		if (newbook->Init(err) != STATUS_OK) {
			err.Push(DGetText("can't create new book '%s' (%s)"),
				~name, ~path);
			dbm_close(handle);
			delete(newbook);
			DBMBOOK::Remove(path, notused);
			return(NULL);
		}

	} else {

		// Open an existing database.
		//
		if ((handle = dbm_open(path, flags, mode))  ==  NULL) {
			err.Init(DGetText("can't open book '%s' (%s): %s"),
				~name, ~path, SysErrMsg(errno));
			return(NULL);	
		}

		// Create a new book object corresponding to
		// this book database.
		//
		if ((newbook = new DBMBOOK(name, path, handle))  ==  NULL) {
			err.Init(DGetText("can't open book '%s' (%s): %s"),
					~name, ~path, SysErrMsg(ENOMEM));
			dbm_close(handle);		// close database
			return(NULL);
		}

		if ( ! newbook->IsValid(err)) {
			err.Push(DGetText("can't open book '%s' (%s)"),
				~name, ~path);
			dbm_close(handle);
			delete(newbook);
			return(NULL);
		}
	}


	// We're ready to roll...
	//
	newbook->objstate.MarkReady();

	return(newbook);
}

STATUS
DBMBOOK::Init(ERRSTK &err)
{
	ATTRLIST	info_attrs;
	datum		key, record;	// dbm key, record for info record.


	assert(dbhandle != NULL);
	DbgFunc("DBMBOOK::Init: " << this << endl);


	// Create special "Book Info" record and add it to
	// this book database.
	//
	info_attrs[DOC_ID]       = BOOK_INFO_KEY;
	info_attrs[INFO_BOOKID]  = Id();
	info_attrs[INFO_VERSION] = BOOK_VERSION;


	// Convert attribute list to DBM record format.
	//
	if (CvtAttrListToDatum(info_attrs, &record, err)  !=  STATUS_OK) {
		return(STATUS_FAILED);
	}

	// Convert record key to DBM record format.
	//
	if (CvtStringToDatum(BOOK_INFO_KEY, &key, err)  !=  STATUS_OK) {
		return(STATUS_FAILED);
	}

	// Insert record in database.
	//
	if (dbm_store(dbhandle, key, record, DBM_INSERT) != 0) {
		err.Init(DGetText("can't initialize book '%s' (%s): %s"),
				~Name(), ~Path(), SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	dbm_flush(dbhandle);

	if ( ! IsValid(err)) {
		err.Push(DGetText("can't initialize book '%s' (%s)"),
				~Name(), ~Path());
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

BOOL
DBMBOOK::IsValid(ERRSTK &err)
{
	ATTRLIST	info_attrs;
	datum		key, record;

	DbgFunc("BOOK::IsValid: " << this <<endl);


	if (CvtStringToDatum(BOOK_INFO_KEY, &key, err)  !=  STATUS_OK) {
		return(BOOL_FALSE);
	}

	record = dbm_fetch(dbhandle, key);
	if (record.dptr == NULL) {
		err.Init(DGetText("can't validate book '%s' (%s): %s"),
				~Name(), ~Path(), SysErrMsg(errno));
		return(BOOL_FALSE);
	}
	
	if (CvtDatumToAttrList(&record, info_attrs, err) != STATUS_OK) {
		return(BOOL_FALSE);
	}

	// Check book version info.
	//
	if (info_attrs[INFO_VERSION]  !=  BOOK_VERSION) {
		err.Init(DGetText("invalid book '%s' (%s): %s"),
			~Name(), ~Path(),
			DGetText("incorrect version number"));
		return(BOOL_FALSE);
	}

	// Verify that book id in info record matches what we
	// we were looking for.
	//
	if (Id() != info_attrs[INFO_BOOKID]) {
		err.Init(DGetText("invalid book '%s' (%s): wrong book id: %s"),
			~Name(), ~Path(), ~(info_attrs[INFO_BOOKID]));
		return(BOOL_FALSE);
	}

	return(BOOL_TRUE);
}

// Retrieve document by document id.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (can't find record, etc.).
//
DOCUMENT *
DBMBOOK::GetDocById(const STRING &docid, ERRSTK &err)
{
	DBMDOC		*doc;		// document we're looking for
	DOCNAME		docname;
	datum		key;
	
	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	DbgFunc("DBMBOOK::GetDocById: " << docid << endl);


	if (docid == NULL_STRING) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
				~docid, ~Name(), SysErrMsg(EINVAL));
		return(NULL);
	}

	docname.Strict(BOOL_FALSE);	// relax strict DOCNAME syntax checking
	if (docname.SetDocId(docid) != STATUS_OK) {
		err.Init(DGetText("invalid document id '%s'"), ~docid);
		return(NULL);
	}
	docname.Resolve(Name());

	if ( ! docname.IsValid()) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
				~docid, ~Name(), SysErrMsg(EINVAL));
		return(NULL);
	}

	if (CvtStringToDatum(docname.ShortName(), &key, err)  !=  STATUS_OK) {
		err.Push(DGetText("can't find document '%s' in book '%s'"),
				~docid, ~Name());
		return(NULL);
	}

	if ((doc = GetDocByKey(&key, err))  ==  NULL) {
		err.Push(DGetText("can't find document '%s' in book '%s'"),
				~docid, ~Name());
		return(NULL);

	} else {
		return(doc);
	}
}

// Retrieve "first" document from the book.
// Documents are retrieved in no particular order.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (no docs in book, etc.).
//
DOCUMENT *
DBMBOOK::GetFirstDoc(ERRSTK &err)
{
	DBMDOC		*doc;		// document we're looking for
	datum		key;		// ndbm key for first document

	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	DbgFunc("DBMBOOK::GetFirstDoc" << endl);

	key = dbm_firstkey(dbhandle);
	if (key.dptr == NULL) {
		if (dbm_error(dbhandle)) {
			err.Init(DGetText(
				"can't read first document in book '%s': %s"),
				~Name(), DGetText("dbm I/O error"));
		} else {
			err.Init(DGetText("no documents in book '%s'"));
		}
		return(NULL);
	}
	
	if ((doc = GetDocByKey(&key, err))  ==  NULL) {
		err.Push(DGetText("can't read first document in book '%s'"),
				~Name());
		return(NULL);

	} else {
		return(doc);
	}
}


// Documents are retrieved in no particular order.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (no docs in book, etc.).
//
DOCUMENT *
DBMBOOK::GetNextDoc(ERRSTK &err)
{
	DBMDOC		*doc;		// document we're looking for
	datum		key;		// ndbm key for next document

	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	DbgFunc("DBMBOOK::GetNextDoc" << endl);

	key = dbm_nextkey(dbhandle);
	if (key.dptr == NULL) {
		if (dbm_error(dbhandle)) {
			err.Init(DGetText(
				"can't read next document in book '%s': %s"),
				~Name(), DGetText("dbm I/O error"));
		} else {
			err.Init(DGetText("no more documents in book '%s'"));
		}
		return(NULL);
	}
	
	if ((doc = GetDocByKey(&key, err))  ==  NULL) {
		err.Push(DGetText("can't read next document in book '%s'"),
				~Name());
		return(NULL);

	} else {
		return(doc);
	}
}

// Retrieve document by DBM key (datum).
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (can't find record, etc.).
//
DBMDOC *
DBMBOOK::GetDocByKey(datum *key, ERRSTK &err)
{
	DBMDOC		*doc;		// document we're looking for
	datum		rec;		// ndbm record for document
	ATTRLIST	attrs;		// document stored as attribute list

	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	assert(key->dptr != NULL  &&  key->dsize > 0);
	DbgFunc("DBMBOOK::GetDocByKey" << endl);


	rec = dbm_fetch(dbhandle, *key);
	if (rec.dptr == NULL) {
		if (dbm_error(dbhandle)) {
			err.Init(DGetText(
				"can't read document: dbm I/O error"));
		} else {
			err.Init(DGetText("no such document in book"));
		}
		return(NULL);
	}
	
	if (CvtDatumToAttrList(&rec, attrs, err) != STATUS_OK) {
		return(NULL);
	}

	if ((doc = new DBMDOC(attrs, this))  ==  NULL) {
		err.Init(DGetText("out of memory"));
		return(NULL);
	} else if ( ! doc->IsValid()) {
		err.Init(DGetText("document '%s' is invalid"), ~doc->Name());
		delete(doc);
		return(NULL);
	} else {
		return(doc);
	}
}

STATUS
DBMBOOK::InsertDoc(const XXDOC *doc, ERRSTK &err)
{
	datum		key, record;	// dbm key, record for this document
	ATTRLIST	attrs;
	DOCUMENT	*verify;
	STRING		shortname = doc->Name().ShortName();

	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	assert(doc != NULL);
	DbgFunc("DBMBOOK::InsertDoc: " << doc << endl);


	// Convert document to attribute list format.
	//
	if (CvtDocToAttrList(doc, attrs, err)  !=  STATUS_OK) {
		err.Push(DGetText("can't add document '%s' to book '%s'"),
			~shortname, ~Name());
		return(STATUS_FAILED);
	}

	// Convert attribute list to DBM record format.
	//
	if (CvtAttrListToDatum(attrs, &record, err)  !=  STATUS_OK) {
		return(STATUS_FAILED);
	}

	if (CvtStringToDatum(shortname, &key, err)  !=  STATUS_OK) {
		return(STATUS_FAILED);
	}

	// Insert record in database.
	//
	if (dbm_store(dbhandle, key, record, DBM_INSERT) != 0) {
		err.Init(DGetText("can't add document '%s' to book '%s': %s"),
			~shortname, ~Name(), SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	// Verify that record was added correctly.
	//
	if ((verify = GetDocById(doc->Name().DocId(), err))  ==  NULL) {
		err.Push(DGetText("can't verify new document '%s'"),~shortname);
		return(STATUS_FAILED);
	}

	if ( ! DOCUMENT::Equal(doc, verify)) {
		err.Init(DGetText("verification failed for document '%s': "),
			~shortname);
		delete(verify);
		return(STATUS_FAILED);

	} else {
		delete(verify);
		return(STATUS_OK);
	}
}

STATUS
DBMBOOK::ReplaceDoc(const XXDOC *doc, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	assert(doc != NULL);
	DbgFunc("DBMBOOK::ReplaceDoc: " << doc << endl);

	(void) DeleteDoc(doc->Name().ShortName(), err);

	return(InsertDoc(doc, err));
}

STATUS
DBMBOOK::DeleteDoc(const STRING &docid, ERRSTK &err)
{
	datum	key;

	assert(objstate.IsReady());
	assert(dbhandle != NULL);
	DbgFunc("DBMBOOK::DeleteDoc: " << docid << endl);


	// Make sure doc id non-null.
	//
	if (docid == NULL_STRING) {
		err.Init(DGetText(
			"can't delete document '%s' from book '%s': %s"),
			~docid, ~Name(), SysErrMsg(EINVAL));
		return(STATUS_FAILED);
	}

	// And make sure document exists.
	//
	if ( ! DocIsPresent(docid)) {
		err.Init(DGetText(
			"can't delete document '%s' from book '%s': %s"),
			~docid, ~Name(), DGetText("no such document"));
		return(STATUS_FAILED);
	}

	if (CvtStringToDatum(docid, &key, err)  !=  STATUS_OK) {
		err.Push(DGetText("can't delete document '%s' from book '%s'"),
			~docid, ~Name());
		return(STATUS_FAILED);
	}

	if (dbm_delete(dbhandle, key)  !=  0) {
		err.Init(DGetText(
			"can't delete document '%s' from book '%s': %s"),
			~docid, ~Name(), SysErrMsg(errno));
		return(STATUS_FAILED);

	} else {
		return(STATUS_OK);
	}
}

BOOL
DBMBOOK::DocIsPresent(const STRING &docid)
{
	DOCUMENT	*doc;
	ERRSTK		notused;

	assert(objstate.IsReady());
	DbgFunc("DBMBOOK::DocIsPresent: " << docid << endl);

	if ((doc = GetDocById(docid, notused))  !=  NULL) {
		delete(doc);
		return(BOOL_TRUE);

	} else {
		return(BOOL_FALSE);
	}
}

STATUS
CvtStringToDatum(const STRING &s, datum *d, ERRSTK &err)
{
	XDR		xdrs;
	STRING		tmp;
	static char	xdrbuf[DBLKSIZ];


	assert(s != NULL_STRING);
	assert(d != NULL);
	DbgFunc("CvtStringToDatum: " << s << endl);


	xdrmem_create(&xdrs, xdrbuf, DBLKSIZ, XDR_ENCODE);

	tmp = s; // needed because xdr_STRING takes a non-const STRING
	if ( ! xdr_STRING(&xdrs, &tmp)) {
		err.Init(DGetText("XDR conversion failed"));
		d->dptr  = NULL;
		d->dsize = 0;
		return(STATUS_FAILED);
	}


	d->dptr  = xdrbuf;
	d->dsize = xdr_getpos(&xdrs);	//XXX - does this really work?

	xdr_destroy(&xdrs);	//XXX necessary?

	DbgFunc("CvtStringToDatum: " << s << "(" << d->dsize << ")\n");
	return(STATUS_OK);
}

STATUS
CvtAttrListToDatum(ATTRLIST &attrs, datum *d, ERRSTK &err)
{
	XDR		xdrs;
	static char	xdrbuf[DBLKSIZ];

	assert(d != NULL);
	DbgFunc("CvtAttrListToDatum" << endl);

	xdrmem_create(&xdrs, xdrbuf, DBLKSIZ, XDR_ENCODE);

	if ( ! xdr_ATTRLIST(&xdrs, &attrs)) {
		err.Init(DGetText("XDR conversion failed"));
		d->dptr  = NULL;
		d->dsize = 0;
		return(STATUS_FAILED);
	}

	d->dptr  = xdrbuf;
	d->dsize = xdr_getpos(&xdrs);	//XXX - does this really work?

	xdr_destroy(&xdrs);	//XXX necessary?

	return(STATUS_OK);
}

STATUS
CvtDatumToAttrList(datum *d, ATTRLIST &attrs, ERRSTK &err)
{
	XDR		xdrs;		// XDR stream used in conversion

	assert(d != NULL);
	assert(d->dptr != NULL);
	assert((d->dsize & 0x3) == 0);	// xdrmem_create expects longword align
	DbgFunc("CvtDatumToAttrList" << endl);

	xdrmem_create(&xdrs, d->dptr, d->dsize, XDR_DECODE);

	if ( ! xdr_ATTRLIST(&xdrs, &attrs)) {
		err.Init(DGetText("XDR conversion failed"));
		return(STATUS_FAILED);
	}

	xdr_destroy(&xdrs);	//XXX necessary?

	return(STATUS_OK);
}
