/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _RADIO_NETWORK_H
#define	_RADIO_NETWORK_H

#ident	"@(#)radio_network.h	1.6	91/08/20 SMI"

#include "netbroadcast.h"

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/*
 * Radio Free Ethernet packet header
 */

#define	RADIO_CALLNAME_SIZE	(4)

/* 12 byte packet header for radio broadcast packets */
struct radio_hdr {
	char		callname[RADIO_CALLNAME_SIZE];	/* call letters */
	char		type;		/* see types below */
	char		host[3];	/* originator id (low order host id) */
	union {
	    char	b[4];		/* individual bytes */
	    unsigned	u;
	}		seqno;		/* sequence number (big-endian) */
};

/* radio_hdr message types and flags */
#define	RADIO_TYPE_SIGNOFF	(0x00)		/* station sign-off */
#define	RADIO_TYPE_ULAW		(0x01)		/* 8kHz u-law audio data */
#define	RADIO_TYPE_G721		(0x02)		/* 8kHz G.721 encoded audio */
#define	RADIO_TYPE_STATIONID	(0x0f)		/* station identification */
#define	RADIO_TYPE_MASK		(0x0f)		/* bitmask of types */

/* Miscellaneous defines */
#define	RADIO_ULAW_RATE		(8000)		/* samples per second */

/* number of seconds between station id transmissions */
#define	RADIO_ID_TIME	5

/* Default names for UDP broadcast port and IP multicast base address */
#define	RADIO_DEFAULT_SERVICE	"radio"			/* /etc/services name */
#define	RADIO_SERVICE_TYPE	"udp"
#define	RADIO_DEFAULT_ADDRESS	"RadioFreeEthernet"	/* /etc/hosts name */
#define	RADIO_DEFAULT_RANGE	(8)			/* default hop count */

/* Last ditch defaults: hard-wire the NIC assigned base address and port */
#define	RADIO_NIC_PORT		"5002"
#define	RADIO_NIC_ADDRESS	"224.0.3.255"


#define	RADIO_ID_ADDRESS	((unsigned)255)		/* low order byte */
#define	RADIO_DATA_ADDRESS	((unsigned)0)		/* low order byte */

#define	RADIO_HDR_SIZE		(sizeof (struct radio_hdr))
#define	RADIO_MAX_MULTICAST	(2800 - RADIO_HDR_SIZE)
#define	RADIO_MAX_BROADCAST	(1472 - RADIO_HDR_SIZE)
#define	RADIO_MAX_LEN		RADIO_MAX_MULTICAST

#define	RADIO_RCVBUF		(48 * 1024)		/* receive bufsiz */

/* sizeof (station id (ie, inet addr)) - sizeof (host id in radio_hdr) */
#define	RADIO_HOSTOFF		(1)


/* Station Identification message structure */
struct radio_id {
	char		callname[RADIO_CALLNAME_SIZE];	/* call letters */
	char		station[4];		/* host ID of sender */
	char		username[L_cuserid];	/* disc jockey name */
	char		freq[4];		/* data IP address */
};

/* Radio broadcast/receiver address structure */
struct radio_address {
	struct sockaddr_in	id;		/* address to send station id */
	struct sockaddr_in	data;		/* address to send audio data */
};

/* Radio broadcast control structure */
struct radio_broadcast {
	struct radio_id		id;		/* station identification */
	unsigned		seqno;		/* sequence number */
	struct radio_address	addrs;		/* broadcast addresses */
	struct net_broadcast*	broadcast;	/* ptr to broadcast structure */
	int			maxpkt;		/* packetsize limit */
};


/* Radio receiver control structure */
struct radio_receiver {
	int			fd;		/* socket descriptor */
	struct radio_id		tuner;		/* filters incoming data */
	struct radio_address	addrs;		/* addresses to monitor */
	struct sockaddr		xmitr;		/* transmitter ID */
	int			datasize;	/* received data pkt size */
	struct radio_hdr	hdr;		/* message header */
	union {
	    struct radio_id	id;		/* station id msg */
	    unsigned char	data[RADIO_MAX_LEN];
	} msg;
};

/* Functions declared in radio_subr.c */
extern int	radio_set_service(char*);
extern int	radio_set_address(char*);
extern int	radio_initbroadcast(char**, char*, int,
		    struct radio_broadcast*);
extern void	radio_closebroadcast(struct radio_broadcast*);
extern int	radio_broadcastdata(struct radio_broadcast*, unsigned char*,
		    int, char);
extern int	radio_broadcastid(struct radio_broadcast*);
extern int	radio_broadcastsignoff(struct radio_broadcast*);
extern int	radio_initreceiver(struct radio_receiver*);
extern void	radio_closereceiver(struct radio_receiver*);
extern int	radio_tunereceiver(struct radio_receiver*, int*);
extern void	radio_cleartuner(struct radio_receiver*);
extern int	radio_fdset(struct radio_receiver*, fd_set*);
extern int	radio_recv(struct radio_receiver*, int*);
extern int	radio_recvid(struct radio_receiver*, int*);
extern int	radio_recvdata(struct radio_receiver*, int*);
extern int	radio_recvstation(struct radio_receiver*, int*);
extern int	radio_match_station(struct radio_receiver*);
extern char*	radio_callname(char*);
extern void	radio_callname_copy(char*, char*);
extern char*	radio_hostname(char*);
extern int	radio_join_datagroup(struct radio_receiver*, char*);
extern int	radio_leave_datagroup(struct radio_receiver*, char*);

#endif /* !_RADIO_NETWORK_H */
