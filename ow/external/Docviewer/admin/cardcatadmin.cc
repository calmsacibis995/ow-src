#ident "@(#)cardcatadmin.cc	1.13 06/22/93 Copyright 1992 Sun Microsystems, Inc."
#include <locale.h>
#include "cardcat_work.h"
#include <doc/utils.h>

typedef enum {
	OP_UNDEFINED,
	OP_LISTPATHS,
	NIS_OP,
	NFS_OP_ADD_FROM_CC,
	NFS_OP_ADD_FROM_ARGS,
	NFS_OP_ADD_FROM_BOOKINFO,
	NFS_OP_LIST,
	NFS_OP_REMOVE,
	NFS_OP_MATCH,
	NFS_OP_MODIFY,
        NFS_OP_VERIFY} OPERATION_TYPE;



int		debug = 0;

static STRING	pgmname;

// Text domain name for localization of error messages, etc.
// Do not change this name - it is registered with the text domain name
// registry (textdomain@Sun.COM).
//
static const STRING	DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_ADMIN");

static void	OutOfMemory();
static void	Usage();
static STATUS 	PerformOp(OPERATION_TYPE op, ERRSTK &err);


// flags to program
const 	STRING	arg_nis       =	"-nis";
const	STRING	arg_file      =	"-file";
const	STRING	arg_input     =	"-input";
const	STRING	arg_add       =	"-add";
const	STRING	arg_list      =	"-list";
const	STRING	arg_listpaths =	"-listpaths";
const	STRING	arg_match     =	"-match";
const	STRING  arg_remove    = "-remove";
const	STRING 	arg_modify    =	"-modify";
const   STRING  arg_merge     = "-merge";
const   STRING  arg_convert   = "-convert";
const   STRING  arg_verify    = "-verify";
// globals used by op routines
static 	STRING 	input_cc_name;
static	STRING	AB_id;
static	STRING	AB_version;
static 	STRING	AB_title;
static	STRING	AB_tocpath;
static	STRING	AB_indexpath;
static 	STRING	AB_pspath;
static	STRING	add_file_name;
static  STRING  IdVersionString;


const	STRING	arg_id	      = "id";
const	STRING	arg_title     =	"title";
const	STRING	arg_tocpath   =	"tocpath";
const	STRING	arg_pspath    =	"pspath";
const	STRING	arg_indexpath = "indexpath";

// Function to tella user that an id is not found in cardcatalog for
// remove operation:

void
displayRemoveError()
{
  if (AB_version != NULL_STRING) {
	fprintf(stderr, gettext("AnswerBook '%s,%s' not in card catalog\n"),
		~AB_id, ~AB_version);
  } else {
	fprintf(stderr, gettext("AnswerBook '%s' not in card catalog\n"),
		~AB_id);
  }
}

//------------------------------------------------------------------
// check valid option: checks if a user supplied option is legal
// or not:

BOOL
checkValidOption(STRING& userOption)
{
  int dash = userOption.Index('-');
  BOOL legal = BOOL_FALSE;

  if (dash != 0)
  {
    legal = BOOL_TRUE;
    return legal;
  }

  if ((userOption == arg_nis)
      || 
      (userOption == arg_file)
      ||
      (userOption == arg_input)
      ||
      (userOption == arg_add)
      ||
      (userOption == arg_list)
      ||
      (userOption == arg_listpaths)
      ||
      (userOption == arg_match)
      ||
      (userOption == arg_remove)
      ||
      (userOption == arg_modify)
      ||
      (userOption == arg_merge)
      ||
      (userOption == arg_convert)
      ||
      (userOption == arg_verify))
  {
    legal = BOOL_TRUE;
  }

  return legal;
  
  
}


//-------------------------------------------------------------------
// This functions extracts attribute name from an attribute value pair
// of the form <attribure=val>
//

