#ifndef lint
static  char sccsid[] = "@(#)rtable4.c 1.44 94/10/21 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1991 Sun Microsystems, Inc.
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

/*
 * veneer layered on top of the real data structures for abstraction.
 */

#define RPCGEN_ACTION(routine) routine

#include <stdio.h>
#include <errno.h>
#include <sys/types.h> /* for stat() */
#include <sys/stat.h>
#include "rtable4.h"
#include <sys/param.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <rpc/rpc.h>
#include <values.h>
#include <string.h>
#include <pwd.h>
#ifdef SVR4
#include <netdir.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif
#include "rpcextras.h"
#include "util.h"
#include "timeops.h"
#include "log.h"
#include "tree.h"
#include "list.h"
#include "iappt.h"		/* Internal appointment data structure */
#include "rtable4_tbl.i"

extern int nexttickcalled = 0;
extern int totalskipped = 0;
extern	int	debug, debug1;
extern	char	*pgname;

#define	APPT(p_node)		p_node->data
#define	NOT_FOUND		(Appt *) -1
#define	NUM_CACHE		50	/* cache for unix user name */
#define FAUX_STRING		"Appointment"

typedef enum {rt_ok, rt_access, rt_notable, rt_other} Rt_Status;

typedef	struct lists {
	char	*owner;
	char	*calendar;
	Info	info;			/* internal appt data structure */
	struct lists *next;
} Tree_info;

typedef struct uname_cache {
	int	uid;			/* unix user id */
	char	*name;			/* user name */
	struct	uname_cache *next;
} Uname_cache;

typedef struct wrapper {
	char	*source;
	struct	Registration r;
	time_t	check_time;
	struct	wrapper *next;
} Registration_Wrapper;

typedef struct reminder_q {
	int     remind_at;
	int     remind_ord;
	Attr    attr;
	Appt    *appt;
	struct reminder_q *next;
} Rm_que;

static	Tree_info	*tree_list = NULL;
static	Registration_Wrapper *registration_list=NULL;
static	Uname_cache	*ucache_list = NULL;
static	char		*c_time();

static void
get_startdate(appt)
	Appt *appt;
{
	struct tm *tm;

	switch (appt->period.period) {
	case monThruFri:
		tm = localtime(&(appt->appt_id.tick));
		if (tm->tm_wday < 1 || tm->tm_wday > 5)
			appt->appt_id.tick = next_tick(appt->appt_id.tick,
						appt->period);
		break;
	case monWedFri:
		tm = localtime(&(appt->appt_id.tick));
		if ((tm->tm_wday % 2) == 0)
			appt->appt_id.tick = next_tick(appt->appt_id.tick,
						appt->period);
		break;
	case tueThur:
		tm = localtime(&(appt->appt_id.tick));
		if (tm->tm_wday != 2 && tm->tm_wday != 4)
			appt->appt_id.tick = next_tick(appt->appt_id.tick,
						appt->period);
		break;
	}
}

/*
 * find the number of instances to be subtracted
 */
static int
get_ndelta(startdate, period, ntimes)
	long startdate; Period period; int ntimes;
{
	struct tm *tm;
	int ndelta = 0;
	long lastdate;
	double dlastdate;

	if (period.enddate == 0)
		return(ndelta);

	/* find last day of the series */
	dlastdate = startdate + (double)wksec * (ntimes - 1); /* last week */
	if (dlastdate > EOT)
		return(ndelta);
	else
		lastdate = (long)dlastdate;

	tm = localtime(&lastdate);
	if (period.period == monThruFri || period.period == monWedFri)
		lastdate = lastdate + daysec * (5 - tm->tm_wday);
	else if (period.period == tueThur)
		lastdate = lastdate + daysec * (4 - tm->tm_wday);
	else if (period.period == daysOfWeek)
		lastdate = lastdate + daysec *
			(lastapptofweek((u_int)period.nth) - tm->tm_wday);

	if (period.enddate > lastdate)
		return(ndelta);

	tm = localtime(&period.enddate);
	switch (period.period) {
	case monThruFri:
		ndelta = 5 - tm->tm_wday;
		break;
	case monWedFri:
		if (tm->tm_wday < 3)
			ndelta = 2;
		else if (tm->tm_wday < 5)
			ndelta = 1;
		break;
	case tueThur:
		if (tm->tm_wday < 2)
			ndelta = 2;
		else if (tm->tm_wday < 4)
			ndelta = 1;
		break;
	case daysOfWeek:
		ndelta = ntimes_this_week((u_int)period.nth, tm->tm_wday) - 1;
		break;
	}
	return(ndelta);
}

/*
 * Calculate the actual number of instances of the repeating event.
 */
static int
get_ninstance(appt)
	Appt *appt;
{
	struct tm *tm;
	int i, pdelta, ndelta, ninstance, timesperweek;
	double dninstance;

	switch (appt->period.period) {
	case everyNthDay:
	case everyNthWeek:
	case everyNthMonth:
		ninstance = (appt->ntimes+(appt->period.nth-1))/appt->period.nth;
		break;

	case monThruFri:
		tm = localtime(&(appt->appt_id.tick));
		pdelta = 6 - tm->tm_wday;
		ndelta = get_ndelta(appt->appt_id.tick, appt->period,
				appt->ntimes);
		dninstance = (appt->ntimes - 1) * (double)5 + pdelta - ndelta;
		ninstance = (dninstance > CMFOREVER) ? CMFOREVER : (int)dninstance;
		break;

	case monWedFri:
		tm = localtime(&(appt->appt_id.tick));
		pdelta = (7 - tm->tm_wday) / 2;
		ndelta = get_ndelta(appt->appt_id.tick, appt->period,
				appt->ntimes);
		dninstance = (appt->ntimes - 1) * (double)3 + pdelta - ndelta;
		ninstance = (dninstance > CMFOREVER) ? CMFOREVER : (int)dninstance;
		break;

	case tueThur:
		tm = localtime(&(appt->appt_id.tick));
		pdelta = (tm->tm_wday == 2) ? 2 : 1;
		ndelta = get_ndelta(appt->appt_id.tick, appt->period,
				appt->ntimes);
		dninstance = (appt->ntimes - 1) * (double)2 + pdelta - ndelta;
		ninstance = (dninstance > CMFOREVER) ? CMFOREVER : (int)dninstance;
		break;

	case daysOfWeek:
		tm = localtime(&(appt->appt_id.tick));
		timesperweek = ntimes_this_week((u_int)appt->period.nth, 0);
		pdelta = ntimes_this_week((u_int)appt->period.nth, tm->tm_wday);
		ndelta = get_ndelta(appt->appt_id.tick, appt->period,
				appt->ntimes);
		dninstance = (appt->ntimes-1) * (double)timesperweek +
				pdelta - ndelta;
		ninstance = (dninstance > CMFOREVER) ? CMFOREVER : (int)dninstance;

		break;
	default:
		ninstance = appt->ntimes;
	}

	return ninstance;
}

/*
 * calculate the ntimes value which, depending on the
 * repeating event type, may not be the same
 * as the actual number of instances
 */
static int
get_new_ntimes(period, tick, ninstance)
	Period period; Tick tick; int ninstance;
{
	struct tm *tm;
	int ntimes;
	int delta = 0, firstweek, timesperweek;

	switch (period.period) {
	case everyNthDay:
	case everyNthWeek:
	case everyNthMonth:
		ntimes = ninstance * period.nth;
		break;
	case monThruFri:
		tm = localtime(&tick);
		if (ninstance % 5)
			delta = ((ninstance % 5) > (6 - tm->tm_wday)) ? 2 : 1;
		else if (tm->tm_wday != 1)
			delta = 1;
		ntimes = (ninstance/5) + delta;
		break;
	case monWedFri:
		tm = localtime(&tick);
		if (ninstance % 3)
			delta = ((ninstance % 3) > ((7-tm->tm_wday)/2)) ? 2:1;
		else if (tm->tm_wday != 1)
			delta = 1;
		ntimes = (ninstance/3) + delta;
		break;
	case tueThur:
		tm = localtime(&tick);
		if (ninstance % 2 || tm->tm_wday != 2)
			delta = 1;
		ntimes = (ninstance/2) + delta;
		break;
	case daysOfWeek:
		tm = localtime(&tick);
		timesperweek = ntimes_this_week((u_int)period.nth, 0);
		firstweek=ntimes_this_week((u_int)period.nth,tm->tm_wday);
		if (ninstance % timesperweek)
			delta = ((ninstance % timesperweek) > firstweek) ? 2:1;
		else if (firstweek != timesperweek)
			delta = 1;
		ntimes = (ninstance/timesperweek) + delta;
		break;
	default:
		ntimes = ninstance;
		break;
	}

	return ntimes;
}

static Registration_Status
rtstat2regstat(rtstat)
Rt_Status rtstat;
{
	switch (rtstat) {
	case rt_ok:
		return(registered);
	case rt_access:
		return(failed);
	case rt_notable:
		return(reg_notable);
	case rt_other:
	default:
		return(failed);
	}
}

static Access_Status
rtstat2accessstat(rtstat)
Rt_Status rtstat;
{
	switch (rtstat) {
	case rt_ok:
		return(access_ok);
	case rt_access:
		return(access_failed);
	case rt_notable:
		return(access_notable);
	case rt_other:
	default:
		return(access_other);
	}
}

static Table_Status
rtstat2tablestat(rtstat)
Rt_Status rtstat;
{
	switch (rtstat) {
	case rt_ok:
		return(ok);
	case rt_access:
		return(denied);
	case rt_notable:
		return(notable);
	case rt_other:
	default:
		return(other);
	}
}

/* return user.localdomain, client should free the result when done */
static char *
get_local_user(uid)
int uid;
{
	char buf[BUFSIZ];
	struct passwd *pw;
	int n;

	if ((pw = getpwuid(uid)) == NULL)
		sprintf(buf, "%d", uid);
	else
		strcpy(buf, pw->pw_name);

	strcat(buf, ".");
	strcat(buf, cm_get_local_domain());

	return(cm_strdup(buf));
}

/* good format assumed: user = user@host.domain */
static char *
getuseratdomain(user)
char *user;
{
        char *target;
        char *ptr1, *ptr2;

        if (user == NULL)
                return NULL;
        target = ckalloc(cm_strlen(user));
        ptr1 = strchr(user, '@');
        ptr2 = strchr(user, '.');
        *ptr1 = NULL;
        if (ptr2 != NULL)
                sprintf(target, "%s@%s", user, ++ptr2);
        else
                cm_strcpy(target, user);
        *ptr1 = '@';
        return(target);
}

/* user = login@localdomain */
static Rt_Status
get_owner(log, owner)
char *log;
char **owner;
{
	struct stat info;
	char buf[BUFSIZ];
	char *ptr;
	int n;

	*owner = NULL;
	if (stat(log, &info) == 0) {

		/* assume that user is in local domain */
		*owner = get_local_user(info.st_uid);

		/* change format from user.domain to user@domain */
		ptr = strchr(*owner, '.');
		if (ptr)
			*ptr = '@';

		/* get rid of trailing '.' */
		n = cm_strlen(*owner);
		if ((*owner)[n-1] == '.')
			(*owner)[n-1] = NULL;

		return(rt_ok);
	}
	if (errno == ENOENT)
		return(rt_notable);
	else
		return(rt_other);
}


static Uname_cache *
in_u_cache (uid)
register int uid;
{
#if NUM_CACHE > 1
	register int	cache = NUM_CACHE;
	register Uname_cache *p_prev;
	register Uname_cache *p_cache;

	p_prev = NULL;
	p_cache = ucache_list;
	while (p_cache != NULL)
	{
		if (p_cache->uid == uid)
			return (p_cache);
		if (--cache < 0)
		{
			/* Assume that the cache size is at least 1 */
			p_prev->next = p_cache->next;
			free (p_cache->name);
			free (p_cache);
			p_cache = p_prev->next;
		}
		else
		{
			p_prev = p_cache;
			p_cache = p_cache->next;
		}
	}
#else
	if (ucache_list != NULL)
	{
		free (ucache_list->netname);
		free (ucache_list->name);
		free (ucache_list);
		ucache_list = NULL;
	}
#endif NUM_CACHE
	return (NULL);
}

static char *
get_uname(uid)
int	uid;
{
	char	*name;
	struct passwd *pw;
	static char buff[16];
	Uname_cache *ucache, *prev;

	if ((ucache = in_u_cache (uid)) == NULL)
	{
		if ((pw = getpwuid (uid)) != NULL)
			name = pw->pw_name;
		else
		{
			/* Can't map uid to name.  Don't cache the uid. */
			sprintf (buff, "%d", uid);
			endpwent();
			return (buff);
		}
		ucache = (Uname_cache *) ckalloc(sizeof(Uname_cache));
		ucache->uid = uid;
		ucache->name = cm_strdup (name);
		ucache->next = ucache_list;
		ucache_list = ucache;
		endpwent();
	}

	return (ucache->name);
}

#ifdef SVR4

static void
hostlist2hostname(hostlist, hostname)
struct nd_hostservlist *hostlist;
char **hostname;
{
        *hostname = NULL;

        if (hostlist->h_cnt > 0) {
	        *hostname = ckalloc(cm_strlen(hostlist->h_hostservs->h_host)+1);
		if (*hostname)
		        strcpy(*hostname, hostlist->h_hostservs->h_host);
	}
}

#endif /* SVR4 */

static char *
auth_check (svcrq)
struct svc_req *svcrq;
{
	char *name;
	static char *uname;
	struct authunix_parms *unix_cred;
	char *hostname = NULL, *ptr;
#ifdef SVR4
	struct netconfig *nconf;
	struct nd_hostservlist *hostlist;
#else
	struct sockaddr_in *addr;
	struct hostent *hp;
#endif

	if (uname != NULL)
	{
		free (uname);
		uname = NULL;
	}

