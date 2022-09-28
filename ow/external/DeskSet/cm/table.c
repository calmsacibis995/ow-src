#ifndef lint
static  char sccsid[] = "@(#)table.c 3.31 93/12/06 Copyr 1991 Sun Microsystems, Inc.";
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
#include <fcntl.h>
#include <pwd.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#ifdef SVR4
#include <netdb.h>   /* define MAXHOSTNAMELEN */
#include <sys/systeminfo.h>
#endif "SVR4"
#include "util.h"
#include "timeops.h"
#include "rtable2.h"
#include "rtable3.h"
#include "rtable4.h" 
#include "convert4-2.h"
#include "convert2-4.h"
#include "convert4-3.h"
#include "convert3-4.h"
#include "agent.h"
#include "table.h"
#include "abbcache.h"

#define DEFAULT_TIMEOUT	5
#define INITIAL_TIMEOUT 10

/* must add one second here to compensate for the goofy backend
weirdness that requries your range values to surround the actual
lookup range that you are interested in. */
#define MAXDAY_LOOKUP	 15
#define MAXDAY_LOOKUP_SECS  ((daysec*MAXDAY_LOOKUP)+1) 

#define MAXTCP_COUNT	40

typedef enum Transport_type
{
	tcp_transport,
	udp_transport
} Transport_Type;

typedef struct targetlist {
	char *target;
	struct targetlist *next;
} Target_List;

typedef struct cl_info {
	char	*host;
	CLIENT	*tcpcl;
	CLIENT	*udpcl;
	u_long	vers_out;
	int	nregistered;
	Target_List *tlist;
	Boolean	retry;
	Transport_Type use;
	long	last_used;
	struct cl_info *next;
	struct cl_info *prev;
} Client_Info;

extern int errno;
extern int debug;
extern u_long transient;
struct timeval timeout_tv;
struct timeval retry_tv;
static AUTH *unix_credential = NULL;	/* always cache it */

static char *errfmt = "cm: %s() unexpected return status %d.\n";
static char *svcfmt = "Error on server %s\n";
static char *errmsg;

static tcp_count = 0;
static Client_Info *client_cache_head = NULL;
static Client_Info *client_cache_tail = NULL;

/* Minimum Size of abbcache in days.  Default to 3 times
lookup range to optimize the rpc calls and to 
handle weeks which cross month bounderies */
static int	abbrev_cache_size = 3*MAXDAY_LOOKUP;

static Register_Status
regstat_2_registerstatus(stat)
	Registration_Status stat;
{
	switch (stat) {
	case registered:
		return(register_succeeded);
	case deregistered:
		return(de_registered);
	case confused:
		return(register_confused);
	case reg_notable:
		return(register_notable);
	case failed:
	default:
		return(register_failed);
	}
}

static char *
get_credentials()
{
	char *name, *host;
	static char *login = NULL;

	if (login==NULL)
	{
		name = (char*)cm_get_uname();
		host = (char*)cm_get_local_host();
		login = (char *) ckalloc (cm_strlen(name) + cm_strlen(host) + 2);
		sprintf(login, "%s@%s", name, host);
	}
	return (login);
}

static void
create_auth(cl)
CLIENT *cl;
{
	/* Always cache the Unix style credentials. */
	if (unix_credential == NULL)
#ifdef SVR4
		unix_credential = authsys_create_default ();
#else
		unix_credential = authunix_create_default ();
#endif

	cl->cl_auth = unix_credential;
}

static void
destroy_auth(cl)
CLIENT *cl;
{
	/* It is a no-op for unix-authentication because we always cache it.
	 * But we have to destroy it when secure RPC is used.
	 */
}

/*
 * Given a host name, find the Client_Info structure which contains
 * both udp and tcp handle to the server running in the host.
 */
static Client_Info *
get_client_info(host)
	char *host;
{
	Client_Info *ci;
	int result;

	if (host==NULL) return(NULL);
	for (ci = client_cache_head; ci != NULL; ci = ci->next) {
		if ((result = strcmp(ci->host, host)) == 0) 
			return(ci);
		else if (result > 0)
			break;
	}
	return(NULL);
}

static void
destroy_target_list(tlist)
	Target_List *tlist;
{
	Target_List *listp, *listitem;

	for (listp = tlist; listp != NULL; ) {
		listitem = listp;
		listp = listp->next;

		if (listitem->target)
			free(listitem->target);
		free(listitem);
	}
}

static void
destroy_client_info(ci)
	Client_Info *ci;
{
	if (ci==NULL) return;

	if (ci->host != NULL)
		free(ci->host);
	if (ci->tcpcl) {
		destroy_auth(ci->tcpcl);
		clnt_destroy(ci->tcpcl);
		tcp_count--;
	}
	if (ci->udpcl) {
		destroy_auth(ci->udpcl);
		clnt_destroy(ci->udpcl);
	}
	destroy_target_list(ci->tlist);
	free(ci);
}

/*
 * Dont limit the number of cached connections right now.
 * Udp client handle does not use up file descriptor only space.
 * Tcp client handle is kept open only when there's at least one
 * calendar registered with the host and the user probably won't
 * be browsing more than 50 calendar at the same time.
 */
static void
insert_client_info(ci)
	Client_Info *ci;
{
	Client_Info *citem;

	/* insert new item alphabetically */
	for (citem = client_cache_head; citem != NULL; citem = citem->next) {
		/* there shouldn't be an entry with the same host name
		 * if there's, it would be picked up in get_client_info()
		 */
		if (strcmp(citem->host, ci->host) > 0)
			break;
	}

	if (citem == NULL) {
		if (client_cache_head == NULL)
			client_cache_head = client_cache_tail = ci;
		else {
			ci->prev = client_cache_tail;
			client_cache_tail->next = ci;
			client_cache_tail = ci;
		}
	} else {
		ci->next = citem;
		ci->prev = citem->prev;
		if (citem == client_cache_head)
			client_cache_head = ci;
		else
			citem->prev->next = ci;
		citem->prev = ci;
	}

	if (debug) {
		fprintf(stderr, "%s: head = %d, tail = %d, newitem = %d\n",
			"insert_client_info", client_cache_head,
			client_cache_tail, ci);
	}
}

/*
 * remove the client info structure from the list
 */
static void
delete_client_info(oldci)
	Client_Info *oldci;
{
	int result;

	if (oldci == NULL) return;

	if (oldci == client_cache_head) {
		client_cache_head = oldci->next;
		if (client_cache_head)
			client_cache_head->prev = NULL;
	} else if (oldci == client_cache_tail) {
		client_cache_tail = oldci->prev;
		if (client_cache_tail)
			client_cache_tail->next = NULL;
	} else {
		oldci->prev->next = oldci->next;
		oldci->next->prev = oldci->prev;
	}

	if (oldci == client_cache_tail)
		client_cache_tail = NULL;

	destroy_client_info(oldci);

	if (debug) {
		fprintf(stderr, "%s: head = %d, tail = %d, olditem = %d\n",
			"delete_client_info", client_cache_head,
			client_cache_tail, oldci);
	}
}

/*
 * Number of open tcp connections reaches the maximum.
 * This is very unlikely in the normal case since
 * a tcp connection is kept open if at least one calendar
 * is registered with the host and a user would not be
 * browsing a large number of calendars at one time.
 * However, when a calendar is deselected in the calendar
 * list on the multi-browser window, a lookup call using
 * the tcp connection is made after the calendar is
 * deregistered.  This keeps the tcp connection open
 * even if that's the last calendar registered with the
 * host.  This routine is used to clean up such tcp connections.
 * This is a good time to clean up connections that are not
 * used for a long time.
 */
