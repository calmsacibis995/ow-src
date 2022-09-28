#ifndef lint
static  char sccsid[] = "@(#)convert4-2.c 1.5 92/10/27 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* convert4-2.c */

#include <string.h>
#include <rpc/rpc.h>
#include "util.h"
#include "rtable4.h"
#include "rtable2.h"


/**************** DATA TYPE (4->2) CONVERSION ROUTINES **************/
static Buffer_2
buffer4_to_buffer2(b)
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
 * Worse yet it uses Period_2 to index into an array which
 * contains strings up to "yearly", any period types beyond that
 * would index beyond the array and cause the front end to dump core.
 */
static Period_2
period4_to_period2(p)
	Period *p;
{
	if (p==NULL) return(otherPeriod_2);
        switch (p->period) {
        case single:
                return(single_2);
        case daily:
                return(daily_2);
        case weekly:
                return(weekly_2);
	case biweekly:
		return(biweekly_2);
	case monthly:
		return(monthly_2);
        case yearly:
                return(yearly_2);
        default:
                return(single_2);
        }
}

static Tag_2
tag4_to_tag2(t)
	Tag *t;
{
	if (t==NULL) return(otherTag_2);
	switch(t->tag) {
	case appointment:
		return(appointment_2);
	case reminder:
		return(reminder_2);
	case otherTag:
	case holiday:
	case toDo:
	default:
		return(otherTag_2);
	}
}

static Attribute_2 *
attr4_to_attr2(a4)
        Attribute *a4;
{
        Attribute_2 *a2, *head, *prev;
 
	prev = head = NULL;
	while (a4 != NULL) {
		a2 = (Attribute_2 *)ckalloc(sizeof(Attribute_2));
		a2->next = NULL;
		a2->attr = cm_strdup(a4->attr);
		a2->value = cm_strdup(a4->value);

		if (head == NULL)
			head = a2;
		else
			prev->next = a2;
		prev = a2;

		a4 = a4->next;
	}
	return(head);
}

static Except_2 *
except4_to_except2(e4)
        Except *e4;
{
        Except_2 *e2, *head, *prev;

	prev = head = NULL;
	while (e4 != NULL) {
		e2 = (Except_2 *)ckalloc(sizeof(Except_2));
		e2->ordinal = e4->ordinal;
		e2->next=NULL;

		if (head == NULL)
			head = e2;
		else
			prev->next = e2;
		prev = e2;

		e4 = e4->next;
	}
	return(head);
}
 
extern void
id4_to_id2(from, to)
        Id *from; Id_2 *to;
{
        if ((from==NULL) || (to==NULL)) return;	
        to->tick = from->tick;
        to->key = from->key;
}

extern Uid_2 *
uid4_to_uid2(ui4)
        Uid *ui4;
{
        Uid_2 *ui2, *head, *prev;

	prev = head = NULL;
	while (ui4 != NULL) {
		ui2 = (Uid_2 *)ckalloc(sizeof(Uid_2));
		id4_to_id2(&(ui4->appt_id), &(ui2->appt_id));
		ui2->next = NULL;

		if (head == NULL)
			head = ui2;
		else
			prev->next = ui2;
		prev = ui2;

		ui4 = ui4->next;
	}
	return(head);
}

extern Appt_2 *
appt4_to_appt2(a4)
        Appt *a4;
{
        Appt_2 *a2, *head, *prev;
	struct Attribute *item;

	prev = head = NULL;
	while (a4 != NULL) {
		a2  = (Appt_2 *)ckalloc(sizeof(Appt_2));
		id4_to_id2(&(a4->appt_id), &(a2->appt_id));
		a2->tag = tag4_to_tag2(a4->tag);
		a2->duration = a4->duration;
		a2->ntimes = a4->ntimes;
		a2->what = buffer4_to_buffer2(a4->what);
		a2->script = "";
		a2->period = period4_to_period2(&(a4->period));
		a2->author = buffer4_to_buffer2(a4->author);
		a2->client_data = buffer4_to_buffer2(a4->client_data);
		a2->attr = attr4_to_attr2(a4->attr);

		/* Pick the mailto field out of the attribute list
		   client data field; put it back into the appt struct
		   proper.
		*/
		item = a4->attr;
		while(item!=NULL) {
			if (strcmp(item->attr, "ml")==0) {
				a2->mailto=buffer4_to_buffer2(item->clientdata);
				break;
			}
			item=item->next;
		}

		a2->exception = except4_to_except2(a4->exception);
		a2->next = NULL;

		if (head == NULL)
			head = a2;
		else
			prev->next = a2;
		prev = a2;

		a4 = a4->next;
	}
	return(head);
}