	switch (svcrq->rq_cred.oa_flavor) {
	case AUTH_UNIX:
		unix_cred = (struct authunix_parms *) svcrq->rq_clntcred;
		if (unix_cred == NULL)
			return (NULL);
		if ((name = get_uname (unix_cred->aup_uid)) == NULL)
			return (NULL);
		break;
	case AUTH_NULL:
	default:
		svcerr_weakauth(svcrq->rq_xprt);
		return (NULL);
	}

	/* get hostname */
#ifdef CM_FUTURE
#ifdef SVR4
	nconf = getnetconfigent(svcrq->rq_xprt->xp_netid);
	netdir_getbyaddr(nconf, &hostlist,
			svc_getrpccaller(svcrq->rq_xprt));
	hostlist2hostname(hostlist, &hostname);
	if (nconf)
		freenetconfigent(nconf);
#else
	addr = svc_getcaller(svcrq->rq_xprt);
	hp = gethostbyaddr((char *)&addr->sin_addr.s_addr,
			strlen((char *)&addr->sin_addr.s_addr), addr->sin_family);
	endhostent();
	if (hp)
		hostname = cm_strdup(hp->h_name);
#endif /* SVR4 */
#endif

	/*
	 * if hostname is from unix_cred, we won't know which
	 * domain it's from.
	 */
	if (hostname == NULL)
		hostname = cm_strdup(unix_cred->aup_machname);

	uname = ckalloc(cm_strlen(name)+cm_strlen(hostname)+2);
	sprintf (uname, "%s@%s", name, hostname);
	free(hostname);
	if (debug)
                fprintf(stderr, "rpc.cmsd: sender is %s\n",
                        ((uname == NULL) ? "NULL" : uname));	
	return (uname);
}

static Registration_Wrapper *
make_wrapper(r, src)
	Registration *r;
	char *src;
{
	Registration_Wrapper *w;
	if (r==NULL) return(NULL);

	w = (Registration_Wrapper *) ckalloc (sizeof(Registration_Wrapper));
	w->source	= cm_strdup (src);
	w->r.target	= cm_target2name(r->target);
	w->r.prognum	= r->prognum;
	w->r.versnum	= r->versnum;
	w->r.procnum	= r->procnum;
	w->r.pid	= r->pid;
	w->next		= NULL;
	return(w);
}

static void
destroy_wrapper(w)
	Registration_Wrapper *w;
{
	if (w==NULL) return;

	if (debug) {
		fprintf(stderr,
		"...dumping %s, registered on prognum %d, versnum %d, \n\tfor target %s. registered pid= %d.\n",

		w->source, w->r.prognum, w->r.versnum, w->r.target, w->r.pid);
	}
	if (w->source != NULL)
		free (w->source);
	if (w->r.target != NULL)
		free (w->r.target);
	free(w);
}

static void
show_access_list(l)
	Access_Entry *l;
{
	while (l!=NULL) {
		fprintf(stderr, "Access: %s(%c%c%c)\n", l->who,
			l->access_type & access_read   ? 'r' : '_',
			l->access_type & access_write  ? 'w' : '_',
			l->access_type & access_delete ? 'd' : '_');
		l = l->next;
	}
}

/*
 * good format of owner and user assumed:
 * owner: user@domain
 * user: user@host.domain
 */
static Boolean
file_owner(owner, user)
char *owner, *user;
{
	char *ptr1, *ptr2;

	if (debug)
                fprintf(stderr, "rpc.cmsd: %s, comparing '%s' and '%s'\n",
                        "check file owner",
                        ((owner == NULL) ? "NULL" : owner),
                        ((user == NULL) ? "NULL" : user));

	ptr1 = get_head(owner, '@');
	ptr2 = get_head(user, '@');
	if (strcmp(ptr1, ptr2)) {
		free(ptr1);
		free(ptr2);
		return(false);
	} else
		return(true);   /* compare user names only */

}

/*
 * Check if "world" or user is in the access list.
 */
static Boolean
has_permission(l, user)
	Access_Entry *l; char *user;
{
	if (l==NULL || user==NULL) return(false);

	while(l != NULL) {
		if (strcmp (l->who, WORLD) == 0)
			break;
		if (same_user(user, l->who))
			break;
		l = l->next;
	}
	if (l == NULL)
		return(false);
	return(true);
}
static Access_Entry * 
in_access_list(l, s)
        Access_Entry *l; char *s; 
{
        if (l==NULL || s==NULL) return(NULL); 
        while(l != NULL) { 
                /* only for combining lists, not for authentication */ 
                if (strcmp(l->who, s) == 0) 
                        break; 
                l = l->next; 
        } 
        return(l);
}

static Access_Entry *
combine_access_list(p_list,p_head,type,p_world)
Access_Entry	*p_list;
Access_Entry	*p_head;
int		type;
int		*p_world;
{
	Access_Entry	*a;
	Access_Entry	*h = p_head;

	while (p_list != NULL)
	{
		/* Delay to put the WORLD into the combined list because 
		 * in_access_list() may return wrong result.
		 */
		if (strcmp (p_list->who, WORLD) == 0)
			*p_world |= type;
		else
		{
			/* The user is not in the combined list, add to list. */
			if ((a = in_access_list (h, p_list->who)) == NULL)
			{
				a = make_access_entry (p_list->who, type);
				a->next = p_head;
				p_head = a;
			}
			a->access_type |= type;
		}
		p_list = p_list->next;
	}
	return (p_head);
}

static Access_Entry *
make_access_list (p_info)
Info	*p_info;
{
	int		world = access_none;
	Access_Entry	*a;
	Access_Entry	*l = NULL;

	l = combine_access_list(GET_R_ACCESS(p_info), l, access_read, &world);
	l = combine_access_list(GET_W_ACCESS(p_info), l, access_write, &world);
	l = combine_access_list(GET_D_ACCESS(p_info), l, access_delete, &world);
	l = combine_access_list(GET_X_ACCESS(p_info), l, access_exec, &world);

	/* WORLD exists in one of the lists, add her to the combined list. */
	if (world != access_none)
	{
		a = make_access_entry (WORLD, world);
		a->next = l;
		l = a;
	}
	return (l);
}

static Privacy_Level
check_privacy_level(p_src, p_appt)
char **p_src;
Appt *p_appt;
{
	if (*p_src == NULL)
		return(public);

	if (p_appt != NULL) {
		/*
		 * if p_src is the author of the appointment,
		 * it should see everything.
		 */
		if (same_user(*p_src, p_appt->author)) {
			*p_src = NULL;
			return(public);
		} else
			return(p_appt->privacy);
	} else
		return(private);
}

static Access_Status
source_access_control (access_type, p_info,p_src,p_appt)
int     access_type;
Info    *p_info;
char    *p_src;
Appt    *p_appt;
{
        Access_Entry    *access_list;

        switch (access_type)
        {
        case access_none:
                if ((p_appt != NULL) && (p_appt->privacy == private))
                        return (access_failed);
		return (access_ok);
        case access_read:
                access_list = GET_R_ACCESS(p_info);
                break;
        case access_write:
                access_list = GET_W_ACCESS(p_info);
                break;
        case access_delete:
                /* Is author? */
                if ((p_appt != NULL) && same_user(p_src, p_appt->author))
                        return (access_ok);
                access_list = GET_D_ACCESS(p_info);
                break;
        case access_exec:
                access_list = GET_X_ACCESS(p_info);
                break;
        default:
                return (access_failed);
        }
 
        if (source_has_access (access_list, p_src))
                return (access_ok);
 
        return (access_failed);
}

static Access_Status
access_control (access_type,p_info,p_src,p_appt)
int	access_type;
Info	*p_info;
char	*p_src;
Appt	*p_appt;
{
	Access_Entry	*access_list;

	/* Is owner? */
	if (p_src == NULL)
		return (access_ok);

	switch (access_type)
	{
	case access_none:
		if ((p_appt != NULL) && (p_appt->privacy == private))
			return (access_failed);
		else
			return (access_ok);
	case access_read:
		access_list = GET_R_ACCESS(p_info);
		break;
	case access_write:
		access_list = GET_W_ACCESS(p_info);
		break;
	case access_delete:
		/* Is author? */
		if ((p_appt != NULL) && same_user(p_src, p_appt->author))
			return (access_ok);
		access_list = GET_D_ACCESS(p_info);
		break;
	case access_exec:
		access_list = GET_X_ACCESS(p_info);
		break;
		break;
	default:
		return (access_failed);
	}

	/* In the permission list? */
	if (has_permission (access_list, p_src))
		return (access_ok);
	else
		return (access_failed);
}

static int
already_registered(r, src)
	Registration *r;
	char	*src;
{
	Registration_Wrapper *p;
	char *target;

	if ((target = cm_target2name(r->target)) == NULL)
		return(0);

	p = registration_list;
	while (p != NULL) {
		if ((strcmp(p->source, src)==0) &&
		    (strcmp(p->r.target, target) == 0) && 
		    (p->r.prognum==r->prognum)  &&
		    (p->r.versnum==r->versnum)  &&
		    (p->r.procnum==r->procnum)  && 
		    (p->r.pid = r->pid))
		{
			free(target);
			return(1);
		}
		p = p->next;
	}
	free(target);
	return(0);
}

static int
count_clients(target, show_ticketholder)
	char *target; Boolean show_ticketholder;
{
	/* this not only returns the number of registered
	   clients, but checks the integrity of the list
	   by looping through it.  */

	int n=0;
	Registration_Wrapper *l = registration_list;

	while(l != NULL) {
		if (strcmp(target, l->r.target)==0) {
			if (show_ticketholder)
			  fprintf(stderr, "\t%s registered on %s\n", l->source, target);
			n++;
		}
		l=l->next;
	}
	return(n);
}

/*
 * Go through the registration list and ping each client.
 * Deregister clients that do not respond.
 */
static void
cleanup_registration_list()
{
	int nclients=0, ndereg=0;
	char *sourcehost=NULL;
	register Registration_Wrapper *p_next;
	register Registration_Wrapper *p;
	register Registration_Wrapper *p_prev;
	struct timeval timeout_tv;
	CLIENT *cl;
	Boolean advance = true;

	timeout_tv.tv_sec = 10;
	timeout_tv.tv_usec = 0;

	/* loop through the registration list */

	p = p_prev = registration_list;
	while (p != NULL) {

		p_next = p->next;

		if (debug) {
			fprintf(stderr,
			  "%s: pinging %s on prog: %d, vers: %d, proc: %d\n",
			  pgname, p->source, p->r.prognum, p->r.versnum,
			  p->r.procnum);
		}

		sourcehost = cm_target2location(p->source);
		cl = clnt_create(sourcehost, p->r.prognum, p->r.versnum, "udp");

		if (cl != NULL) {
			clnt_control(cl, CLSET_TIMEOUT, (char *)&timeout_tv);
			timeout_tv.tv_sec = 5;
			clnt_control(cl, CLSET_RETRY_TIMEOUT, (char*)&timeout_tv);
		}
				
		/* no client or client not responding */
		if (cl == NULL || clnt_call(cl, 0, xdr_void, (char *)NULL,
			xdr_void, (char *)NULL, timeout_tv) != RPC_SUCCESS)
		{
			if (debug) {
				clnt_pcreateerror(sourcehost);
				fprintf(stderr, "%s: %s deregistered, pid %d\n",
					pgname, p->source, p->r.pid);
			}

			/* deregister client */
			destroy_wrapper(p);

			if (p == p_prev) { /* top of list */
				registration_list = p_next;
				p = p_prev = p_next;
				advance = false;
			} else {
				p_prev->next = p_next;
				p = p_prev;
			}

			ndereg++;
		}

		if (cl)
			clnt_destroy(cl);

		free(sourcehost);
		nclients++;

		if (advance) {
			p_prev = p;
                	p = p_next;
		} else
			advance = true;
        }

	if (debug) {
		fprintf(stderr, "%s: number of clients before cleanup = %d\n",
			pgname, nclients);
		fprintf(stderr, "%s: number of clients deregistered = %d\n",
			pgname, ndereg);
	}

}

static void
do_callbacks(source, a, target, pid)
	char *source;
	Appt *a;
	char *target;
{
	Uid *k, *ids = NULL;
	Table_Res res;
        int nclients=0, ncallbacks=0;
	char *sourcehost=NULL;
        register Registration_Wrapper *p_next;
        register Registration_Wrapper *p;
        register Registration_Wrapper *q;
	struct timeval timeout_tv;
	CLIENT *cl;
	Boolean advance = true;
        
	/* Callback with appointment ids only for security reason. */
	while (a != NULL)
	{
		k = make_keyentry ();
		k->appt_id = a->appt_id;
		k->next = ids;
		ids = k;
		a = a->next;
	}
	/* Might be the source of my grimy, sneaky little bug 
	   where xdr thinks it's serializing an appointment
	   when it's got a reminder in it's hand
	*/

	res.status = access_ok;
	res.res.tag = ID;
	res.res.Table_Res_List_u.i = ids;

	/* Set timeout to zero so that the call returns right away. */
	timeout_tv.tv_sec = 0;
	timeout_tv.tv_usec = 0;

	/*	loop through the registration list looking for parties
		interested in this transaction.		*/

	target = cm_target2name(target);
	p=q=registration_list;
        while (p != NULL) {

		/* The caller will get the results of the rpc call.
		 * If he's registered on the callback list, don't call him -
		 * UNLESS the process id of his client differs from the
		 * original ticket. However, if the pid is a VOIDPID (-1),
		 * a version 2 client has registered and there's no way
	         * of telling which instance of the client it is.  So,
		 * to be safe (avoid deadlock) we won't callback version
		 * 2 clients registered on version 3 daemons if their
		 * registration name entry matches the caller's. [Nanno]
		 */
		p_next = p->next;

		if ((strcmp (source, p->source) == 0) && ((pid==p->r.pid) ||
			(p->r.pid==-1))) {
			p = p_next;
			nclients++;
			continue;
		}

		if (strcmp(target, p->r.target) == 0) {
			sourcehost = cm_target2location(p->source);
			if (debug) {
				fprintf(stderr,
				  "%s: calling back %s on prog: %d, vers: %d, proc: %d\n",
				  pgname, p->source, p->r.prognum, p->r.versnum, p->r.procnum);
			}
			cl = clnt_create(sourcehost, p->r.prognum, p->r.versnum, "udp");

			/* deregister client if fails to create handle */
			if (cl == NULL) {
				if (debug) {
					clnt_pcreateerror(sourcehost);
				}

				/* deregister client */
				destroy_wrapper(p);

				if (p == q) { /* top of list */
					registration_list = p_next;
					p = q = p_next;
					advance = false;
				} else {
					q->next = p_next;
					p = q;
				}

				--ncallbacks;
				--nclients;
			} else {
				/* Set timeout to zero so that the call
				 * returns right away.
				 */
#ifndef SVR4
				/* for non-svr4 systems, clnt_call won't
				 * return right away unless timeout is set
				 * to zero using clnt_control(), (rpc bug?)
				 */
				clnt_control(cl, CLSET_TIMEOUT,
						(char *)&timeout_tv);
#endif
				(void)clnt_call(cl, p->r.procnum,
					xdr_Table_Res, (char *)&res,
					xdr_void, (char *)0, timeout_tv);
			}

			if (cl)
				clnt_destroy(cl);

			free(sourcehost);
			ncallbacks++;
		}
		nclients++;

		if (advance) {
			q = p;
                	p = p_next;
		} else
			advance = true;
        }

	free (target);
	if (ids != NULL)
		destroy_keyentry (ids);

	if (debug) {
		fprintf(stderr, "%s: number of registered clients = %d\n",
			pgname, nclients);
		fprintf(stderr, "%s: number of clients called back= %d\n",
			pgname, ncallbacks);
	}
}

