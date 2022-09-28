#ifndef lint
static  char sccsid[] = "@(#)convert2-4.c 1.3 92/10/21 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* convert2-4.c */

#include <string.h>
#include <rpc/rpc.h>
#include "util.h"
#include "rtable4.h"
#include "rtable2.h"


/**************** DATA TYPE (2->4) CONVERSION ROUTINES **************/
static Buffer
buffer2_to_buffer4(b)
        Buffer_2 b;
{
	Buffer copy;
	if (b!=NULL)
		copy = cm_strdup(b);
	else
		copy = ckalloc(1);
        return(copy);
}

static void
period2_to_period4(p2, p4)
        Period_2 p2; Period *p4;
{
	if (p4==NULL) return;
        switch (p2) {
        case single_2:
		p4->period = single;
		p4->nth = 0;
		break;
        case daily_2:
		p4->period = daily;
		p4->nth = 0;
		break;
        case weekly_2:
		p4->period = weekly;
		p4->nth = 0;
		break;
	case biweekly_2:
		p4->period = biweekly;
		p4->nth = 0;
		break;
	case monthly_2:
		p4->period = monthly;
		p4->nth = 0;
		break;
        case yearly_2:
		p4->period = yearly;
		p4->nth = 0;
		break;
        case nthWeekday_2:
		p4->period = nthWeekday;
		p4->nth = 0;
		break;
        case everyNthDay_2:
		p4->period = everyNthDay;
		p4->nth = 0;
		break;
        case everyNthWeek_2:
		p4->period = everyNthWeek;
		p4->nth = 0;
		break;
        case everyNthMonth_2:
		p4->period = everyNthMonth;
		p4->nth = 0;
		break;
        case otherPeriod_2:
		p4->period = otherPeriod;
		p4->nth = 0;
		break;
        default:
		p4->period = single;
		p4->nth = 0;
		break;
        }
}

static void
tag2_to_tag4(t2, t4)
	Tag_2 t2; Tag *t4;
{
	if (t4==NULL) return;
	switch(t2) {
	case appointment_2:
		t4->tag = appointment;
		t4->showtime = true;
		t4->next = NULL;
		break;
	case reminder_2:
		t4->tag = reminder;
		t4->showtime = false;
		t4->next = NULL;
		break;
	case otherTag_2:
		t4->tag = otherTag;
		t4->showtime = false;
		t4->next = NULL;
		break;
	default:
		t4->tag = appointment;	
		t4->showtime = true;
		t4->next = NULL;
		break;
	}
}

static Attribute *
attr2_to_attr4(a2)
        Attribute_2 *a2;
{
        Attribute *a4, *head, *prev;
 
	prev = head = NULL;
	while (a2 != NULL) {
		a4 = (Attribute *)ckalloc(sizeof(Attribute));
		a4->next = NULL;
		a4->attr = cm_strdup(a2->attr);
		a4->value = cm_strdup(a2->value);
		a4->clientdata = NULL;

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a2 = a2->next;
	}
	return(head);
}

static Except *
except2_to_except4(e2)
	Except_2 *e2;
{
	Except *e4, *head, *prev;

	prev = head = NULL;
	while (e2 != NULL) {
		e4  = (Except *)ckalloc(sizeof(Except));
		e4->ordinal = e2->ordinal;
		e4->next=NULL;

		if (head == NULL)
			head = e4;
		else
			prev->next = e4;
		prev = e4;

		e2 = e2->next;
	}
	return(head);
}

static void
id2_to_id4(from, to)
        Id_2 *from; Id *to;
{
	if ((from==NULL) || (to==NULL)) return;
        to->tick = from->tick;
        to->key = from->key;
}

static Uid *
uid2_to_uid4(ui2)
        Uid_2 *ui2;
{
        Uid *ui4, *head, *prev;
 
	prev = head = NULL;
	while (ui2 != NULL) {
        	ui4 = (Uid *)ckalloc(sizeof(Uid));
        	id2_to_id4(&(ui2->appt_id), &(ui4->appt_id));
        	ui4->next = NULL;

		if (head == NULL)
			head = ui4;
		else
			prev->next = ui4;
		prev = ui4;

		ui2 = ui2->next;
	}
	return(head);
}
 
static Appt *
appt2_to_appt4(a2)
        Appt_2 *a2;
{
        Appt *a4, *head, *prev;

	prev = head = NULL;
	while (a2 != NULL) {
		a4  = (Appt *)ckalloc(sizeof(Appt));
		a4->tag = (Tag *)ckalloc(sizeof(Tag));
		id2_to_id4(&(a2->appt_id), &(a4->appt_id));
		tag2_to_tag4(&(a2->tag), a4->tag);
		a4->duration = a2->duration;
		a4->ntimes = a2->ntimes;
		a4->what = buffer2_to_buffer4(a2->what);
		period2_to_period4(a2->period, &(a4->period));
		a4->author = buffer2_to_buffer4(a2->author);
		a4->client_data = buffer2_to_buffer4(a2->client_data);
		a4->attr = attr2_to_attr4(a2->attr);

		/* mailto is being removed from the appt struct proper,
		   and held instead as client data on the "ml" attribute */
		if (a2->mailto != NULL) {
			struct Attribute *item = a4->attr;
			while(item!=NULL) {
				if(strcmp(item->attr, "ml")==0) {
			  	item->clientdata=buffer2_to_buffer4(a2->mailto);
			  	break;
				}
				item=item->next;
			}
		}

		a4->exception = except2_to_except4(a2->exception);
		a4->appt_status = active;
		a4->privacy = public;
		a4->next = NULL;

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a2 = a2->next;
	}
	return(head);
}