static void
cleanup_tcp_connection(dontclose)
	Client_Info *dontclose;
{
	Client_Info *ci, *oldci;
	long thismoment = now();
	int total = 0, deleted = 0;

	for (ci = client_cache_head; ci != NULL; )
	{
		total++;
		if (ci != dontclose &&
		    ((ci->tcpcl && ci->nregistered == 0) ||
		     (ci->tcpcl==NULL && (thismoment-ci->last_used)>daysec)))
		{
			deleted++;
			oldci = ci;
			ci = ci->next;
			delete_client_info(oldci);
		} else
			ci = ci->next;
	}
	if (debug)
		fprintf(stderr, "%s: total = %d, deleted = %d\n",
			"cleanup_tcp_connection", total, deleted);
}

static Register_Status
deregister_target(ci, target)
	Client_Info *ci; char *target;
{
	Registration_Status stat;
	Boolean nullreturned = false;

	switch(ci->vers_out) {
		Registration_2 r2;
		Registration_3 r3;
		Registration r4;
		Registration_Status_2 *stat2;
		Registration_Status_3 *stat3;
		Registration_Status *stat4;
	case TABLEVERS_2:
		r2.target = target;
        	r2.prognum = transient;
        	r2.versnum = AGENTVERS;
        	r2.procnum = update_callback;
		r2.next = NULL;
		stat2 = deregister_callback_2(&r2, ci);
		if (stat2 != NULL)
			stat = regstat2_to_regstat4(*stat2);
		else
			nullreturned = true;
		break;
	case TABLEVERS_3:
		r3.target = target;
        	r3.prognum = transient;
        	r3.versnum = AGENTVERS;
        	r3.procnum = update_callback;
		r3.next = NULL;
		r3.pid = getpid();
		stat3 = deregister_callback_3(&r3, ci);
		if (stat3 != NULL)
			stat = regstat3_to_regstat4(*stat3);
		else
			nullreturned = true;
		break;
	case TABLEVERS:
		r4.target = target;
        	r4.prognum = transient;
        	r4.versnum = AGENTVERS;
        	r4.procnum = update_callback;
		r4.next = NULL;
		r4.pid = getpid();
		stat4 = deregister_callback_4(&r4, ci);
		if (stat4 != NULL)
			stat = *stat4;
		else
			nullreturned = true;
		break;
	default:
		stat = failed;
	}

	if (nullreturned)
		return(register_rpc);
	else
		return(regstat_2_registerstatus(stat));
}

static Register_Status
register_target(ci, target)
	Client_Info *ci; char *target;
{
	Registration_Status stat;
	Boolean nullreturned = false;

	switch(ci->vers_out) {
		Registration_2 r2;
		Registration_3 r3;
		Registration r4;
		Registration_Status_2 *stat2;
		Registration_Status_3 *stat3;
		Registration_Status *stat4;
	case TABLEVERS_2:
		r2.target = target;
        	r2.prognum = transient;
        	r2.versnum = AGENTVERS;
        	r2.procnum = update_callback;
		r2.next = NULL;
		stat2 = register_callback_2(&r2, ci);
		if (stat2 != NULL)
			stat = regstat2_to_regstat4(*stat2);
		else
			nullreturned = true;
		break;
	case TABLEVERS_3:
		r3.target = target;
        	r3.prognum = transient;
        	r3.versnum = AGENTVERS;
        	r3.procnum = update_callback;
		r3.next = NULL;
		r3.pid = getpid();
		stat3 = register_callback_3(&r3, ci);
		if (stat3 != NULL)
			stat = regstat3_to_regstat4(*stat3);
		else
			nullreturned = true;
		break;
	case TABLEVERS:
		r4.target = target;
        	r4.prognum = transient;
        	r4.versnum = AGENTVERS;
        	r4.procnum = update_callback;
		r4.next = NULL;
		r4.pid = getpid();
		stat4 = register_callback_4(&r4, ci);
		if (stat4 != NULL)
			stat = *stat4;
		else
			nullreturned = true;
		break;
	default:
		stat = failed;
	}

	if (nullreturned)
		return(register_rpc);
	else
		return(regstat_2_registerstatus(stat));
}

/*
 * check registration
 * Deergister the first target:
 * if it succeeded, the old server is still running, just re-register it;
 * else assume that it's a new server so re-register the whole list again.
 */
static void
check_registration(ci)
	Client_Info *ci;
{
	Register_Status stat;
	Target_List *listp, *prev;
	Transport_Type olduse;

	if (ci->tlist == NULL)
		return;

	olduse = ci->use;
	ci->use = udp_transport;
	ci->retry = false;
	if (deregister_target(ci, ci->tlist->target) == de_registered) {
		if (register_target(ci,ci->tlist->target)!=register_succeeded) {
			ci->nregistered--;
			listp = ci->tlist;
			ci->tlist = listp->next;
			free(listp->target);
			free(listp);
		}
	} else {
		for (listp = prev = ci->tlist; listp != NULL; ) {
			if (register_target(ci, listp->target)
				!= register_succeeded)
			{
				ci->nregistered--;
				if (listp == prev)
					ci->tlist = prev = listp->next;
				else
					prev->next = listp->next;
				/* free target item */
				free(listp->target);
				free(listp);
				listp = (prev ? prev->next : NULL);
			} else {
				prev = listp;
				listp = listp->next;
			}
		}
	}
	ci->use = olduse;
}

static Client_Info *
get_new_client_handle(oldci)
	Client_Info *oldci;
{
	CLIENT *cl;
	enum clnt_stat status;
	int ver;

	if (oldci == NULL) return(NULL);

	/* always get a udp client handle first */
	cl = clnt_create_vers(oldci->host, TABLEPROG, &(oldci->vers_out),
			TABLEVERS_2, TABLEVERS, "udp");
	if (cl == NULL) {
		if (debug)
			clnt_pcreateerror(oldci->host);
		delete_client_info(oldci);
		return(NULL);
	} else {
		create_auth(cl);

		/* adjust timeout */
		timeout_tv.tv_sec = INITIAL_TIMEOUT;
		timeout_tv.tv_usec = 0;
		clnt_control(cl, CLSET_TIMEOUT, (char *)&timeout_tv);
		retry_tv.tv_sec = INITIAL_TIMEOUT + 10;
		retry_tv.tv_usec = 0;
		clnt_control(cl, CLSET_RETRY_TIMEOUT, (char *)&retry_tv);

		destroy_auth(oldci->udpcl);
		clnt_destroy(oldci->udpcl);
		oldci->udpcl = cl;
	}

	/* check registration */
	/* if there's anything wrong, nregistered could be zero */
	check_registration(oldci);

	/* now deal with tcp handle */

	/* get rid of old handle first */
	if (oldci->tcpcl) {
		destroy_auth(oldci->tcpcl);
		clnt_destroy(oldci->tcpcl);
		tcp_count--;
		oldci->tcpcl = NULL;
	}

	if (oldci->use == udp_transport) {
		return(oldci);
	} else {

		/* get a tcp client handle */
		ver = oldci->vers_out;
		cl = clnt_create_vers(oldci->host, TABLEPROG,
			&(oldci->vers_out), TABLEVERS_2, TABLEVERS, "tcp");

		if (cl == NULL) {
			if (debug)
				clnt_pcreateerror(oldci->host);
			oldci->vers_out = ver;
			return(NULL);
		} else {
			create_auth(cl);

			/* adjust timeout */
			timeout_tv.tv_sec = INITIAL_TIMEOUT;
			timeout_tv.tv_usec = 0;
			clnt_control(cl, CLSET_TIMEOUT, (char *)&timeout_tv);

			oldci->tcpcl = cl;
			tcp_count++;
			return(oldci);
		}
	}
}

