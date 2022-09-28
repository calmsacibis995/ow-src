/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isschema.y
 *
 * Description:
 *	yacc file for NetISAM Programmer Tool schema.
 */

%{
#ifndef lint
static char sccsid[] = "@(#)isschema.y	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif

#include "isam_impl.h"

struct istableinfo *istblinf_create();
struct isfldmap *findfield();

/* Some variables used to fill information while parsing schema file.   */
/* Is seems easier to use variable compared to using attributed grammar */
/* for this simple parser.						*/
static struct istableinfo *pistableinfo;

static struct keydesc curkeydesc;	     /* Semantic functions will */
					     /* fill this structure     */

static int  (*tabfunc)();		     /* Call this function after */
					     /* each table is parsed. */
%}
%start schema

%union {
    struct fld2 {
	int	type;
	int	length;
    }		     fld2;
    char	     *cval;
    int		     ival;
}

%token	TABLE LONG SHORT INT DOUBLE FLOAT CHAR BIN INTNUMB IDENT
%token  KEY AS PRIMARY ALTERNATE DUPLICATES DESC ASC

%type  <cval> IDENT field_name
%type  <ival> INTNUMB option option_list primary_alternate opt_order
%type  <fld2> type_spec

%%
schema	:  	table_define_list
        ;

table_define_list : table_define table_define_list
                  | table_define 
                  ;

table_define : { 
     pistableinfo = istblinf_create ();
	       } 
             table_declare  key_declare_list 
             { 
     check_primkey ();			     /* 0 or 1 primary keys allowed */

     (*tabfunc) (pistableinfo);		     /* Call user specified function */
     istblinf_free (pistableinfo);	     /* Free allocated memory */
     pistableinfo = (struct istableinfo *) 0;
	     }
             ;

key_declare_list : /* empty */
                 | key_declare_list key_declare
                 ;

table_declare	:   TABLE  IDENT '(' field_decl_list ')'
                {
    pistableinfo->tbi_tablename = $2;		    
		}
	        ;

key_declare	:  primary_alternate KEY IDENT AS '(' field_name_list ')' option_list
                {
    decl_key ($3,$8,$1);
		}
	        ;

field_decl_list : field_decl
                | field_decl_list ',' field_decl
                ;

field_decl	: IDENT	type_spec  { decl_field ($1,$2); }
                ;

type_spec	: LONG { $$.type = LONGTYPE; $$.length = LONGSIZE; };
                | SHORT { $$.type = SHORTTYPE; $$.length = SHORTSIZE; };
                | INT  { $$.type = INTTYPE; $$.length = INTSIZE; };
                | DOUBLE  { $$.type = DOUBLETYPE; $$.length = DOUBLESIZE; };
                | FLOAT  { $$.type = FLOATTYPE; $$.length = FLOATSIZE; };
                | CHAR '(' INTNUMB ')' 
                    { $$.type = CHARTYPE; $$.length = $3; };
                | BIN '(' INTNUMB ')' 
                    { $$.type = BINTYPE; $$.length = $3; };
                ;

primary_alternate : PRIMARY   { $$ = 1; }
                  | ALTERNATE { $$ = 0; }
                  ;
field_name_list	:  field_name opt_order
                {
     memset ((void *)&curkeydesc, 0, sizeof (curkeydesc)); /* Reset the structure */
     decl_key_field ($1, $2);
     		}  
                |  field_name_list ',' field_name opt_order
                {
     decl_key_field ($3, $4);
     		}  
                ;

opt_order	: /* empty */
                {
	$$ = 0;
		}
	        | DESC
	      	{
	$$ = ISDESC;
		}
		| ASC
		{
	$$ = 0;
		}
                ;

field_name	: IDENT { $$ = $1; }
                ;

option_list	: /* empty */  { $$ = ISNODUPS; }
                | option_list option  { $$ = $1 | $2; }
                ;

option          : DUPLICATES  { $$ = ISDUPS; }
                ;
%%

#include "isschemalex.c"

isschema (schfile,func)
    FILE	*schfile;
    int		(*func)();
{
    yyin = schfile;
    tabfunc = func;			     /* Parser will call this function*/
					     /* after each table is parsed */
    yyparse();
}

static decl_field (fldname,fld2)
    char	*fldname;
    struct fld2	fld2;
{
    register int		nfields;
    register struct isfldmap	*p;

/*    printf ("%s %d %d\n",fldname,fld2.type,fld2.length); */

    nfields = pistableinfo->tbi_nfields++;

    pistableinfo->tbi_fields = (struct isfldmap *) 
	_isrealloc((char *)pistableinfo->tbi_fields, 
			 (nfields+1) * sizeof (pistableinfo->tbi_fields[0]));
    
    p = pistableinfo->tbi_fields + nfields;

    memset ((void *) p,  0, sizeof (*p));
    p->flm_fldname = fldname;
    p->flm_recoff = pistableinfo->tbi_reclength;
    p->flm_type   = fld2.type;
    p->flm_length = fld2.length;

    pistableinfo->tbi_reclength += fld2.length;
}

