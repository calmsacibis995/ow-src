#ifndef lint
static  char sccsid[] = "@(#)convert3-4.c 1.3 92/10/21 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* convert3-4.c */

#include <string.h>
#include <rpc/rpc.h>
#include "util.h"
#include "rtable4.h"
#include "rtable3.h"


/**************** DATA TYPE (3->4) CONVERSION ROUTINES **************/
static Buffer
buffer3_to_buffer4(b)
        Buffer_3 b;
{
	Buffer copy;
	if (b!=NULL)
		copy = cm_strdup(b);
	else
		copy = ckalloc(1);
        return(copy);
}

static void
period3_to_period4(p3, p4)
        Period_3 *p3; Period *p4;
{
	if (p3 == NULL || p4==NULL) return;
	p4->period = p3->period;
	p4->nth = p3->nth;
}

static Tag *
tag3_to_tag4(t3)
	Tag_3 *t3;
{
	Tag *t4, *head, *prev;

	prev = head = NULL;
	while (t3 != NULL) {
		t4 = (Tag *)ckalloc(sizeof(Tag));
		t4->tag = t3->tag;
		t4->showtime = t3->showtime;
		t4->next = NULL;

		if (head == NULL)
			head = t4;
		else
			prev->next = t4;
		prev = t4;

		t3 = t3->next;
	}
	return(head);
}

static Attribute *
attr3_to_attr4(a3)
        Attribute_3 *a3;
{
        Attribute *a4, *head, *prev;
 
	prev = head = NULL;
	while (a3 != NULL) {
		a4 = (Attribute *)ckalloc(sizeof(Attribute));
		a4->next = NULL;
		a4->attr = buffer3_to_buffer4(a3->attr);
		a4->value = buffer3_to_buffer4(a3->value);
		a4->clientdata = buffer3_to_buffer4(a3->clientdata);

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a3 = a3->next;
	}
	return(head);
}

static Except *
except3_to_except4(e3)
	Except_3 *e3;
{
	Except *e4, *head, *prev;

	prev = head = NULL;
	while (e3 != NULL) {
		e4  = (Except *)ckalloc(sizeof(Except));
		e4->ordinal = e3->ordinal;
		e4->next=NULL;

		if (head == NULL)
			head = e4;
		else
			prev->next = e4;
		prev = e4;

		e3 = e3->next;
	}
	return(head);
}

static void
id3_to_id4(from, to)
        Id_3 *from; Id *to;
{
	if ((from==NULL) || (to==NULL)) return;
        to->tick = from->tick;
        to->key = from->key;
}

static Uid *
uid3_to_uid4(ui3)
        Uid_3 *ui3;
{
        Uid *ui4, *head, *prev;
 
	prev = head = NULL;
	while (ui3 != NULL) {
        	ui4 = (Uid *)ckalloc(sizeof(Uid));
        	id3_to_id4(&(ui3->appt_id), &(ui4->appt_id));
        	ui4->next = NULL;

		if (head == NULL)
			head = ui4;
		else
			prev->next = ui4;
		prev = ui4;

		ui3 = ui3->next;
	}
	return(head);
}
 
static Appt *
appt3_to_appt4(a3)
        Appt_3 *a3;
{
        Appt *a4, *head, *prev;

	prev = head = NULL;
	while (a3 != NULL) {
		a4  = (Appt *)ckalloc(sizeof(Appt));
		id3_to_id4(&(a3->appt_id), &(a4->appt_id));
		a4->tag = tag3_to_tag4(a3->tag);
		a4->duration = a3->duration;
		a4->ntimes = a3->ntimes;
		a4->what = buffer3_to_buffer4(a3->what);
		period3_to_period4(&(a3->period), &(a4->period));
		a4->author = buffer3_to_buffer4(a3->author);
		a4->client_data = buffer3_to_buffer4(a3->client_data);
		a4->exception = except3_to_except4(a3->exception);
		a4->attr = attr3_to_attr4(a3->attr);
		a4->appt_status = a3->appt_status;
		a4->privacy = a3->privacy;
		a4->next = NULL;

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a3 = a3->next;
	}
	return(head);
}

static Abb_Appt *
abb3_to_abb4(a3)
        Abb_Appt_3 *a3;
{
        Abb_Appt *a4, *head, *prev;
 
	prev = head = NULL;
	while (a3 != NULL) {
		a4 = (Abb_Appt *)ckalloc(sizeof(Abb_Appt));
		id3_to_id4(&(a3->appt_id), &(a4->appt_id));
		a4->tag = tag3_to_tag4(a3->tag);
		a4->what = buffer3_to_buffer4(a3->what);
		a4->duration = a3->duration;
		period3_to_period4(&(a3->period), &(a4->period));
		a4->appt_status = a3->appt_status;
		a4->privacy = a3->privacy;
		a4->next = NULL;

		if (head == NULL)
			head = a4;
		else
			prev->next = a4;
		prev = a4;

		a3 = a3->next;
	}
	return(head);
}

