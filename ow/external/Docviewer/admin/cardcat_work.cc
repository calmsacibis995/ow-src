#ident "@(#)cardcat_work.cc	1.14 06/22/93 Copyright 1992 Sun Microsystems, Inc."


#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <doc/token_list.h>
#include "cardcat_work.h"

// Valid attributes names in ABINFO records.
//
static const STRING	ATTR_ID("id");
static const STRING	ATTR_TITLE("title");
static const STRING	ATTR_VERSION("version");
static const STRING	ATTR_TOCPATH("tocpath");
static const STRING	ATTR_PSPATH("pspath");
static const STRING	ATTR_INDEXPATH("indexpath");
static char this_line_buf [500];
static const STRING     BLANK_STRING(" ");
static const STRING     SEARCH_STRING("fooBarZotTimbaktoo");
const  MAXCOLONS = 4;

// written because the real fscanf stops scanning on white-space!
int
my_fscanf (FILE *fp, STRING &line)
{
	char	*tmp;
	BOOL done = BOOL_FALSE;
	int result = 0;
	line = NULL_STRING;
	
	while (done == BOOL_FALSE)     {
		
		tmp = fgets (this_line_buf, 500, fp);
		if (tmp != NULL)	{

			int len = strlen (this_line_buf);
			this_line_buf [len - 1] = '\0';
			line += this_line_buf;
			if (len < 499)
				done = BOOL_TRUE;
		}
		else	{
			result = EOF;
			done = BOOL_TRUE;
		}
		

	}
	return (result);
	
}

// returns BOOL_TRUE if a file exists
BOOL
FileExists (STRING &name)
{

	int fd;
	BOOL result = BOOL_TRUE;
	

	fd = open (name, O_RDONLY);
	
	if (fd == -1) 
		result = BOOL_FALSE;
	else
		close (fd);
	
	return (result);
}

		
CARDCAT_WORK::CARDCAT_WORK()
{
	DbgFunc("CARDCAT_WORK::CARDCAT_WORK" << endl);

	objstate.MarkReady();
}

CARDCAT_WORK *
CARDCAT_WORK::Init ()
{
	DbgFunc ("CARDCAT_WORK::Init" << endl);
	
	return (new CARDCAT_WORK());
}

void 
CARDCAT_WORK::SetCardCatName (const STRING &in_cc_name)
{
	cc_name = in_cc_name;
}

void 
CARDCAT_WORK::SetABId (const STRING &in_AB_id)
{
	AB_id = in_AB_id;
}

void 
CARDCAT_WORK::SetABVersion (const STRING &in_AB_version)
{
	AB_version = in_AB_version;
}

void 
CARDCAT_WORK::SetABTOCPath (const STRING &in_AB_tocpath)
{
	AB_tocpath = in_AB_tocpath;
}

void 
CARDCAT_WORK::SetABIndexPath (const STRING &in_AB_indexpath)
{
	AB_indexpath = in_AB_indexpath;
}

void 
CARDCAT_WORK::SetABPSPath (const STRING &in_AB_pspath)
{
	AB_pspath = in_AB_pspath;
}

void 
CARDCAT_WORK::SetABTitle (const STRING &in_AB_title)
{
	AB_title = in_AB_title;
}

STATUS
CARDCAT_WORK::ListPaths (ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCATS cardcats;
	LIST<STRING> paths;
	
	cardcats.AppendDefaults (err);
	cardcats.GetPaths (paths);
	
	for (int i = 0; i < paths.Count(); i++)
		cout << paths [i] << endl;
	
	return(STATUS_OK);
}
	
	
BOOL
CARDCAT_WORK::TestMatch (LISTX<ABINFO*> &info_list, int& index)
{
	BOOL is_match = BOOL_FALSE;
	ABNAME name;
	
	for (index = 0; index < info_list.Count(); index++)	{
		name = info_list[index]->Name();
		is_match = BOOL_FALSE;

		
		if (AB_id == name.ABId())	{
				
			if (AB_version != NULL_STRING)	{
					
				if (AB_version == name.ABVersion())
					is_match = BOOL_TRUE;
			}
			else
		        {
			  if (name.ABVersion() == NULL_STRING)
		          {
			    is_match = BOOL_TRUE;
			  }
			}

			if (is_match == BOOL_TRUE)
				break;
			
			
		}
	}
	return (is_match);
}