static void
incr_reg_list(ci, target)
	Client_Info *ci; char *target;
{
	Target_List *listp, *prev;
	Target_List *listitem;
	char *host;
	int result;

	if (target == NULL) return;

	for (listp = prev = ci->tlist; listp != NULL;
	     prev = listp, listp = listp->next) {
		if ((result = strcmp(listp->target, target)) == 0)
			/* registered already */
			return;
		else if (result > 0)
			break;
	}

	/* register the first time, insert in list in ascending order */
	listitem = (Target_List *)ckalloc(sizeof(Target_List));
	listitem->target = cm_strdup(target);

	if (prev == NULL || listp == prev)
		ci->tlist = listitem;
	else
		prev->next = listitem;
	listitem->next = listp;

	ci->nregistered++;
}

static void
decr_reg_list(ci, target)
	Client_Info *ci; char *target;
{
	Target_List *listp, *prev;
	int result;

	if (target == NULL) return;

	/* if found, just increment the number of registration */
	for (listp = prev = ci->tlist; listp != NULL;
	     prev = listp, listp = listp->next) {
		if ((result = strcmp(listp->target, target)) == 0) {
			if (listp == prev)
				ci->tlist = listp->next;
			else
				prev->next = listp->next;

			/* free target item */
			free(listp->target);
			free(listp);

			/* if no calendar is registered, close tcp connection */
			if (--(ci->nregistered) == 0) {
				if (ci->tcpcl) {
					destroy_auth(ci->tcpcl);
					clnt_destroy(ci->tcpcl);
					ci->tcpcl = NULL;
					tcp_count--;
				}
			}
			return;
		} else if (result > 0)
			break;
	}
	/* not found; impossible */
}

static Client_Info *
create_udp_client(host, timeout)
	char *host; int timeout;
{
	Client_Info *ci;
	u_long vers_out;
	CLIENT *cl=NULL;

	if (host == NULL) return((Client_Info *)NULL);

	/* if client info is found, we have at least the udp handle */
	if ((ci = get_client_info(host)) != NULL) {
		ci->use = udp_transport;
		return(ci);
	}

	cl = clnt_create_vers(host, TABLEPROG, 
		&vers_out, TABLEVERS_2, TABLEVERS, "udp");
	if (cl==NULL) {
		if (debug)
			clnt_pcreateerror(host);
		return((Client_Info *)NULL);
	}
	create_auth(cl);

	/* Adjust Timeout */
	if (timeout==0) timeout = DEFAULT_TIMEOUT;
	timeout_tv.tv_sec =  timeout;
	timeout_tv.tv_usec = 0;
	clnt_control(cl, CLSET_TIMEOUT, (char*)&timeout_tv);		

	/*	UDP only!
		time rpc waits for server to reply before retransmission =
		'timeout'. since the retry timeout is set to timeout + 10;
		this guarantees there won't
		be any retransmisssions resulting in duplicate 
		transactions in the database.
	*/

	retry_tv.tv_sec =  timeout + 10;
	retry_tv.tv_usec = 0;
	clnt_control(cl, CLSET_RETRY_TIMEOUT, (char*)&retry_tv);

	ci  = (Client_Info *)ckalloc(sizeof(Client_Info));
	ci->host = cm_strdup(host);
	ci->udpcl = cl;
	ci->vers_out = vers_out;
	ci->use = udp_transport;
	insert_client_info(ci);
	return(ci);
}

/*
 * Creates tcp client handle.  Used for calls that potentially return
 * large amount of data.  If it fails to create a tcp client handle,
 * a udp client handle will be returned.
 */
static Client_Info *
create_tcp_client(host, timeout)
	char *host; int timeout;
{
	Client_Info *ci;
	u_long vers_out;
	CLIENT *cl=NULL;

	if (host == NULL)
		return((Client_Info *)NULL);

	/* Get a udp client handle.  This serves two purposes:	       	   */ 
	/* - to get a udp handle for an old server which talks only udp    */
	/* - to invoke a server through inetd since only udp is registered.*/
	if ((ci = create_udp_client(host, timeout)) == NULL)
		return((Client_Info *)NULL);
	else if (ci->tcpcl) {
		ci->use = tcp_transport;
		return(ci);
	} else {
		/* create tcp connection */
		cl = clnt_create_vers(host, TABLEPROG, &vers_out,
			TABLEVERS_2, TABLEVERS, "tcp");

		/* if can't create tcp connection, use udp */
		if (cl==NULL) {
			if (debug)
				clnt_pcreateerror(host);
			return(ci);
		}
		create_auth(cl);

		/* Adjust Timeout */
		if (timeout==0) timeout = DEFAULT_TIMEOUT;
		timeout_tv.tv_sec =  timeout;
		timeout_tv.tv_usec = 0;
		clnt_control(cl, CLSET_TIMEOUT, (char*)&timeout_tv);		

		/* dont need to set vers_out since it should
		 * be the same as that of the udp transport
		 */
		ci->tcpcl = cl;
		ci->use = tcp_transport;
		if (++tcp_count == MAXTCP_COUNT)
			/* clean up tcp connections */
			cleanup_tcp_connection(ci);
		return(ci);
	}
}

/*
 * Used instead of clnt_call by rtableX_clnt.c
 */
extern enum clnt_stat
my_clnt_call(clnt, proc, inproc, in, outproc, out, tout)
	Client_Info *clnt;
	u_long proc;
	xdrproc_t inproc;
	caddr_t in;
	xdrproc_t outproc;
	caddr_t out;
	struct timeval tout;
{
	enum clnt_stat status = RPC_FAILED;
	char errbuffer[100];
	int retry = 0;

	if (clnt == NULL)
		goto done;
	else if (clnt->retry)
		/* give one retry for now */
		retry = 1;

	clnt->last_used = now();
	for (;;) {
		status = clnt_call((clnt->use == tcp_transport ? clnt->tcpcl :
				clnt->udpcl), proc, inproc, in,
				outproc, out, tout);

		if (status != RPC_SUCCESS) {
			sprintf(errbuffer, svcfmt, clnt->host);
                	errmsg = clnt_sperror((clnt->use == tcp_transport ?
				clnt->tcpcl : clnt->udpcl), errbuffer);
		}

		if (retry &&
		    ((clnt->use == udp_transport && status == RPC_TIMEDOUT) ||
		     (status == RPC_CANTRECV))) {

			/* don't retry when stat is RPC_TIMEDOUT and
			 * transpart is tcp since if the server is down,
			 * stat would be something else like RPC_CANTRECV
			 */

			/* get new client handle */
			if ((clnt = get_new_client_handle(clnt)) == NULL)
				goto done;

			retry--;
		} else
			goto done;
	}
done:
	return status;
}

/*
 * table api's.
 */

extern char *
table_get_credentials()
{
	return (cm_strdup (get_credentials()));
}