/* Comparison function for tree with composite key */
static	Comparison
compare (key,data)
register Id	 *key;
register caddr_t data;
{

	/* Composite key */
	if (key->tick < CM_TICK(data))
		return (less);
	if (key->tick > CM_TICK(data))
		return (greater);

	/* For backward compatible with version 1. */
	if (key->key == 0)
		return (equal);

	/* tick's are equal */
	if (key->key < KEY(data))
		return (less);
	if (key->key > KEY(data))
		return (greater);

	return (equal);
}

static	Comparison
compare_rpt (key,data)
register Id	 *key;
register caddr_t data;
{
	/* For backward compatible with version 1. */
	if (key->key == 0)
	{
		if (key->tick < CM_TICK(data))
			return (less);
		if (key->tick > CM_TICK(data))
			return (greater);
		return (equal);
	}

	if (key->key < KEY(data))
		return (less);
	if (key->key > KEY(data))
		return (greater);
	return (equal);
}

static caddr_t
get_key (data)
caddr_t	data;
{
	return ((caddr_t) &(((Appt *) data)->appt_id));
}

/*
 * return status:
 * rt_ok - table loaded successfully
 * rt_notable - table does not exist
 * rt_other - miscellaneous problem
 */
extern Rt_Status
rtable_load(target, svcrq, infoptr)
char	*target;
struct svc_req *svcrq;
Info **infoptr;
{
	Rt_Status	rtstat;
	char		*name;
	char		*log;
	Tree_info	*p_info = NULL;
	int err;

	rtstat = rt_other;
	*infoptr = NULL;

	if ((name = cm_target2name (target)) == NULL)
		return(rtstat);

	if ((log = get_log(name)) == NULL)
	{
		free(name);
		return(rtstat);
	}

	p_info = (Tree_info *) ckalloc (sizeof(Tree_info));
	if (p_info == NULL)
		goto ERROR;
	if (!(p_info->calendar = cm_strdup (name)))
		goto ERROR;
	if (!(p_info->info.rb_tree = (caddr_t) rb_create(get_key, compare)))
		goto ERROR;
	if (!(p_info->info.hc_list = (caddr_t) hc_create(get_key, compare_rpt)))
		goto ERROR;
	p_info->info.rm_queue = NULL;
	SET_R_ACCESS(&(p_info->info), make_access_entry(WORLD, access_read));
	SET_W_ACCESS(&(p_info->info), NULL);
	SET_D_ACCESS(&(p_info->info), NULL);
	SET_X_ACCESS(&(p_info->info), NULL);
	p_info->next = tree_list;
	tree_list = p_info;

	/* start the transaction logging */
	if ((err = start_log(name, log)) ==(int)FILEERROR)
	{
		/* detach from the list */
		tree_list = p_info->next;
		rtstat = rt_notable;

		if (debug)
			fprintf(stderr, "%s: file %s does not exist\n",
				pgname, log);
		goto ERROR;
	}
	else if (err == (int)CERROR)
	{
		/* detach from the list */
		tree_list = p_info->next;

		fprintf(stderr, "%s: file error on %s\n", pgname, log);
		goto ERROR;
	}
	else if (err == (int)PWERROR) 
	{
		/* detach from the list */
		tree_list = p_info->next;

		fprintf(stderr, "%s: file error on %s\n", pgname, log);
		fprintf(stderr, "Cannot access passwd entry.\n");
		goto ERROR;
	}

	/*
	 * get file owner after the file is loaded since file
	 * ownership might be fixed in start_log()
	 */
	if ((rtstat = get_owner(log, &p_info->owner)) != rt_ok)
		goto ERROR;

	insert_holidays(target);

	p_info->info.modified = false;

	free(log);
	free(name);
	if (debug1)
	{
		fprintf(stderr, "%s: rbsize=%d\n", pgname,
			rb_size (APPT_TREE(&p_info->info)) +
			hc_size (REPT_LIST(&p_info->info)));
	}
	*infoptr = &p_info->info;
	endpwent();
	return (rt_ok);
ERROR:
	free(name);
	free(log);
	/* this almost can make up a "rtable_destroy" function */
	if (p_info != NULL)
	{
		if (p_info->owner)
			free (p_info->owner);
		if (p_info->info.rb_tree)
			rb_destroy (p_info->info.rb_tree, NULL);
		if (p_info->info.hc_list)
			hc_destroy (p_info->info.hc_list, NULL);
		destroy_access_list (GET_R_ACCESS ((&p_info->info)));
		destroy_access_list (GET_W_ACCESS ((&p_info->info)));
		destroy_access_list (GET_D_ACCESS ((&p_info->info)));
		destroy_access_list (GET_X_ACCESS ((&p_info->info)));
		free (p_info);
	}
	endpwent();
	return (rtstat);
}

extern char *
rtable_get_owner(target)
char *target;
{
	char *name;
	Tree_info *t = tree_list;
	if ((name = cm_target2name(target, '@')) == NULL)
		return(NULL);

	while (t != NULL) {
		if (strcmp(name, t->calendar) == 0) {
			free(name);
			return(t->owner);
		}
		t = t->next;
	}
	free(name);
	return(NULL);
}

extern Info *
rtable_get_table(target)
char	*target;
{
	register char	*name;
	register Tree_info *t = tree_list;

	if ((name = cm_target2name(target)) == NULL)
		return (NULL);

	while (t != NULL)
	{
		if (strcmp (name, t->calendar) == 0)
		{
			free (name);
			return (&t->info);
		}
		t = t->next;
	}
	free (name);
	return (NULL);
}

extern void
rtable_enumerate_up(p_info, doit)
Info	*p_info;
Enumerate_proc doit;
{
	if (p_info == NULL || doit == NULL)
		return;
	rb_enumerate_up (APPT_TREE(p_info), doit);
	hc_enumerate_up (REPT_LIST(p_info), doit);
}

extern void
rtable_enumerate_down(p_info, doit)
Info	*p_info;
Enumerate_proc doit;
{
	if (p_info == NULL || doit == NULL)
		return;
	rb_enumerate_down (APPT_TREE(p_info), doit);
	hc_enumerate_down (REPT_LIST(p_info), doit);
}

static Appt *
rtable_lookup_internal (p_info,p_src,key)
Info		*p_info;
char		**p_src;
register Id	*key;
{
	register Appt	*p_appt;
	Privacy_Level	p_level;

	/* Check if it hits a single appointment */
	p_appt = (Appt *) rb_lookup (APPT_TREE(p_info), (Key) key);
	if (p_appt != NULL)
	{
		switch (check_privacy_level(p_src, p_appt)) {
		case public:
			p_appt = copy_appt(p_appt);
			return(p_appt);
		case semiprivate:
			p_appt = copy_semiprivate_appt(p_appt);
			return(p_appt);
		case private:
		default:
			return(NULL);
		}
	}
	
	/* Check if it hits an event in any repeating appointment */
	p_appt = (Appt *) hc_lookup (REPT_LIST(p_info), (Key) key);
	if (p_appt != NULL)
	{
		if ((p_level = check_privacy_level(p_src, p_appt)) != private) {
			if (in_repeater (key, p_appt, FALSE)) {
				if (p_level == public)
					p_appt = copy_appt(p_appt);
				else
					p_appt = copy_semiprivate_appt(p_appt);
				CM_TICK(p_appt) = key->tick;
				return(p_appt);
			}
		}
	}

	return (NULL);
}

/*
 * If the requester is the owner, "*p_src" will be set to NULL.
 */
static	Rt_Status
check_read_access(args, svcrq, res, p_src, p_info)
Table_Args *args;
struct svc_req *svcrq;
Table_Res *res;
char **p_src;
Info **p_info;
{
	Rt_Status	rtstat;
	char		*target;

	*p_info = NULL;
	if ((*p_src = auth_check (svcrq)) == NULL)
		return (rt_other);

	if ((target = args->target) == NULL)
		return (rt_other);

	if ((*p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load (target, svcrq, p_info)) != rt_ok)
			return (rtstat);
	}

	/* Is owner or in read access list? */
	if (file_owner(rtable_get_owner(target), *p_src) &&
		source_access_control(access_read, *p_info, *p_src, NULL) == access_ok)
			*p_src = NULL;
	else if (access_control(access_read, *p_info, *p_src, NULL) != access_ok)
	{
		res->status = access_failed;
		return (rt_access);
	}

	res->status = access_ok;
	return (rt_ok);
}

Registration_Status *
register_callback_4(r, svcrq)
Registration *r;
struct svc_req *svcrq;
{
	static Registration_Status stat;
	int nclients = 0;
	char *source;
	Info *p_info = NULL;
        Registration_Wrapper *copy=NULL, *p=NULL;
	Rt_Status rtstat;

	if (debug)
		fprintf(stderr, "register_callback_4 called\n");

	stat = failed;

	if ((source = auth_check (svcrq)) == NULL)
		return(&stat);

	/* Error */
        if (r==NULL)
                return(&stat);

	/* Check for duplicate registrations */
	if (already_registered(r, source)) {
		stat = registered;
		return(&stat);
	}

	/* check if the target exists */
	if ((p_info = rtable_get_table (r->target)) == NULL)
	{
		if ((rtstat = rtable_load (r->target, svcrq, &p_info)) != rt_ok)
		{
			stat = rtstat2regstat(rtstat);
			return (&stat);
		}
	}

        /* Make a copy of the callback info. Note: 3rd copy of this  */

	copy = make_wrapper(r, source);

        /* Store it away so that it can later be called. */
	copy->next = registration_list;
	registration_list = copy;

	if (debug) {
		fprintf(stderr, "%s requested registration on %s. registered pid= %d\n",
			source, copy->r.target, copy->r.pid);
		nclients = count_clients(copy->r.target, true);
		fprintf(stderr, "\tnumber of registered clients on %s=%d\n",
			copy->r.target, nclients);
	}
	stat = registered;
        return(&stat);
}

/* de-register an rpc callback proc from the client */
Registration_Status *
deregister_callback_4(r, svcrq)
Registration *r;
struct svc_req *svcrq;
{
	static Registration_Status stat;
	int nclients=0;
	char *source, *name;
        Registration_Wrapper *p = NULL, *q = NULL;

	if (debug)
		fprintf(stderr, "deregister_callback_4 called\n");

	stat = failed;
 
	if ((source = auth_check (svcrq)) == NULL)
		return(&stat);

        if (r==NULL)
                return(&stat);

	q = p = registration_list;
	name = cm_target2name(r->target);
	while (p != NULL) {

		/* This says:
		 * 1) if the name of the caller requesting deregistration
		 * is the same as the original caller who requested
		 * requested registration, and
		 * 2) if the (transient) program, version, & procnum match
		 * the original registration, and
		 * 3) if the process id of the client matches the
		 *  orignal registration 
		 *  
		 *  ... only then is it ok to decommission the ticket.
		 */


		if ((strcmp(p->source, source)==0) &&
		    (strcmp(p->r.target, name)==0) &&
		    (p->r.prognum==r->prognum) &&
		    (p->r.versnum==r->versnum) &&
		    (p->r.procnum==r->procnum) &&
		    (p->r.pid==r->pid)) {	/* a match */
			if (debug) {
			fprintf(stderr, "%s requested deregistration on %s. registered pid= %d\n", source, r->target, r->pid);
			}
			if (p==q)
				registration_list = p->next;
			else
				q->next=p->next;
			destroy_wrapper(p);
			if (debug) {
			  nclients = count_clients(p->r.target, true);
			  fprintf(stderr, "\tnumber of clients on %s = %d\n", p->r.target, nclients);
			}
			free(name);
			stat = deregistered;
			return(&stat);
		}
		q = p;
		p = p->next;
	}
	free(name);
	return(&stat); /* stat == failed */
}

