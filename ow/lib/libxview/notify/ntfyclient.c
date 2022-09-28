#ifndef	lint
#ifdef sccs
static char     sccsid[] = "@(#)ntfyclient.c 20.18 93/06/17 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Ntfy_client.c - NTFY_CLIENT specific operations that both the detector and
 * dispatcher share.
 */

#include <xview/xv_error.h>
#include <xview_private/ntfy.h>
#include <xview_private/ndis.h>	/* For ndis_default_prioritizer */
#include <xview_private/ndet.h>	
#include <xview_private/portable.h>
#include <xview_private/xv_slots.h>

/* Variables used in paranoid enumerator (see ntfy_condition) */
pkg_private_data NTFY_CLIENT *ntfy_enum_client = 0;
pkg_private_data NTFY_CLIENT *ntfy_enum_client_next = 0;

static NTFY_CLIENT dummy_client;
static void *ndet_tbl = NULL;


static void *create_tbl( old_tbl )
    void *old_tbl;
{
    void       *tbl;
    static int page = 339;

    if( !old_tbl ) {
	tbl = malloc( xv_slots_bytes( page ));
	xv_slots_init( tbl, xv_slots_bytes( page ));
    }
    else {
	if( page >= 65526 ) {
fprintf(stderr, "Tried to create more than 65526 clients. This should never happen.\n");
	   exit(1);
	}
	page *= 3;
	if( page > 65526 )
	    page = 65526;

	tbl = malloc( xv_slots_bytes( page ));
        xv_slots_copy( tbl, xv_slots_bytes( page ), old_tbl);
	free( old_tbl );
    }
    return tbl;
}

pkg_private NTFY_CLIENT *
ntfy_find_nclient(client_list, nclient, client_latest)
    NTFY_CLIENT    *client_list;
    Notify_client   nclient;
    register NTFY_CLIENT **client_latest;
{
    register NTFY_CLIENT *client;
    NTFY_CLIENT    *next;

    ntfy_assert(NTFY_IN_CRITICAL, 36 /* Unprotected list search */);
    /* See if hint matches */
    if (*client_latest && (*client_latest)->nclient == nclient)
	return (*client_latest);

    if(( client_list == ndet_clients ) && ndet_clients ) {
       /* Find client */
       if( client = (NTFY_CLIENT *)xv_slots_find( ndet_tbl, nclient )) {
           *client_latest = *(NTFY_CLIENT **)client;
           return (*(NTFY_CLIENT **)client);
       }
    }

    else 
        /* Search entire list */
        for (client = client_list; client; client = next) {
            next = client->next;
            if (client->nclient == nclient) {
                /* Set up hint for next time */
                *client_latest = client;
                return (client);
            }
        }    

    return (NTFY_CLIENT_NULL);
}

/*
 * Find/create client that corresponds to nclient
 */
pkg_private NTFY_CLIENT *
ntfy_new_nclient(client_list, nclient, client_latest)
    NTFY_CLIENT   **client_list;
    Notify_client   nclient;
    NTFY_CLIENT   **client_latest;
{
    register NTFY_CLIENT *client;

    if( client_list == &ndet_clients ) {
        if( !ndet_tbl )
	    ndet_tbl = create_tbl( NULL );
        client = (NTFY_CLIENT *)xv_slots_get( ndet_tbl, nclient );
	if( !client ) {
	    ndet_tbl = create_tbl( ndet_tbl );
            client = (NTFY_CLIENT *)xv_slots_get( ndet_tbl, nclient );
	}
        if( *(NTFY_CLIENT **)client == 0 ) {
            if ((*(NTFY_CLIENT **)client = ntfy_alloc_client()) == NTFY_CLIENT_NULL)
                return (NTFY_CLIENT_NULL);
	    client = *(NTFY_CLIENT **)client;
        }
        else
            return *(NTFY_CLIENT **)client;
    }

    else if ((client = ntfy_find_nclient(*client_list, nclient,
				    client_latest)) != NTFY_CLIENT_NULL)
        return client;

    /* Allocate client */
    else if ((client = ntfy_alloc_client()) == NTFY_CLIENT_NULL)
        return (NTFY_CLIENT_NULL);

    /* Initialize client */
    client->next = NTFY_CLIENT_NULL;
    client->conditions = NTFY_CONDITION_NULL;
    client->condition_latest = NTFY_CONDITION_NULL;
    client->nclient = nclient;
    client->prioritizer = ndis_default_prioritizer;
    client->flags = 0;
    /* Append to client list */
    ntfy_append_client(client_list, client);
    /* Set up client hint */
    *client_latest = client;

    return (client);
}

pkg_private void
ntfy_remove_client(client_list, client, client_latest, who)
    NTFY_CLIENT   **client_list;
    NTFY_CLIENT    *client;
    NTFY_CLIENT   **client_latest;
    NTFY_WHO        who;
{
    register NTFY_CONDITION *condition;
    NTFY_CONDITION *next;

    /* Fixup enumeration variables if client matches one of them */
    if (client == ntfy_enum_client)
	ntfy_enum_client = NTFY_CLIENT_NULL;
    if (client == ntfy_enum_client_next)
	ntfy_enum_client_next = ntfy_enum_client_next->next;
    /* Make sure that remove all conditions */
    for (condition = client->conditions; condition; condition = next) {
	next = condition->next;
	ntfy_remove_condition(client, condition, who);
    }
    /* Remove & free client from client_list */
    if( client_list == &ndet_clients )
        xv_slots_delete( ndet_tbl, client->nclient );
    ntfy_remove_node((NTFY_NODE **) client_list, (NTFY_NODE *) client);
    /* Invalidate condition hint */
    *client_latest = NTFY_CLIENT_NULL;
}
