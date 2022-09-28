/* static  char sccsid[] = "@(#)table.h 3.10 93/05/03 Copyr 1991 Sun Microsystems, Inc.";
 *
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

/*	table.h
	interface over the main data structure for abstraction.  */


/*********	Public Types & Procs:  ***************/
/*	target = name@host = desination calendar     */

typedef enum Stat
{
	status_ok,
	status_denied,
	status_rpc,
	status_param,
	status_other,
	status_notable,
	status_tblexist,
	status_notsupported,
	status_incomplete
} Stat;

typedef enum Register_Status
{
	register_succeeded,
	register_failed,
	de_registered,
	register_confused,
	register_notable,
	register_rpc
} Register_Status;

extern int table_ping(/* char *host */);

extern Stat table_lookup(/* char *target; Uid *key; Appt **r */);

extern Stat table_lookup_next_larger(/* char *target; Id *key; Appt **r */);

extern Stat table_lookup_next_smaller(/* char *target; Id *key; Appt **r */);

extern Stat table_lookup_range(/* char *target; Range *range; Appt **r */);

extern Stat table_insert(/* char *target; Appt *a; Appt **r */);

extern Stat table_delete(/* char *target; Uidopt *key; Appt **r */);

extern Stat table_change(/* char *target; Id *key; Appt *a; Options opt, Appt **r */);

extern Stat table_lookup_next_reminder(/* char *target; long key; Reminder **r */);

extern Table_Status table_check(/* char *target */);

extern int table_size(/* char *target */);

extern Stat table_abbrev_lookup_range(/* char *target; Range *range; Abb_Appt **r */);

extern Stat table_abbrev_lookup_key_range(/* char *target; Keyrange *krange; Abb_Appt **r */);

extern Stat table_get_access(/* char *target; Access_Entry **accesslist */);

extern Stat table_set_access(/* char *target; Access_Entry *accesslist */);

extern char *table_get_credentials();

extern long table_gmtoff(/* char *host */);

extern u_long table_version(/* char *target; */);

extern int table_abbrev_get_cache_size();

extern void table_abbrev_set_cache_size(/* int */);

extern Register_Status table_request_deregistration(/* char *target */);

extern Register_Status table_request_registration(/* char *target */);

extern Stat table_create(/* char *target */);

extern Stat table_remove(/* char *target */);

extern Stat table_rename(/* char *target */);