extern Table_Res *
rtable_lookup_4 (args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static Table_Res res;
	Rt_Status rtstat;
	Appt	*p_appt;
	Appt	*h = NULL;
	Info	*p_info;
	Uid	*p_keys;
	Id	*key;
	char	*src;
	char	*source;

	if (debug)
		fprintf(stderr, "rtable_lookup_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.status = access_other;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;
	if ((p_keys = args->args.Args_u.key) == NULL)
		return (&res);

	if (((rtstat = check_read_access (args, svcrq, &res, &src, &p_info))
			!= rt_ok) || p_info == NULL) {
		res.status = rtstat2accessstat(rtstat);
		return (&res);
	}

	while (p_keys != NULL)
	{
		key = &p_keys->appt_id;

		if (debug1)
		{
			fprintf (stderr, "rtable_lookup at %s(%d)\n",
				c_time(key->tick), key->key);
		}

		/* source will be set to NULL if it's the author of the appt */
		source = src;
		p_appt = rtable_lookup_internal(p_info, &source, key);
		if (p_appt != NULL)
		{
			/* No read permission, mask out the what string. */
			if ((source != NULL) && (res.status != access_ok) &&
				(p_appt->what != NULL))
			{
				free (p_appt->what);
				p_appt->what = cm_strdup(FAUX_STRING);
			}
			p_appt->next = h;
			h = p_appt;
		}

		p_keys = p_keys->next;
	}

	res.res.Table_Res_List_u.a = h;
	return (&res);
}

static
num_exception (p_appt)
Appt	*p_appt;
{
	register int ntimes;
	register Except *p;

	ntimes = 0;
	p = p_appt->exception;
	while (p != NULL)
	{
		ntimes++;
		p = p->next;
	}
	return (ntimes);
}

static Appt *
repeater_next_larger (p_lnode,key)
register Lnode	*p_lnode;
register Id	*key;
{
	static Appt appt;
	Period	period;
	Appt	*p_appt;
	Appt	*p_save = NULL;
	Id	id, next_larger_id;
	int	ord;
	register int	ntimes;

	next_larger_id.tick = MAXINT;
	next_larger_id.key = MAXINT;
	while (p_lnode != NULL)
	{
		p_appt = (Appt*)APPT(p_lnode);

		/* check last tick: if it's before the lookup range, skip it */
		if (p_lnode->lasttick == 0)
			p_lnode->lasttick = last_tick(CM_TICK(p_appt),
				p_appt->period, p_appt->ntimes);

		if ((p_lnode->lasttick < key->tick) ||
		    (p_lnode->lasttick == key->tick && KEY(p_appt) <= key->key))
		{
			p_lnode = hc_lookup_next(p_lnode);
			continue;
		}

		period = p_appt->period;
		ntimes = get_ninstance(p_appt);
		id.tick = closest_tick(key->tick, CM_TICK(p_appt),
				period, &ord);
		ord--;
		id.key = KEY(p_appt);
		while (++ord <= ntimes)
		{
			if ((id.tick < key->tick) || (id.tick == key->tick &&
					id.key <= key->key))
		     		id.tick = next_tick (id.tick, period);
			else if (!marked_4_cancellation (p_appt, ord)) {
				if ((id.tick < next_larger_id.tick) ||
				    (id.tick == next_larger_id.tick &&
					id.key < next_larger_id.key))
				{
					next_larger_id = id;
					p_save = p_appt;
				}
				break;
			} else
				id.tick = next_tick(id.tick, period);
		}
		p_lnode = hc_lookup_next (p_lnode);
	}

	if (p_save != NULL)
	{
		appt = *p_save;
		CM_TICK(&appt) = next_larger_id.tick;
		p_save = &appt;
	}

	return (p_save);
}

static Appt *
repeater_next_smaller (p_lnode,key)
register Lnode	*p_lnode;
register Id	*key;
{
	static Appt appt;
	Appt	*p_appt;
	Period	period;
	Appt	*p_save = NULL;
	Id	id, next_smaller_id;
	register int	ord;
	register int	ntimes;

	next_smaller_id.tick = 0;
	next_smaller_id.key = 0;
	while (p_lnode != NULL)
	{
		p_appt = (Appt*)APPT (p_lnode);
		ord = 0;
		ntimes = get_ninstance(p_appt);
		period = p_appt->period;
		id.tick = CM_TICK(p_appt);
		id.key = KEY(p_appt);

		/* Very inefficient loop because it has to check if each
		 * instance is cancelled.  If there is a function to calculate
		 * last tick, this loop can be rewritten in an efficient way.
		 */
		while ((++ord <= ntimes) && (id.tick <= key->tick))
		{
			if (id.tick == key->tick && id.key >= key->key)
				/* this will get us out of the loop */
				/* faster than continue.	    */
				id.tick = next_tick (id.tick, period);
			else {
				if (!marked_4_cancellation (p_appt, ord)) {
					if ((id.tick > next_smaller_id.tick) ||
					    (id.tick == next_smaller_id.tick &&
						id.key > next_smaller_id.key))
					{
						next_smaller_id = id;
						p_save = p_appt;
					}
				}
				id.tick = next_tick (id.tick, period);
			}
		}

		p_lnode = hc_lookup_next (p_lnode);
	}

	if (p_save != NULL)
	{
		appt = *p_save;
		CM_TICK(&appt) = next_smaller_id.tick;
		p_save = &appt;
	}

	return (p_save);
}

static Table_Res *
table_lookup_next (args, svcrq, rb_func, rp_func)
Table_Args *args;
struct svc_req *svcrq;
caddr_t (* rb_func)();
caddr_t (* rp_func)();
{
	static	Table_Res res;
	Rt_Status rtstat;
	Privacy_Level p_level;
	Id	key;
	Info	*p_info;
	Appt	*p_appt;
	Appt	*p_appt1;
	char	*src;

	xdr_free (xdr_Table_Res, (char*)&res);

	res.status = access_other;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	switch (args->args.tag) {
	case TICK_4:
		key.tick = args->args.Args_u.tick;
		key.key = 0;
		break;
	case UID:
		key = args->args.Args_u.key->appt_id;
		break;
	default:
		return(&res);
	}

	if (((rtstat = check_read_access (args, svcrq, &res, &src, &p_info))
			!= rt_ok) || p_info == NULL) {
		res.status = rtstat2accessstat(rtstat);
		return (&res);
	}

	p_appt = (Appt *) (*rb_func) (APPT_TREE(p_info), &key);
	p_appt1 = (Appt *) (*rp_func) (REPT_LIST(p_info)->root, &key);

	if (p_appt1 != NULL) {
		if (rb_func == rb_lookup_next_larger) {
			if ((p_appt==NULL) ||
			    (CM_TICK(p_appt) > CM_TICK(p_appt1)) ||
			    ((CM_TICK(p_appt) == CM_TICK(p_appt1)) &&
				(KEY(p_appt) > KEY(p_appt1)))) {
				p_appt = p_appt1;
			}
		} else {
			if ((p_appt==NULL) ||
			    (CM_TICK(p_appt) < CM_TICK(p_appt1)) ||
			    ((CM_TICK(p_appt) == CM_TICK(p_appt1)) &&
				(KEY(p_appt) < KEY(p_appt1)))) {
				p_appt = p_appt1;
			}
		}
	}

	if (p_appt != NULL) {
		if ((p_level = check_privacy_level(&src, p_appt)) != private) {
			if (p_level == public)
				p_appt = copy_appt(p_appt);
			else
				p_appt = copy_semiprivate_appt(p_appt);

			if ((src != NULL) && (res.status != access_ok) &&
					(p_appt->what != NULL))
			{
				free(p_appt->what);
				p_appt->what = cm_strdup(FAUX_STRING);
			}
		} else
			p_appt = NULL;
	}

	res.res.Table_Res_List_u.a = p_appt;
	return (&res);
}

extern Table_Res *
rtable_lookup_next_larger_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	Table_Res *res;

	if (debug)
		fprintf(stderr, "rtable_lookup_next_larger_4 called\n");

	res = table_lookup_next (args, svcrq, rb_lookup_next_larger,
				repeater_next_larger);
	return (res);
}

extern Table_Res *
rtable_lookup_next_smaller_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	Table_Res *res;

	if (debug)
		fprintf(stderr, "rtable_lookup_next_smaller_4 called\n");

	res = table_lookup_next (args, svcrq, rb_lookup_next_smaller,
				repeater_next_smaller);
	return (res);
}

static void
add_reminder (p_info,p_reminder)
Info	*p_info;
register Rm_que	*p_reminder;
{
	register Rm_que	*p_prev;
	register Rm_que	*p_node;

	if (p_reminder == NULL)
		return;
	p_prev = NULL;
	p_node = RMND_QUEUE(p_info);
	while (p_node != NULL)
	{
		if (p_reminder->remind_at < p_node->remind_at)
			break;
		p_prev = p_node;
		p_node = p_node->next;
	}

	if (p_prev == NULL)
	{
		p_reminder->next = RMND_QUEUE(p_info);
		p_info->rm_queue = (caddr_t)p_reminder;
	}
	else
	{
		p_reminder->next = p_prev->next;
		p_prev->next = p_reminder;
	}
}

static Rm_que *
remove_reminder (p_info,p_prev,p_curr)
Info	*p_info;
Rm_que	*p_prev;
Rm_que	*p_curr;
{
	if (p_prev == NULL)
		p_info->rm_queue = (caddr_t)(p_curr->next);
	else
		p_prev->next = p_curr->next;
	return (p_curr->next);
}

static Rm_que *
build_reminder (current_time,p_appt,p_attr,start_tick,start_ord)
register Tick	current_time;
register Appt	*p_appt;
Attr		p_attr;
register Tick	start_tick;
register u_int	start_ord;
{
	register int	ntimes;
	register Period period;
	register Rm_que	*p_reminder = NULL;

	/* Ignore the expired or unqualified reminder. */
	p_reminder = NULL;
	if (is_appointment (p_appt))
	{
		/* The event is not expired yet, build the reminder */
		if (start_tick >= current_time)
		{
			p_reminder = (Rm_que *) ckalloc (sizeof(*p_reminder));
			p_reminder->remind_ord = 0;
		}
	}
	else
	{
		period = p_appt->period;
		ntimes = get_ninstance(p_appt);
		while (start_ord <= ntimes)
		{
			/* Event is not expired */
			if (start_tick >= current_time)
			{
				/* Event is not cancelled */
				if (!marked_4_cancellation (p_appt, start_ord))
				{
					p_reminder = (Rm_que *)ckalloc(
							sizeof(*p_reminder));
					p_reminder->remind_ord = start_ord;
					break;
				}
			}
			/* Event is expired, advance to next event */
			start_tick = next_tick (start_tick, period);
			start_ord++;
		}
	}

	if (p_reminder != NULL)
	{
		p_reminder->remind_at = start_tick;
		p_reminder->appt = p_appt;
		p_reminder->attr = p_attr;
	}

	return (p_reminder);
}

static Reminder *
copy_rm_reminder(original)
Rm_que	*original;
{
	Reminder *copy;

	if (original == NULL)
		return (NULL);
	
	copy = make_reminder();
	if (copy != NULL)
	{
		copy->tick = original->remind_at;
		copy->next = NULL;
		copy->attr.attr = cm_strdup (original->attr->attr);
		copy->attr.value = cm_strdup (original->attr->value);
		copy->attr.clientdata = cm_strdup(original->attr->clientdata);
		copy->appt_id.tick = copy->tick + atol (copy->attr.value);
		copy->appt_id.key = KEY(original->appt);
	}

	return (copy);
}

extern Table_Res *
rtable_lookup_next_reminder_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;
	char		*source;
	char		*target;
	Info		*p_info;
	Reminder	*p_reminder;
	Rt_Status	rtstat;
	register Rm_que	*p_node;
	register Rm_que	*p_prev;
	register Rm_que	*p_new;
	register Rm_que	*p_next;
	register long	tick;

	if (debug)
		fprintf(stderr, "rtable_lookup_next_reminder_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.status = access_other;
	res.res.tag = RM;
	res.res.Table_Res_List_u.r = NULL;

	if ((source = auth_check (svcrq)) == NULL)
		return (&res);
	if ((target = args->target) == NULL)
		return (&res);

/*	This dictates policy on who can lookup
	reminders, so I'm removing it. 
	
	if (strcmp (source, target) != 0)
	{
		res.status = access_failed;
		return (&res);
	}
*/

	if ((p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load(target, svcrq, &p_info)) != rt_ok) {
			res.status = rtstat2accessstat(rtstat);
			return (&res);
		}
	}

	tick = args->args.Args_u.tick;

	if (debug1)
		fprintf (stderr, "Next reminder after %s\n", c_time (tick));

	p_prev = NULL;
	p_node = RMND_QUEUE(p_info);
	while (p_node != NULL)
	{
		/* It is still a future reminder. */
		if (tick < p_node->remind_at)
			break;

		/* The reminder expired.  It either needs to be recalculated
		 * (repeating appointment) or dropped (non-repeating appt.)
		 */
		p_next = remove_reminder (p_info, p_prev, p_node);

		if (is_repeater(p_node->appt))
		{
			/* Calculate next reminder for repeating appointment */
			p_new = build_reminder (tick+1, p_node->appt,
					p_node->attr, p_node->remind_at,
					p_node->remind_ord);
			if (p_new != NULL)
			{
				add_reminder (p_info, p_new);
				if (p_new->next == p_next)
					p_prev = p_new;
			}
		}

		free (p_node);
		p_node = p_next;
	}

	/* Pick the first one from the active reminder queue because it is
	 * always >= the given key.
	 */
	p_node = RMND_QUEUE (p_info);
	if (p_node != NULL)
	{
		tick = p_node->remind_at;
		do
		{
			p_reminder = copy_rm_reminder(p_node);
			p_reminder->next = res.res.Table_Res_List_u.r;
			res.res.Table_Res_List_u.r = p_reminder;
			p_node = p_node->next;
		} while ((p_node != NULL) && (p_node->remind_at == tick));
	}
	res.status = access_ok;

	if (debug1)
		print_reminder (p_info);

	return (&res);
}

