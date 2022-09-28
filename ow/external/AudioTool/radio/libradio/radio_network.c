/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)radio_network.c	1.15	92/06/24 SMI"

/*
 * Radio Free Ethernet support subroutines
 */
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <multimedia/archdep.h>
#include "radio_network.h"

/*
 * Radio Free Ethernet has two modes of broadcast operation:
 *	IP Multicast
 *	UDP Broadcast
 * Though IP Multicasting is generally preferred, UDP Broadcast
 * is provided for networks or systems that do not support multicasting.
 *
 * When using IP Multicasting, radio station identification messages are
 * broadcast to a well-known address in a multicast group.  This Id Address
 * is constructed by replacing the low order byte of a Base Address with 255.
 * Each active station broadcasts audio data to a Data Address constructed
 * by replacing the low order byte of the Base Address with a value between
 * 0 and 254 (determined by hashing the station call letters and host
 * IP address).  The default Base Address is determined by looking up
 * the name 'RadioFreeEthernet' in the hosts NIS map.  An alternative
 * address or hostname may be set by calling radio_set_address().
 *
 * When using UDP Broadcast, radio station identification and audio data
 * are all broadcast on the net, differentiated only by a header flag.
 * UDP Broadcasting is enabled by calling radio_set_address() with a
 * hostname of "BROADCAST".
 *
 * The IP port number is determined by looking up the name 'radio' in
 * the services NIS map.  An alternative port number or service name
 * may be set by calling radio_set_service().
 */

static int			radio_port = 0;	/* service port number */
static struct sockaddr_in	radio_base;	/* base broadcast address */
static int			Nfds = 0;	/* getdtablesize() return val */


#ifndef SUNOS41
/* Get the number of file descriptors */
int
getdtablesize()
{
	return (sysconf(_SC_OPEN_MAX));
}

#include <sys/utsname.h>
#include <sys/systeminfo.h>
/* Get the host id */
long
gethostid()
{
	char			buf[257];

	(void) sysinfo(SI_HW_SERIAL, buf, sizeof (buf));
	return (atol(buf));
}
#endif /* SUNOS41 */


/* Get the number of file descriptors */
static int
nfds()
{
	if (Nfds == 0)
		Nfds = getdtablesize();
	return (Nfds);
}

/*
 * Parse a string as either a port number or NIS services name.
 * Set the radio IP port number accordingly.
 *
 * Returns -1 if error, else 0.
 */
int
radio_set_service(
	char*			str)
{
	int			port;

	/* Try to parse as an integer */
	port = atoi(str);
	if (port == 0) {
		struct servent*	serv;

		/* Try to parse as an NIS services name */
		serv = getservbyname(str, RADIO_SERVICE_TYPE);
		if (serv == NULL) {
			errno = EPROTONOSUPPORT;
			return (-1);
		}
		port = serv->s_port;
	}
	radio_port = port;
	return (0);
}

/*
 * Parse a string as either an IP address or NIS hostname.
 * Set the Multicast Base Address accordingly.
 * If the string is "BROADCAST", initialize for UDP Broadcast.
 *
 * Return -1 if error, else 0.
 */
int
radio_set_address(
	char*			str)
{
	struct sockaddr_in	base;

	if (net_parse_address(str, &base) < 0)
		return (-1);
	radio_base = base;
	return (0);
}

/*
 * Reset the low order byte of a given base IP address.
 */
static void
adjust_addr(
	struct sockaddr_in*	addr,
	unsigned		adjust)
{
	/* Mask off the low byte and set in a new one */
	addr->sin_addr.s_addr &= ~0xff;
	addr->sin_addr.s_addr |= (adjust & 0xff);
}

/*
 * Initialize the broadcast/multicast port and addresses.
 * The id and data addresses are set into the supplied structure.
 * If the kernel doesn't support multicasting revert to broadcast.
 *
 * Returns -1 if error, else 0.
 */