extern int
table_ping(host)
	char *host;
{
	int dead=0;
	Client_Info *ci=NULL;
	enum clnt_stat stat;
	struct timeval tv;

	if (host==NULL) return(dead);
	ci = create_udp_client(host, INITIAL_TIMEOUT);
	if (ci==NULL) return(dead);
	tv.tv_sec=3;
	tv.tv_usec=0;
	clnt_control(ci->udpcl, CLSET_TIMEOUT, (char*)&tv);
	stat = clnt_call(ci->udpcl, 0, xdr_void, (char*)NULL,
				xdr_void, (char *)NULL, tv);

	/* reset timeout */
	tv.tv_sec = INITIAL_TIMEOUT;
	tv.tv_usec = 0;
	clnt_control(ci->udpcl, CLSET_TIMEOUT, (char*)&tv);
	if (stat != RPC_SUCCESS) return(dead);
	return(!dead);
}

extern Stat
table_lookup(target, key, r)
	char *target; Uid *key; Appt **r;
{
	Table_Res *res=NULL;
	Stat stat;
	Client_Info *ci = NULL;
	char *host = NULL;

	if (debug)
		fprintf(stderr, "call to table_lookup()\n");

	if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
		return(status_param);
	*r = NULL;

	ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
	switch(ci->vers_out) {
		Table_Args_2 a2;
		Table_Args_3 a3;
		Table_Args a4;
                Table_Res_2 *res2;
		Table_Res_3 *res3;
	case TABLEVERS_2:
		a2.target = target;
        	a2.args.tag = UID_2;
        	a2.args.Args_2_u.key = uid4_to_uid2(key);
        	res2 = rtable_lookup_2(&a2, ci);
		res = tableres2_to_tableres4(res2);
		if (a2.args.Args_2_u.key != NULL)
			xdr_free(xdr_Uid_2, (char *)a2.args.Args_2_u.key);
		if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
		break;
	case TABLEVERS_3:
		a3.target = target;
        	a3.args.tag = UID_3;
        	a3.args.Args_3_u.key = uid4_to_uid3(key);
		a3.pid = getpid();
        	res3 = rtable_lookup_3(&a3, ci);
		res = tableres3_to_tableres4(res3);
		if (a3.args.Args_3_u.key != NULL)
			xdr_free(xdr_Uid_3, (char *)a3.args.Args_3_u.key);
		if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
		break;
	case TABLEVERS:
		a4.target = target;
        	a4.args.tag = UID;
        	a4.args.Args_u.key = key;
		a4.pid = getpid();
        	res = rtable_lookup_4(&a4, ci);
		break;
	default:
		free(host);
		return(status_other);
	}

	if (res != NULL) {
        	switch(res->status) {
        	case access_ok:
		case access_failed:
                	*r = copy_appt(res->res.Table_Res_List_u.a);
			stat = status_ok;
			break;
		case access_other:
			stat = status_param;
			break;
		case access_notable:
			stat = status_notable;
			break;
        	default:
                	/* remote system error */
                	if (debug)
                        	fprintf(stderr, errfmt,
					"table_lookup", res->status);
			stat = status_other;
			break;
        	}
		xdr_free(xdr_Table_Res, (char*)res);
	}
	else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
	}

	free(host);
	return(stat);
}

extern Stat
table_lookup_next_larger(target, id, r)
	char *target; Id *id; Appt **r;
{
	Table_Res *res=NULL;
	Stat stat;
	Client_Info *ci = NULL;
	char *host;
	Uid uid;

	if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
		return(status_param);
	*r = NULL;

	ci = create_udp_client(host, INITIAL_TIMEOUT);
	if (ci==NULL) {
		free(host);
		return(status_rpc);
	}

	ci->retry = true;
	switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = TICK_2;
                a2.args.Args_2_u.tick = id->tick;
                res2 = rtable_lookup_next_larger_2(&a2, ci);
                res = tableres2_to_tableres4(res2);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = TICK_3;
                a3.args.Args_3_u.tick = id->tick;
		a3.pid = getpid();
                res3 = rtable_lookup_next_larger_3(&a3, ci);
                res = tableres3_to_tableres4(res3);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
		uid.appt_id = *id;
		uid.next = NULL;
		a4.target = target;
		a4.args.tag = UID;
		a4.args.Args_u.key = &uid;
		a4.pid = getpid();
                res = rtable_lookup_next_larger_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }

        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_failed:
                        *r = copy_appt(res->res.Table_Res_List_u.a);
                        stat = status_ok;
                        break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_lookup_next_larger", res->status);
                        stat = status_other;
                        break;
                }
		xdr_free(xdr_Table_Res, (char*)res);
        }
        else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
        }
        free(host);
        return(stat);
}

extern Stat
table_lookup_next_smaller(target, id, r)
	char *target; Id *id; Appt **r;
{
        Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host;
	Uid uid;

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);
        *r = NULL;

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
        switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = TICK_2;
                a2.args.Args_2_u.tick = id->tick;
                res2 = rtable_lookup_next_smaller_2(&a2, ci);
                res = tableres2_to_tableres4(res2);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = TICK_3;
                a3.args.Args_3_u.tick = id->tick;
		a3.pid = getpid();
                res3 = rtable_lookup_next_smaller_3(&a3, ci);
                res = tableres3_to_tableres4(res3);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
		uid.appt_id = *id;
		uid.next = NULL;
                a4.target = target;
                a4.args.tag = UID;
                a4.args.Args_u.key = &uid;
		a4.pid = getpid();
                res = rtable_lookup_next_smaller_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }
        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_failed:
                        *r = copy_appt(res->res.Table_Res_List_u.a);
                        stat = status_ok;
                        break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_lookup_next_smaller", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);
        }
        else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
        }
        free(host);
        return(stat);
}

extern Stat
table_lookup_range(target, range, r)
	char *target; Range *range; Appt **r;
{
        Table_Res *res=NULL;
        Stat stat=status_ok;
        Client_Info *ci = NULL;
        char *host;
	Range new_range;
	Appt *last_r=NULL, *tmp_r=NULL, *r_ptr=NULL;
 
	if (debug)
		fprintf(stderr, "call to table_lookup_range()\n");

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);

        *r = NULL;
 
        ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }
 
	ci->retry = true;
	new_range.key1 = range->key1;
        new_range.key2 = range->key2;
        new_range.next = NULL;
        if (ci->use == udp_transport)
                if ((new_range.key1 + MAXDAY_LOOKUP_SECS) < range->key2)
                        new_range.key2 = new_range.key1 + MAXDAY_LOOKUP_SECS;

	do {
		switch(ci->vers_out) {
			Table_Args_2 a2;
			Table_Args_3 a3;
			Table_Args a4;
			Table_Res_2 *res2;
			Table_Res_3 *res3;
		case TABLEVERS_2:
			a2.target = target;
			a2.args.tag = RANGE_2;
			a2.args.Args_2_u.range = range4_to_range2(&new_range);
			res2 = rtable_lookup_range_2(&a2, ci);
			res = tableres2_to_tableres4(res2);
			if (a2.args.Args_2_u.range != NULL)
				xdr_free(xdr_Range_2,
					(char *)a2.args.Args_2_u.range);
			if (res2 != NULL)
				xdr_free(xdr_Table_Res_2, (char*)res2);
			break;
		case TABLEVERS_3:
			a3.target = target;
			a3.args.tag = RANGE_3;
			a3.args.Args_3_u.range = range4_to_range3(&new_range);
			a3.pid = getpid();
			res3 = rtable_lookup_range_3(&a3, ci);
			res = tableres3_to_tableres4(res3);
			if (a3.args.Args_3_u.range != NULL)
				xdr_free(xdr_Range_3,
					(char *)a3.args.Args_3_u.range);
			if (res3 != NULL)
				xdr_free(xdr_Table_Res_3, (char*)res3);
			break;
		case TABLEVERS:
			a4.target = target;
			a4.args.tag = RANGE;
			a4.args.Args_u.range = &new_range;
			a4.pid = getpid();
			res = rtable_lookup_range_4(&a4, ci);
			break;
		default:
			free(host);
			return(status_other);
		}

		if (res!=NULL) {
			switch(res->status) {
			case access_ok:
			case access_failed:
				tmp_r = copy_appt(res->res.Table_Res_List_u.a);
				stat = status_ok;
				if (ci->use == tcp_transport)
                                        *r = tmp_r;
                                else { /* udp transport */
                                        if (*r == NULL)
                                                *r = last_r = tmp_r;
                                        else {
                                                /* traverse from the last res ptr */
                                                for (r_ptr = last_r; r_ptr != NULL && 
							r_ptr->next != NULL; r_ptr = r_ptr->next);
						if (tmp_r != NULL)
                                                	r_ptr->next = last_r = tmp_r;
						else
							last_r = r_ptr;
                                        }
                                }
				break;
			case access_other:
				stat = status_param;
				break;
			case access_notable:
				stat = status_notable;
				break;
			default:
				/* remote system error */
				if (debug)
                                fprintf(stderr, errfmt,
                                        "table_lookup_range", res->status);
				stat = status_other;
				break;
			}
			xdr_free(xdr_Table_Res, (char*)res); res = NULL;
			/* range must encompass interested ticks */
			new_range.key1 = new_range.key2-1;
                        if ((new_range.key2 = 
				(new_range.key1+MAXDAY_LOOKUP_SECS)) >
                                        range->key2)
                                new_range.key2 = range->key2;
		}
		else { /* res == NULL */
			fprintf(stderr, "%s\n", errmsg);
			stat = status_rpc;
		}
	}
	while (stat == status_ok && new_range.key1 < range->key2 && 
		(new_range.key1+1) != range->key2);

        if (stat != status_ok && *r != NULL)
                destroy_abbrev_appt(*r);

        free(host);
        return(stat);
}