extern Table_Status *
rtable_flush_table_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static Table_Status s;
	Id		key;
	Rt_Status	rtstat;
	Info	*p_info;
	register int	n = 0;
	register Node	*p_node;
	register Lnode	*p_lnode;
	register Lnode	*p_next;
	register Appt	*p_appt;

	if (!debug1)
	{
		s = ok;
		return (&s);
	}

	if (debug1)
		fprintf (stderr, "rtable_flush_table\n");

	if ((p_info = rtable_get_table (args->target)) == NULL)
	{
		if ((rtstat = rtable_load(args->target, svcrq, &p_info)) != rt_ok) {
			s = rtstat2tablestat(rtstat);
			return (&s);
		}
	}

	if (debug1)
	{
		fprintf(stderr, "%s: before flush.. rbsize= %d\n", pgname,
			rb_size(APPT_TREE(p_info))+hc_size(REPT_LIST(p_info)));
	}
	
	/* Flushing the single appointment tree. */
	key.key = 0;
	key.tick = args->args.Args_u.tick;
	while (p_appt = (Appt *) rb_lookup_next_larger(APPT_TREE(p_info), &key))
	{
		p_node = rb_delete (APPT_TREE(p_info), &(p_appt->appt_id));
		if (p_node != NULL)
		{
			n++;
			destroy_appt ((Appt *) p_node->data);
			free (p_node);
		}
	}

	/* Flushing the repeating appointment list */
	key.key = 0;
	key.tick = args->args.Args_u.tick;
	p_lnode = REPT_LIST(p_info)->root;
	while (p_lnode != NULL)
	{
		p_next = hc_lookup_next (p_lnode);
		p_appt = (Appt*)APPT(p_lnode);
		if (CM_TICK(p_appt) > key.tick)
		{
			n++;
			destroy_appt (p_appt);
			(void) hc_delete_node (REPT_LIST(p_info), p_lnode);
			free (p_lnode);
		}
		p_lnode = p_next;
	}
	s = ok;
	if (debug1)
	{
		fprintf (stderr, "%s: entries deleted= %d\n", pgname, n);
		fprintf (stderr, "%s: after flush.. rbsize= %d\n", pgname,
			rb_size(APPT_TREE(p_info))+hc_size(REPT_LIST(p_info)));
	}

	return (&s);
}

static caddr_t
add_to_link_range (p_appt,ilp,src,status,p_level)
register Appt	*p_appt;
caddr_t		ilp;
char		*src;
Access_Status	status;
Privacy_Level	p_level;
{
	register Appt	*p_prev;
	register Appt	*copy;

	if (p_level == public)
		copy = copy_appt(p_appt);
	else
		copy = copy_semiprivate_appt(p_appt);
	if (copy == NULL)
		return(ilp);

	if ((src != NULL) && (status != access_ok) && (copy->what != NULL))
	{
		free (copy->what);
		copy->what = cm_strdup(FAUX_STRING);
	}

	/* Add the item to the linked list in ascending order */
	p_prev = NULL;
	p_appt = (Appt *) ilp;
	while (p_appt != NULL)
	{
		if (CM_TICK(copy) <= CM_TICK(p_appt))
			break;
		p_prev = p_appt;
		p_appt = p_appt->next;
	}
	if (p_prev == NULL)
	{
		copy->next = p_appt;
		return ((caddr_t) copy);
	}
	else
	{
		copy->next = p_prev->next;
		p_prev->next = copy;
		return (ilp);
	}
}


static caddr_t
add_to_link_abbr (p_appt,ilp,src,status,p_level)
Appt		*p_appt;
caddr_t		ilp;
char		*src;
Access_Status	status;
Privacy_Level	p_level;
{
	register Abb_Appt *copy;
	register Abb_Appt *p_node;
	register Abb_Appt *p_prev;

	if (p_level == public)
		copy = appt_to_abbrev(p_appt);
	else
		copy = appt_to_semiprivate_abbrev(p_appt);
	if (copy == NULL)
		return(ilp);

	if ((src != NULL) && (status != access_ok) && (copy->what != NULL))
	{
		free (copy->what);
		copy->what = cm_strdup(FAUX_STRING);
	}

	/* Add the item to the linked list in ascending order */
	p_prev = NULL;
	p_node = (Abb_Appt *) ilp;
	while (p_node != NULL)
	{
		if (copy->appt_id.tick <= p_node->appt_id.tick)
			break;
		p_prev = p_node;
		p_node = p_node->next;
	}
	if (p_prev == NULL)
	{
		copy->next = p_node;
		return ((caddr_t) copy);
	}
	else
	{
		copy->next = p_prev->next;
		p_prev->next = copy;
		return (ilp);
	}
}

static void
rtable_internal_range(args,svcrq,add_list_func,res)
Table_Args *args;
struct svc_req *svcrq;
caddr_t	(*add_list_func) ();
Table_Res *res;
{
	Info		*p_info;
	Period		period;
	caddr_t		ilp = NULL;
	int		tmp_tick;
	Id		lo_key;
	char		*src;
	char		*source;
	Rt_Status	rtstat;
	Privacy_Level	p_level;
	register int	n;
	register Lnode	*p_lnode;
	register long	hi_tick;
	register int	tick;
	int	ordinal;
	register int	ntimes;
	register Appt	*p_appt;
	Range		*p_range;
	int		total = 0, skipped = 0;


	res->status = access_other;
	if ((p_range = args->args.Args_u.range) == NULL)
		return;

	if (((rtstat = check_read_access (args, svcrq, res, &src, &p_info))
			!= rt_ok) || p_info == NULL) {
		res->status = rtstat2accessstat(rtstat);
		return;
	}

	while (p_range != NULL)
	{
		lo_key.key = MAXINT;
		lo_key.tick = p_range->key1;
		hi_tick = p_range->key2;

		if (debug1)
		{
			fprintf(stderr,"Range lookup from %s",
				c_time(lo_key.tick));
			fprintf(stderr, " to %s\n", c_time(hi_tick));
		}

		n = 0;

		/* Get a range of appointments in single appointment tree */
		while ((p_appt = (Appt *)rb_lookup_next_larger(
			APPT_TREE(p_info),&lo_key)) && (CM_TICK(p_appt) < hi_tick))
		{
			/* source will be set to NULL if it's the
			 * author of the appt
			 */
			source = src;
			if ((p_level = check_privacy_level(&source, p_appt))
					!= private) {
				n++;
				ilp = (*add_list_func) (p_appt, ilp, source,
						res->status, p_level);
				if (ilp == NULL)
					break;
			}

			lo_key.key = KEY(p_appt);
			lo_key.tick = CM_TICK(p_appt);
		}

		/* Get a range of events from repeating appointment list */
		lo_key.key = MAXINT;
		lo_key.tick = p_range->key1;
		p_lnode = REPT_LIST(p_info)->root;
		nexttickcalled = 0;
		totalskipped = 0;
		while (p_lnode != NULL)
		{
			total++;
			p_appt = (Appt*)APPT(p_lnode);

			/* calculate the last tick */
			if (p_lnode->lasttick == 0)
				p_lnode->lasttick = last_tick(CM_TICK(p_appt),
					p_appt->period, p_appt->ntimes);

			if (p_lnode->lasttick <= lo_key.tick ||
			    CM_TICK(p_appt) >= hi_tick) { 
				p_lnode = hc_lookup_next (p_lnode);
				skipped++;
				continue;
			}

			ntimes = get_ninstance(p_appt);
			period = p_appt->period;
			for (tick = closest_tick(lo_key.tick, CM_TICK(p_appt),
				period, &ordinal), ordinal--;
			     tick < hi_tick;
			     tick = next_tick(tick, period))
			{
				/* Repeater but beyond the scope */
				if (++ordinal > ntimes)
					break;
				if (tick <= lo_key.tick)
					continue;

				/* source will be set to NULL if it's
				 * the author of the appt
				 */
				source = src;
				if ((p_level = check_privacy_level(&source,
						p_appt)) == private)
					continue;

				/* If not cancelled, add to linked list */
				if (!marked_4_cancellation (p_appt, ordinal))
				{
					n++;

					/* Replace the parent key by the
					 * current tick for the repeating event
					 */
					tmp_tick = CM_TICK(p_appt);
					CM_TICK(p_appt) = tick; 

					/* Add to list, restore parent key */
					ilp = (*add_list_func)(p_appt, ilp,
						source, res->status, p_level);
					CM_TICK(p_appt) = tmp_tick;
					if (ilp == NULL)
						break;
				}
			}

			p_lnode = hc_lookup_next (p_lnode);
		}
		if (debug1) {
			fprintf(stderr, "%s: total = %d, skipped = %d\n",
				"rtable_internal_range", total, skipped);
			fprintf(stderr, "%s: %d call to next_tick\n",
				"rtable_internal_range", nexttickcalled);
			fprintf(stderr, "%s: next_tick calls skipped %d\n",
				"rtable_internal_range", totalskipped);
		}

		p_range = p_range->next;
	}

	if (debug1)
		fprintf (stderr, "Found %d entries in range lookup\n", n);

	if (res->res.tag == AP)
		res->res.Table_Res_List_u.a = (Appt *) ilp;
	else if (res->res.tag == AB)
		res->res.Table_Res_List_u.b = (Abb_Appt *) ilp;
	else /* impossible */
		res->status = access_other;
}

extern Table_Res *
rtable_lookup_range_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;

	if (debug)
		fprintf(stderr, "rtable_lookup_range_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	rtable_internal_range(args, svcrq, add_to_link_range, &res);

	return (&res);
}

extern Table_Res *
rtable_abbreviated_lookup_range_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;

	if (debug)
		fprintf(stderr, "rtable_abbreviated_lookup_range_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.res.tag = AB;
	res.res.Table_Res_List_u.b = NULL;

	rtable_internal_range(args, svcrq, add_to_link_abbr, &res);

	return (&res);
}

static int
marked_4_cancellation(a, i)
Appt *a; int i;
{
	register Except *p;

	if (a==NULL) return(0);
	p = a->exception;	/* in descending order for faster access */
	while (p!=NULL) {
		if (i > p->ordinal) break;
		if (i == p->ordinal) return(1);
		p = p->next;
	}
	return(0);
}

static int
rtable_transact_log(args, a, transaction)
Table_Args *args;
Appt *a;
Transaction transaction;
{
	int status = 0;
	char *name = NULL, *log=NULL;

	if ((name = cm_target2name(args->target)) == NULL)
		return (CERROR);
	log = get_log(name);
	free(name);
	if (log == NULL)
		 status = CERROR;
	else {
		status = append_log(log, a, transaction);
		free(log);
	}
	return(status);
}

static long
unique_key(new_key)
int     new_key;
{
	static	long	uvalue = 0;
 
	if (new_key == 0)
		return (++uvalue);
	if (new_key > uvalue)
		uvalue = new_key;
	return (uvalue);
}

static Rb_Status
insert_internal (p_info,ap)
Info	*p_info;
Appt	*ap;
{
	Rb_Status s;
	Appt	*p_appt;
	Attr	p_attr;
	Tick	current_time;

	/* Insert one appointment at a time. */
	p_appt = (Appt*)copy_single_appt(ap);
	if (p_appt == NULL)
		return (rb_failed);
	if (KEY(p_appt) == 0)
		p_appt->appt_id.key = ap->appt_id.key = unique_key (0);
	else
		(void) unique_key (KEY(p_appt));

	if (debug1)
	{
		fprintf (stderr, "Insert: %s(%d)\n", c_time(CM_TICK(p_appt)),
			KEY(p_appt));
	}

	/* Add the appointment into the data structure */
	if (is_appointment (p_appt))
		s = rb_insert (APPT_TREE(p_info), (caddr_t)p_appt,
				(Key) &(p_appt->appt_id));
	else
		s = hc_insert (REPT_LIST(p_info), (caddr_t)p_appt,
				(Key) &(p_appt->appt_id));

	if (s == rb_failed)
		destroy_appt (p_appt);
	else
	{
		/* Add the qualified reminder attrs to the reminder queue */
		current_time = now();
		p_attr = p_appt->attr;
		while (p_attr != NULL)
		{
			Tick	tick;
			Rm_que	*p_reminder;

			tick = (Tick) CM_TICK(p_appt) - (Tick) atol(p_attr->value);
			p_reminder = build_reminder (current_time, p_appt,
							p_attr, tick, 1);
			if (p_reminder != NULL)
				add_reminder (p_info, p_reminder);
			p_attr = p_attr->next;
		}
	}

	return (s);
}

/*
 * For parser only (Backward compatiable) and permission is not checked.
 */
extern Rb_Status
rtable_insert_internal(target,ap)
char	*target;
Appt	*ap;
{
	Info	*p_info;
	Rt_Status rtstat;

	if ((p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load (target, NULL, &p_info)) != rt_ok)
			return (rtstat);
	}

	return (insert_internal (p_info, ap));
}

/*
 * Obsolete all reminders (iff ord == 0) whose parent appointment matches a
 * given appointment.  If ord != 0, then obsolete all active reminders whose
 * serving ordinal matches ord in additional to the matching of its parent
 * appointment.  The reminder of the next available instance will be put on the
 * reminder queue.
 */
static void
obsolete_reminder (p_info,p_appt,ord,delforward)
Info		*p_info;
register Appt	*p_appt;
register int	ord;
Boolean		delforward;
{
	register Rm_que *p_prev;
	register Rm_que *p_next;
	register Rm_que *p_node;
	register Rm_que *p_hdr = NULL;

	p_prev = NULL;
	p_node = RMND_QUEUE (p_info);
	while (p_node != NULL)
	{
		if ((p_node->appt != p_appt) ||
		    ((ord != 0) && (ord != p_node->remind_ord)) ||
		    ((ord != 0) && delforward && p_node->remind_ord < ord))
			p_next = p_node->next;
		else
		{
			/* Found the obsolete reminder. */
			p_next = remove_reminder (p_info,p_prev,p_node);
			if (ord == 0)
				free (p_node);
			else
			{
				/* Chain the obsolete reminders together to
				 * re-calculate the new reminders.
				 */
				p_node->next = p_hdr;
				p_hdr = p_node;
			}

			p_node = p_prev;
		}

		p_prev = p_node;
		p_node = p_next;
	}

	/* Build the reminders of the next instance from obsoleted reminders.
	 * Note, we can't put this code in the above 'while'-loop because it
	 * may confuse the loop.
	 */
	while (p_hdr != NULL)
	{
		p_next = p_hdr->next;
		p_node = build_reminder (p_hdr->remind_at+1,
					p_hdr->appt, p_hdr->attr,
					p_hdr->remind_at, ord);
		add_reminder (p_info, p_node);
		free (p_hdr);
		p_hdr = p_next;
	}
}