static decl_key_field (fldname, desc_flag)
    char	*fldname;
    int	        desc_flag;
{
    struct isfldmap	*pmap;		     /* Will point to entry in tbi_fields*/
    char		errmsg [80];
    struct keypart	*kp;

    if ((pmap = findfield (fldname)) == (struct isfldmap *) 0) {
	(void)sprintf (errmsg,"field '%s' not declared in table",fldname);
	yyerror (errmsg);
    }
    
    if (curkeydesc.k_nparts >= NPARTS) {
	yyerror ("too many key parts");
    }

    kp = curkeydesc.k_part + curkeydesc.k_nparts++;

    kp->kp_start = pmap->flm_recoff;
    kp->kp_leng  = pmap->flm_length;
    kp->kp_type  = pmap->flm_type + desc_flag;
}

static decl_key (keyname,options,prim)
    char		*keyname;
    int			options;
    int			prim;		     /* is 1 for primary key */
{
    register int		nkeys;
    char			errmsg[120];

    curkeydesc.k_flags = options;

    /*
     * Don't change the flags because of compatibility with C-ISAM
    if (prim) curkeydesc.k_flags |= ISPRIMKEY;
    */

    if (prim) {

	if (pistableinfo->tbi_keys[0].k_nparts > 0) {

	    (void)sprintf(errmsg,"table '%s' has more than one primary key\n",
		      pistableinfo->tbi_tablename);
	    yyerror (errmsg);
	    
	    return;
	}
	
	pistableinfo->tbi_keys[0] = curkeydesc;
	pistableinfo->tbi_keynames[0] = keyname;
    }
    else {
	nkeys = pistableinfo->tbi_nkeys++;
	
	pistableinfo->tbi_keys = (struct keydesc *) 
	    _isrealloc ((char *)pistableinfo->tbi_keys, 
			(nkeys+1) * sizeof (pistableinfo->tbi_keys[0]));
	
	pistableinfo->tbi_keys[nkeys] = curkeydesc;
	
	pistableinfo->tbi_keynames = (char **) 
	    _isrealloc ((char *)pistableinfo->tbi_keynames, 
			(nkeys+1) * sizeof (pistableinfo->tbi_keynames[0]));
	
	pistableinfo->tbi_keynames[nkeys] = keyname;
    }
}

yywrap ()
{
    return (1);
}


yyerror (s)
    char	*s;
{
    (void)fprintf (stderr,"line %d: %s",linenumber,s);
    if (strcmp (s,"syntax error")==0)
	(void)fprintf (stderr," near token '%s'",yytext);
    (void)fputc ('\n',stderr);
    exit (1);
}

struct istableinfo *istblinf_create ()
{
    register struct istableinfo 	*p;
    extern char 			*strdup();

    p = (struct istableinfo *) _ismalloc (sizeof (*pistableinfo));
    memset ((void *) p, 0, sizeof (*p));

    p->tbi_nkeys = 1;	        /* reserve a slot for primary key */

    p->tbi_fields = (struct isfldmap *) 
	_ismalloc (sizeof (pistableinfo->tbi_fields[0]));

    p->tbi_keys = (struct keydesc *) 
	_ismalloc (sizeof (pistableinfo->tbi_keys[0]));

    memset((void *)p->tbi_keys, 0, sizeof (pistableinfo->tbi_keys[0]));

    p->tbi_keynames = (char **)
	_ismalloc (sizeof (pistableinfo->tbi_keynames[0]));

    p->tbi_keynames[0] = strdup("nokey");

    return (p);
}

istblinf_free (p)
    struct istableinfo	*p;
{
    register int	i;

    for (i = 0; i < p->tbi_nfields; i++) {
	free (p->tbi_fields[i].flm_fldname);
    }

    for (i = 0; i < p->tbi_nkeys; i++) {
	free (p->tbi_keynames[i]);
    }

    free (p->tbi_tablename);
    free ((char *) p->tbi_fields);
    free ((char *) p->tbi_keys);
    free ((char *) p->tbi_keynames);
    free ((char *) p);
}

static struct isfldmap *findfield (fldname)
    char	*fldname;
{
    int		nfields = pistableinfo->tbi_nfields;
    char	errmsg [100];
    register int   i;

    for (i = 0; i < nfields; i++) {
	if (strcmp (pistableinfo->tbi_fields[i].flm_fldname,fldname)==0)
	    break;
    }
    
    return ((i<nfields)?(pistableinfo->tbi_fields+i):(struct isfldmap *) 0);
}

static check_primkey ()
{
    register int 	nkeys;
    int			primkey_count = 0;
    int			primindex = -1;
    register int	i;
    struct keydesc	tempkeydesc;
    char		*tempname;
    char		errmsg [100];

    nkeys = pistableinfo->tbi_nkeys;

#if 0 /*  Don't do any check on primary key existence */
    for (i = 0; i < nkeys; i++) {
	if (pistableinfo->tbi_keys[i].k_flags & ISPRIMKEY) {
	    primkey_count++;
	    primindex = i;
	}
    }

    if (primkey_count > 1) {
	(void)sprintf(errmsg,"table '%s' has more than one primary key\n",
		      pistableinfo->tbi_tablename);
	yyerror (errmsg);
    }
#endif /* Don't do any check on primary key existence */

    /* Make sure that primary key is first in tbi_keys table. */
    if (primindex != -1 && primindex != 0) {		
	tempkeydesc = pistableinfo->tbi_keys[primindex];
	tempname    = pistableinfo->tbi_keynames[primindex];

	pistableinfo->tbi_keys[primindex] = pistableinfo->tbi_keys[0];
	pistableinfo->tbi_keynames[primindex] = pistableinfo->tbi_keynames[0];

	pistableinfo->tbi_keys[0] = tempkeydesc;
	pistableinfo->tbi_keynames[0] = tempname;
    }

}
