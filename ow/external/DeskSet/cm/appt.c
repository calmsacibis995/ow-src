#ifndef lint
static  char sccsid[] = "@(#)appt.c 3.3 92/10/28 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*
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

#include <stdio.h>
#ifdef SVR4
#include <stdlib.h>
#endif "SVR4"
#include "appt.h"
#include <sys/param.h>
#include "util.h"
#include <rpc/rpc.h>

#define	FAUX_STRING	"Appointment"

int debug;

extern Appt*
make_appt ()
{
	Appt *a = (Appt *) ckalloc (sizeof(Appt));
	a->tag = (Tag *)ckalloc (sizeof(Tag));
	a->tag->tag = appointment;
	a->tag->next = NULL;
	a->tag->showtime = true;
	a->duration=3600;	/* 1 hr */
	a->period.period=single;
	a->appt_status=active;
	a->privacy=public;
	a->what=ckalloc(1);
	a->author=ckalloc(1);
	a->client_data=ckalloc(1);
	a->next = NULL;
	return(a);
}
/*	Copy an attribute list recursively */
Attribute *
copy_attr(l)
	Attribute *l;
{
	Attribute *c;
	extern Attribute *copy_attr();

	if (l==NULL) return(NULL);
	c = (Attribute *) ckalloc(sizeof(Attribute));
	c->attr = cm_strdup(l->attr);
	c->value = cm_strdup(l->value);
	c->clientdata = cm_strdup(l->clientdata);
	c->next = copy_attr(l->next);
	return(c);
}
Attribute *
make_attr()
{
	Attribute *a = (Attribute *)ckalloc(sizeof(Attribute));
	a->attr=ckalloc(1);
	a->value=ckalloc(1);
	a->clientdata=ckalloc(1);
	return(a);
}
Except *
copy_excpt(l)
	Except *l;
{
	Except *c;
	extern Except *copy_excpt();
	
	if (l==NULL) return(NULL);
	c = (Except *) ckalloc(sizeof(Except));
	c->ordinal = l->ordinal;
	c->next = copy_excpt(l->next);
	return(c);
}
Tag *
copy_tag(l)
	Tag *l;
{
	Tag *c;
	extern Tag *copy_tag();
	
	if (l==NULL) return(NULL);
	c = (Tag *) ckalloc(sizeof(Tag));
	c->tag = l->tag;
	c->showtime = l->showtime;
	c->next = copy_tag(l->next);
	return(c);
}
void
copy_id(old, new)
	Id *old; Id *new;
{
	if ((old==NULL) || (new==NULL)) return;
	new->tick = old->tick;
	new->key = old->key;
}
void
copy_period(old, new)
	Period *old; Period *new;
{
	if ((old==NULL) || (new==NULL)) return;
	new->period = old->period;
	new->nth = old->nth;
	new->enddate = old->enddate;
}
extern Appt *
copy_single_appt(original)
        Appt *original;
{
        Appt *copy=NULL;
        if (original!=NULL) {
		copy = (Appt *) ckalloc (sizeof(Appt));
		copy_id(&(original->appt_id), &(copy->appt_id));
		copy->tag = copy_tag(original->tag);
		copy->duration = original->duration;
		copy->ntimes = original->ntimes;
        	copy->what = cm_strdup(original->what);
		copy_period(&(original->period), &(copy->period));
        	copy->author = cm_strdup(original->author);
        	copy->client_data = cm_strdup(original->client_data);
		copy->exception = copy_excpt(original->exception);
		copy->attr = copy_attr(original->attr);
		copy->appt_status = original->appt_status;
		copy->privacy = original->privacy;
          	copy->next = NULL;
        }
        return(copy);
}
extern Appt *
copy_appt(original)
        Appt *original;
{
        Appt *copy=NULL;
        if (original!=NULL) {
		copy = copy_single_appt(original);
                copy->next = copy_appt(original->next);
        }
        return(copy);
}

/*
 * Copy one appointment.
 * The what field is set to "Appointment".
 */
extern Appt *
copy_semiprivate_appt(original)
	Appt *original;
{
	Appt *copy = NULL;
	if (original != NULL) {
		copy = copy_single_appt(original);
		if (copy->what != NULL)
			free(copy->what);
		copy->what = cm_strdup(FAUX_STRING);
		copy->next = NULL;
	}
	return(copy);
}

extern Abb_Appt *
make_abbrev_appt()
{
	Abb_Appt *a;
	a = (Abb_Appt *) ckalloc(sizeof(Abb_Appt));
	a->tag = (Tag *) ckalloc(sizeof(Tag));
	a->tag->tag = appointment;
	a->tag->showtime =  true;
	a->tag->next = NULL;
	a->what = ckalloc(1);
	a->next = NULL;
	return(a);
}
void
destroy_abbrev_appt(a)
	Abb_Appt *a;
{
	extern void appt_free_tag();

	if (a==NULL) return;
	destroy_abbrev_appt(a->next);
	if (a->what != NULL) {
		free(a->what);
		a->what=NULL;
	}
	if (a->tag != NULL) {
		appt_free_tag(a->tag);
		a->tag = NULL;
	}
	free((char *)a);
	a = NULL;
}

