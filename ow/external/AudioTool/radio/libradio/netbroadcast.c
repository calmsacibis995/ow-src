/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)netbroadcast.c	1.10	92/03/19 SMI"

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <netinet/in.h>
#include <netdb.h>

#include "netbroadcast.h"

#ifndef NULL
#define	NULL	0
#endif


/* XXX - If compiling on a system IP multicasting */
#ifdef IP_ADD_MEMBERSHIP
#define	MULTICAST

#else /* !MULTICAST */
/* XXX - If compiling on a system w/o IP multicasting */
#undef	IN_CLASSD
#define	IN_CLASSD(addr) 0
#endif /* !MULTICAST */


/*
 * Add a given sockaddr structure to a broadcast structure.
 */
static int
add_sockaddr(
	struct sockaddr_in*	sap,
	struct net_broadcast*	nbp)
{
	nbp->cnt++;
	nbp->target = (struct sockaddr_in*) realloc((char*)nbp->target,
	    (int)(nbp->cnt * sizeof (struct sockaddr_in)));
	if (nbp->target == NULL)
		return (-1);
	nbp->target[nbp->cnt - 1] = *sap;
	return (0);
}

/*
 * Find all the network interfaces capable of UDP broadcast and add them
 * to the given broadcast structure.  Multicast addresses are not added.
 */
static int
add_allbroadcast(
	struct net_broadcast*	nbp)
{
	struct ifconf		ifc;
	struct ifreq		ifbuf[NET_BROADCAST_IFCONF];
	struct ifreq*		ifr;
	int			i;
	int			count;

	/* Get a list of all network interfaces */
	ifc.ifc_len = sizeof (ifbuf);
	ifc.ifc_buf = (caddr_t)ifbuf;
	if (ioctl(nbp->sockfd, SIOCGIFCONF, (char*)&ifc) < 0) {
		return (-1);
	}

	/* Loop through the interfaces, looking for appropriate ones */
	errno = 0;
	count = 0;
	ifr = ifc.ifc_req;
	for (i = ifc.ifc_len / sizeof (struct ifreq); i > 0; i--, ifr++) {
		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;

		if (ioctl(nbp->sockfd, SIOCGIFFLAGS, (char*)ifr) < 0) {
			continue;
		}

		/* Skip uninteresting interfaces */
		if (((ifr->ifr_flags & IFF_UP) == 0) ||
		    (ifr->ifr_flags & IFF_LOOPBACK) ||
		    ((ifr->ifr_flags & IFF_BROADCAST) == 0)) {
			continue;
		}

		/* This one is set IFF_BROADCAST */
		if (ioctl(nbp->sockfd, SIOCGIFBRDADDR, (char*)ifr) < 0) {
			continue;
		}

		if ((add_sockaddr((struct sockaddr_in*)&ifr->ifr_broadaddr,
		    nbp)) < 0) {
			return (-1);
		}
		count++;
	}
	if (count == 0) {
		/* Could not find a suitable interface */
		if (errno == 0)
			errno = ENOTCONN;
		return (-1);
	}
	return (0);
}


/*
 * Initialize a structure describing network broadcast destinations.
 * If 'interfaces' is non-NULL, it is a pointer to a NULL-terminated list
 * of broadcast interface names.
 * Otherwise, locate all network interfaces that can be broadcasted to.
 */
int
net_initbroadcast(
	char**			interfaces,
	struct net_broadcast**	nbpp)
{
	int			fd;
	int			i;

	/* Open the socket (XXX - this is very internet-ty) */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return (-1);
	i = 1;

#ifndef sun
	/* 
	 * XXX - on Sun's, SO_BROADCAST is not required for sending 
	 * broadcast packets (real BSD systems have this req').
	 */
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
	    (char*)&i, sizeof (i)) < 0) {
		(void) close(fd);
		return (-1);
	}
#endif /* !sun */

	/* Allocate and init a broadcast structure */
	*nbpp = (struct net_broadcast*) malloc(sizeof (**nbpp));
	if (nbpp == NULL) {
		(void) close(fd);
		return (-1);
	}

	/* Allocate a dummy address slot */
	(*nbpp)->target =
	    (struct sockaddr_in*) malloc(sizeof (struct sockaddr_in));
	if ((*nbpp)->target == NULL) {
		(void) close(fd);
		return (-1);
	}

	(*nbpp)->sockfd = fd;
	(*nbpp)->cnt = 0;

	/*
	 * If the given interface list is non-NULL, use it.
	 * Otherwise, broadcast to all reasonable interfaces that can be found.
	 */
	if (interfaces != NULL) {
		while (*interfaces != NULL) {
			i = net_addbroadcast(*interfaces++, *nbpp);
			if (i < 0)
				break;
		}
	} else {
		i = add_allbroadcast(*nbpp);
	}

	/* If error, close and deallocate everything */
	if (i < 0) {
		net_closebroadcast(*nbpp);
		*nbpp = NULL;
	}
	return (i);
}