extern Stat
table_abbrev_lookup_key_range(target, keyrange, r)
	char *target; Keyrange *keyrange; Abb_Appt **r;
{
        Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host;
 
	if (debug)
		fprintf(stderr, "call to table_abbrev_lookup_key_range()\n");

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);
        *r = NULL;
 
        ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }
 
	ci->retry = true;
        switch(ci->vers_out) {
		Table_Args_3 a3;
		Table_Args a4;
		Table_Res_3 *res3;
        case TABLEVERS_2:
		free(host);
		return(status_notsupported);
                break;
	case TABLEVERS_3:
		a3.target = target;
		a3.args.tag = KEYRANGE_3;
		a3.args.Args_3_u.keyrange = keyrange4_to_keyrange3(keyrange);
		a3.pid = getpid();
		res3 = rtable_abbreviated_lookup_key_range_3(&a3, ci);
		res = tableres3_to_tableres4(res3);
		if (a3.args.Args_3_u.keyrange != NULL)
			xdr_free(xdr_Keyrange_3,
				(char *)a3.args.Args_3_u.keyrange);
		if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
		break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = KEYRANGE;
                a4.args.Args_u.keyrange = keyrange;
		a4.pid = getpid();
                res = rtable_abbreviated_lookup_key_range_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }

        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_failed:
                        *r = copy_abbrev_appt(res->res.Table_Res_List_u.b);
                        stat = status_ok;
                        break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_lookup_key_range", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);
        }
        else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
        }

        free(host);
        return(stat);
}

extern Stat
table_abbrev_lookup_range(target, range, r)
	char *target; Range *range; Abb_Appt **r;
{
        Table_Res *res=NULL;
        Stat stat = status_ok;
        Client_Info *ci = NULL;
        char *host;
	Range	cache_range,	*user_range;
	Range	new_range;
	Abb_Cache	*abb_cache = NULL;
	Abb_Appt *last_r=NULL, *tmp_r=NULL, *r_ptr=NULL;

	if (debug)
		fprintf(stderr, "call to table_abbrev_lookup_range()\n");

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);

        *r = NULL;

	/*
	 * Get the cache for this calendar and create it if it doesn't
	 * exist.
	 *
	 * XXX dipol: Currently we never destory a cache once we create it.
	 *	      A typical empty cache consumes around 50 bytes and
	 *	      as the # of caches increase so does the time it
	 *	      takes to look one up (open).
	 *
	 *	      But the number of calendars browsed is typically low
	 *   	      (in the tens) and the default calendar should almost
	 *	      always land at the head of the list since it is usually
	 *            created first.  This shouldn't be a problem, but it is
	 * 	      something to keep in mind.
	 */
	if (abbrev_cache_size > 0 &&
	    (abb_cache = abbcache_open(target, O_CREAT)) != NULL) {

		if (abbcache_fetch_range(abb_cache, range, r) == cacheHit) {
			/* Data was in cache.  Use it */
			return status_ok;
		}

		/*
	 	 * Data not in cache. Fetch data for the month the range is in
	 	 */
		user_range = range;

		/* We have to do this check since first_dom() actually returns
 		 * you the last tick of the last day of the previous month
 		 * So if the caller computed key1 using first_dom() and we
 		 * automatically did first_dom(key1) then we would actually
 		 * be getting the first day of the previous month.  Got it?
 		 */
		if (dom(range->key1 + 1) == 1) {
			/* Already at start of month */
 			cache_range.key1 = range->key1;
 		} else {
 			cache_range.key1 = first_dom(range->key1);
 		}

		cache_range.key2 = next_ndays(cache_range.key1 + 1,
					      abbrev_cache_size);
		cache_range.next = NULL;

		/* 
	 	* Make sure we get enough data to cover user's request
	 	*/
		if (cache_range.key2 < range->key2)
			cache_range.key2 = range->key2;

		range = &cache_range;
	}
	

        ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
	new_range.key1 = range->key1;
	new_range.key2 = range->key2;
	new_range.next = NULL;
	if (ci->use == udp_transport) 
		if ((new_range.key1 + MAXDAY_LOOKUP_SECS) < range->key2) 
			new_range.key2 = new_range.key1 + MAXDAY_LOOKUP_SECS;

	do {
        	switch(ci->vers_out) {
			Table_Args_2 a2;
			Table_Args_3 a3;
			Table_Args a4;
			Table_Res_2 *res2;
			Table_Res_3 *res3;
		case TABLEVERS_2:
			a2.target = target;
			a2.args.tag = RANGE_2;
			a2.args.Args_2_u.range = range4_to_range2(&new_range);
			res2 = rtable_abbreviated_lookup_range_2(&a2, ci);
			res = tableres2_to_tableres4(res2);
			if (a2.args.Args_2_u.range != NULL)
				xdr_free(xdr_Range_2,
					(char *)a2.args.Args_2_u.range);
			if (res2 != NULL)
				xdr_free(xdr_Table_Res_2, (char*)res2); 
			break;
		case TABLEVERS_3:
			a3.target = target;
			a3.args.tag = RANGE_3;
			a3.args.Args_3_u.range = range4_to_range3(&new_range);
			a3.pid = getpid();
			res3 = rtable_abbreviated_lookup_range_3(&a3, ci);
			res = tableres3_to_tableres4(res3);
			if (a3.args.Args_3_u.range != NULL)
				xdr_free(xdr_Range_3,
					(char *)a3.args.Args_3_u.range);
			if (res3 != NULL)
				xdr_free(xdr_Table_Res_3, (char*)res3); 
			break;
		case TABLEVERS:
			a4.target = target;
			a4.args.tag = RANGE;
			a4.args.Args_u.range = &new_range;
			a4.pid = getpid();
			res = rtable_abbreviated_lookup_range_4(&a4, ci);
			break;
		default:
			free(host);
			return(status_other);
		}

		if (res != NULL) {
			switch(res->status) {
			case access_ok:
			case access_failed:
				tmp_r = copy_abbrev_appt(res->res.Table_Res_List_u.b);
				stat = status_ok;
				if (ci->use == tcp_transport) 
					*r = tmp_r;
				else { /* udp transport */
					if (*r == NULL) 
						*r = last_r = tmp_r;
					else {
						/* traverse from the last res ptr */
						for (r_ptr = last_r; r_ptr != NULL && 
							r_ptr->next != NULL; r_ptr = r_ptr->next);
						if (tmp_r != NULL)
                                                	r_ptr->next = last_r = tmp_r;
						else
							last_r = r_ptr;
					}
				}
				break;
			case access_other:
				stat = status_param;
				break;
			case access_notable:
				stat = status_notable;
				break;
			default:
				/* remote system error */
				if (debug)
                                fprintf(stderr, errfmt,
                                        "table_abbrev_lookup_range", res->status);
				stat = status_other;
				break;
			}
			xdr_free(xdr_Table_Res, (char*)res); res = NULL;
			/* ange must encompass interested ticks */
			new_range.key1 = new_range.key2-1;
			if ((new_range.key2 = 
				(new_range.key1+MAXDAY_LOOKUP_SECS)) >
					 range->key2) 
				new_range.key2 = range->key2;
		}
		else { /* res == NULL */
			fprintf(stderr, "%s\n", errmsg);
			stat = status_rpc;
		}
	}
	while (stat == status_ok && new_range.key1 < range->key2 && 
		(new_range.key1+1) != range->key2);

	if (stat != status_ok && *r != NULL)
		destroy_abbrev_appt(*r);

        free(host);

	/* Insert data into cache and retrieve a copy to return to user */
	if (abb_cache != NULL) {
		if (stat == status_ok) {
			abbcache_store_range(abb_cache, range, *r);
			abbcache_fetch_range(abb_cache, user_range, r);
		}
		abbcache_close(abb_cache);
	}


        return(stat);
}