/*
 * dont_care_cancel:
 *	TRUE - it is a match regardless the event is cancelled, 
 *	FALSE - it is not a match if the event is cancelled.
 */
static int
in_repeater (key,p_appt,dont_care_cancel)
register Id	 *key;
register Appt	 *p_appt;
register Boolean dont_care_cancel;
{
	register Period	period;
	int	ordinal;
	register int	ntimes;
	register long	tick;

	ntimes = get_ninstance(p_appt);
	period = p_appt->period;
	tick = closest_tick(key->tick, CM_TICK(p_appt), period, &ordinal);
	ordinal--;
	while (++ordinal <= ntimes)
	{
		if (tick > key->tick)		/* out-of-bound */
			break;
		if (tick == key->tick)
		{
			if (dont_care_cancel)
				return (ordinal);
			if (!marked_4_cancellation (p_appt, ordinal))
				return (ordinal);
		}
		tick = next_tick (tick, period);
	}

	return (0);
}

/*
 * If p_auth is null, the initiator is the owner of the calendar.  Permission
 * to delete any appointment is always granted.  If p_auth is not null, we need
 * to check if it matches the author of the deleting appointment.  Only the
 * author can delete his/her appointment.
 */
static Appt *
delete_internal (p_info,p_auth,p_key)
Info		*p_info;
char		*p_auth;
register Id	*p_key;
{
	register Appt	*p_appt;
	register Lnode	*p_lnode;
	register Node	*p_node;


	/* Before we delete an event from the single appointment tree, we
	 * need to check if the initiator is the author of the appointment.
	 */
	if ((p_auth != NULL) && 
	    ((p_appt = (Appt *) rb_lookup (APPT_TREE(p_info), p_key)) != NULL))
	{
		if (access_control (access_delete, p_info, p_auth, p_appt)
				!= access_ok)
			return (NULL);
	}
	if ((p_node = rb_delete (APPT_TREE(p_info), p_key)) != NULL)
	{
		p_appt = (Appt*)APPT(p_node);
		obsolete_reminder (p_info, p_appt, 0, false);
		free (p_node);
		if (debug1)
		{
			fprintf (stderr, "Deleted %s(%d)\n",
				c_time(CM_TICK(p_appt)), KEY(p_appt));
		}
		return (p_appt);
	}

	/* Attempt to delete the event from the repeating appointment list */
	p_lnode = (Lnode *) hc_lookup_node (REPT_LIST(p_info), p_key);
	if (p_lnode != NULL)
	{
		p_appt = (Appt*)APPT(p_lnode);
		if (access_control (access_delete, p_info, p_auth, p_appt)
				!= access_ok)
			return (NULL);
		obsolete_reminder (p_info, p_appt, 0, false);
		(void) hc_delete_node (REPT_LIST(p_info), p_lnode);
		free (p_lnode);
		if (debug1)
		{
			fprintf (stderr, "Deleted %s(%d)\n",
				c_time(CM_TICK(p_appt)), KEY(p_appt));
		}
		return (p_appt);
	}
	return (NOT_FOUND);
}

/*
 * This function is for parser (backward compatibility) and permission is not
 * checked because it is the owner's request from parser.
 */
extern Appt *
rtable_delete_internal(target, parent_uid)
char	*target;
Id	*parent_uid;
{
	Info	*p_info;
	Appt	*p_appt;

	if (debug1)
	{
		fprintf (stderr, "Remove: %s(%d)\n", c_time(parent_uid->tick),
			parent_uid->key);
	}
	if ((p_info = rtable_get_table (target)) == NULL)
		return (NULL);
	p_appt = delete_internal (p_info, NULL, parent_uid);

	/* Ignore the error if the appointment is not found. */
	if (p_appt == NOT_FOUND)
		p_appt = NULL;

	return (p_appt);
}

extern Table_Res *
rtable_insert_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;
	Rb_Status status;
	Rt_Status rtstat;
	char	*target;
	char	*source;
	Info	*p_info;
	Appt	*ap, *appt, *prev=NULL, *a;
	int 	stat=0;

	if (debug)
		fprintf(stderr, "rtable_insert_4 called\n");

	res.status = access_other;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	if ((source = auth_check (svcrq)) == NULL)
		return (&res);
	if ((target = args->target) == NULL)
		return (&res);
	if ((ap = args->args.Args_u.appt) == NULL)
		return (&res);

	/* do some sanity checks before inserting : check appt data */
	for (appt = ap; appt != NULL; appt = appt->next) {
		/* ntimes should be 0 or positive */
		if (appt->ntimes < 0 ||
		    (appt->period.period > single && appt->ntimes == 0))
			return(&res);

		/* period beyond daysOfWeek is not supported */
		if (appt->period.period > daysOfWeek) {
			res.status = access_notsupported;
			return(&res);
		}

		/* if weekmask of daysOfWeek appt is set incorrectly, return */
		if (appt->period.period == daysOfWeek &&
			(appt->period.nth == 0 || appt->period.nth > 127))
			return(&res);
	}

	if ((p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load (target, svcrq, &p_info)) != rt_ok) {
			res.status = rtstat2accessstat(rtstat);
			return (&res);
		}
	}

	/* Check for ADD permission. */
	if ((!file_owner(rtable_get_owner(target), source) ||
	    source_access_control(access_write, p_info, source, NULL) != access_ok) &&
	    access_control (access_write, p_info, source, NULL) != access_ok)
	{
		res.status = access_failed;
		return (&res);
	}

	appt = ap;
	while (appt != NULL)
	{
		/* get start date */
		get_startdate(appt);

		/* We don't trust the author field; we set our own. */
		if (appt->author)
			free(appt->author);
#if 0
		appt->author = getuseratdomain(source);
#endif
		appt->author = ckalloc(strlen(source)+1);
		strcpy(appt->author, source);

		/* Note, the key in appt will be set if its value is 0. */
		if (insert_internal(p_info, appt) != rb_ok)
			return (&res);

		if ((stat = rtable_transact_log(args, appt, add)) != 0) {
			if (stat == SPACEERROR) {
				if ((a = delete_internal(p_info, source, 
					&appt->appt_id)) != NULL && a != NOT_FOUND)
					destroy_appt(a);
			}
			/* some appts were inserted successfully */
			if (prev != NULL) {
				res.status = access_partial;
				prev->next = NULL;
				destroy_appt(appt);
				do_callbacks(source, ap, target, args->pid);
				res.res.Table_Res_List_u.a = ap;
			}
			/* first appt in bunch that failed */
			else {
				res.status = access_incomplete;
				destroy_appt(ap);
			}
			p_info->modified = true;
			return (&res);
		}
		prev = appt;
		appt = appt->next;
	}

	p_info->modified = true;
	do_callbacks(source, ap, target, args->pid);
	res.status = access_ok;
	res.res.Table_Res_List_u.a = ap;

	return (&res);
}

static Exception
append_exception_list (p_appt, ordinal)
Appt	*p_appt;
int	ordinal;
{
	Exception p_excpt;
	Exception p_prev;
	Exception p_ex;

	if ((p_excpt = (Exception) ckalloc (sizeof(*p_excpt))) == NULL)
		return (NULL);
	p_excpt->ordinal = ordinal;
	p_prev = NULL;
	p_ex = p_appt->exception;
	while (p_ex != NULL)
	{
		/* Exception list is in descending order for faster access */
		if (ordinal > p_ex->ordinal)
			break;
		p_prev = p_ex;
		p_ex = p_ex->next;
	}
	if (p_prev == NULL)
	{
		p_excpt->next = p_appt->exception;
		p_appt->exception = p_excpt;
	}
	else
	{
		p_excpt->next = p_prev->next;
		p_prev->next = p_excpt;
	}

	return (p_excpt);
}

/*
 * remove exceptions that are larger than ordinal
 */
static void
trunc_exception_list (p_appt, ordinal)
Appt	*p_appt;
int	ordinal;
{
	Exception p_next;
	Exception p_ex;

	p_ex = p_appt->exception;

	/* Exception list is in descending order for faster access */
	while ((p_ex != NULL) && (p_ex->ordinal > ordinal))
	{
		p_next = p_ex->next;
		free(p_ex);
		p_ex = p_next;
	}
	p_appt->exception = p_ex;
}

static Appt *
delete_instance (args,p_info,source,key,option,remain)
Table_Args *args;
Info	*p_info;
char	*source;
Id	*key;
Options	option;
int	*remain;
{
	Lnode		*p_lnode;
	register Appt	*p_appt, *new_p;
	register int	ordinal;
	int		f, file_size, ntimes, ninstance, status = 0;
	struct stat info;
	char *log, *name;


	p_lnode = (Lnode *) hc_lookup_node (REPT_LIST(p_info), (Key) key);
	if (p_lnode == NULL)
		return (NOT_FOUND);

	p_appt = (Appt*)APPT(p_lnode);
	if (access_control (access_delete, p_info, source, p_appt) != access_ok)
		return (NULL);

	if ((ordinal = in_repeater (key, p_appt, TRUE)) == 0)
		return (NOT_FOUND);

	if (debug1)
		fprintf(stderr,"Delete instance: Ordinal=%d\n",ordinal);

	/* save file size in case the first log transaction succeeds 
		but the second log transaction fails.*/
	file_size = 0;
	name = cm_target2name(args->target);
	log = get_log(name); free(name);
	if (log != NULL && stat(log, &info) == 0)
		file_size = info.st_size;

	/* remove from log */
	if ((status = rtable_transact_log (args, p_appt, cm_remove)) != 0) {
		if (log != NULL) free(log);
		return ((Appt*)status);
	}

	/* remove from memory */
	ninstance = get_ninstance(p_appt);
	if (remain != NULL)
		*remain = get_new_ntimes(p_appt->period, key->tick,
				(ninstance - ordinal + 1));

	ntimes = p_appt->ntimes;

	/* make changes to new appt in case log transaction fails and 
	/* we have to restore database back to original state. */
	new_p = (Appt*)copy_single_appt(p_appt);

	if (option == do_one) {
		if (!append_exception_list (new_p, ordinal)) {
			if (log != NULL) free(log);
			destroy_appt(new_p);
			return (NULL);
		}
	} 
	else {
		ninstance = ordinal - 1;
		new_p->ntimes = get_new_ntimes(new_p->period,
					new_p->appt_id.tick, ninstance);
		new_p->period.enddate = prev_tick(key->tick, new_p->period);
		trunc_exception_list(new_p, ordinal);
	}

	/* The last one from the series has been deleted, no more reminder. */
	if (ninstance == num_exception (new_p))
		ordinal = 0;

	if (ordinal == 0) {
		/* Obsolete the reminders which match the ordinal */
		obsolete_reminder (p_info, p_appt, ordinal,
			(option == do_one ? false : true));
		destroy_appt(p_appt);
		p_appt = (Appt*)copy_single_appt(new_p);
		hc_delete_node (REPT_LIST(p_info), p_lnode);
		free (p_lnode);
	} else {
		/* Write out the series with new exception list. */
		if ((status = rtable_transact_log (args, new_p, add)) != 0) {
			if (status == SPACEERROR) {
				if (file_size != 0) {
					if (log != NULL) {
						if ((f = open(log, O_RDWR | O_APPEND | O_SYNC)) == -1) {
							destroy_appt(new_p);
							free(log); 
							return ((Appt*)status);
						}
						free(log); log=NULL;
						ftruncate(f, file_size);
						close(f);
					}
				}
			}
			if (log != NULL) free(log);
			destroy_appt(new_p);
			return ((Appt*)status);
		}
		/* we have to retain the original appt because the 
		reminder list searches on appt addresses */
		p_appt->ntimes = new_p->ntimes;
		p_appt->period.enddate = new_p->period.enddate;
		p_appt->exception = copy_excpt(new_p->exception);
		/* Obsolete the reminders which match the ordinal */
		obsolete_reminder (p_info, p_appt, ordinal,
			(option == do_one ? false : true));
		new_p->ntimes = ntimes;
		p_appt = new_p;
	}
	if (log != NULL) free(log);

	return (p_appt);
}

extern Table_Res *
rtable_delete_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;
	Rt_Status rtstat;
	Info	*p_info;
	char	*src;
	char	*source;
	char	*target;
	Uidopt	*p_keys;
	Appt	*h = NULL;
	Appt	*a;
	int	stat = 0, d, n, nf;

	if (debug)
		fprintf(stderr, "rtable_delete_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.status = access_other;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	if ((src = source = auth_check (svcrq)) == NULL)
		return (&res);
	if ((target = args->target) == NULL)
		return (&res);
	if ((p_keys = args->args.Args_u.uidopt) == NULL)
		return (&res);

	if ((p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load (target, svcrq, &p_info)) != rt_ok) {
			res.status = rtstat2accessstat(rtstat);
			return (&res);
		}
	}

	/* Check for DELETE permission: owner or in the delete access list. */
	if ((file_owner(rtable_get_owner(target), source) &&
	    source_access_control(access_delete, p_info, source, NULL) == access_ok) ||
	    (access_control (access_delete, p_info, source, NULL) == access_ok))
		source = NULL;

	nf = d = n = 0;
	while (p_keys != NULL)
	{
		n++;
		if (debug1)
		{
			fprintf (stderr, "Delete: %s(%d)\n",
			      c_time(p_keys->appt_id.tick),p_keys->appt_id.key);
		}

		/* single or all in a repeating series */
		if (p_keys->option == do_all)
			a = delete_internal (p_info, source, &p_keys->appt_id);
		else {
			a = delete_instance (args, p_info, source,
				&p_keys->appt_id, p_keys->option, NULL);
			/* delete instance does the log transacting too*/
			if (a == (Appt*)CERROR || a == (Appt*)SPACEERROR) {
				stat = SPACEERROR;
				break;
			}
		}

		if (a == NOT_FOUND)
			nf++;
		else if (a != NULL) {
			if (p_keys->option == do_all) {
				/* Transact the log */
				if ((stat = rtable_transact_log (args, a, cm_remove))
						    != 0) {
					if (stat == SPACEERROR) {
						insert_internal(p_info, a);
						destroy_appt(a);
					}
					/* either CERROR or SPACEERROR */
					break;
				}
			} 
			else
				CM_TICK(a) = p_keys->appt_id.tick;

			d++;
			a->next = h;
			h = a;
		}

		p_keys = p_keys->next;
	}

	if (h != NULL) {
		p_info->modified = true;
		do_callbacks (src, h, target, args->pid);
	}

	if (d == 0) {
		if (stat != 0)
			res.status = access_incomplete;
		else
			res.status = (nf < n) ? access_failed : access_ok;
	}
	else if (d < n)
		res.status = access_partial;
	else
		res.status = access_ok;

	res.res.Table_Res_List_u.a = h;

	return (&res);
}

