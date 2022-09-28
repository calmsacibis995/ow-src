#ident "@(#)bookshelf.cc	1.21 93/12/20 Copyright 1992 Sun Microsystems, Inc."

#include <doc/bookshelf.h>
#include <doc/bookmark.h>
#include <doc/pathname.h>
#include <doc/utils.h>
#include <fcntl.h>
#include "dvlocale.h"
#include <time.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>


// Bookshelf file header, version number.
//
static const STRING	BS_HEADER_MAGIC	("#<AnswerBook Library>");
static const STRING	BS_HEADER_FMT	("#<AnswerBook Library> version %d\n");
static const STRING	BS_OLD_HEADER_MAGIC	("#<library>");
static const int	BS_HEADER_VERSION = 1;


// Bookshelf attribute names.
//
static const STRING	BS_BOOKSHELF		("bookshelf");
static const STRING	BS_BOOKSHELF_NAME	("bookshelf.name");

static const STRING	BS_BOOKMARK		("bookmark");
static const STRING	BS_BOOKMARK_NAME	("bookmark.name");
static const STRING	BS_BOOKMARK_TITLE	("bookmark.title");
static const STRING	BS_BOOKMARK_BSTITLE	("bookmark.bstitle");
static const STRING	BS_BOOKMARK_COMMENT	("bookmark.comment");

static const STRING	BS_SEARCH		("search");
static const STRING	BS_SEARCH_SCOPED	("search.scoped");
static const STRING	BS_SEARCH_TITLESONLY	("search.titlesonly");

static const STRING	BS_NAVIGATOR		("navigator");
static const STRING	BS_NAVIGATOR_MODE	("navigator.mode");


// BOOKSHELF constructor.
//
BOOKSHELF::BOOKSHELF(	const STRING	&path_arg,
			FILE		*fp,
			BOOL		rdonly_flag,
			BOOL		lock_flag) :
	path		(path_arg),
	bsfp		(fp),
	read_only	(rdonly_flag),
	locked		(lock_flag),
	dirty		(BOOL_FALSE)
{
	assert(path != NULL_STRING);
	assert(bsfp != NULL);
	DbgFunc("BOOKSHELF::BOOKSHELF: " << path << endl);


	// Ready to roll...
	//
	objstate.MarkReady();
}

// BOOKSHELF destructor.
//
BOOKSHELF::~BOOKSHELF()
{
	ERRSTK	notused;

	DbgFunc("BOOKSHELF::~BOOKSHELF" << endl);

	if (bsfp)
		fclose(bsfp);
	if (locked)
		ClearLockFile(path, notused);
}

