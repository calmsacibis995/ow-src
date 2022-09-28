/* static  char sccsid[] = "@(#)convert2-4.h 1.3 92/10/21 Copyr 1991 Sun Microsystems, Inc."; */
/* convert2-4.h:  conversion routines for rpc.cmsd version 2 to
   rpc.cmsd version 4 data types
*/

extern Table_Args *tableargs2_to_tableargs4();
extern Table_Args *tabledelargs2_to_tabledelargs4();
extern Registration *reg2_to_reg4();
extern Access_Args *accargs2_to_accargs4();
extern Table_Res *tableres2_to_tableres4();
extern Table_Status tablestat2_to_tablestat4();
extern Access_Status accstat2_to_accstat4();
extern Registration_Status regstat2_to_regstat4();
