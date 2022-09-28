/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _NETBROADCAST_H
#define	_NETBROADCAST_H

#ident	"@(#)netbroadcast.h	1.6	92/06/24 SMI"

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/socket.h>
#ifdef SUNOS41
#include <sys/mbuf.h>
#endif

/*
 * The following structure describes a broadcast target.
 * To broadcast to multiple networks, a separate 'sendto' is issued
 * to each of the destinations in the list;
 */
struct net_broadcast {
	int			sockfd;		/* file descriptor */
	unsigned		cnt;		/* count of broadcast targets */
	struct sockaddr_in*	target;		/* list of target addresses */
};

/* Define the special hostname to enable broadcast instead of multicast */
#define	NET_BROADCAST_NAME	"BROADCAST"

/* Define the ifreq buffer size for SIOCGIFCONF requests */
/* XXX - changed from 256 to 32 for SVR4 alpha-4 bug */
#define	NET_BROADCAST_IFCONF	(32)


extern int	net_initbroadcast(char**, struct net_broadcast**);
extern int	net_addbroadcast(char*, struct net_broadcast*);
extern void	net_closebroadcast(struct net_broadcast*);
extern int	net_broadcast(struct net_broadcast*, struct sockaddr_in*,
		    char*, int);
extern int	net_join_multicast_group(int, struct sockaddr_in*);
extern int	net_leave_multicast_group(int, struct sockaddr_in*);
extern int	net_set_range(struct net_broadcast*, int);
extern int	net_parse_address(char*, struct sockaddr_in*);
extern int	net_extract_multicast_address(char*, struct sockaddr_in*);
extern int	net_is_multicast_addr(struct sockaddr_in*);
extern int	net_multicast_supported(void);

#endif /* !_NETBROADCAST_H */