extern Stat
table_insert(target, ap, r)
	char *target; Appt *ap; Appt **r;
{
        Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host = NULL;
	Appt *apptptr;

	if (debug)
		fprintf(stderr, "call to table_insert()\n");

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);
        *r = NULL;

        ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	/* if the backend's version is less than 4, then it
	 * does not support repeating event types beyond yearly
	 */
	for (apptptr = ap; apptptr != NULL; apptptr = apptptr->next) {
		if (ci->vers_out < TABLEVERS && apptptr->period.period > yearly)
		{
			free(host);
			return(status_notsupported);
		}
	}

	/* Nuke contents of cache since we are modifing data */
	abbcache_cflush(target);

	ci->retry = false;
        switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = APPT_2;
                a2.args.Args_2_u.appt = appt4_to_appt2(ap);
                res2 = rtable_insert_2(&a2, ci);
                res = tableres2_to_tableres4(res2);
		if (a2.args.Args_2_u.appt != NULL)
			xdr_free(xdr_Appt_2, (char *)a2.args.Args_2_u.appt);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = APPT_3;
                a3.args.Args_3_u.appt = appt4_to_appt3(ap);
		a3.pid = getpid();
                res3 = rtable_insert_3(&a3, ci);
                res = tableres3_to_tableres4(res3);
		if (a3.args.Args_3_u.appt != NULL)
			xdr_free(xdr_Appt_3, (char *)a3.args.Args_3_u.appt);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = APPT;
                a4.args.Args_u.appt = ap;
		a4.pid = getpid();
                res = rtable_insert_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }

        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_partial:
                        *r = copy_appt(res->res.Table_Res_List_u.a);
                        stat = status_ok;
                        break;
		case access_failed:
			stat = status_denied;
			break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
		case access_incomplete:
			stat = status_incomplete;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_insert", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);
        }
        else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
        }

        free(host);
        return(stat);
}

/*
 * Delete a single appointment or a repeating event.
 * For a repeating event,
 *	do_all deletes the whole event,
 *	do_one deletes a single instance, and
 *	do_forward deletes the specified instance and the rest.
 */
extern Stat
table_delete(target, key, r)
	char *target; Uidopt *key; Appt **r;
{
        Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host = NULL;
	Uidopt *keyptr;
	Options opt;
 
	if (debug)
		fprintf(stderr, "call to table_delete()\n");

        if ((r==NULL) || (target==NULL) || (host = cm_target2location(target))==NULL)
                return(status_param);
        *r = NULL;
 
        ci = create_tcp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	/* if the backend's version is less than 4, then it
	 * does not support do_forward nor does it support
	 * mixing do_all and do_one.
	 */
	for (keyptr = key, opt = key->option; keyptr != NULL;
	     keyptr = keyptr->next) {
		if (keyptr->option == do_forward && ci->vers_out < TABLEVERS) {
			free(host);
			return(status_notsupported);
		}
		if (opt != keyptr->option) {
			free(host);
			return(status_notsupported);
		}
	}

	/* Nuke contents of cache since we are modifing data */
	abbcache_cflush(target);

	ci->retry = false;
        switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = UID_2;
                a2.args.Args_2_u.key = uidopt4_to_uid2(key);

		if (key->option == do_all)
                    res2 = rtable_delete_2(&a2, ci);
		else
		    res2 = rtable_delete_instance_2(&a2, ci);

                res = tableres2_to_tableres4(res2);
		if (a2.args.Args_2_u.key != NULL)
			xdr_free(xdr_Uid_2, (char *)a2.args.Args_2_u.key);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = UID_3;
                a3.args.Args_3_u.key = uidopt4_to_uid3(key);
		a3.pid = getpid();

		if (key->option == do_all)
                    res3 = rtable_delete_3(&a3, ci);
		else
		    res3 = rtable_delete_instance_3(&a3, ci);

                res = tableres3_to_tableres4(res3);
		if (a3.args.Args_3_u.key != NULL)
			xdr_free(xdr_Uid_3, (char *)a3.args.Args_3_u.key);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = UIDOPT;
                a4.args.Args_u.uidopt = key;
		a4.pid = getpid();
                res = rtable_delete_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }

        if (res!=NULL) {
              switch(res->status) {
                case access_ok:
                case access_partial:
                        *r = copy_appt(res->res.Table_Res_List_u.a);
                        stat = status_ok;
                        break;
		case access_failed:
			stat = status_denied;
			break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
		case access_incomplete:
			stat = status_incomplete;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_delete", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);

        } else {
		fprintf(stderr, "%s\n", errmsg);
                stat = status_rpc;
        }

        free(host);
        return(stat);
}

/*
 * Change a single appointment or a repeating event.
 * For a repeating event,
 *	do_all changes the whole event,
 *	do_one changes a single instance, and
 *	do_forward changes the specified instance and the rest.
 */