static int
radio_initaddress(
	struct radio_address*	rap,
	char*			callname)
{
	long			rnd;

	/*
	 * Initialize the Base Address, if necessary.
	 * If multicasting is not supported, or the default name is not
	 * found, revert to UDP broadcast.
	 */
	if (radio_base.sin_family == 0) {
		if (radio_set_address(RADIO_DEFAULT_ADDRESS) < 0) {
			if (radio_set_address(NET_BROADCAST_NAME) < 0)
				return (-1);
		}
	}

	/*
	 * Init the id and data addresses from the base address.
	 * If multicasting, reset id and data, if they indicate
	 * a 'generic' address.  Otherwise, use the address verbatim.
	 */
	rap->id = radio_base;
	rap->data = radio_base;
	rnd = radio_base.sin_addr.s_addr & 0xff;

	if ((net_is_multicast_addr(&radio_base)) &&
	    ((rnd == RADIO_ID_ADDRESS) || (rnd == RADIO_DATA_ADDRESS))) {
		/*
		 * Construct a 'random' data address by hashing
		 * the low order byte of our host id
		 * with the station call letters (if any).
		 */
		rnd = gethostid();
		if (callname != NULL) {
			while (*callname != '\0') {
				rnd ^= *callname;
				rnd += *callname++;
			}
		}

		/*
		 * Set the data address.
		 * Don't allow collision with the id address.
		 */
		if (rnd == RADIO_ID_ADDRESS)
			rnd = RADIO_DATA_ADDRESS;
		adjust_addr(&rap->data, rnd);

		/* Set id address to well-known address */
		adjust_addr(&rap->id, RADIO_ID_ADDRESS);
	}

	/* Initialize the port number, if necessary */
	if (radio_port == 0) {
		if (radio_set_service(RADIO_DEFAULT_SERVICE) < 0)
			return (-1);
	}

	/* Set the port number into the broadcast addresses */
	rap->id.sin_port = (u_short) radio_port;
	rap->data.sin_port = (u_short) radio_port;
	return (0);
}

/*
 * Initialize and return a radio_broadcast structure.
 * 'interfaces' is a NULL-terminated list of interface name strings.
 * If 'interfaces' is NULL, broadcast to all likely network interfaces.
 * 'callname', if non-NULL, points to 4-character call letter string.
 * 'range' denotes signal strength (multicast hop count).
 */
int
radio_initbroadcast(
	char**			interfaces,
	char*			callname,
	int			range,
	struct radio_broadcast*	rbp)
{
	char			hostname[MAXHOSTNAMELEN + 1];
	struct hostent*		host;
	struct sockaddr_in*	sip;
	struct net_broadcast*	broad;
	int			i;

	/* Clear the supplied structure */
	(void) memset((void *)rbp, 0, sizeof (*rbp));

	/* Init the broadcast addresses */
	if (radio_initaddress(&rbp->addrs, callname) < 0)
		return (-1);

	/* Find the host number */
	if (gethostname(hostname, sizeof (hostname)) < 0)
		return (-1);
	host = gethostbyname(hostname);
	if (host == NULL) {
		errno = ENXIO;
		return (-1);
	}

	/* Init the broadcast interfaces */
	if (net_initbroadcast(interfaces, &broad) < 0)
		return (-1);

	/* Init the radio_broadcast structure */
	(void) memmove(rbp->id.station, host->h_addr, sizeof (rbp->id.station));
	if (callname != NULL) {
		radio_callname_copy(callname, rbp->id.callname);
	}
	(void) cuserid(rbp->id.username);
	rbp->seqno = 0;
	rbp->broadcast = broad;

	/* Set the upper limit on packetsize */
	if (net_is_multicast_addr(&rbp->addrs.data)) {
		rbp->maxpkt = RADIO_MAX_MULTICAST;

		/* Set the IP time-to-live (hop count) for multicast packets */
		if (net_set_range(broad, range) < 0) {
			net_closebroadcast(broad);
			return (-1);
		}
	} else {
		rbp->maxpkt = RADIO_MAX_BROADCAST;
	}

	/* Set the broadcast destination port number */
	sip = (struct sockaddr_in*) broad->target;
	for (i = 0; i < broad->cnt; i++) {
		sip->sin_port = (u_short) radio_port;
		sip++;
	}

	/* Broadcast a station identification packet */
	i = radio_broadcastid(rbp);
	if (i < 0) {
		net_closebroadcast(broad);
	}
	return (i);
}

/*
 * Close a radio broadcast structure.
 * Transmits an Off-The-Air message.
 */