extern Table_Res *
rtable_delete_instance_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;

	if (debug)
		fprintf(stderr, "rtable_delete_instance_4 called\n");

	res.status = access_notsupported;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	return(&res);
}

static Tick
first_tick(t, period, ordinal)
Tick t;
Period period;
int ordinal;
{
	int i;
	Tick ftick;

	ftick = t;
	for (i = 1; i < ordinal; i++)
		ftick = prev_tick(ftick, period);

	return(ftick);
}

static Appt*
get_new_elist(newp, parent_p, remain)
Appt *newp, *parent_p;
int remain;
{
        int except_no, ntimes_diff;
        Except *e, *p, *last_e = NULL;

        newp->exception=NULL;
	ntimes_diff = get_ninstance(parent_p) - remain;
	p = parent_p->exception;
	while(p != NULL) {
		if ((except_no = (p->ordinal - ntimes_diff)) > 0 && 
			except_no <= remain) {
			e = (Except*)malloc(sizeof(Except));
			e->ordinal = except_no;
			e->next = NULL;
			if (last_e != NULL) 
				last_e->next = e;
			else
				newp->exception = e;
			last_e = e;
		}
                p = p->next;
        }
	return newp;
}
extern Table_Res *
rtable_change_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;
	Rt_Status rtstat;
	Info	*p_info;
	char	*src, *source,	*target;
	char	*name, *log = NULL;
	Id	*p_key;
	Appt	*p_appt, *save_a=NULL, *a, *appt;
	Options	option;
	int	f, file_size, status, remain,	ordinal;
	Lnode           *p_lnode;
	struct stat info;

	if (debug)
		fprintf(stderr, "rtable_change_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.status = access_other;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	if ((src = source = auth_check (svcrq)) == NULL)
		return (&res);
	if ((target = args->target) == NULL)
		return (&res);
	if ((p_key = args->args.Args_u.apptid.oid) == NULL)
		return (&res);
	if ((p_appt = args->args.Args_u.apptid.new_appt) == NULL)
		return (&res);

	/* ntimes should be 0 or positive */
	if (p_appt->ntimes < 0 ||
	    (p_appt->period.period > single && p_appt->ntimes == 0))
		return(&res);

	/* period beyond daysOfWeek is not supported */
	if (p_appt->period.period > daysOfWeek) {
		res.status = access_notsupported;
		return(&res);
	}

	/* if weekmask of daysOfWeek appt is not set correctly, return */
	if (p_appt->period.period == daysOfWeek &&
	    (p_appt->period.nth == 0 || p_appt->period.nth > 127))
		return(&res);

	option = args->args.Args_u.apptid.option;

	if ((p_info = rtable_get_table (target)) == NULL)
	{
		if ((rtstat = rtable_load (target, svcrq, &p_info)) != rt_ok) {
			res.status = rtstat2accessstat(rtstat);
			return (&res);
		}
	}

	/* Is owner? */
	if (file_owner(rtable_get_owner(target), source) &&
		source_access_control(access_write, p_info, source, NULL) == access_ok)
		source = NULL;
	else {
		/* Not owner; check if caller has Insert permission. */
		if (access_control (access_write, p_info, source, NULL)
		    != access_ok)
		{
			res.status = access_failed;
			return (&res);
		}

		/* Not owner; check if caller has DELETE permission. */
		if (access_control (access_delete, p_info, source, NULL)
		    == access_ok)
			source = NULL;
	}

	/* save file size in case the first log transaction succeeds 
		but the second log transaction fails.*/
	file_size = 0;
	name = cm_target2name(args->target);
	log = get_log(name); free(name);

	if (log != NULL && stat(log, &info) == 0)
		file_size = info.st_size;

	if (option == do_all)
		a = delete_internal (p_info, source, p_key);
	else {  
		if (option == do_forward) {
			/* We need to save the parent appt to preserve the 
			exception list for later recalculation. delete_instance
			truncates this list. */
			p_lnode = (Lnode*)hc_lookup_node(REPT_LIST(p_info), (Key)p_key);
        		if (p_lnode == NULL) 
				return (&res);
			appt = (Appt*)APPT(p_lnode);
			/* if repeating info hasnt changed and there is
			 * an exception list in the old appt than save
			 * the original appt for later recalculation of
			 * exception list */
			if (appt->period.period == 
				p_appt->period.period &&
			    	appt->period.nth == 
					p_appt->period.nth &&
				 appt->exception != NULL) 
				save_a = copy_appt(appt); 
		}
		a = delete_instance (args, p_info, source, p_key, option, &remain);
		if (a == (Appt*)CERROR || a == (Appt*)SPACEERROR) {
			if (save_a != NULL)
				destroy_appt(save_a);
			res.status = access_incomplete;
			return (&res);
		}
	}

	if (a == NULL) {
		if (save_a != NULL)
			destroy_appt(save_a);
		res.status = access_failed;
		return (&res);
	}
	if (a != NOT_FOUND) {
		switch (option) {
		case do_all:
			/* Remove the old appointment from the log */
			if ((status = rtable_transact_log(args, a, cm_remove)) != 0) {
				if (status == SPACEERROR)
					insert_internal(p_info, a);
				destroy_appt(a); 
				/* either CERROR or SPACEERROR */
				res.status = access_incomplete;
				return(&res);
			}
			if (is_repeater(a) && is_repeater(p_appt)) {
				if (lowerbound(CM_TICK(p_appt)) ==
				    lowerbound(p_key->tick) &&
				    a->period.period == p_appt->period.period)
				{
					/* keep the start day of the original
					 * appointment if the date of the
					 * key matches that of the new
					 * appointment and the interval
					 * is not changed
					 */
					CM_TICK(p_appt) -=
						lowerbound(CM_TICK(p_appt)) -
							lowerbound(CM_TICK(a));
				} else {
					/* otherwise, calculate new parent */
					if ((ordinal = in_repeater(p_key, a,
							TRUE)) == 0)
						return(&res);
					CM_TICK(p_appt) = first_tick(
						CM_TICK(p_appt),
						p_appt->period, ordinal);
				}

				/* We use the same exception list for the
				 * new appt.  ?? is this reasonable for
				 * all cases ??
				 */
				p_appt->exception = a->exception;
			}
			break;

		case do_forward:
			/* if repeating info is not changed, replace
			 * the deleted part with the new series, i.e.,
			 * ntimes of new series equals to the number
			 * of deleted instances. Also get new 
			 * exception list.	
			 */
			if (a->period.period == p_appt->period.period &&
			    a->period.nth == p_appt->period.nth &&
				a->ntimes == p_appt->ntimes) 
					p_appt->ntimes = remain;
			if (save_a != NULL) {
				p_appt = (Appt*)get_new_elist(p_appt, save_a, remain);
				destroy_appt(save_a);
			}
			break;
		}

		/* get start date */
		get_startdate(p_appt);

		/* keep the original author */
		if (p_appt->author)
			free(p_appt->author);
		p_appt->author = cm_strdup(a->author);

		if (option == do_all)
			/* Reuse the same key */
			KEY(p_appt) = p_key->key;
		else
			KEY(p_appt) = 0;

		if (insert_internal(p_info, p_appt) != rb_ok) {
			destroy_appt(a);
			return (&res);
		}

		/* Add the new appointment to the log */
		if ((status = rtable_transact_log(args, p_appt, add)) != 0) {
			res.status = access_incomplete;
			if (status == SPACEERROR) {
				if ((appt = delete_internal(p_info, source,
                                        &p_appt->appt_id)) != NULL && appt != NOT_FOUND)
                                        destroy_appt(appt);
				/* reinsert original appt that was previously deleted appt */
				if (insert_internal(p_info, a) != rb_ok) {
					destroy_appt(a);
					return (&res); 
				}
				/* return log file to original size */
				if (log != NULL) {
					if (file_size != 0) {
						if ((f = open(log, O_RDWR | O_APPEND | O_SYNC)) == -1) {
                                                	destroy_appt(a);
                                                	free(log); 
							return(&res);
						}
						free(log); log = NULL;
                                        	ftruncate(f, file_size);
                                        	close(f);
                                        }
                                }
                        }
			if (log != NULL) free(log);
			/* either CERROR or SPACEERROR */
			destroy_appt(a);
			return(&res);
		}
		else if (log != NULL) free(log);

		/* If the date/time is changed, we do a callback with the
		 * old and new appointments.  Otherwise, we only do callback
		 * with the new appointmnt.
		 */
		if (CM_TICK(p_appt) == CM_TICK(a))
			do_callbacks(src, p_appt, target, args->pid);
		else
		{
			a->next = p_appt;
			do_callbacks(src, a, target,args->pid);
			a->next = NULL;
		}

		p_info->modified = true;

		/* Return the new appointment. */
		/* need to copy the appointment because p_appt will
		 * be delete automatically by the dispatching routine
		 */
		res.res.Table_Res_List_u.a = copy_appt(p_appt);
		destroy_appt(a);
		res.status = access_ok;
		return (&res);
	}
	else if (save_a != NULL)
		destroy_appt(save_a);
	return (&res);
}

extern Table_Res *
rtable_change_instance_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;

	if (debug)
		fprintf(stderr, "rtable_change_instance_4 called\n");

	res.status = access_notsupported;
	res.res.tag = AP;
	res.res.Table_Res_List_u.a = NULL;

	return(&res);
}

extern Table_Status *
rtable_check_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Status s;
	Rt_Status rtstat;
	Info	*p_info;

	if (debug)
		fprintf(stderr, "rtable_check_4 called\n");

	if ((p_info = rtable_get_table (args->target)) == NULL)
	{
		if ((rtstat = rtable_load(args->target, svcrq, &p_info))!=rt_ok)
		{
			s = rtstat2tablestat(rtstat);
			return (&s);
		}
	}

	s = (Table_Status) rb_check_tree (APPT_TREE(p_info));
	if (s == rb_ok)
		s = (Table_Status) hc_check_list (REPT_LIST(p_info));
	return (&s);
}

extern int *
rtable_size_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	/* must be static! */
	static int size;
	Rt_Status rtstat;
	Info	*p_info;

	if (debug)
		fprintf(stderr, "rtable_size_4 called\n");

	size = 0;
	if ((p_info = rtable_get_table(args->target)) == NULL)
	{
		if ((rtstat = rtable_load(args->target, svcrq, &p_info))!=rt_ok)
			return(&size);
	}
	size = rb_size (APPT_TREE(p_info)) + hc_size (REPT_LIST(p_info));

	return(&size);
}

static Access_Status 
set_access_internal(p_info, e)
Info *p_info;
register Access_Entry *e;
{
	register Access_Entry	*q;

	/* Wipe out the old access lists. */
	destroy_access_list (GET_R_ACCESS (p_info));
	destroy_access_list (GET_W_ACCESS (p_info));
	destroy_access_list (GET_D_ACCESS (p_info));
	destroy_access_list (GET_X_ACCESS (p_info));
	SET_R_ACCESS(p_info, NULL);
	SET_W_ACCESS(p_info, NULL);
	SET_D_ACCESS(p_info, NULL);
	SET_X_ACCESS(p_info, NULL);

	/* Split the access list to 3 differnt operation lists */
	while (e != NULL)
	{
		if (e->access_type & access_read)
		{
			q = make_access_entry (e->who, e->access_type);
			q->next = GET_R_ACCESS(p_info);
			SET_R_ACCESS(p_info, q);
		}
		if (e->access_type & access_write)
		{
			q = make_access_entry (e->who, e->access_type);
			q->next = GET_W_ACCESS(p_info);
			SET_W_ACCESS(p_info, q);
		}
		if (e->access_type & access_delete)
		{
			q = make_access_entry (e->who, e->access_type);
			q->next = GET_D_ACCESS(p_info);
			SET_D_ACCESS(p_info, q);
		}
		if (e->access_type & access_exec)
		{
			q = make_access_entry (e->who, e->access_type);
			q->next = GET_X_ACCESS(p_info);
			SET_X_ACCESS(p_info, q);
		}
		e = e->next;
	}

	if (debug1) {
		Access_Entry *l;
		l = make_access_list(p_info);
		show_access_list (l);
		destroy_access_list (l);
	}

	return(access_ok);
}

/*
 * For parser only.
 */
extern Access_Status
rtable_set_access_internal(target, list, type)
char *target; Access_Entry *list; int type;
{
	Info	*p_info = NULL;

	p_info = rtable_get_table(target);
	if (p_info==NULL)
		return (access_failed);

	if (type == access_read)
		SET_R_ACCESS(p_info, list);
	else if (type == access_write)
		SET_W_ACCESS(p_info, list);
	else if (type == access_delete)
		SET_D_ACCESS(p_info, list);
	else if (type == access_exec)
		SET_X_ACCESS(p_info, list);

	return (access_ok);
}

