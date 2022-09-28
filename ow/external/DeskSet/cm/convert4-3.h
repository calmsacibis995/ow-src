/* static  char sccsid[] = "@(#)convert4-3.h 1.2 92/08/30 Copyr 1991 Sun Microsystems, Inc."; */
/* convert4-3.h:  conversion routines for rpc.cmsd version 4 to
   rpc.cmsd version 3 data types
*/

extern Table_Res_3 *tableres4_to_tableres3();
extern Table_Status_3 tablestat4_to_tablestat3();
extern Registration_Status_3 regstat4_to_regstat3();
extern Access_Status_3 accstat4_to_accstat3();
extern Access_Args_3 *accargs4_to_accargs3();
extern Uid_3 *uid4_to_uid3();
extern Range_3 *range4_to_range3();
extern Appt_3 *appt4_to_appt3();
extern void id4_to_id3();
extern Access_Entry_3 *acclist4_to_acclist3();
extern Keyrange_3 *keyrange4_to_keyrange3();
extern Uid_3 *uidopt4_to_uid3();