void
radio_closebroadcast(
	struct radio_broadcast*	rbp)
{
	(void) radio_broadcastsignoff(rbp);
	net_closebroadcast(rbp->broadcast);
	rbp->broadcast = NULL;
}

/*
 * Set up header for radio broadcast
 * Inits callname, host, seqno fields.  Caller must set type field.
 */
static void
radio_broadhdr(
	struct radio_broadcast*	rbp,
	struct radio_hdr*	hdr)
{
	radio_callname_copy(rbp->id.callname, hdr->callname);
	(void) memmove(hdr->host, &rbp->id.station[RADIO_HOSTOFF],
	    sizeof (hdr->host));
	ENCODE_LONG(&rbp->seqno, hdr->seqno.b);
}

/*
 * Broadcast audio data.
 * Breaks up large requests into broadcast-able pieces.
 */
int
radio_broadcastdata(
	struct radio_broadcast*	rbp,
	unsigned char*		buf,
	int			bufsiz,
	char			type)
{
	struct radio_receiver	x;
	int			size;

	radio_broadhdr(rbp, &x.hdr);
	x.hdr.type = type;
	size = rbp->maxpkt;

	/* Break data into multiple transmissions, if necessary */
	while (bufsiz > 0) {
		if (size > bufsiz)
			size = bufsiz;
		(void) memmove(x.msg.data, buf, size);
		if (net_broadcast(rbp->broadcast, &rbp->addrs.data,
		    (char*)&x.hdr, (size + RADIO_HDR_SIZE)) < 0)
			return (-1);
		/* update ptr, ctr, sequence number; reset seqno in hdr */
		bufsiz -= size;
		buf += size;
		rbp->seqno += size;
		ENCODE_LONG(&rbp->seqno, x.hdr.seqno.b);
	}
	return (0);
}

/*
 * Broadcast a station identification packet.
 * If the multicast data address is a multicast address we encode that address
 * in the frequency field so that the receiver knows what multicast address
 * to "tune in to" for the data packets.
 */
int
radio_broadcastid(
	struct radio_broadcast*	rbp)
{
	struct radio_receiver	x;

	radio_broadhdr(rbp, &x.hdr);
	x.hdr.type = RADIO_TYPE_STATIONID;
	x.msg.id = rbp->id;

	/* If multicast output, set the data address in the frequency field */
	if (net_is_multicast_addr(&rbp->addrs.data)) {
		struct sockaddr_in*	sin;

		sin = (struct sockaddr_in*)&rbp->addrs.data;
		(void) memmove(x.msg.id.freq, &sin->sin_addr.s_addr,
		    sizeof (x.msg.id.freq));
	}

	if (net_broadcast(rbp->broadcast, &rbp->addrs.id,
	    (char*)&x.hdr, (sizeof (x.msg.id) + RADIO_HDR_SIZE)) < 0)
		return (-1);
	return (0);
}

/*
 * Broadcast an off-the-air packet.
 */
int
radio_broadcastsignoff(
	struct radio_broadcast*	rbp)
{
	struct radio_receiver	x;

	radio_broadhdr(rbp, &x.hdr);
	x.hdr.type = RADIO_TYPE_SIGNOFF;
	x.msg.id = rbp->id;
	if (net_broadcast(rbp->broadcast, &rbp->addrs.id,
	    (char*)&x.hdr, (sizeof (x.msg.id) + RADIO_HDR_SIZE)) < 0)
		return (-1);
	return (0);
}

/*
 * Initialize a radio receiver structure.
 * The 'tuner' structure is cleared.
 * Join the ID multicast group.
 */
int
radio_initreceiver(
	struct radio_receiver*	rrp)
{
	int			fd;
	int			rcv;
	struct sockaddr_in	sin;

	/* Clear the supplied structure */
	(void) memset((void *)rrp, 0, sizeof (*rrp));

	/* Setup default listener addresses */
	if (radio_initaddress(&rrp->addrs, (char*)NULL) < 0)
		return (-1);

	/* Open a socket to listen to */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return (-1);

