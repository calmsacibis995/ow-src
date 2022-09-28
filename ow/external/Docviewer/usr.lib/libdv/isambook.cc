#ident "@(#)isambook.cc	1.16 94/08/25 Copyright 1989-1991 Sun Microsystems, Inc."

#include <doc/isambook.h>
#include <doc/isamdoc.h>
#include <doc/xxdoc.h>
#include <isam.h>
#include <fcntl.h>
#include "dvlocale.h"
#include "isamrec.h"

//
// Kludge to force iserrno to not be unrelocatable
// in the case where the wrong libtt.so is used.
//
int iserrno;

static const char	*IsamErrMsg(int err);


ISAMBOOK::ISAMBOOK(const BOOKNAME &name, const STRING &path, int isfd)
			: BOOK(name, path)
{
	assert(isfd >= 0);
	DbgFunc("ISAMBOOK::ISAMBOOK" << endl);

	isam_fd = isfd;
}

ISAMBOOK::~ISAMBOOK()
{
	DbgFunc("ISAMBOOK::~ISAMBOOK" << endl);

	if (objstate.IsReady()) {
		(void) isclose(isam_fd);
	}
}

BOOL
ISAMBOOK::Exists(const STRING &path)
{
	DbgFunc("ISAMBOOK::Exists: " << path << endl);

	if (access(path + ".rec", F_OK) == 0  &&
	    access(path + ".ind", F_OK) == 0) {
		return(BOOL_TRUE);
	} else {
		return(BOOL_FALSE);
	}
}

STATUS
ISAMBOOK::Remove(const STRING &path, ERRSTK &err)
{
	DbgFunc("ISAMBOOK::Remove: " << path << endl);

#if	0
	// XXX iserase() doesn't seem to work - don't know why.
	//
	if (iserase(path) == 0) {
#else	0
	unlink(path + ".lock");	// "path.lock" may not exist - doesn't matter
	if (unlink(path + ".ind")  == 0  &&
	    unlink(path + ".rec")  == 0) {
#endif	0
		return(STATUS_OK);
	} else {
		err.Init(DGetText("can't remove book database %s: %s"),
			~path, IsamErrMsg(iserrno));
		return(STATUS_FAILED);
	}
}

ISAMBOOK *
ISAMBOOK::Open(	const BOOKNAME &name,
		const STRING &path,
		int flags,
		int mode,
		ERRSTK &err)
{
	ISAMBOOK	*newbook;
	int		ismode;
	int		isfd;


	DbgFunc("ISAMBOOK::Open " << name
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


	ismode = (flags & O_RDWR) ? ISINOUT : ISINPUT;
	ismode += ISMANULOCK;
	ismode += ISNFS;	// always access files via NFS rather than
				// through the netisam remote access daemon


	// Create new database.
	//
	if (flags & O_CREAT) {

		if (ISAMBOOK::Exists(path)) {
			err.Init(DGetText(
				"can't create new book '%s' (%s): %s"),
				~name, ~path, DGetText("book already exists"));
			return(NULL);
		}

		// Create isam document database, with its indices.
		//
		isfd = isbuild(	path,
				ISAMREC_RECLEN,
				&ISAMREC_t.tbi_keys[0],
				ISFIXLEN + ismode);

		if (isfd == ISERROR) {
			err.Init(DGetText(
				"can't create new book '%s' (%s): %s"),
				~name, ~path, IsamErrMsg(iserrno));
			return(NULL);
		}

	} else {

		// OPEN an existing database.
		//
		if ((isfd = isopen(path, ismode)) == ISERROR) {
			err.Init(DGetText("can't open book '%s' (%s): %s"),
				~name, ~path, IsamErrMsg(iserrno));
			return(NULL);
		}
	}


	if ((newbook = new ISAMBOOK(name, path, isfd))  ==  NULL) {
		err.Init(DGetText("can't open book '%s' (%s): %s"),
				~name, ~path, SysErrMsg(ENOMEM));
		return(NULL);
	}


	// We're ready to roll...
	//
	newbook->objstate.MarkReady();

	return(newbook);
}

STATUS
ISAMBOOK::Flush(ERRSTK &err)
{
	assert(objstate.IsReady());

	if (isfsync(isam_fd) == 0) {
		return(STATUS_OK);
	} else {
		err.Init(DGetText("can't synch book database '%s' (%s): %s"),
			~Name(), ~Path(), IsamErrMsg(iserrno));
		return(STATUS_FAILED);
	}
}

// Retrieve document by document id.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (can't find record, etc.).
//
DOCUMENT *
ISAMBOOK::GetDocById(const STRING &docid, ERRSTK &err)
{
	ISAMDOC	*doc;
	ISAMREC	isamrec;
	char	buf[ISMAXRECLEN];
	
	assert(objstate.IsReady());
	DbgFunc("ISAMBOOK::GetDocById: " << docid << endl);


	// Make sure doc id non-null and isn't too long
	// to fit into ISAM record's id field.
	//
	if (docid == NULL_STRING  ||  docid.Length() > ISAMREC_ID_LEN) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
				~docid, ~Name(), SysErrMsg(EINVAL));
		return(NULL);
	}

	// Position current record pointer at the record
	// we're looking for (assuming it exists).
	//
	StrCpy(isamrec.id, docid);
	(void) isstrec(&ISAMREC_t, (char *)&isamrec, buf);
 	if (isstart(isam_fd, ISAMREC_k, 0, buf, ISEQUAL)  !=  0) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
			~docid, ~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Read the current record.
	//
	if(isread(isam_fd, buf, ISCURR) < 0) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
			~docid, ~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Convert record from on-disk format into in-memory format.
	//
	(void) isldrec(&ISAMREC_t, (char *)&isamrec, buf);

	// Convert record into document object.
	//
	if ((doc = new ISAMDOC(&isamrec, this))  ==  NULL) {
		err.Init(DGetText("can't find document '%s' in book '%s': %s"),
			~docid, ~Name(), SysErrMsg(ENOMEM));
		return(NULL);
	} else if ( ! doc->IsValid()) {
		delete(doc);
		return(NULL);

	} else {
		return(doc);
	}
}