void 
extractIdVersion(STRING& idversion, STRING* abId, STRING* abVersion)
{
  STRING* idStr  = new STRING();
  STRING* verStr = new STRING();

  int comma      = idversion.Index(',');
  int strLen     = idversion.Length();
 
 if (comma < 0)
  {
    *idStr  = idversion;
    *verStr = NULL_STRING;


  }

  else
  {
    *idStr  = idversion.SubString(0, comma-1);

    *verStr = idversion.SubString(comma+1, strLen-1);
  }

  *abId      = *idStr;

  *abVersion = *verStr;
}

//----------------------------------------------------------------------------
// Family of functions to support add and modify operation:

BOOL
IsValidAttribute(STRING* atrStr)
{
  BOOL valid = BOOL_FALSE;

  if ( (*atrStr == arg_id)
       ||       
       (*atrStr == arg_title)
       ||
       (*atrStr == arg_tocpath)
       ||
       (*atrStr == arg_pspath)
       ||
       (*atrStr == arg_indexpath)
     )
  {
    valid = BOOL_TRUE;
  }

  return valid;
}


void
SetAttribute(STRING* atrStr, STRING* valStr)
{
  BOOL isValid = IsValidAttribute(atrStr);

  if (isValid == BOOL_TRUE)
  {
    if (*atrStr == arg_id) 
      	extractIdVersion(*valStr, &AB_id, &AB_version);

    if (*atrStr == arg_title) AB_title = *valStr;

    if (*atrStr == arg_tocpath) AB_tocpath = *valStr;

    if (*atrStr == arg_pspath) AB_pspath = *valStr;

    if (*atrStr == arg_indexpath) AB_indexpath = *valStr;
  }

  else
  {
    fprintf(stderr, gettext("Invalid add or modify attribute: '%s'\n"),
		~(*atrStr));
    Usage();
    exit(1);
  }
    

}