	/*
	 * Bind the socket to the radio broadcast port.
	 * Increase the receive buffer size, too.
	 */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = (u_short) radio_port;
	rcv = RADIO_RCVBUF;
	if ((bind(fd, (struct sockaddr*)&sin, sizeof (sin)) < 0) ||
	    (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&rcv, sizeof (rcv))
	    < 0)) {
		(void) close(fd);
		return (-1);
	}

	/* Init the receiver structure */
	rrp->fd = fd;

	if (net_is_multicast_addr(&rrp->addrs.id)) {
		/* join the station id multicast address */
		if (net_join_multicast_group(rrp->fd, &rrp->addrs.id) < 0) {
			radio_closereceiver(rrp);
			return (-1);
		}
	}
	return (0);
}

/*
 * Close and deallocate a radio receiver structure.
 * Leave multicast group for both data and id addresses.
 */
void
radio_closereceiver(
	struct radio_receiver*	rrp)
{
	/* leave the station id multicast group */
	(void) net_leave_multicast_group(rrp->fd, &rrp->addrs.id);

	(void) close(rrp->fd);
	rrp->fd = -1;		/* Used in radio_fdset */
}

/*
 * Tune in a radio station.
 * Listens for station identification of a station that matches
 * the radio_id of the given radio_receiver.
 * NULL fields in the 'tuner' structure are treated as wildcards.
 * If 'timeout' is NULL, wait forever.
 * Otherwise, update the timeout value to reflect the time remaining.
 * Minimum timeout is increased to RADIO_ID_TIME * 1.5.
 *
 * If 0 returned, rewrites tuner structure with complete radio_id.
 */
int
radio_tunereceiver(
	struct radio_receiver*	rrp,
	int*			timeout)
{
	int			mintime;
	int			i;

	if (timeout != NULL) {
		mintime = RADIO_ID_TIME + ((RADIO_ID_TIME + 1) / 2);
		if (*timeout < mintime)
			*timeout = mintime;
	}

	/* Loop until timeout is up or station is found */
	while ((timeout == NULL) || (*timeout > 0)) {
		/* Wait for the next station identification to come our way */
		if  (radio_recvid(rrp, timeout) < 0)
			break;

		/* Does the station id/callname match? */
		if (!radio_match_station(rrp))
			continue;		/* no match */

		/* Did this come on the specified multicast address? */
		for (i = 0; i < sizeof (rrp->tuner.freq); i++) {
			if ((rrp->tuner.freq[i] != '\0') &&
			    (rrp->tuner.freq[i] != rrp->msg.id.freq[i]))
				continue;	/* no match */
		}
		/* Did this come from the specified disc jockey? */
		if (rrp->tuner.username[0] != '\0') {
			if (strncmp(rrp->tuner.username, rrp->msg.id.username,
			    sizeof (rrp->tuner.username)) != 0)
				continue;	/* no match */
		}
		/* Found a match */
		rrp->tuner = rrp->msg.id;

		/* Join multicast group specified by frequency field */
		/* XXX - how/when do we ever leave this group? */
		return (radio_join_datagroup(rrp, rrp->tuner.freq));
	}
	return (-1);
}

/* Reset receiver tuner structure */
void
radio_cleartuner(
	struct radio_receiver*	rrp)
{
	(void) memset((void *)&rrp->tuner, 0, sizeof (rrp->tuner));
}

/*
 * Set receiver fds so that user can select()
 * Clears the supplied fd set and sets in the radio receiver fds.
 * Returns the total number of fds (getdtablesize()).
 */
int
radio_fdset(
	struct radio_receiver*	rrp,
	fd_set*			fdset)
{
	FD_ZERO(fdset);
	if ((rrp != NULL) && (rrp->fd >= 0))
		FD_SET(rrp->fd, fdset);
	return (nfds());
}


/*
 * Listen for a radio broadcast message, with the given timeout.
 * If 'timeout' is NULL, wait forever.
 * Otherwise, update the timeout value to reflect the time remaining.
 * Returns 0 if a packet was read, else -1 (errno set).
 *
 * If interrupted by a signal, return immediately if either polling
 * or waiting forever.  If timeout specified, ignore EINTR.
 */