extern Stat
table_change(target, key, appt, opt, r)
	char *target; Id *key; Appt *appt; Options opt; Appt **r;
{
	Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host = NULL;

	if (debug)
		fprintf(stderr, "call to table_change()\n");

	if ((r==NULL) || (target==NULL) || ((host = cm_target2location(target))==NULL))
		return(status_param);
	*r = NULL;

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	/* if the backend's version is less than 4, then it
	 * does not support repeating event types beyond yearly
	 * nor does it support do_forward
	 */
	if (ci->vers_out < TABLEVERS &&
	    (appt->period.period > yearly || opt == do_forward)) {
		free(host);
		return(status_notsupported);
	}

	/* Nuke contents of cache since we are modifing data */
	abbcache_cflush(target);

	ci->retry = false;
        switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = APPTID_2;
		a2.args.Args_2_u.apptid.oid = (Id_2 *)ckalloc(sizeof(Id_2));
                id4_to_id2(key, a2.args.Args_2_u.apptid.oid);
		a2.args.Args_2_u.apptid.new_appt = appt4_to_appt2(appt);

		if (opt == do_all)
                    res2 = rtable_change_2(&a2, ci);
		else
		    res2 = rtable_change_instance_2(&a2, ci);

                res = tableres2_to_tableres4(res2);
		free(a2.args.Args_2_u.apptid.oid);
		if (a2.args.Args_2_u.apptid.new_appt != NULL)
			xdr_free(xdr_Appt_2,
				(char *)a2.args.Args_2_u.apptid.new_appt);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = APPTID_3;
		a3.args.Args_3_u.apptid.oid = (Id_3 *)ckalloc(sizeof(Id_3));
                id4_to_id3(key, a3.args.Args_3_u.apptid.oid);
		a3.args.Args_3_u.apptid.new_appt = appt4_to_appt3(appt);
		a3.pid = getpid();

		if (opt == do_all)
                    res3 = rtable_change_3(&a3, ci);
		else
		    res3 = rtable_change_instance_3(&a3, ci);

                res = tableres3_to_tableres4(res3);
		free(a3.args.Args_3_u.apptid.oid);
		if (a3.args.Args_3_u.apptid.new_appt != NULL)
			xdr_free(xdr_Appt_3,
				(char *)a3.args.Args_3_u.apptid.new_appt);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = APPTID;
                a4.args.Args_u.apptid.oid = key;
		a4.args.Args_u.apptid.new_appt = appt;
		a4.args.Args_u.apptid.option = opt;
		a4.pid = getpid();
                res = rtable_change_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }
        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_partial:
                        *r = copy_appt(res->res.Table_Res_List_u.a);
                        stat = status_ok;
                        break;
		case access_failed:
			stat = status_denied;
			break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
		case access_incomplete:
			stat = status_incomplete;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_change", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);

        } else {
		fprintf(stderr, "%s\n", errmsg);
                stat = status_rpc;
        }
        free(host);
        return(stat);
}

extern Table_Status
table_check(target)
	char *target; 
{
        Table_Status s = other;
	Table_Status *res;
        Client_Info *ci = NULL;
        char *host;

	if (debug)
		fprintf(stderr, "call to table_check()\n");

	if (target==NULL || (host = cm_target2location(target))==NULL)
		return(other);

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(other);
        }

	ci->retry = true;
	switch(ci->vers_out) {
        	Table_Args_2 a2;
        	Table_Args_3 a3;
        	Table_Args a4;
        	Table_Status_2 *s2;
        	Table_Status_3 *s3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = TICK_2;
                a2.args.Args_2_u.tick = 0;
                s2 = rtable_check_2(&a2, ci);
		if (s2 != NULL)
                	s = tablestat2_to_tablestat4(*s2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = TICK_3;
                a3.args.Args_3_u.tick = 0;
		a3.pid = getpid();
                s3 = rtable_check_3(&a3, ci);
		if (s3 != NULL)
                	s = tablestat3_to_tablestat4(*s3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = TICK_4;
                a4.args.Args_u.tick = 0;
		a4.pid = getpid();
                res = rtable_check_4(&a4, ci);
		if (res != NULL)
			s = *res;
                break;
        default:
		s = other;
                break;
        }
        free(host);
	return(s);
}


extern Stat
table_set_access(target, accesslist)
	char *target; Access_Entry *accesslist;
{
	Access_Status s = access_other;
	Access_Status *res;
	Stat stat;
	char *host = NULL;
	Client_Info *ci;
	Boolean nullreturned = false;

	if (debug)
		fprintf(stderr, "call to table_set_access()\n");

	if (target==NULL || (host = cm_target2location(target))==NULL)
		return(status_param);

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }
		
	ci->retry = true;
	switch(ci->vers_out) {
		Access_Args_2 a2;
		Access_Args_3 a3;
		Access_Args a4;
		Access_Status_2 *s2;
		Access_Status_3 *s3;
	case TABLEVERS_2:
		a2.target = target;
		a2.access_list = acclist4_to_acclist2(accesslist);
		s2 = rtable_set_access_2(&a2,  ci);
		if (s2 != NULL)
			s =  accstat2_to_accstat4(*s2);
		else
			nullreturned = true;
		if (a2.access_list != NULL)
			xdr_free(xdr_Access_Entry_2, (char *)a2.access_list);
		break;
	case TABLEVERS_3:
		a3.target = target;
		a3.access_list = acclist4_to_acclist3(accesslist);
		s3 = rtable_set_access_3(&a3,  ci);
		if (s3 != NULL)
			s =  accstat3_to_accstat4(*s3);
		else
			nullreturned = true;
		if (a3.access_list != NULL)
			xdr_free(xdr_Access_Entry_3, (char *)a3.access_list);
		break;
	case TABLEVERS:
		a4.target = target;
		a4.access_list = accesslist;
		res = rtable_set_access_4(&a4, ci);
		if (res != NULL)
			s = *res;
		else
			nullreturned = true;
		break;
	default:
		free(host);
		return(status_other);
	}

	if (nullreturned)
		stat = status_rpc;
	else {
		switch(s) {
		case access_ok:
			stat = status_ok;
			break;
		case access_failed:
			stat = status_denied;
			break;
		case access_notable:
			stat = status_notable;
			break;
		default:
			stat = status_other;
			if (debug)
                        	fprintf(stderr, errfmt, "table_set_access", s);
		}
	}
        free(host);
        return(stat);
}

extern Stat
table_get_access(target, accesslist)
	char *target; Access_Entry **accesslist;
{
	Access_Args *res=NULL;
	char *host;
	Client_Info *ci;
	Stat stat = status_ok;

	if (debug)
		fprintf(stderr, "call to table_get_access()\n");

	if ((accesslist==NULL) || (target==NULL) ||
	    ((host = cm_target2location(target))==NULL))
		return(status_param);
	*accesslist = NULL;

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
        switch(ci->vers_out) {
                Access_Args_2 a2;
                Access_Args_3 a3;
                Access_Args a4;
                Access_Args_2 *res2;
                Access_Args_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.access_list = NULL;
                res2 = rtable_get_access_2(&a2,  ci);
		res = accargs2_to_accargs4(res2);
		if (res != NULL) {
			*accesslist = copy_access_list(res->access_list);
			xdr_free (xdr_Access_Args, (char*)res);
		} else
			stat = status_rpc;
		if (res2 != NULL)
			 xdr_free(xdr_Access_Args_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.access_list = NULL;
                res3 = rtable_get_access_3(&a3,  ci);
		res = accargs3_to_accargs4(res3);
		if (res != NULL) {
			*accesslist = copy_access_list(res->access_list);
			xdr_free (xdr_Access_Args, (char*)res);
		} else
			stat = status_rpc;
		if (res3 != NULL)
			 xdr_free(xdr_Access_Args_3, (char*)res3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.access_list = NULL;
		res = rtable_get_access_4(&a4,  ci);
		if (res != NULL) {
			*accesslist = copy_access_list(res->access_list);
			xdr_free (xdr_Access_Args, (char*)res);
		} else
			stat = status_rpc;
                break;
        default:
                stat = status_other;
                break;
        }
        free(host);
        return(stat);
}

extern int 
table_size(target)
	char *target;
{
	Table_Args a;
	int size = 0;
	int *res;
	char *host;
	Client_Info *ci;

	if (debug)
		fprintf(stderr, "call to table_size()\n");

	if (target==NULL || (host = cm_target2location(target))==NULL)
		return(0);

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(0);
        }

	ci->retry = true;
        switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = TICK_2;
                a2.args.Args_2_u.tick = 0;
                res = rtable_size_2(&a2, ci);
		if (res != NULL)
			size = *res;
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = TICK_3;
                a3.args.Args_3_u.tick = 0;
		a3.pid = getpid();
                res = rtable_size_3(&a3, ci);
		if (res != NULL)
			size = *res;
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = TICK_4;
                a4.args.Args_u.tick = 0;
		a4.pid = getpid();
                res = rtable_size_4(&a4, ci);
		if (res != NULL)
			size = *res;
                break;
        default:
		size = 0;
                break;
        }
        free(host);
        return(size);
}