static void
apptid3_to_apptid4(from, to)
        Apptid_3 *from; Apptid *to;
{
        if (from==NULL || to==NULL) return;
        id3_to_id4(from->oid, to->oid);
        to->new_appt = appt3_to_appt4(from->new_appt);
	/* do_all is the default, the caller needs to
	 * modify it to the appropriate value
	 */
	to->option = do_all;
}

static Reminder *
reminder3_to_reminder4(r3)
        Reminder_3 *r3;
{
        Reminder *r4, *head, *prev;
	Attribute *attr4;

	prev = head = NULL;
	while (r3 != NULL) {
		r4 = (Reminder *)ckalloc(sizeof(Reminder));
		id3_to_id4(&(r3->appt_id), &(r4->appt_id));
		r4->tick = r3->tick;
		attr4 = attr3_to_attr4(&(r3->attr));
		r4->attr = *attr4;
		free(attr4);
		r4->next = NULL;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r3 = r3->next;
	}
	return(head);
}
      
static Table_Res_Type
tablerestype3_to_tablerestype4(t)
        Table_Res_Type_3 t;
{
        switch(t) {
        case AP_3:
                return(AP);
        case RM_3:
                return(RM);
        case AB_3:
                return(AB);
        case ID_3:
                return(ID);
        default:
                return(AP);
        }
}

static void
tablereslist3_to_tablereslist4(from, to)
        Table_Res_List_3 *from;
        Table_Res_List *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = tablerestype3_to_tablerestype4(from->tag);
        switch (from->tag) {
        case AP_3:
                to->Table_Res_List_u.a = appt3_to_appt4(
                        from->Table_Res_List_3_u.a);
                break;
        case RM_3:
                to->Table_Res_List_u.r = reminder3_to_reminder4(
                        from->Table_Res_List_3_u.r);
                break;
        case AB_3:
                to->Table_Res_List_u.b = abb3_to_abb4(
                        from->Table_Res_List_3_u.b);
                break;
        case ID_3:
                to->Table_Res_List_u.i = uid3_to_uid4(
                        from->Table_Res_List_3_u.i);
		break;
        default:
                return;
        }
}

extern Access_Status
accstat3_to_accstat4(s)
        Access_Status_3 s;
{
        switch (s) {
        case access_ok_3:
                return(access_ok);
        case access_added_3:
                return(access_added);
        case access_removed_3:
                return(access_removed);
        case access_failed_3:
                return(access_failed);
        case access_exists_3:
                return(access_exists);
        case access_partial_3:
                return(access_partial);
        case access_other_3:
        default:
                return(access_other);
        }
}

extern Table_Res *
tableres3_to_tableres4(r3)
        Table_Res_3 *r3;
{
        Table_Res *r4;
	
	if (r3==NULL) return((Table_Res *)NULL);
	r4 = (Table_Res *)ckalloc(sizeof(Table_Res));
        r4->status = accstat3_to_accstat4(r3->status);
        tablereslist3_to_tablereslist4(&(r3->res), &(r4->res));
        return(r4);
}

static Access_Entry *
acclist3_to_acclist4(l3)
	Access_Entry_3 *l3;
{
	Access_Entry *l4, *head, *prev;

	prev = head = NULL;
	while (l3 != NULL) {
		l4 = (Access_Entry *)ckalloc(sizeof(Access_Entry));
		l4->who = buffer3_to_buffer4(l3->who);
		l4->access_type = l3->access_type;
		l4->next = NULL;

		if (head == NULL)
			head = l4;
		else
			prev->next = l4;
		prev = l4;

		l3 = l3->next;
	}
	return(head);
}

extern Access_Args *
accargs3_to_accargs4(a3)
	Access_Args_3 *a3;
{
	Access_Args *a4;

	if (a3==NULL) return((Access_Args *)NULL);
	a4 = (Access_Args *)ckalloc(sizeof(Access_Args));
	a4->target = buffer3_to_buffer4(a3->target);
	a4->access_list = acclist3_to_acclist4(a3->access_list);
	return(a4);
}

static Range *
range3_to_range4(r3)
        Range_3 *r3;
{
        Range *r4, *head, *prev;
 
	prev = head = NULL;
	while (r3 != NULL) {
		r4 = (Range *)ckalloc(sizeof(Range));
		r4->key1 = r3->key1;
		r4->key2 = r3->key2;
		r4->next = NULL;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r3 = r3->next;
	}
	return(head);
}