/*
 * Add a UDP broadcast address to a broadcast structure.
 * Multicast addresses are not added.
 */
int
net_addbroadcast(
	char*			interface,
	struct net_broadcast*	nbp)
{
	struct ifreq		ifreq;

	/* Look for the specified interface */
	(void) strncpy(ifreq.ifr_name, interface, sizeof (ifreq.ifr_name));
	if (ioctl(nbp->sockfd, SIOCGIFADDR, (char*)&ifreq) < 0) {
		return (-1);
	}
	if (ifreq.ifr_addr.sa_family != AF_INET) {
		errno = EADDRNOTAVAIL;
		return (-1);
	}

	if (ioctl(nbp->sockfd, SIOCGIFFLAGS, (char*)&ifreq) < 0) {
		return (-1);
	}

	if (((ifreq.ifr_flags & IFF_UP) == 0) ||
	    (ifreq.ifr_flags & IFF_LOOPBACK) ||
	    ((ifreq.ifr_flags & (IFF_BROADCAST | IFF_POINTOPOINT)) == 0)) {
		errno = EADDRNOTAVAIL;
		return (-1);
	}

	if ((ifreq.ifr_flags & IFF_BROADCAST) != 0) {
		if (ioctl(nbp->sockfd, SIOCGIFBRDADDR, (char*)&ifreq) < 0) {
			return (-1);
		}
		return (add_sockaddr((struct sockaddr_in*)&ifreq.ifr_broadaddr,
		    nbp));
	}

	/* Point-to-point network address is handled differently */
	if (ioctl(nbp->sockfd, SIOCGIFDSTADDR, (char*)&ifreq) < 0) {
		return (-1);
	}
	return (add_sockaddr((struct sockaddr_in*)&ifreq.ifr_dstaddr, nbp));
}

/*
 * Clean up and deallocate a broadcast structure.
 */
void
net_closebroadcast(
	struct net_broadcast*	nbp)
{
	(void) close(nbp->sockfd);
	if (nbp->target != NULL)
		(void) free((char*)nbp->target);
	(void) free((char*)nbp);
}


/*
 * Send a multicast or broadcast message.
 * If the address is a multicast address it is sent to that multicast address.
 * If not it is sent to all broadcast addresses in the specified structure.
 */
int
net_broadcast(
	struct net_broadcast*	nbp,
	struct sockaddr_in*	addr,
	char*			msg,
	int			msglen)
{
	int			i;
	int			err;
	struct sockaddr_in*	sap;

	err = 0;
	if (net_is_multicast_addr(addr))
		return (sendto(nbp->sockfd, msg, msglen, 0,
		    (struct sockaddr*)addr, sizeof (*addr)));

	/* Loop through all multicast capable interfaces */
	for (i = 0, sap = nbp->target; i < nbp->cnt; i++, sap++) {
		if (sendto(nbp->sockfd, msg, msglen, 0,
		    (struct sockaddr*)sap, sizeof (*sap)) < 0)
			err = -1;
	}
	return (err);
}

#ifdef MULTICAST
/*
 * Keep reference counts on the multicast addresses
 * since the kernel only allows us to join the same group once
 * per socket.
 */
struct multiaddr {
	struct multiaddr*	next;
	struct in_addr		group;
	struct in_addr		interface;
	int			socket;
	int			count;
};
static struct multiaddr*	multiaddr;


/* Maintain reference counts to join/leave multicast groups */
static int
join_group(
	int 			fd,
	struct ip_mreq*		mreq)
{
	struct multiaddr*	p;

	p = multiaddr;
	while (p != NULL) {
		if ((p->socket == fd) &&
		    (p->group.s_addr == mreq->imr_multiaddr.s_addr) &&
		    (p->interface.s_addr == mreq->imr_interface.s_addr)) {
			p->count++;
#ifdef DEBUG
			fprintf(stdout,
			    "join_group:  %lx / %lx / %d   references: %d\n",
			    p->group.s_addr, p->interface.s_addr,
			    p->socket, p->count);
#endif
			return (0);
		}
		p = p->next;
	}
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	    (char*)mreq, sizeof (*mreq)) < 0)
		return (-1);
	p = (struct multiaddr *)malloc(sizeof (struct multiaddr));
	if (p == NULL) {
		return (-1);
	}
	p->socket = fd;
	p->group = mreq->imr_multiaddr;
	p->interface = mreq->imr_interface;
	p->count = 1;
	p->next = multiaddr;
	multiaddr = p;
#ifdef DEBUG
	fprintf(stdout, "join_group:  %lx / %lx / %d   references: %d\n",
	    p->group.s_addr, p->interface.s_addr, p->socket, p->count);