extern Abb_Appt *
copy_single_abbrev_appt(original)
	Abb_Appt *original;
{
	Abb_Appt *copy=NULL;
        if (original!=NULL) {
   		copy = (Abb_Appt *)ckalloc(sizeof(Abb_Appt));
		copy_id(&(original->appt_id), &(copy->appt_id));
		copy->tag = copy_tag(original->tag);
          	copy->what = cm_strdup(original->what);
		copy->duration = original->duration;
		copy_period(&(original->period), &(copy->period));
		copy->appt_status = original->appt_status;
		copy->privacy = original->privacy;
          	copy->next = NULL;
        }
        return(copy);
}
extern Abb_Appt *
copy_abbrev_appt(original)
	Abb_Appt *original;
{
	Abb_Appt *copy=NULL;
	if (original!=NULL) {
		copy = copy_single_abbrev_appt(original);
		copy->next = copy_abbrev_appt (original->next);
	}
	return(copy);
}
extern Abb_Appt *
appt_to_abbrev(original)
	Appt *original;
{
	Abb_Appt *new=NULL;
	if (original!=NULL) {
		new = (Abb_Appt *) ckalloc(sizeof(Abb_Appt));
		new->tag = copy_tag(original->tag);
		copy_id(&(original->appt_id), &(new->appt_id));
		new->what = cm_strdup(original->what);
		new->duration = original->duration;
		copy_period(&(original->period), &(new->period));
		new->appt_status = original->appt_status;
		new->privacy = original->privacy;
		new->next=NULL;
	}
	return(new);
}

/*
 * The what field is set to "Appointment".
 */
extern Abb_Appt *
appt_to_semiprivate_abbrev(original)
	Appt *original;
{
	Abb_Appt *new = appt_to_abbrev(original);
	if (new != NULL) {
		if (new->what != NULL)
			free(new->what);
		new->what = cm_strdup(FAUX_STRING);
	}
	return(new);
}

void
destroy_attr(a)
	Attribute *a;
{
	if (a==NULL) return;
	if (a->attr != NULL) free(a->attr);
	if (a->value != NULL) free(a->value);
	if (a->clientdata != NULL) free(a->clientdata);
	free((char *)a);
}
void
appt_free_attr(l)
	Attribute *l;
{
	extern void appt_free_attr();

	if (l==NULL) return;
	appt_free_attr(l->next);
	destroy_attr(l);
}
void
appt_free_excpt(l)
	Except *l;
{
	extern void appt_free_excpt();
	
	if (l==NULL) return;
	appt_free_excpt(l->next);
	free(l);
}
void 
appt_free_tag(l)
	Tag *l;
{
	extern void appt_free_tag();
	
	if (l==NULL) return;
	appt_free_tag(l->next);
	free(l);
}
extern void
destroy_appt(a)
        Appt *a;
{
	extern void appt_free_tag();

     	if (a==NULL) return;
	destroy_appt (a->next);
     	if (a->what != NULL) {
		free(a->what);
		a->what=NULL;
	}
	if (a->author != NULL) {
		free(a->author);
		a->author=NULL;
	}
	if (a->tag != NULL) {
		appt_free_tag(a->tag);
		a->tag=NULL;
	}
	if (a->attr != NULL) {
		appt_free_attr(a->attr);
		a->attr=NULL;
	}
	if (a->exception != NULL) {
		appt_free_excpt(a->exception);
		a->exception=NULL;
	}
     	free((char *)a);
}
extern void
destroy_reminder(r)
	Reminder *r;
{
	if (r==NULL) return;
	destroy_reminder (r->next);
	if (r->attr.attr != NULL) free (r->attr.attr);
	if (r->attr.value != NULL) free (r->attr.value);
	free (r);
}
extern Reminder*
copy_reminder(original)
	Reminder *original;
{
	Reminder *copy;
	if (original==NULL) return(NULL);

	copy = (Reminder *) ckalloc(sizeof(Reminder));
	copy->appt_id.tick = original->appt_id.tick;
	copy->appt_id.key = original->appt_id.key;
	copy->tick = original->tick;
	copy->attr.attr = cm_strdup(original->attr.attr);
	copy->attr.value = cm_strdup(original->attr.value);
	copy->attr.clientdata = cm_strdup(original->attr.clientdata);
	copy->next = copy_reminder(original->next);

	return(copy);
}
extern Reminder*
make_reminder()
{
	Reminder *r = (Reminder *) ckalloc (sizeof(Reminder));
	return(r);
}
extern void
destroy_keyentry(k)
	Uid *k;
{
	if (k==NULL) return;
	destroy_keyentry(k->next);
	free((char *)k);
	k=NULL;
}
extern Uid*
make_keyentry()
{
	Uid *k = (Uid *) ckalloc (sizeof(Uid));
	return(k);
}
extern Uid*
copy_keyentry(k)
	Uid *k;
{
	Uid	*copy = NULL;

	if (k != NULL) {
		copy = make_keyentry();
		*copy = *k;
		copy->next = copy_keyentry (k->next);
	}
	return (copy);
}
extern Access_Entry *
make_access_entry(who, perms)
        char *who;
        int     perms;
{
        Access_Entry *e;

        if (who==NULL) return((Access_Entry *)NULL);
        e = (Access_Entry *) ckalloc(sizeof(Access_Entry));
        e->who = cm_strdup(who);
        e->access_type = perms;
        e->next = NULL;
        return(e);
}
extern Access_Entry *
copy_access_list(l)
        Access_Entry *l;
{
	Access_Entry *e;

	extern Access_Entry *copy_access_list();

	if (l==NULL) return(NULL);
	e = (Access_Entry *) ckalloc(sizeof(Access_Entry));
	e->who = cm_strdup(l->who);
	e->access_type = l->access_type;
	e->next = copy_access_list(l->next);
	return(e);
}
extern void
destroy_access_entry(e)
	Access_Entry *e;
{
	if (e==NULL) return;
	if (e->who != NULL) {
		free(e->who);
		e->who=NULL;
	}
	free((char *)e);
}
extern void
destroy_access_list(l)
	Access_Entry *l;
{
	extern void destroy_access_list();

	if (l==NULL) return;
	destroy_access_list(l->next);
	destroy_access_entry(l);
}
