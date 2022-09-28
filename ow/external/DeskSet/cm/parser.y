%{
#ifndef lint
static  char sccsid[] = "@(#)parser.y 3.7 94/01/19 Copyr 1991 Sun Microsystems, Inc.";
#endif

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include "util.h"
#include "rtable4.h"
#include "lexer.h"

int currentVersion = 1;
int n = 0;

extern Appt *rtable_delete_internal();
extern char *current_owner;

%}

%start calendar

%union	{
	int number;
	char *s;
	Interval periodVal;
	Appt *appointment;
	Attribute *attributelist;
	Except *exceptionlist;
	Access_Entry *accesslist;
	Event_Type tagsVal;
	Tag *taglist;
	Appt_Status apptstatVal;
	Privacy_Level privacyVal;
	}

%token VERSION COLON NUMBER QUOTEDSTRING DETAILS DURATION PERIODVAL NTIMES COMMA
%token OPENPAREN CLOSEPAREN WHAT REMOVE ADD PERIOD EXCEPTION MAILTO NTH ENDDATE
%token AUTHOR ATTRIBUTES ACCESS DENY PRIVACY PRIVACYENUM
%token READACCESS KEY READ WRITE DELETE EXEC
%token TAGS TAGSVAL
%token APPTSTATUS APPTSTATUSVAL
%token PRIVACYLEVEL PRIVACYVAL

%type <s> quotedString what details mailto author
%type <number> number tick key ntimes duration accesstype nth enddate
%type <appointment> appointmentBody 
%type <attributelist> attributes attributelist attributeitem
%type <periodVal> period 
%type <exceptionlist> exceptions exceptionlist exceptitem
%type <accesslist> accesses accesslist accessitem
%type <taglist> tags taglist tagitem 
%type <apptstatVal> apptstat
%type <privacyVal> privacy

%% /* Begining of rules section */

calendar	: versionStamp eventlist
		| versionStamp
		;

versionStamp	: VERSION COLON number
			{
				if ($3 != currentVersion) 
					report_err("rpc.cmsd: Unable to reading callog file");
			}
		| error OPENPAREN
			{
				yyerror("Error reading callog file");
				return -1;
			}
		;

number		: NUMBER
			{
				$$ = externNumberVal;
			}
		;

eventlist	: event
		| eventlist event
		;

event		: add 
		| remove 
		| access
		| deny
		;

add		: OPENPAREN ADD appointmentBody CLOSEPAREN
			{
				Appt *a;
                               	a = $3;
                               	(void)rtable_insert_internal(current_owner, a); 
			}
		;

remove		: OPENPAREN REMOVE tick key CLOSEPAREN
			{
				Appt *a;
				Id key;
                                key.tick = (int)$3;
				key.key = (int) $4;
                                a = rtable_delete_internal(current_owner, &key);
				if (a!=NULL) {
					destroy_appt(a);
				}
			}
		;

access		: OPENPAREN ACCESS accesstype accesses CLOSEPAREN
			{
				int t;
				Access_Entry *e;
				t = $3;
				e = $4;
				(void) rtable_set_access_internal(current_owner,
							e, t);
			}
		;

deny		: OPENPAREN DENY accesses CLOSEPAREN
			{
				Access_Entry *e;
				e = $3;
				/* (void)rtable_deny_access_internal(current_owner, e); */
			}
		;
				

appointmentBody	: tick key what details duration period nth enddate ntimes exceptions mailto author attributes tags apptstat privacy
			{
				char *temp=NULL, *temp1=NULL;
				Attr item;
				Appt *newp = make_appt();
				newp->appt_id.tick = $1;
				newp->appt_id.key = $2;
				newp->what = $3;
				newp->client_data = $4;
				newp->duration = $5;
				if (newp->what==NULL) {
					newp->what = temp;
				}
				temp = str_to_cr(newp->what);
				free(newp->what);
				newp->what = temp;
				newp->period.period = $6;
				newp->period.nth = $7;
				newp->period.enddate = $8;
				newp->ntimes = $9;
				newp->exception = $10;  

				temp1 = $11;

				newp->author = $12;
				newp->attr = $13;

				if (temp1 != NULL && (*temp1)!=NULL) {
				  item = newp->attr;
				  while(item!=NULL) {
				    if(strcmp(item->attr, "ml")==0) {
				      item->clientdata= temp1;
				      break;
				    }
				  item = item->next;
				  }
				}

				newp->tag = $14;
				newp->appt_status = $15;
				newp->privacy = $16;
				newp->next=NULL;
				$$ = newp;
			}
		;

tick		: quotedString 
			{
				$$ = cm_getdate($1, NULL);
			}
		;

quotedString	: QUOTEDSTRING
			{
				$$ = cm_strdup(externQuotedString);
			}
		;

key		: /* empty */
			{
				$$ = NULL;
			}
		| KEY COLON number
			{
				$$ = $3;
			}
		;