static Abb_Appt *
abb2_to_abb4(a2)
        Abb_Appt_2 *a2;
{
        Abb_Appt *a4, *head, *prev;
 
	prev = head = NULL;
	while (a2 != NULL) {
		a4 = (Abb_Appt *)ckalloc(sizeof(Abb_Appt));
		a4->tag = (Tag *)ckalloc(sizeof(Tag));
		id2_to_id4(&(a2->appt_id), &(a4->appt_id));
		a4->tag->tag = appointment;
		a4->tag->showtime = true;
		a4->tag->next = NULL;
		a4->what = buffer2_to_buffer4(a2->what);
		a4->duration = a2->duration;
		period2_to_period4(a2->period, &(a4->period));
		a4->appt_status = active;
		a4->privacy = public;
		a4->next = NULL;

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a2 = a2->next;
	}
	return(head);
}

static void
apptid2_to_apptid4(from, to)
        Apptid_2 *from; Apptid *to;
{
        if (from==NULL || to==NULL) return;
        id2_to_id4(from->oid, to->oid);
        to->new_appt = appt2_to_appt4(from->new_appt);
	/* do_all is the default, the caller needs to
	 * modify it to the appropriate value
	 */
	to->option = do_all;
}

static Reminder *
reminder2_to_reminder4(r2)
        Reminder_2 *r2;
{
        Reminder *r4, *head, *prev;
	Attribute *attr4;

	prev = head = NULL;
	while (r2 != NULL) {
		r4 = (Reminder *)ckalloc(sizeof(Reminder));
		id2_to_id4(&(r2->appt_id), &(r4->appt_id));
		r4->tick = r2->tick;
		attr4 = attr2_to_attr4(&(r2->attr));
		r4->attr = *attr4;
		free(attr4);
		r4->next = NULL;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r2 = r2->next;
	}
	return(head);
}
      
static Table_Res_Type
tablerestype2_to_tablerestype4(t)
        Table_Res_Type_2 t;
{
        switch(t) {
        case AP_2:
                return(AP);
        case RM_2:
                return(RM);
        case AB_2:
                return(AB);
        case ID_2:
                return(ID);
        default:
                return(AP);
        }
}

static void
tablereslist2_to_tablereslist4(from, to)
        Table_Res_List_2 *from;
        Table_Res_List *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = tablerestype2_to_tablerestype4(from->tag);
        switch (from->tag) {
        case AP_2:
                to->Table_Res_List_u.a = appt2_to_appt4(
                        from->Table_Res_List_2_u.a);
                break;
        case RM_2:
                to->Table_Res_List_u.r = reminder2_to_reminder4(
                        from->Table_Res_List_2_u.r);
                break;
        case AB_2:
                to->Table_Res_List_u.b = abb2_to_abb4(
                        from->Table_Res_List_2_u.b);
                break;
        case ID_2:
                to->Table_Res_List_u.i = uid2_to_uid4(
                        from->Table_Res_List_2_u.i);
		break;
        default:
                return;
        }
}

extern Access_Status
accstat2_to_accstat4(s)
        Access_Status_2 s;
{
        switch (s) {
        case access_ok_2:
                return(access_ok);
        case access_added_2:
                return(access_added);
        case access_removed_2:
                return(access_removed);
        case access_failed_2:
                return(access_failed);
        case access_exists_2:
                return(access_exists);
        case access_partial_2:
                return(access_partial);
        case access_other_2:
        default:
                return(access_other);
        }
}

extern Table_Res *
tableres2_to_tableres4(r2)
        Table_Res_2 *r2;
{
        Table_Res *r4;
	
	if (r2==NULL) return((Table_Res *)NULL);
	r4 = (Table_Res *)ckalloc(sizeof(Table_Res));
        r4->status = accstat2_to_accstat4(r2->status);
        tablereslist2_to_tablereslist4(&(r2->res), &(r4->res));
        return(r4);
}

static Access_Entry *
acclist2_to_acclist4(l2)
	Access_Entry_2 *l2;
{
	Access_Entry *l4, *head, *prev;

	prev = head = NULL;
	while (l2 != NULL) {
		l4 = (Access_Entry *)ckalloc(sizeof(Access_Entry));
		l4->who = buffer2_to_buffer4(l2->who);
		l4->access_type = l2->access_type;
		l4->next = NULL;

		if (head == NULL)
			head = l4;
		else
			prev->next = l4;
		prev = l4;

		l2 = l2->next;
	}
	return(head);
}

