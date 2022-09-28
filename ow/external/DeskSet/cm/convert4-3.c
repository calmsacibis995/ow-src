#ifndef lint
static  char sccsid[] = "@(#)convert4-3.c 1.5 92/10/27 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* convert4-3.c */

#include <string.h>
#include <rpc/rpc.h>
#include "util.h"
#include "rtable4.h"
#include "rtable3.h"


/**************** DATA TYPE (4->3) CONVERSION ROUTINES **************/
static Buffer_3
buffer4_to_buffer3(b)
	Buffer b;
{
	Buffer copy;
	if (b!=NULL)
		copy = cm_strdup(b);
	else
		copy = ckalloc(1);
	return(copy);
}

/*
 * Repeating event types beyond "yearly" are mapped to "single"
 * because the old front end does not recognize any other types
 * Worse yet it uses Interval to index into an array which
 * contains strings up to "yearly", any period types beyond that
 * would index beyond the array and cause the front end to dump core.
 */
static void
period4_to_period3(p4, p3)
	Period *p4; Period_3 *p3;
{
	if (p3 == NULL || p4 == NULL) return;

	if (p4->period > yearly) {
		p3->period = single;
		p3->nth = 0;
	} else {
		p3->period = p4->period;
		p3->nth = p4->nth;
	}
}

static Tag_3 *
tag4_to_tag3(t4)
	Tag *t4;
{
	Tag_3 *t3, *head, *prev;

	prev = head = NULL;
	while (t4 != NULL) {
		t3 = (Tag_3 *)ckalloc(sizeof(Tag_3));
		t3->tag = t4->tag;
		t3->showtime = t4->showtime;
		t3->next = NULL;

		if (head == NULL)
			head = t3;
		else
			prev->next = t3;
		prev = t3;

		t4 = t4->next;
	}
	return(head);
}

static Attribute_3 *
attr4_to_attr3(a4)
        Attribute *a4;
{
        Attribute_3 *a3, *head, *prev;
 
	prev = head = NULL;
	while (a4 != NULL) {
		a3 = (Attribute_3 *)ckalloc(sizeof(Attribute_3));
		a3->next = NULL;
		a3->attr = buffer4_to_buffer3(a4->attr);
		a3->value = buffer4_to_buffer3(a4->value);
		a3->clientdata = buffer4_to_buffer3(a4->clientdata);

		if (head == NULL)
			head = a3;
		else
			prev->next = a3;
		prev = a3;

		a4 = a4->next;
	}
	return(head);
}

static Except_3 *
except4_to_except3(e4)
        Except *e4;
{
        Except_3 *e3, *head, *prev;

	prev = head = NULL;
	while (e4 != NULL) {
		e3 = (Except_3 *)ckalloc(sizeof(Except_3));
		e3->ordinal = e4->ordinal;
		e3->next=NULL;

		if (head == NULL)
			head = e3;
		else
			prev->next = e3;
		prev = e3;

		e4 = e4->next;
	}
	return(head);
}
 
extern void
id4_to_id3(from, to)
        Id *from; Id_3 *to;
{
        if ((from==NULL) || (to==NULL)) return;	
        to->tick = from->tick;
        to->key = from->key;
}

extern Uid_3 *
uid4_to_uid3(ui4)
        Uid *ui4;
{
        Uid_3 *ui3, *head, *prev;

	prev = head = NULL;
	while (ui4 != NULL) {
		ui3 = (Uid_3 *)ckalloc(sizeof(Uid_3));
		id4_to_id3(&(ui4->appt_id), &(ui3->appt_id));
		ui3->next = NULL;

		if (head == NULL)
			head = ui3;
		else
			prev->next = ui3;
		prev = ui3;

		ui4 = ui4->next;
	}
	return(head);
}

extern Appt_3 *
appt4_to_appt3(a4)
        Appt *a4;
{
        Appt_3 *a3, *head, *prev;

	prev = head = NULL;
	while (a4 != NULL) {
		a3  = (Appt_3 *)ckalloc(sizeof(Appt_3));
		id4_to_id3(&(a4->appt_id), &(a3->appt_id));
		a3->tag = tag4_to_tag3(a4->tag);
		a3->duration = a4->duration;
		a3->ntimes = a4->ntimes;
		a3->what = buffer4_to_buffer3(a4->what);
		period4_to_period3(&(a4->period), &(a3->period));
		a3->author = buffer4_to_buffer3(a4->author);
		a3->client_data = buffer4_to_buffer3(a4->client_data);
		a3->exception = except4_to_except3(a4->exception);
		a3->attr = attr4_to_attr3(a4->attr);
		a3->appt_status = a4->appt_status;
		a3->privacy = a4->privacy;
		a3->next = NULL;

		if (head == NULL)
			head = a3;
		else
			prev->next = a3;
		prev = a3;

		a4 = a4->next;
	}
	return(head);
}