// Retrieve document by record number.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (can't find record, etc.).
//
DOCUMENT *
ISAMBOOK::GetDocByRecNum(long recnum, ERRSTK &err)
{
	ISAMDOC		*doc;
	ISAMREC		isamrec;
	struct keydesc	tmpkey;
	char		buf[ISMAXRECLEN];
	
	assert(objstate.IsReady());
	assert(isam_fd >= 0);
	DbgFunc("ISAMBOOK::GetDocByRecNum: " << recnum << endl);


	if (recnum < 1)
		return(NULL);


	// Position current record pointer at the physical record 'recnum'.
	// XXX will this work?
	//
	tmpkey.k_flags  = 0;	//XXX is this right?
	tmpkey.k_nparts = 0;
	isrecnum        = recnum;	//XXX correct?
	if (isstart(isam_fd, &tmpkey, 0, NULL, ISEQUAL)  !=  0) {
		err.Init(DGetText("can't find record #%d in book '%s': %s"),
			recnum, ~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Read the current record.
	//
	if(isread(isam_fd, buf, ISCURR) < 0) {
		err.Init(DGetText("can't find record #%d in book '%s': %s"),
			recnum, ~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Convert record from on-disk format into in-memory format.
	//
	(void) isldrec(&ISAMREC_t, (char *)&isamrec, buf);

	// Convert record into document object.
	//
	if ((doc = new ISAMDOC(&isamrec, this))  ==  NULL) {
		err.Init(DGetText("can't find record '%d' in book '%s': %s"),
			recnum, ~Name(), SysErrMsg(ENOMEM));
		return(NULL);
	} else if ( ! doc->IsValid(/*err*/)) {
//XXX		err.Push(0, //XXX
//XXX			DGetText("can't find record #%d in book '%s': %s"),
//XXX			recnum, ~Name(), SysErrMsg(ENOMEM));
		delete(doc);
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
ISAMBOOK::GetFirstDoc(ERRSTK &err)
{
	ISAMDOC		*doc;
	ISAMREC		isamrec;
	char		buf[ISMAXRECLEN];
	struct keydesc	tmpkey;
	
	assert(objstate.IsReady());
	assert(isam_fd >= 0);
	DbgFunc("ISAMBOOK::GetFirstDoc" << endl);



	// Position current record pointer at the physical record 'recnum'.
	// XXX will this work?
	//
	tmpkey.k_flags  = 0;	//XXX is this right?
	tmpkey.k_nparts = 0;
	if (isstart(isam_fd, &tmpkey, 0, NULL, ISFIRST)  !=  0) {
		err.Init(DGetText("can't get first record in book '%s': %s"),
			~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Read the current record.
	//
	if(isread(isam_fd, buf, ISCURR) < 0) {
		err.Init(DGetText("can't read first record in book '%s': %s"),
			~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Convert record from on-disk format into in-memory format.
	//
	(void) isldrec(&ISAMREC_t, (char *)&isamrec, buf);

	// Convert record into document object.
	//
	if ((doc = new ISAMDOC(&isamrec, this))  ==  NULL) {
		err.Init(DGetText("can't read first record in book '%s': %s"),
			~Name(), SysErrMsg(ENOMEM));
		return(NULL);
	} else if ( ! doc->IsValid()) {
		err.Init(DGetText("document '%s' in book '%s' is invalid"),
			~Name(), ~doc->Id());
		delete(doc);
		return(NULL);
	} else {
		return(doc);
	}
}


// Get next document in book.
// Assumes that 'GetFirstDoc()' has already been called,
// and that no intervening calls to 'GetDocById()'
// or 'GetDocByRecNum()' have been made.
//
// Documents are retrieved in no particular order.
//
// Returns new document.  Caller is responsible for deallocation.
// Returns null on error (no docs in book, etc.).
//
DOCUMENT *
ISAMBOOK::GetNextDoc(ERRSTK &err)
{
	ISAMDOC		*doc;
	ISAMREC		isamrec;
	char		buf[ISMAXRECLEN];
	
	assert(objstate.IsReady());
	assert(isam_fd >= 0);
	DbgFunc("ISAMBOOK::GetNextDoc" << endl);


	// Read the next record.
	//
	if(isread(isam_fd, buf, ISNEXT) < 0) {
		err.Init(DGetText("can't read next record in book '%s': %s"),
			~Name(), IsamErrMsg(iserrno));
		return(NULL);
	}

	// Convert record from on-disk format into in-memory format.
	//
	(void) isldrec(&ISAMREC_t, (char *)&isamrec, buf);

	// Convert record into document object.
	//
	if ((doc = new ISAMDOC(&isamrec, this))  ==  NULL) {
		err.Init(DGetText("can't read next record in book '%s': %s"),
			~Name(), SysErrMsg(ENOMEM));
		return(NULL);
	} else if ( ! doc->IsValid()) {
		delete(doc);
		return(NULL);

	} else {
		return(doc);
	}
}

STATUS
ISAMBOOK::InsertDoc(const XXDOC *doc, ERRSTK &err)
{
	ISAMREC		isamrec;
	DOCUMENT	*verify;
	char		buf[ISMAXRECLEN];
	STRING		docid = doc->Id();
	long		recnum;

	assert(objstate.IsReady());
	assert(isam_fd >= 0);
	DbgFunc("ISAMBOOK::InsertDoc" << doc << endl);


	// Get record number, which we assume someone
	// has stored in the private data field.
	//
	recnum = (long ) doc->GetPrivateData();
	assert(IsValidRecNum(recnum));


	// Convert document to ISAM record format.
	//
	if (CvtDocToRec(doc, &isamrec, err)  !=  STATUS_OK) {
		err.Push(DGetText("can't add document '%s' to book '%s'"),
			~docid, ~Name());
		return(STATUS_FAILED);
	}

	// Convert ISAM record to on-disk format,
	// then write it out to the database.
	// Flush the database.
	//
	isstrec(&ISAMREC_t, (char *)&isamrec, buf);
	if (iswrrec(isam_fd, recnum, buf)  !=  0) {
		err.Init(DGetText("can't add document '%s' to book '%s': %s"),
			~docid, ~Name(), IsamErrMsg(iserrno));
		return(STATUS_FAILED);
	}

	if (Flush(err) != STATUS_OK) {
		err.Push(DGetText("can't add document '%s' to book '%s'"),
			~docid, ~Name());
		return(STATUS_FAILED);
	}


	// Verify that record was added correctly.
	//
	if ((verify = GetDocById(docid, err))  ==  NULL) {
		err.Push(DGetText("can't verify new document '%s'"), ~docid);
		return(STATUS_FAILED);
	}

	if ( ! DOCUMENT::Equal(doc, verify)) {
		err.Init(DGetText("verification failed for document '%s': "),
			~docid);
		delete(verify);
		return(STATUS_FAILED);

	} else {
		delete(verify);
		return(STATUS_OK);
	}
}

STATUS
ISAMBOOK::ReplaceDoc(const XXDOC *doc, ERRSTK &/*err*/)
{
	assert(objstate.IsReady());
	DbgFunc("ISAMBOOK::ReplaceDoc: " << doc << endl);

	assert(0);	// not yet implemented
	return(STATUS_FAILED);
}

// Delete document 'docid'.
// Succeeds if record successfully deleted OR if record not present.
//
STATUS
ISAMBOOK::DeleteDoc(const STRING &docid, ERRSTK &err)
{
	ISAMREC		isamrec;
	char		buf[ISMAXRECLEN];
	
	assert(objstate.IsReady());
	assert(isam_fd >= 0);
	DbgFunc("ISAMBOOK::DeleteDoc" << docid << endl);


	// Make sure doc id non-null and isn't too long
	// to fit into ISAM record's id field.
	//
	if (docid == NULL_STRING  ||  docid.Length() > ISAMREC_ID_LEN) {
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

	//
	//
	StrCpy(isamrec.id, docid);
	isstrec(&ISAMREC_t, (char *)&isamrec, buf);
	if (isdelete(isam_fd, buf)  !=  0  &&  iserrno != ENOREC) {
		err.Init(DGetText(
			"can't delete document '%s' from book '%s': %s"),
			~docid, ~Name(), IsamErrMsg(iserrno));
		return(STATUS_FAILED);
	} else {
		return(STATUS_OK);
	}
}

BOOL
ISAMBOOK::DocIsPresent(const STRING &docid)
{
	DOCUMENT	*doc;
	ERRSTK		err;

	assert(objstate.IsReady());
	DbgFunc("ISAMBOOK::DocIsPresent: " << docid << endl);

	if ((doc = GetDocById(docid, err))  !=  NULL) {
		delete(doc);
		return(BOOL_TRUE);
	} else {
		return(BOOL_FALSE);
	}
}

static struct _isam_err {
	int	errnum;
	STRING	errstr;
} isam_errtab[50];

static int	isam_numerrs = 0;

const char *
IsamErrMsg(int err)
{
	static STRING	errmsg;
	static BOOL	inited = BOOL_FALSE;
	int		i;

	if ( ! inited) {

		inited = BOOL_TRUE;
		i = 0;

#define	ErrTab(n, msg)	isam_errtab[i  ].errnum = n; \
			isam_errtab[i++].errstr = DGetText(msg)

		ErrTab(EDUPL,     "duplicate record");
		ErrTab(ENOTOPEN,  "file not open");
		ErrTab(EBADARG,   "illegal argument");
		ErrTab(EBADKEY,   "illegal key desc");
		ErrTab(ETOOMANY,  "too many files open");
		ErrTab(EBADFILE,  "bad ISAM file format");
		ErrTab(ENOTEXCL,  "non-exclusive access");
		ErrTab(ELOCKED,   "record locked");
		ErrTab(EKEXISTS,  "key already exists");
		ErrTab(EPRIMKEY,  "is primary key");
		ErrTab(EENDFILE,  "end/begin of file");
		ErrTab(ENOREC,    "no record found");
		ErrTab(ENOCURR,   "no current record");
		ErrTab(EFLOCKED,  "file locked");
		ErrTab(EFNAME,    "file name too long");
		ErrTab(EBADMEM,   "cannot allocate memory");
		ErrTab(ETIMEOUT,  "RPC timeout");
		ErrTab(ERPC,      "Broken TCP/IP");
		ErrTab(ETCP,      "Cannot connect to server");
		ErrTab(EIMPORT,   "Cannot import");
		ErrTab(ENODAEMON, "no local daemon");
		ErrTab(EFATAL,    "internal fatal error");
		ErrTab(ELANG,     "Locale/LANG mismatch");

		isam_numerrs = i;
	}


	for (i = 0; i < isam_numerrs; i++) {
		if (isam_errtab[i].errnum == err)
			return(errmsg = isam_errtab[i].errstr);
	}

	return(errmsg = strerror(err));
}