static Abb_Appt_2 *
abb4_to_abb2(a4)
        Abb_Appt *a4;
{
        Abb_Appt_2 *a2, *head, *prev;

	prev = head = NULL;
	while (a4 != NULL) {
		a2 = (Abb_Appt_2 *)ckalloc(sizeof(Abb_Appt_2));
		id4_to_id2(&(a4->appt_id), &(a2->appt_id));
		a2->what = buffer4_to_buffer2(a4->what);
		a2->duration = a4->duration;
		a2->period = period4_to_period2(&(a4->period));
		a2->next = NULL;

		if (head == NULL)
			head = a2;
		else
			prev->next = a2;
		prev = a2;

		a4 = a4->next;
	}
	return(head);
}

static void
apptid4_to_apptid2(from, to)
        Apptid *from; Apptid_2 *to;
{
        if (from==NULL || to==NULL) return;
        id4_to_id2(from->oid, to->oid);
        to->new_appt = appt4_to_appt2(from->new_appt);
}

static Reminder_2 *
reminder4_to_reminder2(r4)
        Reminder *r4;
{
        Reminder_2 *r2, *head, *prev;
	Attribute_2 *attr2;

	prev = head = NULL;
	while (r4 != NULL) {
		r2 = (Reminder_2 *)ckalloc(sizeof(Reminder_2));
		id4_to_id2(&(r4->appt_id), &(r2->appt_id));
		r2->tick = r4->tick;
		attr2 = attr4_to_attr2(&(r4->attr));
		r2->attr = *attr2;
		free(attr2);
		r2->next = NULL;

		if (head == NULL)
			head = r2;
		else
			prev->next = r2;
		prev = r2;

		r4 = r4->next;
	}
	return(head);
}

static Table_Res_Type_2
tablerestype4_to_tablerestype2(t)
	Table_Res_Type t;
{
	switch(t) {
	case AP:
		return(AP_2);
	case RM:
		return(RM_2);
	case AB:
		return(AB_2);
	case ID:
		return(ID_2);
	default:
		return(AP_2);
	}
}

static void
tablereslist4_to_tablereslist2(from, to)
        Table_Res_List *from;
        Table_Res_List_2 *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = tablerestype4_to_tablerestype2(from->tag);
        switch (from->tag) {
        case AP:
                to->Table_Res_List_2_u.a = appt4_to_appt2(
                        from->Table_Res_List_u.a);
                break;
        case RM:
                to->Table_Res_List_2_u.r = reminder4_to_reminder2(
                        from->Table_Res_List_u.r);
                break;
        case AB:
                to->Table_Res_List_2_u.b = abb4_to_abb2(
                        from->Table_Res_List_u.b);
                break;
        case ID:
                to->Table_Res_List_2_u.i = uid4_to_uid2(
                        from->Table_Res_List_u.i);
        default:
                return;
        }
}
 
extern Access_Status_2
accstat4_to_accstat2(s)
	Access_Status s;
{
	switch(s) {
	case access_ok:
		return(access_ok_2);
	case access_added:
		return(access_added_2);
	case access_removed:
		return(access_removed_2);
	case access_failed:
		return(access_failed_2);
	case access_exists:
		return(access_exists_2);
	case access_partial:
		return(access_partial_2);
	case access_other:
	default:
		return(access_other_2);
	}
}

extern Table_Res_2 *
tableres4_to_tableres2(r4)
        Table_Res *r4;
{
        Table_Res_2 *r2;

	if (r4==NULL) return((Table_Res_2 *)NULL);
	r2 = (Table_Res_2 *)ckalloc(sizeof(Table_Res_2));
        r2->status = accstat4_to_accstat2(r4->status);
        tablereslist4_to_tablereslist2(&(r4->res), &(r2->res));
        return(r2);
}

extern Access_Entry_2 *
acclist4_to_acclist2(l4)
        Access_Entry *l4;
{
        Access_Entry_2 *l2, *head, *prev;

	prev = head = NULL;
	while (l4 != NULL) {
		l2 = (Access_Entry_2 *)ckalloc(sizeof(Access_Entry_2));
		l2->who = buffer4_to_buffer2(l4->who);
		l2->access_type = l4->access_type;
		l2->next = NULL;

		if (head == NULL)
			head = l2;
		else
			prev->next = l2;
		prev = l2;

		l4 = l4->next;
	}
	return(head);
}
 
