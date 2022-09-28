/*
 *  static  char sccsid[] = "@(#)rtable4.x 1.9 93/05/03 Copyr 1991 Sun Microsystems, Inc.";
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

typedef string Buffer<>;

enum Transaction {
	add, cm_remove
};

enum Interval {
	single, daily, weekly, biweekly, monthly, yearly,
	nthWeekday, everyNthDay, everyNthWeek,
	everyNthMonth, otherPeriod,
	monThruFri, monWedFri, tueThur, daysOfWeek
};

struct Period {
	Interval period;
	int	nth;
	long	enddate;
};

enum Event_Type {
	appointment, reminder, otherTag, holiday, toDo
};

enum Options {
	do_all, do_one, do_forward
};

struct Tag {
	Event_Type tag;
	int showtime;		/* Advisory for formatting time */
	struct Tag *next;
};

enum Privacy_Level {
	public, private, semiprivate
};

struct Attribute {
        struct Attribute *next;
        Buffer          attr;
        Buffer          value;
	Buffer		clientdata;
};
typedef Attribute *Attr;

struct Except {
	int	ordinal;
	struct Except *next;
};
typedef Except *Exception;

struct Id {
	long	tick;			/* user specified time stored as GMT */
	long	key;			/* system assigned id */
};

struct Uid {
	struct Id	appt_id;
	struct Uid	*next;
};

enum Appt_Status {
	active, pendingAdd, pendingDelete, committed, cancelled, completed
};

struct Appt {
	struct Id	appt_id;	/* appointment/repeater id */
	struct Tag	*tag;		/* event type & advisory time display */
	int		duration;	/* appt duration in seconds */
	int		ntimes;		/* n repeat times (0 .. forever) */
	Buffer		what;		/* text of appointment */
	struct Period	period;		/* periodicity of event: single default */
	Buffer		author;		/* user who inserted the appt */
	Buffer		client_data;	/* TBD. */
	struct Except	*exception;	/* list of exceptions to repeating events */
	struct Attribute *attr;		/* list of reminder attributes */
	Appt_Status	appt_status;
	Privacy_Level	privacy;
	struct Appt	*next;		/* next appointment */
};

struct Abb_Appt {
	struct Id	appt_id;
	struct Tag	*tag;
	Buffer		what;
	int		duration;
	struct Period	period;
	struct Abb_Appt	*next;
	Appt_Status	appt_status;
	Privacy_Level	privacy;
};

struct Apptid {
	struct Id	*oid;		/* old appt key */
	struct Appt	*new_appt;	/* new appt */
	Options		option;
};

struct Reminder {
	struct Id	appt_id;	/* actual appt. key */
	long		tick;		/* the happening tick */
	Attribute	attr;		/* (attr, value) */
	struct Reminder	*next;
};

enum Table_Res_Type {AP, RM, AB, ID};

union Table_Res_List switch (Table_Res_Type tag) { 
	case AP: 
		Appt *a; 
	case RM:
		Reminder *r;
	case AB:
		Abb_Appt *b;
	case ID:
		Uid *i;
	default: 
		void; 
};

enum Access_Status {
	access_ok,
	access_added,
	access_removed,
	access_failed,
	access_exists,
	access_partial,
	access_other,
	access_notable,
	access_notsupported,
	access_incomplete
};

struct Table_Res {
	Access_Status	status;
	Table_Res_List	res;
};

%#define access_none   0x0     /* owner only */
%#define access_read   0x1
%#define access_write  0x2
%#define access_delete 0x4
%#define access_exec   0x8     /* execution permission is a hack! */
%#define WORLD "world"	/* special user */

struct Access_Entry {
	Buffer who; 
	int access_type;	/* Bit mask from access_read,write,delete */
	Access_Entry *next;
};
	
struct Access_Args {
	Buffer target;
	Access_Entry *access_list;
};

struct Range {
	long key1;		/* lower bound tick */
	long key2;		/* upper bound tick */
	struct Range *next;
};

struct Keyrange {
	long key;		/* key of appt */
	long tick1;		/* lower bound tick */
	long tick2;		/* upper bound tick */
	struct Keyrange *next;
};

