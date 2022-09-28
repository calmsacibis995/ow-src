/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)at_sel_tcp.c	1.12	92/12/14 SMI"

/* Selection service and Drag N Drop code (XView interface)
 *
 * Alternate Transport Method - TCP Socket routines
 */

#if defined(USE_ATM_TCP) && !defined(OWV2) 
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <values.h>

#include <xview/xview.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <xview/server.h>
#include <xview/panel.h>

#include "segment/segment_canvas.h"
#include "atool_panel.h"
#include "atool_sel_impl.h"
#include "atool_i18n.h"

#include "atool_debug.h"

/* initialize audio data xfer via TCP socket. this only needs to
 * be done once. it sets the port number the TCP socket to be
 * used for the transfer, and opens the socket so we can start accepting
 * connections.
 *
 * all socket info pertains to the selection requestor. when the requestor
 * asks what ATM's the source can support, it'll send the addr/port that
 * it'll accept the connection on for the _SUN_ATM_TCP_SOCKET xfer.
 */
int
audio_atm_tcp_socket_init(
	AudioSelData		*asdp)
{
	struct sockaddr_in	sinfo;	/* TCP socket info */
	int			fd;	/* file descriptor of socket */
	int			count;	/* number of tries to get a port # */
	short			portnum;
	struct hostent		*hp;
	char			hostname[MAXHOSTNAMELEN+1];

	/* init sel data to unitialialized state */
	asdp->req_fd = -1;
	asdp->req_port = 0;
	asdp->req_addr = (unsigned long)0;

	/* set up socket addr info */
	sinfo.sin_family = AF_INET;
	sinfo.sin_addr.s_addr=INADDR_ANY; /* accept connection from any host */
	portnum = AUDIO_PORT;
	sinfo.sin_port = htons(portnum); /* def. port to use (start from) */

	/* create socket */
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DBGOUT((1, "can't create TCP socket\n"));
		return (-1);
	}

	/* a binding we will go... */
	for (count = 0; count < BINDCOUNT; count++) {
		if (bind(fd, (struct sockaddr *)&sinfo, 
			 sizeof(struct sockaddr_in)) == 0) { /* got it */
			break;
		} else if (errno != EADDRINUSE) {
			DBGOUT((1, "can't bind socket\n"));
			return (-1);
		}
		portnum++;
		sinfo.sin_port = htons(portnum);
	}
	
	/* blew it */
	if (count >= BINDCOUNT) {
		DBGOUT((1, "can't bind socket in %d tries\n", count));
		return (-1);
	}

	/* start listening (speak up, i can't here you!) */
	listen(fd, 5);

	DBGOUT((D_DND, "TCP socket initialized, socket=%d, port=%d\n",
	    fd, portnum));

	/* init audio selection data and return */
	asdp->req_fd = fd;
	asdp->req_port = htons(portnum); /* store in nbo */

	/* asdp->this_host is (better be) set by caller */
	if (!(hp = gethostbyname(asdp->this_host))) {
		DBGOUT((1, "can't get host info for %s\n", asdp->this_host));
		asdp->req_addr = (unsigned long)0;
		return (-1);
	}
	asdp->req_addr = *(unsigned long*)hp->h_addr_list[0];
	return (0);
}

/* the selection holder tries to connect to the requestor. 
 * if the connection is made, start to send them the data.
 *
 * the host addr and TCP port are gotten via the selection service
 * when the requestor asks us if we support _SUN_ATM_TCP_SOCKET.
 * they are both send in network byte order, so no conversion is necessary.
 * return file descriptor of socket.
 */

int
audio_tcp_send_start(long unsigned int addr, short unsigned int port)
{
	int			fd;
	struct sockaddr_in	sinfo;
	struct hostent		*hent;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DBGOUT((1, "Audio_TCP_SEND_START, can't open socket\n"));
		return (-1);
	}

	sinfo.sin_family = AF_INET;
	sinfo.sin_addr.s_addr = (unsigned long)addr;
	sinfo.sin_port = port;

	if (connect(fd, &sinfo, sizeof(sinfo)) < 0) {
		DBGOUT((1, "can't connect to TCP socket at 0x%x port %d\n",
		    addr, port));
		close(fd);
		return (-1);
	}

	return fd;
}