static Abb_Appt_3 *
abb4_to_abb3(a4)
        Abb_Appt *a4;
{
        Abb_Appt_3 *a3, *head, *prev;

	prev = head = NULL;
	while (a4 != NULL) {
		a3 = (Abb_Appt_3 *)ckalloc(sizeof(Abb_Appt_3));
		id4_to_id3(&(a4->appt_id), &(a3->appt_id));
		a3->tag = tag4_to_tag3(a4->tag);
		a3->what = buffer4_to_buffer3(a4->what);
		a3->duration = a4->duration;
		period4_to_period3(&(a4->period), &(a3->period));
		a3->appt_status = a4->appt_status;
		a3->privacy = a4->privacy;
		a3->next = NULL;

		if (head == NULL)
			head = a3;
		else
			prev->next = a3;
		prev = a3;

		a4 = a4->next;
	}
	return(head);
}

static void
apptid4_to_apptid3(from, to)
        Apptid *from; Apptid_3 *to;
{
        if (from==NULL || to==NULL) return;
        id4_to_id3(from->oid, to->oid);
        to->new_appt = appt4_to_appt3(from->new_appt);
}

static Reminder_3 *
reminder4_to_reminder3(r4)
        Reminder *r4;
{
        Reminder_3 *r3, *head, *prev;
	Attribute_3 *attr3;

	prev = head = NULL;
	while (r4 != NULL) {
		r3 = (Reminder_3 *)ckalloc(sizeof(Reminder_3));
		id4_to_id3(&(r4->appt_id), &(r3->appt_id));
		r3->tick = r4->tick;
		attr3 = attr4_to_attr3(&(r4->attr));
		r3->attr = *attr3;
		free(attr3);
		r3->next = NULL;

		if (head == NULL)
			head = r3;
		else
			prev->next = r3;
		prev = r3;

		r4 = r4->next;
	}
	return(head);
}

static Table_Res_Type_3
tablerestype4_to_tablerestype3(t)
	Table_Res_Type t;
{
	switch(t) {
	case AP:
		return(AP_3);
	case RM:
		return(RM_3);
	case AB:
		return(AB_3);
	case ID:
		return(ID_3);
	default:
		return(AP_3);
	}
}

static void
tablereslist4_to_tablereslist3(from, to)
        Table_Res_List *from;
        Table_Res_List_3 *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = tablerestype4_to_tablerestype3(from->tag);
        switch (from->tag) {
        case AP:
                to->Table_Res_List_3_u.a = appt4_to_appt3(
                        from->Table_Res_List_u.a);
                break;
        case RM:
                to->Table_Res_List_3_u.r = reminder4_to_reminder3(
                        from->Table_Res_List_u.r);
                break;
        case AB:
                to->Table_Res_List_3_u.b = abb4_to_abb3(
                        from->Table_Res_List_u.b);
                break;
        case ID:
                to->Table_Res_List_3_u.i = uid4_to_uid3(
                        from->Table_Res_List_u.i);
        default:
                return;
        }
}
 
extern Access_Status_3
accstat4_to_accstat3(s)
	Access_Status s;
{
	switch(s) {
	case access_ok:
		return(access_ok_3);
	case access_added:
		return(access_added_3);
	case access_removed:
		return(access_removed_3);
	case access_failed:
		return(access_failed_3);
	case access_exists:
		return(access_exists_3);
	case access_partial:
		return(access_partial_3);
	case access_other:
	default:
		return(access_other_3);
	}
}

extern Table_Res_3 *
tableres4_to_tableres3(r4)
        Table_Res *r4;
{
        Table_Res_3 *r3;

	if (r4==NULL) return((Table_Res_3 *)NULL);
	r3 = (Table_Res_3 *)ckalloc(sizeof(Table_Res_3));
        r3->status = accstat4_to_accstat3(r4->status);
        tablereslist4_to_tablereslist3(&(r4->res), &(r3->res));
        return(r3);
}

extern Access_Entry_3 *
acclist4_to_acclist3(l4)
        Access_Entry *l4;
{
        Access_Entry_3 *l3, *head, *prev;

	prev = head = NULL;
	while (l4 != NULL) {
		l3 = (Access_Entry_3 *)ckalloc(sizeof(Access_Entry_3));
		l3->who = buffer4_to_buffer3(l4->who);
		l3->access_type = l4->access_type;
		l3->next = NULL;

		if (head == NULL)
			head = l3;
		else
			prev->next = l3;
		prev = l3;

		l4 = l4->next;
	}
	return(head);
}
 
extern Access_Args_3 *
accargs4_to_accargs3(a4)
        Access_Args *a4;
{
        Access_Args_3 *a3;

	if (a4==NULL) return((Access_Args_3 *)NULL);
	a3 = (Access_Args_3 *)ckalloc(sizeof(Access_Args_3));
        a3->target = buffer4_to_buffer3(a4->target);
        a3->access_list = acclist4_to_acclist3(a4->access_list);
        return(a3);
}