struct Uidopt {
	struct Id	appt_id;
	Options		option;
	struct Uidopt	*next;
};

enum Table_Args_Type {TICK_4, APPTID, UID, APPT, RANGE, KEYRANGE, UIDOPT};

union Args switch (Table_Args_Type tag) {
	case TICK_4:
		long		tick;
	case APPTID:
		Apptid		apptid;
	case UID:
		Uid		*key;
	case APPT:
		Appt		*appt;
	case RANGE:
		Range		*range;
	case KEYRANGE:
		Keyrange	*keyrange;
	case UIDOPT:
		Uidopt		*uidopt;
};

struct Table_Args {
	Buffer target;
	Args args;
	int pid;
};

struct Registration {
	Buffer	target;
        u_long  prognum;
        u_long  versnum;
        u_long  procnum;
	struct	Registration *next;
	int	pid;
};

struct Table_Op_Args {
	Buffer target;
	Buffer new_target;
};

enum Table_Status {ok, duplicate, badtable, notable, denied, other, tbl_not_owner, tbl_exist, tbl_notsupported};
enum Registration_Status {registered, failed, deregistered, confused, reg_notable};

%
%/*
% * rtable_delete and rtable_change take over the functionality of
% * rtable_delete_instance and rtable_change_instance repectively.
% * rtable_delete_instance and rtable_change_instance are now dummy
% * routines exist for backward compatibility purpose and return
% * access_notsupported.
% */
program TABLEPROG {
	version TABLEVERS {
		void rtable_ping(void)=0;
		Table_Res rtable_lookup(Table_Args) = 1;
		Table_Res rtable_lookup_next_larger(Table_Args) = 2;
		Table_Res rtable_lookup_next_smaller(Table_Args) = 3;
		Table_Res rtable_lookup_range(Table_Args) = 4;
		Table_Res rtable_abbreviated_lookup_range(Table_Args) = 5;
		Table_Res rtable_insert(Table_Args) = 6;
		Table_Res rtable_delete(Table_Args) = 7;
		Table_Res rtable_delete_instance(Table_Args) = 8;
		Table_Res rtable_change(Table_Args) = 9;
		Table_Res rtable_change_instance(Table_Args) = 10;
		Table_Res rtable_lookup_next_reminder(Table_Args) = 11;
		Table_Status rtable_check(Table_Args) = 12;
		Table_Status rtable_flush_table(Table_Args) = 13;
		int rtable_size(Table_Args) = 14;
		Registration_Status register_callback(Registration) = 15;
		Registration_Status deregister_callback(Registration) = 16;
		Access_Status rtable_set_access(Access_Args) = 17;
		Access_Args rtable_get_access(Access_Args) = 18;
		Table_Res rtable_abbreviated_lookup_key_range(Table_Args) = 19;
		long rtable_gmtoff(void) = 20;
		Table_Status rtable_create(Table_Op_Args) = 21;
		Table_Status rtable_remove(Table_Op_Args) = 22;
		Table_Status rtable_rename(Table_Op_Args) = 23;
	} = 4;
} = 100068; 

%
%extern Appt* make_appt();
 
%extern void destroy_appt();

%extern void destroy_list();

%extern Appt *copy_appt();

%extern Appt *copy_semiprivate_appt();

%extern Abb_Appt *make_abbrev_appt();

%extern void destroy_abbrev_appt();

%extern Abb_Appt *copy_abbrev_appt();

%extern Abb_Appt *appt_to_abbrev();

%extern Abb_Appt *appt_to_semiprivate_abbrev();

%extern Reminder* make_reminder();

%extern void destroy_reminder();

%extern Reminder* copy_reminder();
 
%extern Uid* make_keyentry();

%extern void destroy_keyentry();

%extern Uid* copy_keyentry();

%extern Access_Entry* make_access_entry();

%extern Access_Entry* copy_access_list();

%extern void destroy_access_list();

%extern Abb_Appt *copy_single_abbrev_appt();

%extern Attribute *make_attr();