// Open bookshelf file.
// Returns pointer to new BOOKSHELF, or NULL on error.
// Caller is responsible for deallocation.
//
BOOKSHELF *
BOOKSHELF::OpenFile(const STRING &path, int &flags, ERRSTK &err, NOTIFY &notify)
{
	BOOKSHELF	*newbs;
	FILE		*bsfp;
	BOOL		read_only = BOOL_TRUE;
	BOOL		created_lock_file = BOOL_FALSE;
	BOOL		created_bs_file = BOOL_FALSE;
	BOOL		flag_change = BOOL_FALSE;


	assert(path != NULL_STRING);
	assert(flags & (BS_RDONLY|BS_RDWR));
	DbgFunc("BOOKSHELF::Open: " << path << " (" << flags << ")" << endl);



	if (flags & BS_RESET_LOCK) {
		if (ClearLockFile(path, err)  !=  STATUS_OK)
			return(NULL);
	}


	if (flags & (BS_SET_LOCK|BS_RESET_LOCK)) {
		if (CreateLockFile(path, flag_change, notify)  !=  STATUS_OK)
			return(NULL);
		else
			if (flag_change)
				flags = BS_RDONLY;
			else
				created_lock_file = BOOL_TRUE;
	}


	if (flags & BS_RDONLY) {

		// Open file for read-only operations.
		//
		assert((flags & ~BS_RDONLY) == 0);

		if ((bsfp = fopen(path, "r"))  ==  NULL) {
			err.Init(DGetText(
				"Can't open AnswerBook Library file '%s': %s"),
				~path, SysErrMsg(errno));
			return(NULL);
		}

		// Give 'em a new bookshelf object.
		//
		return(new BOOKSHELF(path, bsfp, read_only,created_lock_file));
	}


	read_only = BOOL_FALSE;

	

	if ((flags & BS_CREAT)  &&  access(path, F_OK) != 0) {

		// Create bookmark file if it doesn't already exist.
		//
		assert(flags & BS_RDWR);

		if ((bsfp = fopen(path, "a+"))  ==  NULL) {
			if (created_lock_file)
				ClearLockFile(path, err);
			err.Init(DGetText(
			  "Can't create new AnswerBook Library file '%s': %s"),
			  ~path, SysErrMsg(errno));
			return(NULL);
		}

		newbs = new BOOKSHELF(path, bsfp, read_only,created_lock_file);
		if (newbs->WriteHeader(err) != STATUS_OK) {
			delete newbs;
			err.Init(DGetText(
			  "Can't create new AnswerBook Library file '%s': %s"),
			  ~path, SysErrMsg(errno));
			return(NULL);
		}

	} else {

		// Verify that Library file exists.
		//
		if (access(path, F_OK) != 0) {
			if (created_lock_file)
				ClearLockFile(path, err);
			err.Init(DGetText(
			    "Can't access AnswerBook Library file '%s': %s"),
			    ~path, SysErrMsg(errno));
			return(NULL);
		}

		// Verify that this is indeed a Library file.
		//
		if ( ! IsBookshelfFile(path)) {
			if (created_lock_file)
				ClearLockFile(path, err);
			err.Init(DGetText(
				"Not a valid AnswerBook Library file: %s"),
				~path);
			return(NULL);
		}

		if ((bsfp = fopen(path, "a+"))  ==  NULL) {
			if (created_lock_file)
				ClearLockFile(path, err);
			err.Init(DGetText(
				"Can't open AnswerBook Library file '%s': %s"),
				~path, SysErrMsg(errno));
			return(NULL);
		}

		newbs = new BOOKSHELF(path, bsfp, read_only,created_lock_file);
	}


	// Give 'em a new bookshelf object.
	//
	return(newbs);
}

STATUS
BOOKSHELF::ReadFile(ERRSTK & /*err*/)
{
	int		colon, dot;
	STRING		line;
	STRING		name;
	STRING		value;
	STRING		category;


	assert(objstate.IsReady());
	assert(bsfp != NULL);
	DbgFunc("BOOKSHELF::ReadFile: " << path << endl);


	// Rewind stream.  Reset "AnswerBooks" list.
	//
	rewind(bsfp);
	answerbooks.Clear();


	while (GetLine(bsfp, line)  !=  NULL_STRING) {

		if ((colon = line.Index(':'))  <  1)
			continue;

		name  = STRING::CleanUp(line.SubString(0, colon-1));

		value = line.SubString(colon+1,END_OF_STRING);

		dot   = name.Index('.');
		if (dot < 1)
			category = name;
		else
			category = name.SubString(0, dot-1);

		if (category == BS_BOOKMARK) {
			BookmarkAttr(name, value);

		} else {
			value = STRING::CleanUp(value);
			if (category == BS_BOOKSHELF) {
				AnswerBookAttr(name, value);

			} else if (category == BS_SEARCH) {
				SearchAttr(name, value);

			} else if (category == BS_NAVIGATOR) {
				NavigatorAttr(name, value);

			} else {
				cerr << "bookshelf: unknown category: " 
				     << category
				     << endl;
			}
		}
	}

	return(STATUS_OK);
}