void
parseArg(STRING& argStr)
{
  int equalSign = argStr.Index('=');
  int strLen    = argStr.Length();

  if (equalSign < 0)
  {
    if (AB_id == NULL_STRING)
    {
      extractIdVersion(argStr, &AB_id, &AB_version);
    }
    else
     {
       fprintf(stderr, gettext("Invalid argument: '%s'\n"), ~argStr);
       Usage();
       exit(1);
     }
  }

  else
  {
    STRING* atribStr = new STRING();

    STRING* valStr   = new STRING();

    *atribStr        = argStr.SubString(0, equalSign-1);

    *valStr          = argStr.SubString(equalSign+1, strLen-1);

    SetAttribute(atribStr, valStr);
  }
  
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
main(int argc, char **argv)
{
	char		*slash;
	OPERATION_TYPE	op;
	int	       i;
	ERRSTK		err;
	STATUS		status;
	extern char    *optarg;
	extern int	optind;
	void		set_new_handler(void (*)());

	// init all strings
 	input_cc_name = NULL_STRING;
	AB_id = NULL_STRING;
	AB_version = NULL_STRING;
	AB_title = NULL_STRING;
	AB_tocpath = NULL_STRING;
	AB_indexpath = NULL_STRING;
	AB_pspath = NULL_STRING;
	add_file_name = NULL_STRING;
	
	
	if ((slash = strrchr(argv[0], '/'))  !=  NULL)
		pgmname = slash + 1;
	else
		pgmname = argv[0];


	// Set up for localization.
  	//
  	setlocale(LC_ALL, "");
	InitTextDomain(DOMAINNAME);

  
  	// Handle "out of memory" condition.
	//
	set_new_handler(OutOfMemory);

	// process arguments
	op = OP_UNDEFINED;
	
	for (i = 1; i < argc; i++)	{


	        // check validity of user supplied option. If illegal
	        // option, print Usage and quit:

	        STRING currentArg = argv[i];
	        BOOL check = checkValidOption(currentArg);

		if (check == BOOL_FALSE)
	        {
		  Usage();
		  exit(1);
		}
		// operation args


		if (argv [i] == arg_listpaths)
			op = OP_LISTPATHS;

		if (argv [i] == arg_nis)	{
			op = NIS_OP;
		}

		if (argv [i] == arg_add)   
	        {
			if (op == OP_UNDEFINED)
					  op = NFS_OP_ADD_FROM_ARGS;
			for (int j=i+1; j < argc; j++)
		        {
			  STRING addArg = argv[j];
			  parseArg(addArg);
			}
	        }
		if (argv [i] == arg_remove)
	        {
			if (op == OP_UNDEFINED)
				op = NFS_OP_REMOVE;		
		        STRING removeArg = argv [i + 1];
			extractIdVersion(removeArg, &AB_id, &AB_version);
	        }


		if (argv [i] == arg_match)
	        {
			if (op == OP_UNDEFINED)
				op = NFS_OP_MATCH;		

			STRING matchArg = argv[ i + 1];
			extractIdVersion(matchArg, &AB_id, &AB_version);
	        }

		if (argv [i] == arg_list)
	        {
			if (op == OP_UNDEFINED)
				op = NFS_OP_LIST;
	        }

		if (argv [i] == arg_modify)
	        {
			if (op == OP_UNDEFINED)
				op = NFS_OP_MODIFY;

			for (int j=i+1; j < argc; j++)
		        {
			  STRING addArg = argv[j];
			  parseArg(addArg);
			}
	        }

		if (argv [i] == arg_merge) 
	        {
		        add_file_name = argv [i + 1];
			if (op == OP_UNDEFINED)
					  op = NFS_OP_ADD_FROM_CC;			
	        }

		if (argv [i] == arg_convert)	
	        {
			add_file_name = argv [ i + 1];
			if (op == OP_UNDEFINED)
			                 op = NFS_OP_ADD_FROM_BOOKINFO;
	        }

		if (argv[i] == arg_verify) {
		  STRING verifyArg = argv[i+1];
		  parseArg(verifyArg);
		  op = NFS_OP_VERIFY;
		}
		  

		if (argv [i] == arg_file) 
			input_cc_name = argv [i + 1];
		if (argv [i] == arg_input)	
			add_file_name = argv [ i + 1];

	      }

	if (op == OP_UNDEFINED)	{
		Usage();
		return (1);
	}
	else {
		status = PerformOp (op, err);
		if (status == STATUS_FAILED)	{
			cerr << err;
			return (1);
		}			
		
	}		

	return (0);
      }

BOOL
ArgsAreValid ()
{
	BOOL result = BOOL_FALSE;
	
	if (AB_id != NULL_STRING
	    && AB_title != NULL_STRING
	    && AB_tocpath != NULL_STRING
	    && AB_indexpath != NULL_STRING
	    && AB_pspath != NULL_STRING)
		result = BOOL_TRUE;
	
	return (result);
}

	
STATUS
PerformOp (OPERATION_TYPE op, ERRSTK &err)
{
	STATUS status = STATUS_FAILED;
	CARDCAT_WORK *worker;
	
	DbgFunc ("PerformOp: " << op << endl);
	
	worker = CARDCAT_WORK::Init ();

	if (op == NIS_OP) {
		err.Init(gettext("NIS card catalogs supported\n"));
		return(STATUS_FAILED);
	}


	if (op == NFS_OP_ADD_FROM_ARGS	||
	    op == NFS_OP_REMOVE		||
	    op == NFS_OP_MATCH		||
	    op == NFS_OP_MODIFY		||
	    op == NFS_OP_VERIFY) {
		if (AB_id == NULL_STRING) {
			fprintf(stderr,
				gettext("Must specify AnswerBook id\n"));
			Usage();
			exit(1);
		}
	}

	if (op != OP_LISTPATHS) {
		if (input_cc_name == NULL_STRING) {
			fprintf(stderr,
				gettext("Must specify card catalog file\n"));
			Usage();
			exit(1);
		}
	}


	if (op == NFS_OP_ADD_FROM_CC) {
		if (add_file_name == NULL_STRING) {
			fprintf(stderr, gettext(
				"Must specify card catalog to merge\n"));
			Usage();
			exit(1);
		}
	}


	if (op == NFS_OP_ADD_FROM_BOOKINFO) {
		if (add_file_name == NULL_STRING) {
			fprintf(stderr, gettext(
				"Must specify bookinfo file to convert\n"));
			Usage();
			exit(1);
		}
	}


	// check off args against ops
	switch (op) {
	case OP_LISTPATHS:
		status = worker->ListPaths(err);
		break;

	case NFS_OP_ADD_FROM_CC:
	case NFS_OP_ADD_FROM_BOOKINFO:
		worker->SetCardCatName(input_cc_name);
		status = worker->DoAddFromFile(add_file_name, err);
		break;

	case NFS_OP_ADD_FROM_ARGS:
		if ( ! ArgsAreValid()) {
			fprintf(stderr, gettext(
				"Invalid arguments for add operation\n"));
			Usage();
			exit(1);
		}
					
		worker->SetCardCatName (input_cc_name);
		worker->SetABId (AB_id);
		worker->SetABVersion (AB_version);
		worker->SetABTitle (AB_title);
		worker->SetABTOCPath (AB_tocpath);
		worker->SetABIndexPath (AB_indexpath);
		worker->SetABPSPath (AB_pspath);

		status = worker->DoAddFromArgs (err);
		break;
		
	case NFS_OP_LIST:
		worker->SetCardCatName (input_cc_name);
		status = worker->DoList (err);
		break;
			
	case NFS_OP_MATCH:
		worker->SetCardCatName (input_cc_name); 
		worker->SetABId (AB_id);
		worker->SetABVersion (AB_version);
		status = worker->DoMatch (err);
		break;
			
	case NFS_OP_REMOVE:
		worker->SetCardCatName (input_cc_name); 
		worker->SetABId (AB_id);
		worker->SetABVersion (AB_version);	
		status = worker->DoRemove (err);
		break;
		
	case NFS_OP_MODIFY:
		worker->SetCardCatName (input_cc_name);
		worker->SetABId (AB_id);
		worker->SetABVersion (AB_version);
		worker->SetABTitle (AB_title);
		worker->SetABTOCPath (AB_tocpath);
		worker->SetABIndexPath (AB_indexpath);
		worker->SetABPSPath (AB_pspath);				
		status = worker->DoModify (err);
		break;

	      case NFS_OP_VERIFY:
		worker->SetABId (AB_id);
		worker->SetABVersion (AB_version);
		worker->SetCardCatName (input_cc_name);
		status = worker->Verify(AB_id, AB_version, input_cc_name, err);
		break;
	default:
		Usage();
		exit(1);
	}
	
	return (status);
}

	



void
Usage()
{
	
fprintf(stderr,	gettext("usage:\n"));
fprintf(stderr,	gettext("    %s -file <card-catalog>\n"), ~pgmname);
fprintf(stderr, gettext("               -add <answerbook-id[,version]>\n"));
fprintf(stderr, gettext("                   title=<answerbook-title>\n"));
fprintf(stderr, gettext("                   tocpath=<toc-path>\n"));
fprintf(stderr, gettext("                   indexpath=<index-path>\n"));
fprintf(stderr, gettext("                   pspath=<postscript-path>\n"));
fprintf(stderr, gettext("               -convert <bookinfo-file>\n"));
fprintf(stderr, gettext("               -list\n"));
fprintf(stderr, gettext("               -match  <answerbook-id[,version]>\n"));
fprintf(stderr, gettext("               -merge <card-catalog>\n"));
fprintf(stderr, gettext("               -modify <answerbook-id[,version]>\n"));
fprintf(stderr, gettext("                   title=<answerbook-title>\n"));
fprintf(stderr, gettext("                   tocpath=<toc-path>\n"));
fprintf(stderr, gettext("                   indexpath=<index-path>\n"));
fprintf(stderr, gettext("                   pspath=<postscript-path>\n"));
fprintf(stderr, gettext("               -remove <answerbook-id[,version]>\n"));
fprintf(stderr, gettext("               -verify <answerbook-id[,version]>\n"));
fprintf(stderr, gettext("OR\n"));
fprintf(stderr,	gettext("    %s -listpaths\n"), ~pgmname);
}


void
OutOfMemory()
{
	fprintf(stderr, gettext("Out of memory\n"));
	exit(1);
}