STATUS
CARDCAT_WORK::MakeABINFO (ABINFO &info, ERRSTK &err)
{
	STATUS status;
	STRING infostr;

	// Make an info string
	//

	infostr = ":" + ATTR_ID        + "=" + AB_id + ": \\\n";
	infostr += ":" + ATTR_VERSION   + "=" + AB_version +": \\\n";
	infostr += ":" + ATTR_TITLE     + "=" + AB_title + ": \\\n";
	infostr += ":" + ATTR_TOCPATH   + "=" + AB_tocpath + ": \\\n";
	infostr += ":" + ATTR_PSPATH    + "=" + AB_pspath + ": \\\n";
	infostr += ":" + ATTR_INDEXPATH + "=" + AB_indexpath  + ":\n";

	status = ABINFO::ParseInfoString (infostr, info, err);

	return (status);
}

// Determine if specified file is a card catalog file
//
BOOL
CARDCAT_WORK::IsACardCat (FILE *fp)
{
	int		ccfd = -1;	// cardcat file descriptor
	struct stat	statbuf;	// buffer for getting file status
	char		buf[20];	// buffer for reading file header
	STRING		cc_header_magic = CARDCAT::GetHeaderMagic();
	int		len = cc_header_magic.Length();
	BOOL		retval;
	

	assert(len < sizeof(buf));

	ccfd = fileno (fp);
	

	if (fstat(ccfd, &statbuf) == 0			&&
	    S_ISREG(statbuf.st_mode)			&&
	    read(ccfd, buf, len)  ==  len		&&
	    strncmp(buf, cc_header_magic, len) == 0)	{

		DbgFunc("IsACardCat: " << "? YES" << endl);
		retval = BOOL_TRUE;

	} else {
		DbgFunc("IsACardCat: " << "? no" << endl);
		retval = BOOL_FALSE;
		
	}

	rewind (fp);
	
	return (retval);
	

}