STATUS
BOOKSHELF::SaveToFile(ERRSTK &err)
{
	int	i;


	assert(objstate.IsReady());
	assert(bsfp != NULL);
	assert( ! IsReadOnly());
	DbgFunc("BOOKSHELF::SaveToFile: " << path << endl);


	// Don't perform unnecessary Save operations.
	//
	if ( ! IsDirty())
		return(STATUS_OK);


	if (WriteHeader(err) != STATUS_OK)
		return(STATUS_FAILED);


	// Write AnswerBook names to bookshelf file.
	//
	for (i = 0; i < answerbooks.Count(); i++) {
	      fprintf(bsfp, "%s:\t%s\n", ~BS_BOOKSHELF_NAME, ~answerbooks[i]);
	}


	// Write bookmarks to bookshelf file.
	//
	for (i = 0; i < bookmarks.Count(); i++) {
		fprintf(bsfp, "%s:\t%s\n", ~BS_BOOKMARK_NAME,
			~bookmarks[i]->DocName());
		STRING annot = bookmarks[i]->Annotation();
		if (annot.Index('\n') < 0){
			fprintf(bsfp, "%s:%s\n", ~BS_BOOKMARK_COMMENT,
				~bookmarks[i]->Annotation());
		}
		else{
			int size = annot.Length();
			const char* annot_ptr = ~annot;
			int j, k;
			char* tmpstr = new char [size+1];
			
			j = k = 0;
			
			while (k < annot.Length())
			{
				if ( annot_ptr[k] != '\n'){
					tmpstr[j++] = annot_ptr[k++];
				}
				else {
					tmpstr[j] = '\0';
					fprintf(bsfp, "%s:%s\n", 
						~BS_BOOKMARK_COMMENT, 
						tmpstr);
					j = 0;
					k++;
				}
			}
			if (j != 0){
				tmpstr[j] = '\0';
				fprintf(bsfp, "%s:%s\n", 
					~BS_BOOKMARK_COMMENT, 
					tmpstr);
			}
			delete tmpstr;
		}
		
		fprintf(bsfp, "%s:\t%s\n", ~BS_BOOKMARK_TITLE,
			~bookmarks[i]->Title());
		fprintf(bsfp, "%s:\t%s\n", ~BS_BOOKMARK_BSTITLE,
			~bookmarks[i]->AnswerBookTitle());
	}

	fflush(bsfp);


	// Verify that the Save completed successfully.
	//
	if (ferror(bsfp)) {
		err.Init(DGetText(
			"Can't save AnswerBook Library to file '%s': %s"),
			~path, SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	// Mark bookshelf as no longer dirty.
	//
	dirty = BOOL_FALSE;

	return(STATUS_OK);
}

STATUS
BOOKSHELF::WriteHeader(ERRSTK &err)
{
	assert(bsfp != NULL);
	assert( ! IsReadOnly());
	DbgFunc("BOOKSHELF::WriteHeader" << endl);


	// Truncate the file and rewind the stream.
	//
	ftruncate(fileno(bsfp), 0);
	rewind(bsfp);


	// Write bookshelf header.
	//
	fprintf(bsfp, BS_HEADER_FMT, BS_HEADER_VERSION);
	fprintf(bsfp, "#\n");
	fprintf(bsfp, "# This file was generated by the AnswerBook Navigator.\n");
	fprintf(bsfp, "# DO NOT EDIT THIS FILE BY HAND.\n");
	fprintf(bsfp, "#\n");

	fflush(bsfp);


	// Check for problems.
	//
	if (ferror(bsfp)) {
	    err.Init(DGetText("Can't update AnswerBook Library file '%s': %s"),
		~path, SysErrMsg(errno));
	    return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

void
BOOKSHELF::MarkDirty()
{
	ERRSTK	err;

	assert(objstate.IsReady());
	DbgFunc("BOOKSHELF::MarkDirty" << endl);


	dirty = BOOL_TRUE;

	// XXX it should not be our responsibility to do saves.
	// XXX Someone else should do it for us, perhaps off an N-minute timer.
	//
	if ( ! IsReadOnly()) {
		if (SaveToFile(err) != STATUS_OK) {
			cout << err;
		}
	}
}

void
BOOKSHELF::AnswerBookAttr(const STRING &name, const STRING &value)
{
	ABNAME		abname;


	assert(objstate.IsReady());
	DbgFunc("BOOKSHELF::AnswerBookAttr: " << name << "=" << value << endl);


	if (name == BS_BOOKSHELF_NAME) {
		if (abname.Init(value) != STATUS_OK) {
			cerr << "invalid AnswerBook name: " << value <<endl;
		} else {
			answerbooks.Add(abname);
		}
	} else {
		cerr << "bookshelf: unknown attribute: " << name << endl;
	}
}

void
BOOKSHELF::BookmarkAttr(const STRING &name, const STRING &value)
{
	DOCNAME		docname;
	int		last;		// last bookmark in bookmark list
	STRING		stripped_value;


	assert(objstate.IsReady());
	DbgFunc("BOOKSHELF::BookmarkAttr: " << name << "=" << value << endl);


	last = bookmarks.Count() - 1;


	if (name == BS_BOOKMARK_COMMENT) {

		if (last < 0)
			return;
		STRING annot = bookmarks[last]->Annotation();
		if (annot == NULL_STRING)
			bookmarks[last]->SetAnnotation(value);
		else
			bookmarks[last]->SetAnnotation(annot+"\n"+value);
		

	} else {
		stripped_value = STRING::CleanUp(value);
		if (name == BS_BOOKMARK_NAME) {

			if (docname.Init(stripped_value) != STATUS_OK) {
				cerr << "invalid bookmark name: " 
					<< stripped_value <<endl;
			} else {
				bookmarks.Add(new BOOKMARK(docname));
				++last;
			}

		} else if (name == BS_BOOKMARK_TITLE) {

			if (last < 0)
				return;
			bookmarks[last]->SetTitle(stripped_value);

		} else if (name == BS_BOOKMARK_BSTITLE) {
	

			if (last < 0)
				return;
			bookmarks[last]->SetAnswerBookTitle(stripped_value);

		} else {
			cerr << "bookshelf: unknown attribute: " 
				<< name << endl;
		}
	}
	DbgMed("BOOKSHELF::BookmarkAttr: " << bookmarks[last] << endl);
}

void
BOOKSHELF::SearchAttr(const STRING &name, const STRING &value)
{
	assert(objstate.IsReady());
	DbgFunc("BOOKSHELF::SearchAttr: " << name << "=" << value << endl);

	cerr << "bookshelf: unknown attribute: " << name << endl;
}

void
BOOKSHELF::NavigatorAttr(const STRING &name, const STRING &value)
{
	assert(objstate.IsReady());
	DbgFunc("BOOKSHELF::NavigatorAttr: " << name << "=" << value << endl);

	cerr << "bookshelf: unknown attribute: " << name << endl;
}

// Determine if specified file is a bookshelf file.
//
BOOL
BOOKSHELF::IsBookshelfFile(const STRING &path)
{
	FILE		*bsfp;		// bookshelf file pointer
	struct stat	statbuf;	// buffer for getting file status
	char		buf[50];	// buffer for reading file header
	int		len    = BS_HEADER_MAGIC.Length();
	int		oldlen = BS_OLD_HEADER_MAGIC.Length();
	BOOL		is_bsfile = BOOL_FALSE;


	assert(path != NULL_STRING);
	assert(len < sizeof(buf));
	DbgFunc("BOOKSHELF::IsBookshelfFile: " << path << endl);


	if ((bsfp = fopen(path, "r"))  !=  NULL		&&
	    fstat(fileno(bsfp), &statbuf) == 0		&&
	    S_ISREG(statbuf.st_mode)			&&
	    fgets(buf, 50, bsfp) !=  NULL)		{

		if (strncmp(buf, BS_HEADER_MAGIC, len)         == 0  ||
		    strncmp(buf, BS_OLD_HEADER_MAGIC, oldlen) == 0) {
			is_bsfile = BOOL_TRUE;
		}
	}


	fclose(bsfp);
	return(is_bsfile);
}

// Get path name of default bookshelf file for the current user.
//
void
BOOKSHELF::GetDefaultPath(STRING &bspath)
{
	PATHNAME::Expand("~/.ab_library", bspath);
}

void
BOOKSHELF::MakeLockPath(const STRING &path, STRING &lock_path)
{
	assert(path != NULL_STRING);

	lock_path = path + ".lock";
}

STATUS
BOOKSHELF::CreateLockFile(const STRING &path, BOOL &flag_change, NOTIFY &notify)
{
	STRING		lock_path;
	int		lockfd;
	FILE		*lockfp;
	time_t		timebuf;
	struct passwd	*pwbuf;
	STRING		username;
	STRING		alert_msg;
	STRING		quit_msg;
	STRING		rdonly_msg;
	BOOL		reply;


	DbgFunc("BOOKSHELF::CreateLockFile: " << path << endl);


	MakeLockPath(path, lock_path);

	// Create new lock file.
	// By setting O_CREAT and O_EXCL, we're guaranteed that
	// this operation will only succeed if the lock file
	// does not already exist.  Further, this operation is
	// atomic - if several processes are trying to do this
	// simultaneously, only one can succeed (see open(2) man page).
	// XXX - mode should be current "umask(2)" value, not 0644.
	//
	if ((lockfd = open(lock_path, O_RDWR|O_CREAT|O_EXCL, 0644))  <  0) {
		alert_msg = DGetText("Can't set lock on AnswerBook Library file ");
		alert_msg += ~path;
	 	alert_msg += ": ";
		alert_msg += SysErrMsg(errno);
		rdonly_msg = gettext("Open Read Only");
		quit_msg = gettext("Quit");
		reply = notify.AlertPrompt(rdonly_msg, quit_msg, alert_msg);
		if (reply){
			flag_change = BOOL_TRUE;
			return(STATUS_OK);
		}
		else
			return(STATUS_FAILED);
	}

	lockfp = fdopen(lockfd, "a+");


	// Get helpful info to put in lock file.
	//
	time(&timebuf);
	if ((pwbuf = getpwuid(geteuid()))  ==  NULL)
		username = "(no name)";
	else
		username = pwbuf->pw_name;


	// Write helpful info to lock file.
	//
	fprintf(lockfp, DGetText("#<AnswerBook Library lock file>\n"));
	fprintf(lockfp, DGetText("# Library name:  %s\n"), ~path);
	fprintf(lockfp, DGetText("# Created by:    %s\n"), ~username);
	fprintf(lockfp, DGetText("# Creation time: %s\n"), ctime(&timebuf));


	fclose(lockfp);
	return(STATUS_OK);
}

STATUS
BOOKSHELF::ClearLockFile(const STRING &path, ERRSTK &err)
{
	STRING	lock_path;


	assert(path != NULL_STRING);
	DbgFunc("BOOKSHELF::ClearLockFile: " << path << endl);


	MakeLockPath(path, lock_path);

	// Clear existing lock file, if any.
	//
	if (access(lock_path, F_OK) == 0  &&  
	    unlink(lock_path)       != 0) {
		err.Init(DGetText(
		    "Can't clear lock on AnswerBook Library file '%s': %s"),
		    ~path, SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

BOOL
BOOKSHELF::FileExists(const STRING &path)
{
	assert(path != NULL_STRING);
	DbgFunc("BOOKSHELF::FileExists: " << path << endl);

	if (access(path, F_OK) == 0)
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

BOOL
BOOKSHELF::FileIsLocked(const STRING &path)
{
	STRING	lock_path;


	assert(path != NULL_STRING);
	DbgFunc("BOOKSHELF::FileIsLocked: " << path << endl);


	// See if lock file exists.
	//
	MakeLockPath(path, lock_path);

	if (access(lock_path, F_OK) == 0)
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}
