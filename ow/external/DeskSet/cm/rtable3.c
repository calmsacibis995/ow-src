#ifndef lint
static  char sccsid[] = "@(#)rtable3.c 3.12 92/08/30 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

/*
  veneer layered on top of the real data structures for abstraction.
	implements Version 3 in terms of Version 4 types	
 */

#define RPCGEN_ACTION(routine) routine


#include <stdio.h>
#include "rtable4.h"
#include "rtable3.h"
#include <sys/param.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include "rpcextras.h"
#include "convert3-4.h"
#include "convert4-3.h"
#include "rtable3_tbl.i"


/*************** V3 PROTOCOL IMPLEMENTATION PROCS *****************/
extern void *
rtable_ping_3(svcrq)
struct svc_req *svcrq;
{
	char dummy;	
	return((void *)&dummy); /* for RPC reply */
}

/*	PROC #1		*/
extern Table_Res_3 *
rtable_lookup_3 (args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
	static Table_Res_3 *res = NULL;
	Table_Args *newargs;
	Table_Res *newres;

	if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

	newargs = tableargs3_to_tableargs4(args);
	newres = rtable_lookup_4(newargs, svcrq); 
	res = tableres4_to_tableres3(newres);

	if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);

	return(res);
}

/*	PROC #2		*/
extern Table_Res_3 *
rtable_lookup_next_larger_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
	static Table_Res_3 *res = NULL;
	Table_Args *newargs;   
	Table_Res *newres; 
 
	if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);
 
	newargs = tableargs3_to_tableargs4(args);
	newres = rtable_lookup_next_larger_4(newargs, svcrq);
	res = tableres4_to_tableres3(newres);
 
	if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);
 
	return(res);
}

/*	PROC #3		*/
extern Table_Res_3 *
rtable_lookup_next_smaller_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
	static Table_Res_3 *res = NULL; 
	Table_Args *newargs;    
	Table_Res *newres; 
  
	if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

	newargs = tableargs3_to_tableargs4(args);
	newres = rtable_lookup_next_smaller_4(newargs, svcrq); 
	res = tableres4_to_tableres3(newres);

	if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);

	return(res);
}

/*	PROC #4		*/
extern Table_Res_3 *
rtable_lookup_range_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL;  
        Table_Args *newargs;     
        Table_Res *newres; 
   
        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

        newargs = tableargs3_to_tableargs4(args); 
        newres = rtable_lookup_range_4(newargs, svcrq);  
        res = tableres4_to_tableres3(newres); 

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs); 

        return(res);
}

/*	PROC #5		*/
extern Table_Res_3 *
rtable_abbreviated_lookup_range_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL;   
        Table_Args *newargs;      
        Table_Res *newres; 

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res); 

        newargs = tableargs3_to_tableargs4(args);  
        newres = rtable_abbreviated_lookup_range_4(newargs, svcrq);  
        res = tableres4_to_tableres3(newres);  

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);  

        return(res);
}

/*	PROC #6		*/
extern Table_Res_3 *
rtable_insert_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL;
        Table_Args *newargs;
        Table_Res *newres;

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

        newargs = tableargs3_to_tableargs4(args);
        newres = rtable_insert_4(newargs, svcrq);
        res = tableres4_to_tableres3(newres);

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);

        return(res);
}

/*	PROC #7	*/
extern Table_Res_3 *
rtable_delete_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL; 
        Table_Args *newargs;
        Table_Res *newres; 

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

	newargs = tabledelargs3_to_tabledelargs4(args, do_all);
        newres = rtable_delete_4(newargs, svcrq); 
        res = tableres4_to_tableres3(newres); 

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs); 

        return(res); 
}

/*	PROC #8		*/
extern Table_Res_3 *
rtable_delete_instance_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL; 
        Table_Args *newargs;  
        Table_Res *newres;  

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res); 

	newargs = tabledelargs3_to_tabledelargs4(args, do_one);
        newres = rtable_delete_4(newargs, svcrq);  
        res = tableres4_to_tableres3(newres);   

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);   
       
        return(res);
}

/*	PROC #9	*/
extern Table_Res_3 *
rtable_change_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL;
        Table_Args *newargs; 
        Table_Res *newres;  

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res); 

        newargs = tableargs3_to_tableargs4(args); 
        newres = rtable_change_4(newargs, svcrq); 
        res = tableres4_to_tableres3(newres);  

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);  

        return(res); 
}


/*	PROC #10	*/
extern Table_Res_3 *
rtable_change_instance_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL; 
        Table_Args *newargs;   
        Table_Res *newres;  

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res); 

        newargs = tableargs3_to_tableargs4(args);  
	newargs->args.Args_u.apptid.option = do_one;
        newres = rtable_change_4(newargs, svcrq);  
        res = tableres4_to_tableres3(newres);   

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);   

        return(res);
}