extern Access_Args *
accargs2_to_accargs4(a2)
	Access_Args_2 *a2;
{
	Access_Args *a4;

	if (a2==NULL) return((Access_Args *)NULL);
	a4 = (Access_Args *)ckalloc(sizeof(Access_Args));
	a4->target = buffer2_to_buffer4(a2->target);
	a4->access_list = acclist2_to_acclist4(a2->access_list);
	return(a4);
}

static Range *
range2_to_range4(r2)
        Range_2 *r2;
{
        Range *r4, *head, *prev;
 
	prev = head = NULL;
	while (r2 != NULL) {
		r4 = (Range *)ckalloc(sizeof(Range));
		r4->key1 = r2->key1;
		r4->key2 = r2->key2;
		r4->next = NULL;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r2 = r2->next;
	}
	return(head);
}

static Table_Args_Type
argstag2_to_argstag4(t)
	Table_Args_Type_2 t;
{
	switch(t) {
	case TICK_2:
		return(TICK_4);
	case APPTID_2:
		return(APPTID);
	case UID_2:
		return(UID);
	case APPT_2:
		return(APPT);
	case RANGE_2:
		return(RANGE);
	default:
		return(TICK_4);
	}
}

static void
args2_to_args4(from, to)
        Args_2 *from; Args *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = argstag2_to_argstag4(from->tag);
        switch(from->tag) {
        case TICK_2:
                to->Args_u.tick = from->Args_2_u.tick;
                break;
        case APPTID_2:
		to->Args_u.apptid.oid = (Id *)ckalloc(sizeof(Id));
                apptid2_to_apptid4(
                        &(from->Args_2_u.apptid),
                        &(to->Args_u.apptid));
                break;
        case UID_2:
                to->Args_u.key = uid2_to_uid4(from->Args_2_u.key);
                break;
        case APPT_2:
                to->Args_u.appt = appt2_to_appt4(from->Args_2_u.appt);
                break;
        case RANGE_2:
                to->Args_u.range = range2_to_range4(from->Args_2_u.range);
                break;
        default:
                break;
        }
}

extern Table_Args *
tableargs2_to_tableargs4(a2)
        Table_Args_2 *a2;
{
        Table_Args *a4;

	if (a2==NULL) return((Table_Args *)NULL);
	a4 = (Table_Args *)ckalloc(sizeof(Table_Args));
        a4->target = buffer2_to_buffer4(a2->target);
        args2_to_args4(&(a2->args), &(a4->args));
	a4->pid = VOIDPID;
        return(a4);
}

static Uidopt *
uid2_to_uidopt(uid2, opt)
        Uid_2 *uid2; Options opt;
{
        Uidopt *uidopt, *head, *prev;
 
	prev = head = NULL;
	while (uid2 != NULL) {
        	uidopt = (Uidopt *)ckalloc(sizeof(Uidopt));
        	id2_to_id4(&(uid2->appt_id), &(uidopt->appt_id));
		uidopt->option = opt;
        	uidopt->next = NULL;

		if (head == NULL)
			head = uidopt;
		else
			prev->next = uidopt;
		prev = uidopt;

		uid2 = uid2->next;
	}
	return(head);
}
 
extern Table_Args *
tabledelargs2_to_tabledelargs4(a2, opt)
	Table_Args_2 *a2; Options opt;
{
	Table_Args *a4;

	if (a2 == NULL)
		return((Table_Args *)NULL);

	a4 = (Table_Args *)ckalloc(sizeof(Table_Args));
	a4->target = buffer2_to_buffer4(a2->target);
	a4->pid = VOIDPID;
	a4->args.tag = UIDOPT;
	a4->args.Args_u.uidopt = uid2_to_uidopt(a2->args.Args_2_u.key, opt);
	return(a4);
}

extern Registration_Status
regstat2_to_regstat4(s)
	Registration_Status_2 s;
{
	switch(s) {
	case registered_2:
		return(registered);
	case failed_2:
		return(failed);
	case deregistered_2:
		return(deregistered);
	case confused_2:
		return(confused);
	default:
		return(failed);
	}
}

extern Registration *
reg2_to_reg4(r2)
	Registration_2 *r2;
{
	Registration *r4, *head, *prev;

	prev = head = NULL;
	while (r2 != NULL) {
		r4 = (Registration *)ckalloc(sizeof(Registration));
		r4->target = buffer2_to_buffer4(r2->target);
		r4->prognum = r2->prognum;
		r4->versnum = r2->versnum;
		r4->procnum = r2->procnum;
		r4->next = NULL;
		r4->pid = VOIDPID;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r2 = r2->next;
	}
	return(head);
}

extern Table_Status
tablestat2_to_tablestat4(s)
	Table_Status_2 s;
{
	switch(s) {
	case ok_2:
		return(ok);
	case duplicate_2:
		return(duplicate);
	case badtable_2:
		return(badtable);
	case notable_2:
		return(notable);
	case denied_2:
		return(denied);
	case other_2:
	default:
		return(other);
	}
}