extern Access_Status *
rtable_set_access_4(args, svcrq)
Access_Args *args;
struct svc_req *svcrq;
{
	/* must be static! */
	static Access_Status s;
	Rt_Status rtstat;
	char	*target, *source;
	char	*name = NULL;
	char	*log = NULL;
	Info	*p_info = NULL;
	Access_Entry *p = NULL;

	if (debug)
		fprintf(stderr, "rtable_set_access_4 called\n");

	s = access_other;

	if ((source = auth_check (svcrq)) == NULL)
		return (&s);

	if ((target = args->target) == NULL)
		return (&s);

	if ((p_info = rtable_get_table(target)) == NULL) {
		if ((rtstat = rtable_load(target, svcrq, &p_info)) != rt_ok) {
			s = rtstat2accessstat(rtstat);
			return(&s);
		}
        }

	/* Only the owner can set the access list */
 	if (!file_owner(rtable_get_owner(target), source) ||
		source_access_control(access_write, p_info, source, NULL) != access_ok) {
		s = access_failed;
		return(&s);
	}

	if ((name = cm_target2name(target)) == NULL)
		return(&s);
	if ((log = get_log(name)) == NULL) {
		free(name);
		return(&s);
	}
	p = args->access_list;

	/* Set to the data structure */
	if ((s = set_access_internal (p_info, p)) == access_ok)
	{
		int		type[5];
		Access_Entry	*list[5];

		/* Transact the log */
		type[0] = access_read;
		type[1] = access_write;
		type[2] = access_delete;
		type[3] = access_exec;
		type[4] = 0;
		list[0] = GET_R_ACCESS(p_info);
		list[1] = GET_W_ACCESS(p_info);
		list[2] = GET_D_ACCESS(p_info);
		list[3] = GET_X_ACCESS(p_info);
		list[4] = NULL;
		(void)append_access_log(log, type, list);

		p_info->modified = true;
	}
	else
	{
		if (debug1)
			fprintf(stderr, "%s: rtable_set_access failed.\n", pgname);
	}
	
	if (log != NULL) free(log);
	return(&s);
}

extern Access_Args *
rtable_get_access_4(args, svcrq)
Access_Args *args;
struct svc_req *svcrq;
{
	static Access_Args res;
	Rt_Status rtstat;
	Info	*p_info;
	char	*target, *source;
	Access_Entry *l;

	if (debug)
		fprintf(stderr, "rtable_get_access_4 called\n");

	xdr_free (xdr_Access_Args, (char*)&res);
	res.target = NULL;
	res.access_list = (Access_Entry *) NULL;

	if ((source = auth_check (svcrq)) == NULL)
		return (&res);

	/* Only the owner can get the access list */
	if (((target = args->target) == NULL) 
		/* || (strcmp (source, target) != 0) */) /* so can others */
		return (&res);

	if ((p_info = rtable_get_table(target)) == NULL) {
		if ((rtstat = rtable_load(target, svcrq, &p_info)) != rt_ok)
			return(&res);
        }

	res.target = cm_strdup(args->target); 
	res.access_list = make_access_list (p_info);
	if (debug1)
		show_access_list (res.access_list);

	return (&res);
}


static void
rtable_internal_keyrange(args,svcrq,add_list_func,res)
Table_Args *args;
struct svc_req *svcrq;
caddr_t	(*add_list_func) ();
Table_Res *res;
{
	Info		*p_info;
	Period		period;
	caddr_t		ilp = NULL;
	int		tmp_tick;
	Id		lo_key;
	char		*src;
	char		*source;
	Rt_Status	rtstat;
	Privacy_Level	p_level;
	register int	n;
	register Lnode	*p_lnode;
	register long	hi_tick;
	register int	tick;
	int	ordinal;
	register int	ntimes;
	register Appt	*p_appt;
	Keyrange	*p_range;


	res->status = access_other;
	if ((p_range = args->args.Args_u.keyrange) == NULL)
		return;

	if (((rtstat = check_read_access (args, svcrq, res, &src, &p_info))
			!= rt_ok) || p_info == NULL) {
		res->status = rtstat2accessstat(rtstat);
		return;
	}

	/*
	 * This function is optimized for repeating appointment.
	 */
	while (p_range != NULL)
	{
		lo_key.key = p_range->key;
		lo_key.tick = p_range->tick1;
		hi_tick = p_range->tick2;

		n = 0;

		/* Search from repeating appointments first for optimization */
		p_lnode = (Lnode *) hc_lookup_node (REPT_LIST(p_info), &lo_key);
		if (p_lnode != NULL) {
			/* Get the range of events from this appointment. */
			p_appt = (Appt*)APPT(p_lnode);
			ntimes = get_ninstance(p_appt);
			period = p_appt->period;

			/* calculate the last tick */
			if (p_lnode->lasttick == 0)
				p_lnode->lasttick = last_tick(CM_TICK(p_appt),
					p_appt->period, p_appt->ntimes);

			/* source will be set to NULL if it's the
			 * author of the appt
			 */
			source = src;
			if (p_lnode->lasttick > lo_key.tick &&
			    ((p_level = check_privacy_level(&source, p_appt))
					!= private))
			{
				if (debug1) nexttickcalled = 0;

				for (tick = closest_tick(lo_key.tick,
					CM_TICK(p_appt), period,
					&ordinal), ordinal--;
				     tick < hi_tick;
				     tick = next_tick(tick, period))
				{
					/* Repeater but beyond the scope */
					if (++ordinal > ntimes)
						break;
					if (tick <= lo_key.tick)
						continue;

					/* If not cancelled, add to linked list
					 */
					if (!marked_4_cancellation (p_appt,
							ordinal))
					{
						n++;

						/* Replace the parent key by
						 * the current tick for the
						 * repeating event
						 */
						tmp_tick = CM_TICK(p_appt);
						CM_TICK(p_appt) = tick; 

						/* Add to list, restore parent
						 * key
						 */
						ilp = (*add_list_func)(p_appt,
						      ilp, source, res->status,
						      p_level);
						CM_TICK(p_appt) = tmp_tick;
						if (ilp == NULL)
							break;
					}
				}
				if (debug1) {
				    fprintf(stderr,
					"%s: %d calls to next_tick\n",
					"rtable_internal_keyrange",
					nexttickcalled);
				}
			}
		} else {
			/* Check if it is in single appointment tree */
			while ((p_appt = (Appt *)
				rb_lookup_next_larger(APPT_TREE(p_info),
				&lo_key)) && (CM_TICK(p_appt) < hi_tick))
			{
				if (KEY(p_appt) != p_range->key) {
					lo_key.tick = CM_TICK(p_appt);
					lo_key.key = KEY(p_appt);
				} else {
					/* source will be set to NULL if
					 * it's the author of the appt
					 */
					source = src;
					if ((p_level = check_privacy_level(
						&source, p_appt)) != private)
					{
						n++;
						ilp = (*add_list_func)(p_appt,
						      ilp, source, res->status,
						      p_level);
					}
					break;
				}
			}
		}

		p_range = p_range->next;
	}

	if (debug1)
		fprintf (stderr, "Found %d entries in range lookup\n", n);

	if (res->res.tag == AP)
		res->res.Table_Res_List_u.a = (Appt *) ilp;
	else if (res->res.tag == AB)
		res->res.Table_Res_List_u.b = (Abb_Appt *) ilp;
	else /* impossible */
		res->status = access_other;
}

extern Table_Res *
rtable_abbreviated_lookup_key_range_4(args, svcrq)
Table_Args *args;
struct svc_req *svcrq;
{
	static	Table_Res res;

	if (debug)
		fprintf(stderr, "rtable_abbreviated_lookup_key_range_4 called\n");

	xdr_free (xdr_Table_Res, (char*)&res);

	res.res.tag = AB;
	res.res.Table_Res_List_u.b = NULL;

	rtable_internal_keyrange(args, svcrq, add_to_link_abbr, &res);

	return (&res);
}

extern void
garbage_collect ()
{
	unsigned remain;
	Tree_info *t = tree_list;

	while (t != NULL && t->info.modified == true)
	{
		if (debug)
			fprintf(stderr, "rpc.csmd: %s%s\n",
				"do garbage collection on ", t->calendar);
		collect_one(t->calendar);
		t->info.modified = false;
		t = t->next;
	}

	/* deregister stale client */
	cleanup_registration_list();

	if ((remain = (unsigned) alarm((unsigned) 0)) == 0)
		alarm ((unsigned) (3600L * 24L));
	else
		alarm (remain);
}

static void
debug_switch()
{
	Info	*p_info;
	Access_Entry *l;
	Tree_info *t = tree_list;

	debug = !debug;
	fprintf (stderr, "Debug Mode is %s\n", debug ? "ON" : "OFF");
	if (debug)
	{
		while (t != NULL)
		{
			p_info = rtable_get_table (t->calendar);
			print_reminder (p_info);
			if ((l = make_access_list (p_info)) != NULL)
			{
				show_access_list (l);
				destroy_access_list (l);
			}
			t = t->next;
		}
	}
}

void
init_alarm ()
{
	int n=0, midnight=0, fouram=0, next=0;

	n = now();
	midnight = next_ndays(n, 1);
	fouram = next_nhours(midnight, 4);
	next = fouram-n;
#ifdef SVR4
	sigset(SIGUSR1, garbage_collect);
	sigset(SIGALRM, garbage_collect);
	sigset(SIGUSR2, debug_switch);
#else
	signal(SIGUSR1, garbage_collect);
	signal(SIGALRM, garbage_collect);
	signal(SIGUSR2, debug_switch);
#endif /* SVR4 */

	alarm((unsigned) next);
}


static char *
c_time(tick)
int	tick;
{
	char	*p_time;

	/* p_time = (char *) ctime (&tick);*/
	p_time = (char *) ctime ((time_t *)(&tick));
	p_time[24] = '\0';
	return (p_time);
}

static
print_reminder (p_info)
Info	*p_info;
{
	Rm_que	*p_node;
	char	what [256];
	char	*temp=NULL;

	if (p_info == NULL)
		return;
	fprintf (stderr, "--- Active Reminder Queue ---\n");
	p_node = RMND_QUEUE(p_info);
	while (p_node != NULL)
	{
		cm_strcpy (what, p_node->appt->what);
		temp = strchr(what, '\n');
		if (temp!=NULL) temp='\0';
	/*	*(strchr (what, '\n')) = '\0'; */
		fprintf (stderr, "%s (%d) %s: %s\n", c_time(p_node->remind_at),
			p_node->remind_ord, p_node->attr->attr, what);
		p_node = p_node->next;
	}
}

extern long *
rtable_gmtoff_4(svcrq)
struct svc_req *svcrq;
{
	static long gmtoff;

	if (debug)
		fprintf(stderr, "rtable_gmtoff_4 called\n");

	gmtoff = gmt_off();
	return(&gmtoff);
}

void initrtable4(ph)
        program_handle ph;
{
        ph->program_num = TABLEPROG;
        ph->prog[TABLEVERS].vers = &tableprog_4_table[0];
        ph->prog[TABLEVERS].nproc = sizeof(tableprog_4_table)/sizeof(tableprog_4_table[0]);
}

extern void *
rtable_ping_4(svcrq)
struct svc_req *svcrq;
{
	char dummy;

	if (debug)
		fprintf(stderr, "rtable_ping_4 called\n");

	return((void *)&dummy); /* for RPC reply */
}

extern Table_Status *
rtable_create_4(args, svcrq)
Table_Op_Args *args;
struct svc_req *svcrq;
{
	static	Table_Status res;
	Info	*p_info;
	int	err;
	char	*source;
	char	*calname;
	char	*log;
	char	*ptr;
	char	*id, *user;

	if (debug)
		fprintf(stderr, "rtable_create_4 called\n");

	res = other;
	if ((source = auth_check (svcrq)) == NULL)
		return(&res);

	/* only user in the local domain can create file */
	/*
         * dont check domain since this version of cm doesn't
         * do real authentication across domains anyway
	if (ptr = strchr(source, '.')) {
		if (debug)
                        fprintf(stderr, "rpc.cmsd: %s %s and %s\n",
                                "check domains, comparing",
                                cm_get_local_domain(),
                                (ptr+1));
		if (!same_path(cm_get_local_domain(), ++ptr)) {
			res = denied;
			return(&res);
		}
	}
	*/

	if ((calname = cm_target2name(args->target, '@')) == NULL)
		return(&res);

	/* if the file is loaded in memory, the file already exists */
	if ((p_info = rtable_get_table(calname)) != NULL) {
		res = tbl_exist;
		free(calname);
		return(&res);
	}

	/*
	 * If identifier of the calendar name is a user name,
	 * make sure it's the same as sender.
	 * format of calendar name assumed: identifier.name
	 */
	if ((id = get_head(calname, '.')) == NULL) {
		free(calname);
		return(&res);
	}
	if ((user = get_head(source, '@')) == NULL) {
		free(calname);
		free(id);
		return(&res);
	}

	if (getpwnam(id) && strcmp(user, id)) {
		free(calname);
		free(id);
		free(user);
		res = denied;
		endpwent();
		return(&res);
	}
	free(id);

	if ((log = get_log(calname)) == NULL)
	{
		free(calname);
		free(user);
		endpwent();
		return(&res);
	}

	if ((err = create_log(user, log)) == FILEERROR)
		res = tbl_exist;
	else if (err != 0)
		res = other;
	else
		res = ok;

	free(user);
	free(calname);
	free(log);
	endpwent();
	return (&res);
}

extern Table_Status *
rtable_remove_4(args, svcrq)
Table_Op_Args *args;
struct svc_req *svcrq;
{
	static	Table_Status res;

	res = tbl_notsupported;
	return (&res);
}

extern Table_Status *
rtable_rename_4(args, svcrq)
Table_Op_Args *args;
struct svc_req *svcrq;
{
	static	Table_Status res;

	res = tbl_notsupported;
	return (&res);
}