#endif
	return (0);
}

static int
leave_group(
	int 			fd,
	struct ip_mreq*		mreq)
{
	struct multiaddr*	p;
	struct multiaddr*	prev;

	p = multiaddr;
	prev = NULL;
	while (p != NULL) {
		if (p->socket == fd) {
		  if (p->group.s_addr == mreq->imr_multiaddr.s_addr) {
		  if (p->interface.s_addr == mreq->imr_interface.s_addr) {
			p->count--;
			break;
		}}}
		prev = p;
		p = p->next;
	}
	if (p == NULL) {
#ifdef DEBUG
		fprintf(stdout, "leave_group: %lx / %lx / %d   not a member\n",
		    mreq->imr_multiaddr.s_addr,
		    mreq->imr_interface.s_addr, fd);
#endif
		return (-1);
	}
#ifdef DEBUG
	fprintf(stdout, "leave_group: %lx / %lx / %d   references: %d\n",
	    p->group.s_addr, p->interface.s_addr, p->socket, p->count);
#endif

	if (p->count > 0)	/* address still referenced */
		return (0);

	if (prev)
		prev->next = p->next;
	else
		multiaddr = p->next;
	(void) free((char *)p);

	if (setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
	    (char*)mreq, sizeof (*mreq)) < 0)
		return (-1);
	return (0);
}

/*
 * Find all the network interfaces capable of multicast and join
 * the multicast address on all of them.
 */
int
net_join_multicast_group(
	int			fd,
	struct sockaddr_in*	addr)
{
	struct ifconf		ifc;
	struct ifreq		ifbuf[NET_BROADCAST_IFCONF];
	struct ifreq*		ifr;
	int			i;
	int			count;
	struct sockaddr_in*	sin;

	/* Get a list of all network interfaces */
	ifc.ifc_len = sizeof (ifbuf);
	ifc.ifc_buf = (caddr_t)ifbuf;
	if (ioctl(fd, SIOCGIFCONF, (char*)&ifc) < 0) {
		return (-1);
	}