extern Range_3 *
range4_to_range3(r4)
	Range *r4;
{
	Range_3 *r3, *head, *prev;

	prev = head = NULL;
	while (r4 != NULL) {
		r3 = (Range_3 *)ckalloc(sizeof(Range_3));
		r3->key1 = r4->key1;
		r3->key2 = r4->key2;
		r3->next = NULL;

		if (head == NULL)
			head = r3;
		else
			prev->next = r3;
		prev = r3;

		r4 = r4->next;
	}
	return(head);
}

extern Keyrange_3 *
keyrange4_to_keyrange3(r4)
	Keyrange *r4;
{
        Keyrange_3 *r3, *head, *prev;
 
	prev = head = NULL;
	while (r4 != NULL) {
		r3 = (Keyrange_3 *)ckalloc(sizeof(Keyrange_3));
		r3->key = r4->key;
		r3->tick1 = r4->tick1;
		r3->tick2 = r4->tick2;
		r3->next = NULL;

		if (head == NULL)
			head = r3;
		else
			prev->next = r3;
		prev = r3;

		r4 = r4->next;
	}
	return(head);
}

static Table_Args_Type_3
argstag4_to_argstag3(t)
        Table_Args_Type t;
{
        switch(t) {
        case TICK_4:
                return(TICK_3);
        case APPTID:
                return(APPTID_3);
        case UID:
                return(UID_3);
        case APPT:
                return(APPT_3);
        case RANGE:
                return(RANGE_3);
	case KEYRANGE:
		return(KEYRANGE_3);
        default:
                return(TICK_3);
        }
}

static void
args4_to_args3(from, to)
	Args *from; Args_3 *to;
{
	if (from==NULL || to==NULL) return;
	to->tag = argstag4_to_argstag3(from->tag);
	switch(from->tag) {
	case TICK_4:
		to->Args_3_u.tick = from->Args_u.tick;
		break;
	case APPTID:
		to->Args_3_u.apptid.oid = (Id_3 *)ckalloc(sizeof(Id_3));
		apptid4_to_apptid3(
			&(from->Args_u.apptid),
			&(to->Args_3_u.apptid));
		break;
	case UID:
		to->Args_3_u.key = uid4_to_uid3(from->Args_u.key);
		break;
	case APPT:
		to->Args_3_u.appt = appt4_to_appt3(from->Args_u.appt);
		break;
	case RANGE:
		to->Args_3_u.range = range4_to_range3(from->Args_u.range);
		break;
	case KEYRANGE:
		to->Args_3_u.keyrange = keyrange4_to_keyrange3(
			from->Args_u.keyrange);
	default:
		break;
	}
}

static Table_Args_3 *
tableargs4_to_tableargs3(a4)
	Table_Args *a4;
{
	Table_Args_3 *a3;
	
	if (a4==NULL) return((Table_Args_3 *)NULL);
	a3 = (Table_Args_3 *)ckalloc(sizeof(Table_Args_3));
	a3->target = buffer4_to_buffer3(a4->target);
	args4_to_args3(&(a4->args), &(a3->args));
	a3->pid = a4->pid;
	return(a3);
}

extern Registration_Status_3 
regstat4_to_regstat3(s)
	Registration_Status s;
{
	switch (s) {
	case registered:
		return(registered_3);
	case failed:
		return(failed_3);
	case deregistered:
		return(deregistered_3);
	case confused:
		return(confused_3);
	case reg_notable:
	default:
		return(failed_3);
	}
}

static Registration_3 *
reg4_to_reg3(r4)
        Registration_3 *r4;
{
        Registration_3 *r3, *head, *prev;

	prev = head = NULL;
	while (r4 != NULL) {
		r3 = (Registration_3 *)ckalloc(sizeof(Registration_3));
		r3->target = buffer4_to_buffer3(r4->target);
		r3->prognum = r4->prognum;
		r3->versnum = r4->versnum;
		r3->procnum = r4->procnum;
		r3->next = NULL;
		r3->pid = r4->pid;

		if (head == NULL)
			head = r3;
		else
			prev->next = r3;
		prev = r3;

		r4 = r4->next;
	}
	return(head);
}

extern Table_Status_3
tablestat4_to_tablestat3(s)
	Table_Status s;
{
	switch(s) {
	case ok:
		return(ok_3);
	case duplicate:
		return(duplicate_3);
	case badtable:
		return(badtable_3);
	case notable:
		return(notable_3);
	case denied:
		return(denied_3);
	case other:
	default:
		return(other_3);
	}
}

extern Uid_3 *
uidopt4_to_uid3(uidopt)
        Uidopt *uidopt;
{
        Uid_3 *uid3, *head, *prev;
 
	prev = head = NULL;
	while (uidopt != NULL) {
        	uid3 = (Uid_3 *)ckalloc(sizeof(Uid_3));
        	id4_to_id3(&(uidopt->appt_id), &(uid3->appt_id));
        	uid3->next = NULL;

		if (head == NULL)
			head = uid3;
		else
			prev->next = uid3;
		prev = uid3;

		uidopt = uidopt->next;
	}
	return(head);
}
 