int
radio_recv(
	struct radio_receiver*	rrp,
	int*			timeout)
{
	struct timeval		tval;
	int			poll;
	fd_set			fdset;
	int			err;
	int			s;
	time_t			t;
	time_t			newt;
	unsigned		seqno;

	if (timeout != NULL) {
		tval.tv_usec = 0;
		poll = (*timeout == 0);
		if (!poll) {
			t = time((time_t*)0);
		}
	}

	for (;;) {
		/* If not waiting forever, select() to wait for a packet */
		if (timeout != NULL) {
			tval.tv_sec = *timeout;
			(void) radio_fdset(rrp, &fdset);
			err = select(nfds(), &fdset,
			    (fd_set*)NULL, (fd_set*)NULL, &tval);

			/* update the timeout value */
			if (!poll) {
				newt = time((time_t*)0);
				*timeout -= (newt - t);
				t = newt;
				if (*timeout < 0)
					*timeout = 0;
			}

			/* if select() failed, return (unless EINTR) */
			if (err == 0) {
				errno = ETIMEDOUT;
			}
			if (err <= 0) {
				/* Ignore EINTR if timeout was specified */
				if ((errno != EINTR) || poll ||
				    (timeout == NULL))
					return (-1);
				else
					continue;
			}
		}

		/* Read the next broadcast transmission */
		s = sizeof (rrp->xmitr);
		err = recvfrom(rrp->fd, (char*)&rrp->hdr,
		    (RADIO_HDR_SIZE + RADIO_MAX_LEN), 0, &rrp->xmitr, &s);
		if (err > RADIO_HDR_SIZE)
			break;		/* got a legit packet...we're done */

		if (err < 0)
			return (-1);		/* error in recvfrom() */
		/* If we read a small packet, try again */
	}

	/*
	 * Got a packet:
	 * Set the size of the data block and
	 * Convert the seqno to machine format.
	 */
	rrp->datasize = err - RADIO_HDR_SIZE;
	DECODE_LONG(rrp->hdr.seqno.b, &seqno);
	rrp->hdr.seqno.u = seqno;
	return (0);
}

/*
 * Listen for a radio station identification, with the given timeout.
 */
int
radio_recvid(
	struct radio_receiver*	rrp,
	int*			timeout)
{
	while (radio_recv(rrp, timeout) == 0) {
		if ((rrp->hdr.type & RADIO_TYPE_MASK) == RADIO_TYPE_STATIONID)
			return (0);
	}
	return (-1);
}


/*
 * Listen for a radio station broadcast, with the given timeout.
 * The timeout is used for station id messages, not data broadcasts.
 */
int
radio_recvdata(
	struct radio_receiver*	rrp,
	int*			timeout)
{
	int			t;

	if (timeout != NULL)
		t = *timeout;
	for (;;) {
		/* If timeout, break out of infinite loop */
		if (radio_recv(rrp, timeout) < 0)
			break;
		switch (rrp->hdr.type & RADIO_TYPE_MASK) {
		case RADIO_TYPE_STATIONID:
		case RADIO_TYPE_SIGNOFF:
			/* station id resets timeout */
			if (timeout != NULL)
				*timeout = t;
			break;		/* back to top of loop */
		default:
			return (0);	/* got a data transmission */
		}
	}
	return (-1);
}

/*
 * Listen for a radio station broadcast, matching the tuner frequency.
 * If specified, the timeout value is used to detect station off-the-air.
 */
int
radio_recvstation(
	struct radio_receiver*	rrp,
	int*			timeout)
{
	int			t;

	if (timeout != NULL)
		t = *timeout;
	for (;;) {
		/* If timeout, break out of infinite loop */
		if (radio_recv(rrp, timeout) < 0)
			break;

		/*
		 * If station id, check if it is the desired station.
		 * If so, reset the timeout (station broadcast is muted).
		 */
		switch (rrp->hdr.type & RADIO_TYPE_MASK) {
		case RADIO_TYPE_SIGNOFF:
			if (radio_match_station(rrp)) {
				/* station signed off */
				errno = EHOSTDOWN;
				return (-1);
			}
			break;

		case RADIO_TYPE_STATIONID:
			if (radio_match_station(rrp)) {
				/* station id resets timeout */
				if (timeout != NULL)
					*timeout = t;
				break;
			}
			if ((timeout != NULL) && (*timeout < 2))
				*timeout += 1;	/* allow a little slop */
			break;

		default:
			/* Got a data transmission.  Is it the right station? */
			if (radio_match_station(rrp))
				return (0);	/* got a data transmission */
		}
	}
	return (-1);
}