static Keyrange *
keyrange3_to_keyrange4(r3)
	Keyrange_3 *r3;
{
        Keyrange *r4, *head, *prev;
 
	prev = head = NULL;
	while (r3 != NULL) {
		r4 = (Keyrange *)ckalloc(sizeof(Keyrange));
		r4->key = r3->key;
		r4->tick1 = r3->tick1;
		r4->tick2 = r3->tick2;
		r4->next = NULL;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r3 = r3->next;
	}
	return(head);
}

static Table_Args_Type
argstag3_to_argstag4(t)
	Table_Args_Type_3 t;
{
	switch(t) {
	case TICK_3:
		return(TICK_4);
	case APPTID_3:
		return(APPTID);
	case UID_3:
		return(UID);
	case APPT_3:
		return(APPT);
	case RANGE_3:
		return(RANGE);
	case KEYRANGE_3:
		return(KEYRANGE);
	default:
		return(TICK_4);
	}
}

static void
args3_to_args4(from, to)
        Args_3 *from; Args *to;
{
        if (from==NULL || to==NULL) return;
        to->tag = argstag3_to_argstag4(from->tag);
        switch(from->tag) {
        case TICK_3:
                to->Args_u.tick = from->Args_3_u.tick;
                break;
        case APPTID_3:
		to->Args_u.apptid.oid = (Id *)ckalloc(sizeof(Id));
                apptid3_to_apptid4(
			&(from->Args_3_u.apptid),
                        &(to->Args_u.apptid));
                break;
        case UID_3:
                to->Args_u.key = uid3_to_uid4(from->Args_3_u.key);
                break;
        case APPT_3:
                to->Args_u.appt = appt3_to_appt4(from->Args_3_u.appt);
                break;
        case RANGE_3:
                to->Args_u.range = range3_to_range4(from->Args_3_u.range);
                break;
	case KEYRANGE_3:
		to->Args_u.keyrange = keyrange3_to_keyrange4(
			from->Args_3_u.keyrange);
        default:
                break;
        }
}

extern Table_Args *
tableargs3_to_tableargs4(a3)
        Table_Args_3 *a3;
{
        Table_Args *a4;

	if (a3==NULL) return((Table_Args *)NULL);
	a4 = (Table_Args *)ckalloc(sizeof(Table_Args));
        a4->target = buffer3_to_buffer4(a3->target);
        args3_to_args4(&(a3->args), &(a4->args));
	a4->pid = a3->pid;
        return(a4);
}

static Uidopt *
uid3_to_uidopt(uid3, opt)
        Uid_3 *uid3; Options opt;
{
        Uidopt *uidopt, *head, *prev;
 
	prev = head = NULL;
	while (uid3 != NULL) {
        	uidopt = (Uidopt *)ckalloc(sizeof(Uidopt));
        	id3_to_id4(&(uid3->appt_id), &(uidopt->appt_id));
		uidopt->option = opt;
        	uidopt->next = NULL;

		if (head == NULL)
			head = uidopt;
		else
			prev->next = uidopt;
		prev = uidopt;

		uid3 = uid3->next;
	}
	return(head);
}
 
extern Table_Args *
tabledelargs3_to_tabledelargs4(a3, opt)
	Table_Args_3 *a3; Options opt;
{
	Table_Args *a4;

	if (a3 == NULL)
		return((Table_Args *)NULL);

	a4 = (Table_Args *)ckalloc(sizeof(Table_Args));
	a4->target = buffer3_to_buffer4(a3->target);
	a4->pid = a3->pid;
	a4->args.tag = UIDOPT;
	a4->args.Args_u.uidopt = uid3_to_uidopt(a3->args.Args_3_u.key, opt);
	return(a4);
}

extern Registration_Status
regstat3_to_regstat4(s)
	Registration_Status_3 s;
{
	switch(s) {
	case registered_3:
		return(registered);
	case failed_3:
		return(failed);
	case deregistered_3:
		return(deregistered);
	case confused_3:
		return(confused);
	default:
		return(failed);
	}
}

extern Registration *
reg3_to_reg4(r3)
	Registration_3 *r3;
{
	Registration *r4, *head, *prev;

	prev = head = NULL;
	while (r3 != NULL) {
		r4 = (Registration *)ckalloc(sizeof(Registration));
		r4->target = buffer3_to_buffer4(r3->target);
		r4->prognum = r3->prognum;
		r4->versnum = r3->versnum;
		r4->procnum = r3->procnum;
		r4->next = NULL;
		r4->pid = r3->pid;

		if (head == NULL)
			head = r4;
		else
			prev->next = r4;
		prev = r4;

		r3 = r3->next;
	}
	return(head);
}

extern Table_Status
tablestat3_to_tablestat4(s)
	Table_Status_3 s;
{
	switch(s) {
	case ok_3:
		return(ok);
	case duplicate_3:
		return(duplicate);
	case badtable_3:
		return(badtable);
	case notable_3:
		return(notable);
	case denied_3:
		return(denied);
	case other_3:
	default:
		return(other);
	}
}