	/* Loop through the interfaces, looking for appropriate ones */
	errno = 0;
	count = 0;
	ifr = ifc.ifc_req;
	for (i = ifc.ifc_len / sizeof (struct ifreq); i > 0; i--, ifr++) {
		struct ip_mreq	mreq;

		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;

		if (ioctl(fd, SIOCGIFFLAGS, (char*)ifr) < 0) {
			continue;
		}

		/* Skip uninteresting interfaces */
		if (((ifr->ifr_flags & IFF_UP) == 0) ||
		    (ifr->ifr_flags & IFF_LOOPBACK) ||
		    ((ifr->ifr_flags & IFF_MULTICAST) == 0)) {
			continue;
		}
		if (ioctl(fd, SIOCGIFADDR, (char*)ifr) < 0)
			continue;

		sin = (struct sockaddr_in*)addr;
		mreq.imr_multiaddr = sin->sin_addr;
		mreq.imr_interface.s_addr =
		    ((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr.s_addr;
		if (join_group(fd, &mreq) < 0) {
			return (-1);
		}
		count++;
	}
	if (count == 0) {
		/* Could not find a suitable interface */
		if (errno == 0)
			errno = ENOTCONN;
		return (-1);
	}
	return (0);
}

/*
 * Find all the network interfaces capable of multicast and leave
 * the multicast address on all of them.
 */
int
net_leave_multicast_group(
	int			fd,
	struct sockaddr_in*	addr)
{
	struct ifconf		ifc;
	struct ifreq		ifbuf[NET_BROADCAST_IFCONF];
	struct ifreq*		ifr;
	int			i;
	int			count;
	struct sockaddr_in*	sin;

	/* Get a list of all network interfaces */
	ifc.ifc_len = sizeof (ifbuf);
	ifc.ifc_buf = (caddr_t)ifbuf;
	if (ioctl(fd, SIOCGIFCONF, (char*)&ifc) < 0) {
		return (-1);
	}

	/* Loop through the interfaces, looking for appropriate ones */
	errno = 0;
	count = 0;
	ifr = ifc.ifc_req;
	for (i = ifc.ifc_len / sizeof (struct ifreq); i > 0; i--, ifr++) {
		struct ip_mreq	mreq;

		if (ifr->ifr_addr.sa_family != AF_INET)
			continue;

		if (ioctl(fd, SIOCGIFFLAGS, (char*)ifr) < 0) {
			continue;
		}

		/* Skip uninteresting interfaces */
		if (((ifr->ifr_flags & IFF_UP) == 0) ||
		    (ifr->ifr_flags & IFF_LOOPBACK) ||
		    ((ifr->ifr_flags & IFF_MULTICAST) == 0)) {
			continue;
		}
		if (ioctl(fd, SIOCGIFADDR, (char*)ifr) < 0)
			continue;

		sin = (struct sockaddr_in*)addr;
		mreq.imr_multiaddr = sin->sin_addr;
		mreq.imr_interface.s_addr =
		    ((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr.s_addr;
		if (leave_group(fd, &mreq) < 0) {
			return (-1);
		}
		count++;
	}
	if (count == 0) {
		/* Could not find a suitable interface */
		if (errno == 0)
			errno = ENOTCONN;
		return (-1);
	}
	return (0);
}

#else /* !MULTICAST */
/*ARGSUSED*/
int
net_join_multicast_group(
	int			fd,
	struct sockaddr_in*	addr)
{
	errno = EOPNOTSUPP;
	return (-1);
}

/*ARGSUSED*/
int
net_leave_multicast_group(
	int			fd,
	struct sockaddr_in*	addr)
{
	errno = EOPNOTSUPP;
	return (-1);
}
#endif /* !MULTICAST */


/*
 * Look up a given string in the hosts map.  If found, return the IP address.
 * Returns -1 if error, else 0.
 */
static int
net_lookup_hostname(
	char*			host,
	struct sockaddr_in*	to)
{
	struct hostent*		hp;

	hp = gethostbyname(host);
	if ((hp != NULL) && (hp->h_addrtype == AF_INET)) {
		to->sin_family = AF_INET;
		memmove(&to->sin_addr, hp->h_addr, (int)hp->h_length);
	} else {
		errno = ENONET;
		return (-1);
	}
	return (0);
}

/*
 * Parse a string as either an IP multicast address to an NIS hostname
 * with which to look up an address.
 * If the string is "BROADCAST", set an address to flag UDP broadcast.
 * Store the final address in the 'sap' argument.
 * If 'sap' is NULL, this routine merely checks the validity of 'str'.
 *
 * Return -1 if error, 1 if UDP broadcast, else 0.
 */
int
net_parse_address(
	char*			str,
	struct sockaddr_in*	sap)
{
	struct sockaddr_in	sin;

	/* Check for special case names */
	if (strcmp(str, NET_BROADCAST_NAME) == 0) {
		if (sap != NULL) {
			sap->sin_addr.s_addr = (u_long)-1;
			sap->sin_family = AF_INET;
			sap->sin_port = 0;
		}
		return (1);

	/* Check for a numeric IP address */
	} else if ((sin.sin_addr.s_addr = inet_addr(str)) != -1) {
		sin.sin_family = AF_INET;
		sin.sin_port = 0;

	/* Lookup string as a hostname */
	} else if (net_lookup_hostname(str, &sin) < 0) {
		return (-1);
	}

	/* Verify that multicasting is supported in the first place */
	if (!net_multicast_supported()) {
		errno = EPROTONOSUPPORT;
		return (-1);
	}

	/* Verify that the address is a multicast address before returning it */
	if (!net_is_multicast_addr(&sin)) {
		errno = EADDRINUSE;
		return (-1);
	}

	/* Copy out the parsed address */
	if (sap != NULL)
		*sap = sin;
	return (0);
}

/*
 * Convert four bytes of IP address to a struct sockaddr_in.
 * The address must be a valid multicast address.
 */
int
net_extract_multicast_address(
	char*			addr,
	struct sockaddr_in*	sap)
{
	struct sockaddr_in	sin;

	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = addr[0] << 24 | (addr[1] & 0xff) << 16 |
	    (addr[2] & 0xff) << 8 | (addr[3] & 0xff);
	if (!net_is_multicast_addr(&sin)) {
		errno = EADDRINUSE;
		return (-1);
	}
	*sap = sin;
	return (0);
}

/*
 * Exported routine for checking if an address is a valid multicast address.
 */
int
net_is_multicast_addr(
	struct sockaddr_in*	sap)
{
	return (IN_CLASSD(sap->sin_addr.s_addr));
}

/*
 * Set the multicast time-to-live (hop count)
 */
int
net_set_range(
	struct net_broadcast*	nbp,
	int			val)
{
#ifdef MULTICAST
	unsigned char		ttl;

	ttl = (u_char) val;
	if (setsockopt(nbp->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
	    (char*)&ttl, sizeof (ttl)) < 0) {
		return (-1);
	}
#endif /* MULTICAST */
	return (0);
}


/*
 * Check that this is compiled with multicasting and that the kernel supports
 * multicasting.  Returns TRUE if supported, else FALSE.
 */
int
net_multicast_supported()
{
#ifdef MULTICAST
	int			sock;
	unsigned char		ttl;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return (0);

	ttl = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl,
	    sizeof (ttl)) < 0) {
		(void) close(sock);
		return (0);
	}
	(void) close(sock);
	return (1);
#else /* !MULTICAST */
	return (0);
#endif /* !MULTICAST */
}