/*
 * Check that the most recent input packet matches the radio station
 * set in the 'tuner' structure.  Returns TRUE, if match.
 */
int
radio_match_station(
	struct radio_receiver*	rrp)
{
	int			i;
	int			j;

	/* Check for call letter match, if any supplied */
	if (rrp->tuner.callname[0] != '\0') {
		if (strncmp(rrp->tuner.callname, rrp->hdr.callname,
		    sizeof (rrp->tuner.callname)) != 0)
			return (0);
	}

	/*
	 * Check for transmitter match, if any supplied.
	 * If this is a station identification, check entire id.
	 * Otherwise, check just the low order id.
	 */
	if ((rrp->hdr.type & RADIO_TYPE_MASK) == RADIO_TYPE_STATIONID) {
		for (i = 0; i < sizeof (rrp->tuner.station); i++) {
			if ((rrp->tuner.station[i] != '\0') &&
			    (rrp->tuner.station[i] != rrp->msg.id.station[i]))
				return (0);
		}
	} else {
		for (i = 0, j = RADIO_HOSTOFF;
		    i < sizeof (rrp->hdr.host); i++, j++) {
			if ((rrp->tuner.station[j] != '\0') &&
			    (rrp->tuner.station[j] != rrp->hdr.host[i]))
				return (0);
		}
	}
	return (1);
}

/* Declare static storage for radio_callname() return value */
static char	callname[RADIO_CALLNAME_SIZE + 1];

/*
 * Convert the four-letter call name to a string.
 * Returns a ptr to static storage.
 */
char *
radio_callname(
	char*		name)
{
	char*		l;

	radio_callname_copy(name, callname);

	/* Terminate string, and remove trailing spaces */
	l = &callname[sizeof (callname) - 1];
	*l-- = '\0';
	while ((l >= callname) && (*l == ' '))
		*l-- = '\0';
	return (callname);
}

/*
 * Copy a callname, padding with spaces, if necessary.
 * Shift leading spaces to the end of the string.
 * XXX - filter non-printable characters? [internationalization concerns]
 */
void
radio_callname_copy(
	char*		from,
	char*		to)
{
	int		i;
	char*		tmp;

	tmp = to + RADIO_CALLNAME_SIZE;
	for (i = 0; i < RADIO_CALLNAME_SIZE; i++) {
		if (*from == '\0')
			*to++ = ' ';
		else if (*from == ' ')
			from++;
		else
			*to++ = *from++;
	}
	while (to < tmp)
		*to++ = ' ';
}

/*
 * Convert a broadcast station id to a host name string.
 * Returns a ptr to static storage.
 */
char *
radio_hostname(
	char*		addr)
{
	struct hostent*	h;

	h = gethostbyaddr(addr, sizeof (struct in_addr), AF_INET);
	if (h == NULL)
		return (inet_ntoa(*(struct in_addr*)&addr));
	else
		return (h->h_name);
}


/*
 * Join a multicast group.
 * The multicast address is represented as a four character array.
 * A zero address means the data transmission is UDP broadcast.
 *
 * Returns -1 on error, else 0.
 */
int
radio_join_datagroup(
	struct radio_receiver*	rrp,
	char*			addr)
{
	struct sockaddr_in	sin;

	if (net_extract_multicast_address(addr, &sin) < 0) {
		/* All zeroes indicates broadcast */
		if ((addr[0] == '\0') && (addr[1] == '\0') &&
		    (addr[2] == '\0') && (addr[3] == '\0'))
			return (0);
		else
			return (-1);
	}
	return (net_join_multicast_group(rrp->fd, &sin) < 0);
}

/*
 * Leave a multicast group.
 * The multicast address is represented as a four character array.
 * A zero address means the data transmission is UDP broadcast.
 *
 * Returns -1 on error, else 0.
 */
int
radio_leave_datagroup(
	struct radio_receiver*	rrp,
	char*			addr)
{
	struct sockaddr_in	sin;

	if (net_extract_multicast_address(addr, &sin) < 0) {
		/* All zeroes indicates broadcast */
		if ((addr[0] == '\0') && (addr[1] == '\0') &&
		    (addr[2] == '\0') && (addr[3] == '\0'))
			return (0);
		else
			return (-1);
	}
	return (net_leave_multicast_group(rrp->fd, &sin) < 0);
}
