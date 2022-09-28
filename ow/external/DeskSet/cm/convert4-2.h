/* static  char sccsid[] = "@(#)convert4-2.h 1.2 92/08/30 Copyr 1991 Sun Microsystems, Inc."; */
/* convert4-2.h:  conversion routines for rpc.cmsd version 4 to
   rpc.cmsd version 2 data types
*/

extern Table_Res_2 *tableres4_to_tableres2();
extern Table_Status_2 tablestat4_to_tablestat2();
extern Registration_Status_2 regstat4_to_regstat2();
extern Access_Status_2 accstat4_to_accstat2();
extern Access_Args_2 *accargs4_to_accargs2();
extern Uid_2 *uid4_to_uid2();
extern Range_2 *range4_to_range2();
extern Appt_2 *appt4_to_appt2();
extern void id4_to_id2();
extern Access_Entry_2 *acclist4_to_acclist2();
extern Uid_2 *uidopt4_to_uid2();