/* initiate the xfer of audio data. at this point's we've received
 * a request via the selection service. now we need to register the
 * socket fd with the notifier. when the notifier wakes up, we
 * accept the connection and transfer will begin. when transfer is
 * is done we'll unregister the fd with the notifier so we can't get
 * requests that aren't explicitly initiated by a selection request.
 *
 * these functions return the port/socket name used by the requestor
 * (sent back via selsvc).
 *
 * XXX potential problem here: since listen queues up requests, we've
 * got to be sure that the connection is comming from the same host
 * that initiated the selection.
 */

int audio_tcp_req_start(AudioSelData *asdp)
{
	if (asdp->req_fd < 0) {
		DBGOUT((1, "can't start xfer, TCP socket unset\n"));
		return (-1);
	}

	/* xview will wake up when the remote does a "connect"  */
	DBGOUT((D_DND, "audio_tcp_req_start: waiting for connection\n"));
	notify_set_input_func(asdp, audio_tcp_accept, asdp->req_fd);
	return 0;
}

void audio_tcp_req_done(AudioSelData *asdp)
{
	if (asdp->req_fd < 0) {
		DBGOUT((1, "can't cancel xfer, TCP socket unset?!?\n"));
		return;
	}
	DBGOUT((D_DND, "audio_tcp_req_cancel: cancelling request\n"));
	notify_set_input_func(asdp, NOTIFY_FUNC_NULL, asdp->req_fd);
}

Notify_value audio_tcp_accept(
	AudioSelData		*asdp,
	int			sockfd)		/* socket fd to accept on */
{
	int			fd;		/* fd returned by accept */
	struct sockaddr_in 	from;
	int 			len = sizeof(struct sockaddr_in);
	struct hostent		*hent = NULL;
	char			*addr = NULL;

	DBGOUT((D_DND, "in audio_tcp_accept, about to accept\n"));

	/* ok, i'll talk to you */
	if ((fd = accept(sockfd, (struct sockaddr *)&from, &len)) < 0) {
		DBGOUT((1, "error accepting connection\n"));
		notify_set_input_func(asdp, NOTIFY_FUNC_NULL, sockfd);
		return (NOTIFY_DONE);
	}

	DBGOUT((D_DND, "accepted connection from seln holder.\n"));

	/* save the fd so we can start reading the data, unregister
	 * this input func on the accept descriptor, register the
	 * data descriptor so we'll wake up when the holder starts
	 * sending us stuff.
	 */
	
	asdp->holder_fd = fd;

	notify_set_input_func(asdp, NULL, asdp->req_fd);
	notify_set_input_func(asdp, audio_tcp_read, asdp->holder_fd);

	return NOTIFY_DONE;
}

Notify_value audio_tcp_read(
	AudioSelData		*asdp,
	int			sockfd)		/* socket fd to accept on */
{
	int			fd;		/* fd returned by accept */
	struct sockaddr_in 	from;
	int 			len = sizeof(struct sockaddr_in);
	struct hostent		*hent = NULL;
	char			*addr = NULL;

	DBGOUT((D_DND, "in audio_tcp_read: about to read audio data\n"));

	notify_set_input_func(asdp, NULL, asdp->holder_fd);

	/* we can go ahead and paste the selection */

	/* XXX - the right way to do this is to read from the pipe
	 * into a buffer so we can support enum item's
	 */
	if (AudPanel_InsertFromPipe(asdp->ap, asdp->holder_fd,
		(asdp->xfertype == S_Load), (asdp->xfertype == S_Drop))
		!= AUDIO_SUCCESS) {
		DBGOUT((1, "did not get selection\n"));
	} else {
		DBGOUT((D_DND, "selection transferred!!!!\n"));
		/* set up link, etc... */
		finish_insert_data(asdp);
	}
	close(asdp->holder_fd);

	return NOTIFY_DONE;
}

#else /* OWV2 || !USE_ATM_TCP */

static char *_msg = "Sorry, SUN_ATM_TCP_SOCKET not supported";

#endif /* OWV2 || !USE_ATM_TCP */
