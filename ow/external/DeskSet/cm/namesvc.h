/*
 *  static  char sccsid[] = "@(#)namesvc.h 1.8 92/09/21 Copyr 1992 Sun Microsystems, Inc.";
 *
 *  Copyright (c) 1987-1992 Sun Microsystems, Inc.
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
 *
 */ 

/*	namesvc.h		*/
/*
 * Given a user specification, return calendar name.
 * Input: user[@domain] or user@host[.domain]
 * Output: calendar_name@host[.domain]
 *
 * For now, calendar_name = user.
 * First assume user[@domain] to be the format and get the location
 * from nis+ table. If the table exists but cannot get location,
 * return NULL.  This is for the case where nis+ is used, but
 * no entry exists in the location table yet.
 * Otherwise assume user@host[.domain].  If host information is
 * not supplied, default to local host.
 */

#if 0 /* use this for NIS+ */
extern int cm_get_yptarget(/* char *name, char **target */);

typedef enum Lookup_stat {l_ok, l_no_table, l_nis_error, l_other} Lookup_stat;

/* login.domain -> login */
extern char *principal_to_user( /* char *principal */);

/* login.domain -> domain */
extern char *principal_to_domain( /* char *principal */ );

/* convert netname to nis+ principal name */
extern char *netname2principal( /* char *netname */ );

/* get location info from nis+ table */
Lookup_stat cm_get_nis_location(/* char* name, char* calname, char *loc */);

/* set location in nis+ table */
Lookup_stat cm_set_nis_location(/* char* name, char* calname, char* loc */);

#define CREDDIR         "cred.org_dir"
#define AUTHNAMECOLNAME "auth_name"
#define PRINCIPALCOLNUM 0

#define USERSCONTEXT    "user_dir"
#define USERMAP         "cm_location"
#define RESOURCEMAP     "cm_location"
#define NAMECOLNAME     "cal_name"
#define LOCATIONCOLNAME "cal_location"
#define DEFAULT_CALNAME "default"
#define NAMECOLNUM      0
#define LOCATIONCOLNUM  1
#define NUMTBLCOLS      2
#endif
