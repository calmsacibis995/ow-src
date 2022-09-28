/* static  char sccsid[] = "@(#)convert3-4.h 1.3 92/10/21 Copyr 1991 Sun Microsystems, Inc."; */
/* convert3-4.h:  conversion routines for rpc.cmsd version 3 to
   rpc.cmsd version 4 data types
*/

extern Table_Args *tableargs3_to_tableargs4();
extern Table_Args *tabledelargs3_to_tabledelargs4();
extern Registration *reg3_to_reg4();
extern Access_Args *accargs3_to_accargs4();
extern Table_Res *tableres3_to_tableres4();
extern Table_Status tablestat3_to_tablestat4();
extern Access_Status accstat3_to_accstat4();
extern Registration_Status regstat3_to_regstat4();