STATUS
CARDCAT_WORK::AddInfoListToCardCat (LISTX<ABINFO*> &info_list,
				    ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCAT *cardcat;
	LISTX<ABINFO*>	exist_info_list;
	ABINFO *info;
	BOOL is_match;
	int i;
	STRING infostr;
	ABNAME name;
	
	DbgFunc ("AddInfoListToCardCat: " << cc_name << endl);

	cardcat = CARDCAT::Open (cc_name, "r", err);

	if (cardcat == NULL
	    && (FileExists (cc_name) == BOOL_TRUE))	{
		err.Push(gettext("Unable to open card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		if (cardcat == NULL)	{
			// loop thru eberything in info_list, adding an item at a time
			// to the card catalog
			for (int index = 0; index < info_list.Count(); index++)	{
				info = new ABINFO (*info_list[index]);

				// Add the new info
				if (status == STATUS_OK)	{
					exist_info_list.Add (info);
				}
			}
		}
		else 	{
			
			// get the info already in the card catalog
			cardcat->GetAll (exist_info_list, err);		

			// loop thru eberything in info_list, adding an item at a time
			// to the card catalog
			for (int index = 0; index < info_list.Count(); index++)	{
				name = info_list[index]->Name();
				AB_id = name.ABId();
				AB_version = name.ABVersion();
				AB_title = info_list[index]->Title();
				AB_tocpath = info_list[index]->TOCPathForAB();
				AB_indexpath = info_list[index]->IndexPathForAB();
				AB_pspath = info_list[index]->PSPathForAB ();
			
				// is this AB already in the cardcat?
				is_match = TestMatch (exist_info_list, i );
		
				if (is_match == BOOL_TRUE)	{
					fprintf(stderr, gettext(
		"Can't add AnswerBook '%s': already in card catalog\n"),
						~AB_id);
				}
				else	{
		
					info = new ABINFO (*info_list[index]);

					// Add the new info
					if (status == STATUS_OK)	{
						exist_info_list.Add (info);

					}
				
				}

			}
			// rewrite the cardcat
			delete (cardcat);
		}
		
		cardcat = CARDCAT::Open (cc_name, "w", err);

		if (cardcat == NULL)	{
			err.Push(gettext("Unable to open card catalog"));
			status = STATUS_FAILED;
		}
		else	{
			status = cardcat->Rewrite (exist_info_list, 
						   err);
		}
		

	}
	
	return (status);
}
	
STATUS
CARDCAT_WORK::GetInfoListFromCardCat (const STRING &input_file_name, 
				      LISTX<ABINFO*> &info_list,
				      ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCAT *in_cardcat;

	
	DbgFunc ("GetInfoListFromCardCat: " << input_file_name << endl);

	in_cardcat = CARDCAT::Open (input_file_name, "r", err);
	

	if (in_cardcat == NULL)	{
		err.Push (gettext("Unable to open input card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		in_cardcat->GetAll (info_list, err);		

	}
	
	
	return (status);
}	


STATUS
CARDCAT_WORK::GetInfoListFromBookinfo (const STRING &input_file_name,
				       LISTX<ABINFO*> &info_list,
				       ERRSTK &err)
{
	STATUS status = STATUS_OK;
	LISTX<STRING*> lines;
	FILE *fp;
	int sts;
	STRING *line_ptr;
	STRING line;
	BOOL is_a_bookinfo;
	BOOL IsABentry;
        ABINFO *info;
	

	// read the bookinfo file in & ensure that it has only
	// comments (lines starting with a #) or info (lines
	// with colon separated fields
	if ((fp = fopen(input_file_name, "r"))  ==  NULL) {
		err.Init(gettext("Can't open input file \"%s\": %s"),
			~input_file_name, SysErrMsg(errno));
		status = STATUS_FAILED;
	}	
	else	{
		is_a_bookinfo = BOOL_TRUE;
		
		line_ptr = new STRING();
	       
		sts = my_fscanf (fp, *line_ptr);
		
		while (sts != EOF &&
		       is_a_bookinfo == BOOL_TRUE)	{
			
			if (!(line_ptr->Index ('#') == 0 ||
			    line_ptr->Index (':') != -1))
				is_a_bookinfo = BOOL_FALSE;
			else	{
				lines.Add (line_ptr);
				
			}
			if (is_a_bookinfo == BOOL_TRUE) {
				line_ptr = new STRING();
				sts = my_fscanf (fp, *line_ptr);
			}
			
		}
	}
	
	if (is_a_bookinfo == BOOL_TRUE &&
	    status == STATUS_OK)	{
		// the line with all fields filled in is the bookinfo
		// field for the entire AB
		
		for (int i = 0; i < lines.Count(); i++)	{
			line = (*deleteColonsInTitle(*(lines [i])));
			
			TOKEN_LIST toks (line, ':', BOOL_TRUE);
			
			IsABentry = BOOL_TRUE;
			
			for (int j = 0; j < toks.Count (); j++)	{
				if (toks [j] == NULL_STRING)
					IsABentry = BOOL_FALSE;
			}
			
			if (IsABentry == BOOL_TRUE)
				if (toks.Count () != 5)
					IsABentry = BOOL_FALSE;
			
			if (IsABentry == BOOL_TRUE)	{
				// Extract the fields from the ab entry.
				// Syntax is:
				//
				// <id>:<tocpath>:<pspath>:<indexpath>:<title>
				//	
				AB_id = toks [0];
				AB_tocpath = toks [1];
				AB_pspath = toks [2];
				AB_indexpath = toks [3];
				AB_title = toks [4];
				break;
			}
			
		}


		if (IsABentry == BOOL_TRUE)	{

			// now we have everything - make an ABINFO and add it
			info = new ABINFO();
			
			status = MakeABINFO (*info, err);
			
			if (status == STATUS_OK)
				info_list.Add (info);
		}
	}

	return (status);
}

STATUS
CARDCAT_WORK::DoAddFromFile (const STRING &input_file_name, 
	       ERRSTK &err)
{
	STATUS status = STATUS_OK;
	FILE *fp;
	LISTX<ABINFO*> info_list;
	
	DbgFunc ("DoAddFromFile: " << cc_name << " " << 
		 input_file_name << endl);

	// Open file & determine its type
	if ((fp = fopen(input_file_name, "r"))  ==  NULL) {
		err.Init(gettext("Can't open input file \"%s\": %s"),
			~input_file_name, SysErrMsg(errno));
		status = STATUS_FAILED;
	}	
	else	{
		// figure out the file type
		if (IsACardCat (fp) == BOOL_TRUE)	{
			fclose (fp);
			status = GetInfoListFromCardCat (input_file_name, 
							 info_list, err);
		}
		else {
			fclose (fp);
			status = GetInfoListFromBookinfo (input_file_name, 
							  info_list, err);
		}
		if (status == STATUS_OK)	{
			status = AddInfoListToCardCat (info_list, err);
		}
		
	}

	
	return (status);
}


STATUS
CARDCAT_WORK::DoAddFromArgs (ERRSTK &err)
{
	STATUS status = STATUS_OK;
	LISTX<ABINFO*>	info_list;
	ABINFO *info;
	STRING infostr;
	
	DbgFunc ("DoAddFromArgs: " << cc_name << " " << AB_id << " " <<
		 AB_version << " " << AB_title << " " << AB_tocpath << " "
		 << AB_indexpath << " " << AB_pspath << endl);

	// now we have everything - make an ABINFO and add it
	info = new ABINFO();
			
	status = MakeABINFO (*info, err);
			
	if (status == STATUS_OK)
		info_list.Add (info);

	status = AddInfoListToCardCat (info_list, err);
	
	return (status);
}

STATUS
CARDCAT_WORK::DoRemove (ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCAT *cardcat;
	LISTX<ABINFO*>	info_list;
	ABNAME name;
	BOOL is_match;
	int i = 0;
	
	DbgFunc ("DoRemove: " << cc_name << " " << AB_id << " " <<
		 AB_version << endl);
	
	cardcat = CARDCAT::Open (cc_name, "r+", err);

	if (cardcat == NULL)	{
		err.Push(gettext("Unable to open card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		cardcat->GetAll (info_list, err);

		is_match = TestMatch (info_list, i);
		
		if (is_match == BOOL_TRUE)	{

			// delete the item
			info_list.Delete (i);

			// rewrite the cardcat
			status = cardcat->Rewrite (info_list, err);				
		}
			
	}
		
			
	if (is_match == BOOL_FALSE)	{
		if (AB_version == NULL_STRING)
			err.Init (gettext ("AnswerBook \"%s\" not found."),
				  ~AB_id);
		else
			err.Init (gettext ("AnswerBook \"%s,%s\" not found."),
				  ~AB_id, ~AB_version);
		status = STATUS_FAILED;
	}
	

	return (status);
}

STATUS
CARDCAT_WORK::DoModify (ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCAT *cardcat;
	LISTX<ABINFO*>	info_list;
	ABNAME name;
	BOOL is_match;
	int i;
	
	DbgFunc ("DoModify: " << cc_name << " " << AB_id << " " <<
		 AB_version << " " << AB_title << " " << AB_tocpath << " "
		 << AB_indexpath << " " << AB_pspath << endl);

	cardcat = CARDCAT::Open (cc_name, "r+", err);

	if (cardcat == NULL)	{
		err.Push(gettext("Unable to open card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		cardcat->GetAll (info_list, err);
		
		is_match = TestMatch (info_list,i);
		
			
		if (is_match == BOOL_TRUE)	{
	
			name = info_list[i]->Name();
			status = name.SetABId (AB_id);
			if (AB_version != NULL_STRING)	{
					
				status = name.SetABVersion (AB_version);

			}
				
			if (AB_title != NULL_STRING
			    && status == STATUS_OK) {
					
				status = info_list[i]->
					SetTitleForAB(AB_title);
			}
			if (AB_tocpath != NULL_STRING
			    && status == STATUS_OK) {
					
				status = info_list[i]->
					SetTOCPathForAB(AB_tocpath);

			}
			if (AB_indexpath != NULL_STRING
			    && status == STATUS_OK) {
					
				status = info_list[i]->
					SetIndexPathForAB(AB_indexpath);
			}
			if (AB_pspath != NULL_STRING
			    && status == STATUS_OK) {
					
				status = info_list[i]->
					SetPSPathForAB(AB_pspath);
			}

			status = cardcat->Rewrite (info_list, err);
				
		}
			
	}
		
	if (is_match == BOOL_FALSE)	{
		if (AB_version == NULL_STRING)
			err.Init (gettext ("AnswerBook \"%s\" not found."),
				  ~AB_id);
		else
			err.Init (gettext ("AnswerBook \"%s,%s\" not found."),
				  ~AB_id, ~AB_version);
		status = STATUS_FAILED;
	}
	
	
	return (status);
}

STATUS
CARDCAT_WORK::DoMatch (ERRSTK &err)
{
	STATUS status = STATUS_OK;
	CARDCAT *cardcat;
	LISTX<ABINFO*>	info_list;
	ABNAME name;
	BOOL is_match;
	int i;

	DbgFunc ("DoMatch: " << cc_name << " " << AB_id << " " <<
		 AB_version << endl);

	cardcat = CARDCAT::Open (cc_name, "r", err);

	if (cardcat == NULL)	{
		err.Push (gettext("Unable to open card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		cardcat->GetAll (info_list, err);
		
		is_match = TestMatch (info_list, i);
		
		if (is_match == BOOL_TRUE)	{

			name = info_list[i]->Name();				
			if (name.ABVersion() != NULL_STRING) {
				printf("id=%s,%s\n",
					~name.ABId(), ~name.ABVersion());
			} else {
				printf("id=%s\n", ~name.ABId());
			}
			cout << "title=" << info_list[i]->Title() << endl;
			cout << "tocpath=" << info_list[i]->TOCPathForAB() << 
				endl;
			cout << "pspath=" << info_list[i]->PSPathForAB() << 
				endl;
			cout << "indexpath=" << info_list[i]->IndexPathForAB() 
				<< endl;
		}
		else	{
			err.Init (gettext ("Unable to match AnswerBook %s."),
				  ~AB_id);
			status = STATUS_FAILED;
		}
		
			
	}
		
			
	return (status);
}


STATUS
CARDCAT_WORK::DoList (ERRSTK &err)
{
	STATUS status;
	CARDCAT *cardcat;
	LISTX<ABINFO*>	info_list;
	ABNAME name;
	

	DbgFunc ("DoList: " << cc_name << endl);

	status = STATUS_OK;
	
	cardcat = CARDCAT::Open (cc_name, "r", err);
	
	if (cardcat == NULL)	{
		err.Push (gettext("Unable to open card catalog"));
		status = STATUS_FAILED;
	}
	else	{
		cardcat->GetAll (info_list, err);
		
		for (int i = 0; i < info_list.Count(); i++)	{
			name = info_list[i]->Name();
			printABName(&name);
		}
		
			
	}

	return (status);
}



void
CARDCAT_WORK::printABName(ABNAME* aName)
{
  if ((aName->ABVersion()) != NULL_STRING)
  {
    cout << aName->ABId() << "," << aName->ABVersion() << endl;
  }
  else
  {
    cout << aName->ABId() << endl;
  }
}


STRING*
CARDCAT_WORK::deleteColonsInTitle(STRING& nameStr)
{
  int colonCount = 0;

  STRING* purifiedStr = new STRING();

  int len = nameStr.Length();

  for (int i=0; i<len; i++)
  {
    if (nameStr[i] == ':')
    {
      colonCount++;
    }

    if ((colonCount > MAXCOLONS) && (nameStr[i] == ':'))
    {
      *purifiedStr += BLANK_STRING;
    }
    else
    {
      *purifiedStr += nameStr.SubString(i,i);
    }
  }

  return purifiedStr;
}


STATUS
CARDCAT_WORK::ConductSearch(ABINFO* abInfo, const STRING& cardcatname) {
  CARDCATS cardcats;
  ERRSTK   err;
  STATUS   st;
  cardcats.Append(cardcatname, err);
  ABGROUP abgroup(cardcats);
  QUERY   fastQuery;
  LISTX<SEARCHDOC*> hitlist;
  fastQuery.Text(SEARCH_STRING);

  abgroup.AddAnswerBook(abInfo->Name(), err);
  st = abgroup.Search(fastQuery, hitlist, err);

  return st;
}

STATUS
CARDCAT_WORK::VerifyBookExists(ABINFO* abInfo, const STRING&) {
  ERRSTK err;
  STATUS st = STATUS_OK;
  BOOKNAME abookName;
  ABNAME   abName = abInfo->Name();
  abookName.SetABId(abName.ABId());
  abookName.SetABVersion(abName.ABVersion());
  abookName.SetBookId(abName.ABId());

  STRING book_path = abInfo->TOCPath(abookName);

  if (! BOOK::Exists(book_path)) {
	fprintf(stderr, gettext("Invalid TOC path: '%s'\n"),
		~abInfo->TOCPathForAB());
	return(STATUS_FAILED);
  }

  return(STATUS_OK);
}

STATUS
CARDCAT_WORK::VerifyPaths(ABINFO* tmpInfo) {

  STATUS st = STATUS_OK;

  STRING tocPath   = tmpInfo->TOCPathForAB();
  STRING indexPath = tmpInfo->IndexPath("C");
  STRING psPath    = tmpInfo->PSPathForAB();
  STRING dirPath   = psPath + "/" + tmpInfo->Name().ABId();

  if (access(tocPath, R_OK) != 0) {
	fprintf(stderr, gettext("Invalid TOC path for '%s': '%s'\n"),
		~AB_id, ~tocPath);
	return(STATUS_FAILED);
  }

  if (access(psPath, R_OK) != 0) {
	fprintf(stderr, gettext("Invalid PostScript path for '%s': '%s'\n"),
		~AB_id, ~psPath);
	return(STATUS_FAILED);
  }

  if (access(indexPath+".cfg", R_OK) != 0) {
	fprintf(stderr, gettext("Invalid index path for '%s': '%s'\n"),
		~AB_id, ~indexPath);
	return(STATUS_FAILED);
  }
}


STATUS
CARDCAT_WORK::Verify(const STRING& idStr, const STRING& versionStr, const STRING& cardcatname, ERRSTK &err) {
  
  STATUS status = STATUS_FAILED;
  ABNAME name;
  STRING tocPath;
  STRING psPath;
  STRING indexPath;
  BOOL is_match;
  int i;
  LISTX<ABINFO*> info_list;
  ABINFO* tmpInfo;

  AB_id      = idStr;
  AB_version = versionStr;
  
  status     = GetInfoListFromCardCat(cardcatname, info_list, err);
  if (status != STATUS_OK) {
	return status;
  }
  
  is_match   = TestMatch (info_list, i);

  if (is_match == BOOL_TRUE)	{


    tmpInfo   = info_list[i];
    status    = VerifyPaths(tmpInfo);
    
    if (status != STATUS_OK) {
      return status;
    }
      
    status = VerifyBookExists(tmpInfo, cardcatname);
    if (status != STATUS_OK) {
      return status;
    }

    status = ConductSearch(tmpInfo, cardcatname);
  }
  else {
	if (AB_version == NULL_STRING) {
		err.Init (gettext ("AnswerBook \"%s\" not found."),
			  ~AB_id);
	}
	else {
		err.Init (gettext ("AnswerBook \"%s,%s\" not found."),
			  ~AB_id, ~AB_version);
	}
    	status = STATUS_FAILED;
  }

  return status;

}