what		: /* empty */
			{
				$$ = cm_strdup("Empty Appointment");
			}
		| WHAT COLON quotedString
			{
				$$ = $3;
			}
		;
				
details		: /* empty */
			{
				$$ = "";
			}
		| DETAILS COLON quotedString
			{
				$$ = $3;
			}
		;

mailto		: /* empty */
			{
				$$ = "";
			}
		| MAILTO COLON quotedString
			{
				$$ = $3;
			}
		;

duration	: /* empty */
			{
				$$ = NULL;
			}
		| DURATION COLON number
			{
				$$ = $3;
			}
		;

period		: /* empty */
			{
				$$ = single;
			}
		| PERIOD COLON PERIODVAL
			{
				$$ = externPeriod.period;
			}
		;

nth		: /* empty */
			{
				$$ = 0;
			}
		| NTH COLON number
			{
				$$ = $3;
			}
		;

enddate		: /* empty */
			{
				$$ = 0;
			}
		| ENDDATE COLON quotedString
			{
				$$ = cm_getdate($3, NULL);
			}
		;
				
privacy		: /* empty */
			{
				$$ = public;
			}
		| PRIVACY COLON PRIVACYVAL
			{
				$$ = externPrivacy;
			}
		;

apptstat	: /* empty */
			{
				$$ = active;
			}
		| APPTSTATUS COLON APPTSTATUSVAL
			{
				$$ = externApptStatus;
			}
		;

ntimes		: /* empty */
			{
				$$ = 0;
			}
		| NTIMES COLON number
			{
				$$ = $3;
			}
		;

author		: /* empty */
			{
				$$ = "";
			}
		| AUTHOR COLON quotedString
			{
				$$ = $3;
			}
		;

accesstype	: READ
			{
				$$ = access_read;
			}
		| WRITE
			{
				$$ = access_write;
			}
		| DELETE
			{
				$$ = access_delete;
			}
		| EXEC
			{
				$$ = access_exec;
			}
		;

accesses	: /* empty */
			{
				$$ = NULL;
			}
		| accesslist
			{
				$$ = $1;
			}
		;

accesslist	: accessitem
			{
				$$ = $1;
			}

		| accessitem accesslist
			{
				Access_Entry *e = $1;
				e->next = $2;
				$$ = e;
			}
		;

accessitem	: quotedString 
			{
				Access_Entry *e =
				  (Access_Entry *)ckalloc(sizeof(Access_Entry));
				e->who = $1;
				e->next = (Access_Entry *)NULL;
				$$ = e;
			}
		;

attributes	: /* empty */
			{
				$$ = NULL;
			}
		| ATTRIBUTES COLON OPENPAREN attributelist CLOSEPAREN
			{
				$$ = $4;
			}
		;

attributelist  : attributeitem
			{
				$$ = $1;
			}
		| attributeitem attributelist
			{
				Attr p = $1;
				p->next = $2;
				$$ = p;
			}
		;
attributeitem	:OPENPAREN quotedString COMMA quotedString COMMA quotedString CLOSEPAREN
			{	
				Attr newattr = make_attr();
				newattr->next = (Attr)NULL;
				newattr->attr = $2;
				newattr->value = $4;
				newattr->clientdata = $6;
				$$ = newattr;
			}
		| OPENPAREN quotedString COMMA quotedString CLOSEPAREN
                        {
                                Attr newattr = make_attr();
                                newattr->next = (Attr)NULL;
                                newattr->attr = $2;
                                newattr->value = $4;
                                $$ = newattr;
                        }
                ;

tags		: /* empty */
			{
				Tag *t = (Tag *)ckalloc(sizeof(Tag));
				t->next = (Tag *)NULL;
				t->tag = appointment;
				t->showtime = 1;
				$$ = t;
			}
		| TAGS COLON OPENPAREN taglist CLOSEPAREN
			{
				$$ = $4;
			}
		;

taglist		: tagitem
			{
				$$ = $1;
			}
		| tagitem taglist
			{
				Tag *t = $1;
				t->next = $2;
				$$ = t;
			}
		;

tagitem		: OPENPAREN TAGSVAL COMMA number CLOSEPAREN
			{
				Tag *t = (Tag *)ckalloc(sizeof(Tag));
				t->next = (Tag *)NULL;
				t->tag = externTag.tag;
				t->showtime = $4;
				$$ = t;
			}
		;


exceptions	: /* empty */
			{
				$$ = NULL;
			}
		| EXCEPTION COLON OPENPAREN exceptionlist CLOSEPAREN
			{
				$$ = $4;
			}
		;

exceptionlist : exceptitem
			{
				$$ = $1;
			}
		| exceptitem exceptionlist
			{
				Exception p = $1;
				p->next = $2;
				$$ = p;
			}
		;
exceptitem : number
		{
				Exception newexcept =
					(Exception)ckalloc(sizeof(Except));
				newexcept->next = (Exception)NULL;
				newexcept->ordinal = $1;
				$$ = newexcept;
		}
		;