/*	PROC #11	*/
extern Table_Res_3 *
rtable_lookup_next_reminder_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL; 
        Table_Args *newargs;    
        Table_Res *newres;   

        if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);  

        newargs = tableargs3_to_tableargs4(args);   
        newres = rtable_lookup_next_reminder_4(newargs, svcrq);   
        res = tableres4_to_tableres3(newres);   

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);    

        return(res); 
}

/*	PROC #12	*/
extern Table_Status_3 *
rtable_check_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Status_3 res; 
        Table_Args *newargs;     
        Table_Status *newres;    

        newargs = tableargs3_to_tableargs4(args);    
        newres = rtable_check_4(newargs, svcrq);    
        res = tablestat4_to_tablestat3(*newres);   

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);     

        return(&res); 
}

/*	PROC #13	*/
extern Table_Status_3 *
rtable_flush_table_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Status_3 res;  
        Table_Args *newargs;     
        Table_Status *newres;     

        newargs = tableargs3_to_tableargs4(args);     
        newres = rtable_flush_table_4(newargs, svcrq);     
        res = tablestat4_to_tablestat3(*newres);    

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);      

        return(&res);
}

/*	PROC #14	*/
extern int *
rtable_size_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static int size;   
        Table_Args *newargs;      

        newargs = tableargs3_to_tableargs4(args);     
        size = (*(rtable_size_4(newargs, svcrq)));      

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);       

        return(&size);
}

/*	PROC #15	*/
Registration_Status_3 *
register_callback_3(r, svcrq)
Registration_3 *r;
struct svc_req *svcrq;
{
        static Registration_Status_3 stat;    
        Registration *newreg;       
	Registration_Status *newstat;

        newreg = reg3_to_reg4(r);      
        newstat = register_callback_4(newreg, svcrq);      
        stat = regstat4_to_regstat3(*newstat);      

        if (newreg!=NULL) xdr_free(xdr_Registration, (char*)newreg);        
	return(&stat);
}

/*	PROC #16	*/
Registration_Status_3 *
deregister_callback_3(r, svcrq)
Registration_3 *r;
struct svc_req *svcrq;
{
        static Registration_Status_3 stat;     
        Registration *newreg;          
        Registration_Status *newstat; 

        newreg = reg3_to_reg4(r);       
        newstat = deregister_callback_4(newreg, svcrq);       
        stat = regstat4_to_regstat3(*newstat);       

        if (newreg!=NULL) xdr_free(xdr_Registration, (char*)newreg);        
        return(&stat);
}

/*	PROC #17	*/
extern Access_Status_3 *
rtable_set_access_3(args, svcrq)
Access_Args_3 *args;
struct svc_req *svcrq;
{
	static Access_Status_3 stat;
	Access_Args *newargs;
	Access_Status *newstat;

        newargs = accargs3_to_accargs4(args);     
        newstat = rtable_set_access_4(newargs, svcrq);     
        stat = accstat4_to_accstat3(*newstat);    

        if (newargs!=NULL) xdr_free(xdr_Access_Args, (char*)newargs);      
	return(&stat);
}

/*	PROC #18	*/
extern Access_Args_3 *
rtable_get_access_3(args, svcrq)
Access_Args_3 *args;
struct svc_req *svcrq;
{
        static Access_Args_3 *res = NULL;
        Access_Args *newargs;
        Access_Args *newres;

	if (res!=NULL) xdr_free(xdr_Access_Args_3, (char*)res);

        newargs = accargs3_to_accargs4(args);
        newres = rtable_get_access_4(newargs, svcrq);
        res = accargs4_to_accargs3(newres);

        if (newargs!=NULL) xdr_free(xdr_Access_Args, (char*)newargs); 
        return(res); 
}

/*	PROC #19	*/
extern Table_Res_3 *
rtable_abbreviated_lookup_key_range_3(args, svcrq)
Table_Args_3 *args;
struct svc_req *svcrq;
{
        static Table_Res_3 *res = NULL;
        Table_Args *newargs;   
        Table_Res *newres;  

	if (res!=NULL) xdr_free (xdr_Table_Res_3, (char*)res);

        newargs = tableargs3_to_tableargs4(args);
        newres = rtable_abbreviated_lookup_key_range_4(newargs, svcrq);
        res = tableres4_to_tableres3(newres);

        if (newargs!=NULL) xdr_free(xdr_Table_Args, (char*)newargs);
        return(res); 
}

/*	PROC #20	*/
extern long *
rtable_gmtoff_3(svcrq)
struct svc_req *svcrq;
{
	static long gmtoff;

        gmtoff = (*(rtable_gmtoff_4(svcrq)));

	return(&gmtoff);
}

void initrtable3(ph)
        program_handle ph;
{
        ph->program_num = TABLEPROG;
        ph->prog[TABLEVERS_3].vers = &tableprog_3_table[0];
        ph->prog[TABLEVERS_3].nproc = sizeof(tableprog_3_table)/sizeof(tableprog_3_table[0]);
}

