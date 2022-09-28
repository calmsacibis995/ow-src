%{
/* @(#)ce_yacc.y	@(#)ce_yacc.y	3.1 04/03/92 */

/*
 * Grammar for Name Space Table
 */
#include "ce_int.h"
/* Functions used */
extern caddr_t add_name_space();
extern void set_state ();
extern caddr_t add_attr ();
extern caddr_t add_av ();
extern caddr_t alloc_id ();
%}
/*
 * terminals
 */
%token 	Id
%token  av_token
%token	')'
%token 	'('
%token	'}'
%token	'{'
%token 	'='
%token	NS_NAME
%token	NS_ATTR
%token	NS_ENTRIES
%token	','
%token	'$'

%%

database        :       name_space
                        | database name_space

name_space
		:	'{' name ns_attrs entries '}'
                         {
			   (void)set_state (start_all);
			 }

name
		:	 NS_NAME '=' variable 
                         {
			   (void)set_state (start_ns);
			   $$ = (int) add_name_space ($3);
			   (void)set_state (in_ns_attrs);
			 }

ns_attrs
		:       NS_ATTR '=' '(' av_list  ')'
                        {
			  (void)set_state (in_ns_entries);
			}


av_list
		:	av 
			| av_list av 

av
		:	'(' variable ',' variable ',' av_val ')'
                        {
			  $$ = (int)add_av ($2, $4, $6);
			}

entries
		:       NS_ENTRIES '=' '(' entry_info_list ')'

entry_info_list
		:	entry_ent
			| entry_info_list entry_ent 

entry_ent
		:	'(' av_list ')' 
                        {
			  (void)set_state (start_new_entry);
			}
variable        :
                        Id
                        {
			  $$ = (int)alloc_id ();
			}
av_val          :
                        av_token
                        {
			  $$ = (int)alloc_val ();
			}

	