extern Access_Args_2 *
accargs4_to_accargs2(a4)
        Access_Args *a4;
{
        Access_Args_2 *a2;

	if (a4==NULL) return((Access_Args_2 *)NULL);
	a2 = (Access_Args_2 *)ckalloc(sizeof(Access_Args_2));
        a2->target = buffer4_to_buffer2(a4->target);
        a2->access_list = acclist4_to_acclist2(a4->access_list);
        return(a2);
}

extern Range_2 *
range4_to_range2(r4)
	Range *r4;
{
	Range_2 *r2, *head, *prev;

	prev = head = NULL;
	while (r4 != NULL) {
		r2 = (Range_2 *)ckalloc(sizeof(Range_2));
		r2->key1 = r4->key1;
		r2->key2 = r4->key2;
		r2->next = NULL;

		if (head == NULL)
			head = r2;
		else
			prev->next = r2;
		prev = r2;

		r4 = r4->next;
	}
	return(head);
}

static Table_Args_Type_2
argstag4_to_argstag2(t)
        Table_Args_Type t;
{
        switch(t) {
        case TICK_4:
                return(TICK_2);
        case APPTID:
                return(APPTID_2);
        case UID:
                return(UID_2);
        case APPT:
                return(APPT_2);
        case RANGE:
                return(RANGE_2);
        default:
                return(TICK_2);
        }
}

static void
args4_to_args2(from, to)
	Args *from; Args_2 *to;
{
	if (from==NULL || to==NULL) return;
	to->tag = argstag4_to_argstag2(from->tag);
	switch(from->tag) {
	case TICK_4:
		to->Args_2_u.tick = from->Args_u.tick;
		break;
	case APPTID:
		to->Args_2_u.apptid.oid = (Id_2 *)ckalloc(sizeof(Id_2));
		apptid4_to_apptid2(
			&(from->Args_u.apptid),
			&(to->Args_2_u.apptid));
		break;
	case UID:
		to->Args_2_u.key = uid4_to_uid2(from->Args_u.key);
		break;
	case APPT:
		to->Args_2_u.appt = appt4_to_appt2(from->Args_u.appt);
		break;
	case RANGE:
		to->Args_2_u.range = range4_to_range2(from->Args_u.range);
		break;
	default:
		break;
	}
}

static Table_Args_2 *
tableargs4_to_tableargs2(a4)
	Table_Args *a4;
{
	Table_Args_2 *a2;
	
	if (a4==NULL) return((Table_Args_2 *)NULL);
	a2 = (Table_Args_2 *)ckalloc(sizeof(Table_Args_2));
	a2->target = buffer4_to_buffer2(a4->target);
	args4_to_args2(&(a4->args), &(a2->args));
	return(a2);
}

extern Registration_Status_2 
regstat4_to_regstat2(s)
	Registration_Status s;
{
	switch (s) {
	case registered:
		return(registered_2);
	case failed:
		return(failed_2);
	case deregistered:
		return(deregistered_2);
	case confused:
		return(confused_2);
	case reg_notable:
	default:
		return(failed_2);
	}
}

static Registration_2 *
reg4_to_reg2(r4)
        Registration_2 *r4;
{
        Registration_2 *r2, *head, *prev;

	prev = head = NULL;
	while (r4 != NULL) {
		r2 = (Registration_2 *)ckalloc(sizeof(Registration_2));
		r2->target = buffer4_to_buffer2(r4->target);
		r2->prognum = r4->prognum;
		r2->versnum = r4->versnum;
		r2->procnum = r4->procnum;
		r2->next = NULL;

		if (head == NULL)
			head = r2;
		else
			prev->next = r2;
		prev = r2;

		r4 = r4->next;
	}
	return(head);
}

extern Table_Status_2
tablestat4_to_tablestat2(s)
	Table_Status s;
{
	switch(s) {
	case ok:
		return(ok_2);
	case duplicate:
		return(duplicate_2);
	case badtable:
		return(badtable_2);
	case notable:
		return(notable_2);
	case denied:
		return(denied_2);
	case other:
	default:
		return(other_2);
	}
}

extern Uid_2 *
uidopt4_to_uid2(uidopt)
        Uidopt *uidopt;
{
        Uid_2 *uid2, *head, *prev;
 
	prev = head = NULL;
	while (uidopt != NULL) {
        	uid2 = (Uid_2 *)ckalloc(sizeof(Uid_2));
        	id4_to_id2(&(uidopt->appt_id), &(uid2->appt_id));
        	uid2->next = NULL;

		if (head == NULL)
			head = uid2;
		else
			prev->next = uid2;
		prev = uid2;

		uidopt = uidopt->next;
	}
	return(head);
}
 