extern Stat
table_lookup_next_reminder(target, key, r)
	char *target; long key; Reminder **r;
{
        Table_Res *res=NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host;

	if (debug)
		fprintf(stderr, "call to table_lookup_next_reminder()\n");

	if ((r==NULL) || (target==NULL) || ((host = cm_target2location(target))==NULL))
		return(status_param);
	*r = NULL;

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
	switch(ci->vers_out) {
                Table_Args_2 a2;
                Table_Args_3 a3;
                Table_Args a4;
                Table_Res_2 *res2;
                Table_Res_3 *res3;
        case TABLEVERS_2:
                a2.target = target;
                a2.args.tag = TICK_2;
                a2.args.Args_2_u.tick = key;
                res2 = rtable_lookup_next_reminder_2(&a2, ci);
                res = tableres2_to_tableres4(res2);
                if (res2 != NULL) xdr_free(xdr_Table_Res_2, (char*)res2);
                break;
	case TABLEVERS_3:
                a3.target = target;
                a3.args.tag = TICK_3;
                a3.args.Args_3_u.tick = key;
		a3.pid = getpid();
                res3 = rtable_lookup_next_reminder_3(&a3, ci);
                res = tableres3_to_tableres4(res3);
                if (res3 != NULL) xdr_free(xdr_Table_Res_3, (char*)res3);
                break;
        case TABLEVERS:
                a4.target = target;
                a4.args.tag = TICK_4;
                a4.args.Args_u.tick = key;
		a4.pid = getpid();
                res = rtable_lookup_next_reminder_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }
        if (res!=NULL) {
                switch(res->status) {
                case access_ok:
                case access_failed:
                        *r = copy_reminder(res->res.Table_Res_List_u.r);
                        stat = status_ok;
                        break;
                case access_other:
                        stat = status_param;
                        break;
		case access_notable:
			stat = status_notable;
			break;
                default:
                        /* remote system error */
                        if (debug)
                                fprintf(stderr, errfmt,
                                        "table_lookup_next_reminder", res->status);
                        stat = status_other;
                        break;
                }
                xdr_free(xdr_Table_Res, (char*)res);
        }
        else {
		fprintf(stderr, "%s\n", errmsg);
		stat = status_rpc;
        }
        free(host);
        return(stat);

}

extern long
table_gmtoff(host)
	char *host;
{
	long gmtoff = -1;
	long *res;
	Client_Info *ci;

	if (host==NULL)
		return(gmtoff);

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) 
                return(gmtoff);
	
	ci->retry = true;
	switch(ci->vers_out) {
	case TABLEVERS_3:
		res = rtable_gmtoff_3((void *)NULL, ci);
		if (res != NULL)
			gmtoff = *res;
		break;
	case TABLEVERS:
		res = rtable_gmtoff_4((void *)NULL, ci);
		if (res != NULL)
			gmtoff = *res;
		break;
	case TABLEVERS_2:
	default:
		break;
	}
        return(gmtoff);
}

u_long
table_version(target)
	char *target;
{
	u_long vers = 0;
	Client_Info *ci = create_udp_client(target, INITIAL_TIMEOUT);

	if (debug)
		fprintf(stderr, "call to table_version()\n");

	if (ci!=NULL) vers = ci->vers_out;
	return(vers);
}

/*
 * Get and set the abbreviated appointment cache size (in number of days)
 * For now this is both the size of the cache and how much data we fetch
 * when we want to fill the cache.
 *
 * To turn the cache off set the size to 0.
 *
 * By default the cache is 38 days.
 */
extern int
table_abbrev_get_cache_size()

{
	return	abbrev_cache_size;
}

extern void
table_abbrev_set_cache_size(size)

	int	size;

{
	abbrev_cache_size = size;
	return;
}

extern Register_Status
table_request_deregistration(target)
	char *target;
{
	Register_Status stat;
	char *targethost = cm_target2location(target);
	Client_Info *ci = create_udp_client(targethost, INITIAL_TIMEOUT);

	if (debug)
		fprintf(stderr, "call to table_request_deregistration()\n");

	free(targethost);
	if (ci==NULL)
		return (register_rpc);

	/*
	 * Nuke the contents of the abbreviated appointment cache
	 * since we are no longer notified of changes.
	 */
	abbcache_cflush(target);

	ci->retry = true;
	if ((stat = deregister_target(ci, target)) == de_registered)
		/* remove host from registration list */
		decr_reg_list(ci, target);

	return(stat);
}

extern Register_Status
table_request_registration(target)
char	*target;
{
	Register_Status stat;
	char *targethost = cm_target2location(target);
	Client_Info *ci = create_udp_client(targethost, INITIAL_TIMEOUT);

	if (debug)
		fprintf(stderr, "call to table_request_registration()\n");

	free(targethost);
	if (ci==NULL)
		return (register_rpc);

	ci->retry = true;
	if ((stat = register_target(ci, target)) == register_succeeded)
		/* add host to registration list */
		incr_reg_list(ci, target);

	return(stat);
}

extern Stat
table_create(target)
	char *target;
{
        Table_Status *res = NULL;
        Stat stat;
        Client_Info *ci = NULL;
        char *host;

	if (debug)
		fprintf(stderr, "call to table_create()\n");

	if ((target == NULL) || ((host = cm_target2location(target))==NULL))
		return(status_param);

        ci = create_udp_client(host, INITIAL_TIMEOUT);
        if (ci==NULL) {
		free(host);
		return(status_rpc);
        }

	ci->retry = true;
	switch(ci->vers_out) {
                Table_Op_Args a4;
        case TABLEVERS_2:
        case TABLEVERS_3:
		free(host);
		return(status_notsupported);
	case TABLEVERS:
                a4.target = target;
		a4.new_target = ckalloc(1);
                res = rtable_create_4(&a4, ci);
                break;
        default:
		free(host);
		return(status_other);
        }
	if (res != NULL) {
        	switch(*res) {
        	case ok:
			stat = status_ok;
			break;
        	case tbl_exist:
			stat = status_tblexist;
			break;
        	case denied:
			stat = status_denied;
                	break;
        	default:
			stat = status_other;
                	if (debug)
                		fprintf(stderr, errfmt, "table_create", *res);
		}
        } else
		stat = status_rpc;

        free(host);
        return(stat);
}

extern Stat
table_remove(target)
	char *target;
{
	return(status_notsupported);
}

extern Stat
table_rename(target, newname)
	char *target; char *newname;
{
	return(status_notsupported);
}

