/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)radio_recv.c	1.22	92/06/29 SMI"

/* Radio Free Ethernet receiver utility */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <malloc.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/filio.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>

#ifdef G721
#include <multimedia/audio_encode.h>
#endif /* G721 */

#include "radio.h"
#include "radio_network.h"

/* Macro to translate strings */
#ifndef SUNOS41
#define	T(str)	(dgettext("radio-labels", str))
extern char*	dgettext(char*, char*);
#else
#define	T(str)	str
#endif

/* Define time, in seconds, of no broadcast before assuming station is dead */
#define	RADIO_TIMEOUT	(RADIO_ID_TIME * 6)

/* If two stations broadcast id on the same frequency n times, print a msg */
#define	BOGUS_CNT	(4)


/* Station state structure */
typedef struct station {
	struct station*	next;			/* pointer to next */
	struct station*	last;			/* pointer to previous */
	struct radio_id	id;			/* station id */
	unsigned	datacnt;		/* data msg ctr */
	unsigned	idcnt;			/* station id msg ctr */
	unsigned	timeout;		/* timeout count */
	unsigned	seqno;			/* for duplicate detection */
	int		idset;			/* TRUE if station id set */
	int		idle;			/* TRUE if no recent packet */
	int		scanseen;		/* TRUE if scanned recently */
	int		bogus;			/* bogus msg flag */
	int		joined_group;		/* joined multicast group */
} Station_t;


/* Local variables */
char*	prog;					/* program name */

int	Quit = FALSE;				/* if TRUE, cleanup and quit */
int	Command_mode = FALSE;			/* if TRUE, read stdin cmds */
int	Timer_interval = 2;			/* set time for two seconds */
int	Timer_flag = 0;				/* incremented every timeout */
sigset_t Timer_mask;				/* signal mask */
int	Report_flag = 0;			/* incremented every second */
int	Audio_fd = -1;				/* fd for audio output data */
int	Audioctl_fd = -1;			/* fd for audio control */
int	Checkrelease = FALSE;			/* TRUE if SIGPOLL */
Audio_hdr Output_hdr;				/* output data type */
int	Station_listchanged = FALSE;		/* TRUE if new stations */
int	State = POWER_OFF;			/* current state */

struct radio_receiver	Rcv;			/* Receiver structure */
char	Station[RADIO_CALLNAME_SIZE + 1];	/* station */
char*	Audio_name;				/* audio output */
int	Start_scan = FALSE;			/* scan for another station */
int	Scan_timeout;				/* timeout during scan */
int	Release = TRUE;				/* if TRUE, release audio dev */
int	Report;					/* if TRUE, report stations */

Station_t	Station_list;			/* station list head */
Station_t*	Current_station;		/* current station ptr */

char		Last_type = 0;			/* previous data type */

#ifdef G721
/* Define structures for G.721 decompression */
struct audio_g72x_state	g721_state;		/* decompression state */
unsigned char*		g721_buf = NULL;	/* decompress buffer */
unsigned		g721_bufsiz = 0;	/* decompress buffer size */
#endif /* G721 */


/* Local routines */


/* Global variables */
extern int	optind;
extern char*	optarg;


/*ARGSUSED*/
void
quit_handler(
	int			sig)
{
	if (Quit)
		exit(1);	/* 2nd try really kills it */
	Quit = TRUE;
}

/*ARGSUSED*/
void
timer_handler(
	int			sig)
{
	Timer_flag += Timer_interval;
	Report_flag += Timer_interval;
}

/*ARGSUSED*/
void
sigpoll_handler(
	int			sig)
{
	Checkrelease = TRUE;
}

#ifdef SUNOS41
/* Simulate SVR4 sigprocmask() function.  Caches mask in a static variable. */
#define sigprocmask(a, b, c)	sigprocmask_4x(a, b, c)

static int
sigprocmask_4x(
	int			how,
	const sigset_t*		set,
	sigset_t*		oset)
{
	static sigset_t		prevmask = 0;

	if (oset != NULL)
		*oset = prevmask;

	switch (how) {
	case SIG_BLOCK:
		prevmask = sigblock(*set);
		prevmask |= *set;
		break;
	case SIG_UNBLOCK:
		prevmask &= ~(*set);
		(void) sigsetmask(prevmask);
		break;
	case SIG_SETMASK:
		(void) sigsetmask(*set);
		prevmask = *set;
		break;
	default:
		errno = EINVAL;
		return (-1);
	}
	return (0);
}

/* Simulate SVR4 sigset() function. */
static void(*
sigset(
	int			sig,
	void			(*disp)(int)))(int)
{
	signal(sig, disp);
}
#endif /* 4.x */


void
usage()
{
	FPRINTF(stderr, "%s\n\t%s ",
	    T("Radio Receiver -- usage:"), prog);
	FPRINTF(stderr, T("\t[-C] [commands]\nwhere:\n"));
	FPRINTF(stderr, T("\t-C\tRead commands from stdin\n\n"));
	FPRINTF(stderr, T("Valid commands are:\n"));
	FPRINTF(stderr, T("\t%s=[call letters]\n"), RADIOCMD_STATION);
	FPRINTF(stderr, T("\t%s=[audio device name]\n"), RADIOCMD_OUTPUT);
	FPRINTF(stderr, T("\t%s=[YES|NO]\n"), RADIOCMD_REPORT);
	FPRINTF(stderr, T("\t%s=[YES|NO]\n"), RADIOCMD_RELEASE);
	FPRINTF(stderr, T("\t%s\n"), RADIOCMD_SCAN);
	FPRINTF(stderr, T("\t%s\n"), RADIOCMD_START);
	FPRINTF(stderr, T("\t%s\n"), RADIOCMD_STOP);
	FPRINTF(stderr, T("\t%s\n"), RADIOCMD_QUIT);
	FPRINTF(stderr, T("\t%s=[service name|port number]\n"),
	    RADIOCMD_SERVICE);
	FPRINTF(stderr, T("\t%s=[multicast hostname|ip address]\n"),
	    RADIOCMD_ADDRESS);
	exit(1);
}

void
error()
{
	if (Command_mode)
		return;
	exit(1);
}


/*
 * Join a multicast group to see specific data.
 * XXX - what does it mean if there's an error?
 */
int
join_datagroup(
	Station_t*	sp)
{
	if ((sp->id.freq[0] != '\0') && !sp->joined_group) {
		/* Need to join multicast group */
		if (radio_join_datagroup(&Rcv, sp->id.freq) < 0) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Failed to register into multicast group"));
			return (FALSE);
		}
		sp->joined_group = TRUE;
	}
	return (TRUE);
}

/*
 * Leave a multicast group, to let the network quit forwarding to your net.
 */
int
leave_datagroup(
	Station_t*	sp)
{
	if ((sp->id.freq[0] != '\0') && sp->joined_group) {
		/* Need to leave multicast group */
		if (radio_leave_datagroup(&Rcv, sp->id.freq) < 0) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Failed to deregister from multicast group"));
			return (FALSE);
		}
		sp->joined_group = FALSE;
	}
	return (TRUE);
}

/* Station list handling routines */

/* Add a station to the list, if it is not already there */
Station_t*
add_station(
	char*		id)
{
	Station_t*	sp;		/* linked list head */
	Station_t*	newsp;
	char		name[RADIO_CALLNAME_SIZE];

	/* Make sure name is padded with spaces */
	radio_callname_copy(id, name);

	/*
	 * Look to see if station is already in list.
	 * Make sure that sp is always non-NULL when breaking out of this loop.
	 */
	for (sp = &Station_list; ; sp = sp->next) {
		if (strncmp(name, sp->id.callname,
		    sizeof (sp->id.callname)) == 0) {
			/* station is already in list */
			return (sp);
		}
		if (sp->next == NULL)
			break;
	}

	/* Allocate a station structure and fill it in */
	newsp = (Station_t*) calloc(1, sizeof (Station_t));
	newsp->last = sp;
	newsp->next = NULL;
	radio_callname_copy(name, newsp->id.callname);
	sp->next = newsp;

	Station_listchanged = TRUE;
	return (newsp);
}

/* Remove a station from the cache */
void
remove_station(
	Station_t*	sp)
{
	/* If removing the current station, leave the multicast group */
	if (sp == Current_station) {
		(void) leave_datagroup(sp);
		Current_station = NULL;
	}

	/* Link the list around this entry */
	sp->last->next = sp->next;
	if (sp->next != NULL)
		sp->next->last = sp->last;

	(void) free((char*)sp);
	Station_listchanged = TRUE;
}

/* Quit listening to a particular station */
void
clearcurrent()
{
	if (Current_station != NULL) {
		(void) leave_datagroup(Current_station);
		Current_station = NULL;
	}
}

/* Write a list of active stations */
void
report_stations()
{
	Station_t*	sp;

	/* Only report if there was a change and not too recently */
	if (!Station_listchanged || (Report_flag == 0))
		return;

	Station_listchanged = FALSE;
	Report_flag = 0;

	/* Suppress message if reporting is off */
	if (!Report)
		return;

	FPRINTF(stderr, "%s: %s ", prog, RADIOCMD_STATIONREPORT);

	for (sp = Station_list.next; sp != NULL; sp = sp->next) {
		FPRINTF(stderr, "%s%s%s ",
		    sp->idle ? "[" : "",
		    radio_callname(sp->id.callname),
		    sp->idle ? "]" : "");
	}
	FPRINTF(stderr, "\n");
}

/* Report station identification for the current station */
void
report_id(
	Station_t*	sp)
{
	/* Suppress message if reporting is off */
	if (!Report || (State == POWER_OFF))
		return;

	if (sp->idset) {
		FPRINTF(stderr, "%s: %s %s %s@%s\n", prog, RADIOCMD_IDREPORT,
		    radio_callname(sp->id.callname),
		    sp->id.username, radio_hostname(sp->id.station));
	} else {
		FPRINTF(stderr, "%s: %s %s\n", prog, RADIOCMD_IDREPORT,
		    radio_callname(Rcv.tuner.callname));
	}
}

/*
 * This routine is called when there might be two stations transmitting
 * on the same frequency (call letters match).
 * Wait a bit to see if it continues, then print a message.
 * Then mark this station as reported.
 */
void
bogus(
	Station_t*		sp,
	struct radio_id*	idp)
{
	/* If -1 flag, a message was already delivered */
	if (sp->bogus < 0)
		return;

	/* Otherwise, wait for a few overlapping messages to come through */
	if (++sp->bogus >= BOGUS_CNT) {
		/*
		 * Time to print a message.
		 * This is done in parts because radio_hostname()
		 * returns a ptr to static storage.
		 * If the id came via a gateway, print the relay point.
		 */
		FPRINTF(stderr, "%s: %s %s: ", prog,
		    T("Radio interference on "),
		    radio_callname(idp->callname));

		FPRINTF(stderr, "%s@%s",
		    sp->id.username, radio_hostname(sp->id.station));
		FPRINTF(stderr, " || %s@%s",
		    idp->username, radio_hostname(idp->station));
		FPRINTF(stderr, "\n");

		/* Mark this station as having already sent a warning */
		sp->bogus = -1;
	}
}


/* Wait until audio output is drained */
void
drain_audio()
{
	if (Audio_fd >= 0)
		(void) audio_drain(Audio_fd, FALSE);
}

/* Close the audio device */
void
close_audio()
{
	if (Audio_fd >= 0) {
		(void) audio_flush_play(Audio_fd);
		(void) close(Audio_fd);
		Audio_fd = -1;
	}
}


/* Set up the next likely station to scan */
void
next_scan()
{
	Station_t	*sp;
	int		pass;

	/* Clear out the current station */
	if (Current_station != NULL) {
		Current_station->scanseen = TRUE;
		clearcurrent();
	}
	Scan_timeout = -1;
	pass = 0;

	/* Try two passes through the station list */
	while (pass < 2) {
		/* Find the next likely station */
		for (sp = Station_list.next; sp != NULL; sp = sp->next) {
			/* Skip stations already scanned */
			if (sp->scanseen)
				continue;

			/* Skip stations that have not issued station id yet */
			if (!sp->idset)
				continue;

			if (sp->idle) {
				/* Skip stations that are known to be idle */
				continue;
			} else if (sp->id.freq[0] != '\0') {
				/* Look for the next multicast station */
				(void) join_datagroup(sp);
			}
			Current_station = sp;
			Scan_timeout = 3;
			return;
		}

		/* If no stations, reset flags and try again */
		for (sp = Station_list.next; sp != NULL; sp = sp->next) {
			sp->scanseen = FALSE;
		}
		pass++;
	}
}

/* Set a new state, print a message if necessary */
void
newstate(
	int		state)
{
	if (State == state)
		return;			/* no change */

	State = state;
	if (Report) {
		char*	msg;

		switch (State) {
		case POWER_OFF:
			msg = RADIOCMD_POWEROFF; break;
		case SCANNING:
			/* If playing, flush output */
			if (Audio_fd >= 0)
				audio_flush_play(Audio_fd);
			Start_scan = FALSE;
			next_scan();
			msg = RADIOCMD_SCANNING; break;
		case QUIET:
			msg = RADIOCMD_SQUELCH; break;
		case ON_THE_AIR:
			msg = RADIOCMD_BROADCAST; break;
		case OFF_THE_AIR:
			msg = RADIOCMD_SIGNOFF; break;
		case WAITING:
			msg = RADIOCMD_RELEASE; break;
		}
		FPRINTF(stderr, "%s: %s %s\n", prog,
		    RADIOCMD_STATUS, msg);
	}
}

/* Enable radio reception */
void
poweron()
{
	if (State != POWER_OFF)
		return;

	/* Initialize radio reception */
	if (radio_initreceiver(&Rcv) < 0) {
		if (errno == EADDRINUSE)
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Radio receiver socket is busy"));
		else if (errno == EPROTONOSUPPORT)
			FPRINTF(stderr,
			    T("%s: No '%s' entry in NIS services map\n"),
			    prog, RADIO_DEFAULT_SERVICE);
		else
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Cannot initialize radio receiver"));
		exit(1);
	}

	/* Start up scanning or quiet */
	if (Start_scan) {
		newstate(SCANNING);
	} else {
		newstate(QUIET);
		radio_callname_copy(Station, Rcv.tuner.callname);
	}
}

/* Disable radio reception */
void
poweroff()
{
	Station_t*	sp;

	/* Dismantle the station list */
	while ((sp = Station_list.next) != NULL) {
		remove_station(sp);
	}

	Start_scan = FALSE;
	close_audio();
	radio_closereceiver(&Rcv);
	newstate(POWER_OFF);
}


/*
 * Release audio device if any process is waiting for it.
 * Return TRUE if device was released, else FALSE.
 */
int
release_audio()
{
	Audio_info	info;

	/* Release the device if another process is waiting */
	Checkrelease = FALSE;
	if (Release && (Audio_fd >= 0)) {
		if ((audio_getinfo(Audio_fd, &info) == AUDIO_SUCCESS) &&
		    info.play.waiting) {
			/* Flush and close the device */
			close_audio();
			newstate(WAITING);
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Open and initialize the audio device for a specific data format.
 * If it is already open, check for auto-release.
 * Return 0 if device is open, 1 if device is busy.
 * Return 2 if data format cannot be accomodated.
 * Return -1 if error (poweroff() called).
 */
int
open_audio(
	char	type)
{
	/* If the device is not open, open it */
	if (Audio_fd < 0) {
		if (((Audio_fd = open(Audio_name, O_WRONLY | O_NONBLOCK, 0))
		    < 0) ||
		    (audio_get_play_config(Audio_fd, &Output_hdr)
		    != AUDIO_SUCCESS)) {
			if ((errno == EBUSY) || (errno == EINTR)) {
				newstate(WAITING);
				return (1);
			} else {
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Invalid audio output device"));
				poweroff();
				return (-1);
			}
		} else {
			/* Make sure non-blocking flags are clear */
			(void) fcntl(Audio_fd, F_SETFL,
			    (fcntl(Audio_fd, F_GETFL, 0) &
			    ~(O_NONBLOCK | O_NDELAY)));
		}
	}

	/* Release the device if another process is waiting */
	if (release_audio())
		return (1);

	/* XXX - should try to config output to match radio broadcast */
	switch (type) {
	case RADIO_TYPE_ULAW:
#ifdef G721
	case RADIO_TYPE_G721:
#endif /* G721 */
		if ((Output_hdr.encoding != AUDIO_ENCODING_ULAW) ||
		    (Output_hdr.bytes_per_unit != 1) ||
		    (Output_hdr.channels != 1))
			goto baddata;
		break;
	default:
baddata:
		drain_audio();
		close_audio();
		return (2);
	}
	return (0);
}


/* Input command routines */

/* Station command: set new call letters */
int
station(
	char*		arg)
{
	Station_t*	sp;		/* linked list head */

	/* Disable scanning */
	Start_scan = FALSE;

	/* Copy callname, padding out with spaces and null-terminate */
	radio_callname_copy(arg, Station);
	Station[RADIO_CALLNAME_SIZE] = '\0';

	if (strcmp(Station, Rcv.tuner.callname) == 0)
		return (TRUE);			/* no change to station id */

	/* Clear out current station, if any */
	clearcurrent();

	/* If station is already in list, set to report its id again */
	for (sp = Station_list.next; sp != NULL; sp = sp->next) {
		if (strncmp(Station, sp->id.callname,
		    sizeof (sp->id.callname)) == 0) {
			/* reset the id flag */
			sp->idset = FALSE;
			Current_station = sp;
			(void) join_datagroup(sp);
		}
	}

	/* Reset the receiver tuner to the new station */
	radio_cleartuner(&Rcv);
	radio_callname_copy(Station, Rcv.tuner.callname);

	if (State != POWER_OFF)
		newstate(QUIET);
	return (TRUE);
}


/* Parse a new audio output sink */
int
output(
	char*		arg)
{
	int		fd;

	/* If there's an output filename, free it */
	if (Audio_name != NULL) {
		if (strcmp(arg, Audio_name) == 0)
			return (TRUE);		/* no change to output dev */
		(void) free(Audio_name);
		Audio_name = NULL;
	}

	/* If there's already an open audio output, close it */
	drain_audio();
	close_audio();

	/* Close control device, if any open */
	if (Audioctl_fd >= 0) {
		(void) close(Audioctl_fd);
		Audioctl_fd = -1;
	}

	/* Attempt to verify that this is an audio device */
	fd = open(arg, O_WRONLY | O_NONBLOCK, 0);
	if (fd >= 0) {
		/* Make sure this is an audio device */
		int		err;
		Audio_hdr	hdr;

		err = (audio_get_play_config(fd, &hdr) != AUDIO_SUCCESS) ||
		    (audio_drain(fd, FALSE) != AUDIO_SUCCESS);
		(void) close(fd);
		if (err)
			goto nodev;
	} else {
		/* If we can't open the device, at least check the type */
		struct stat	st;

		if ((stat(arg, &st) < 0) || !S_ISCHR(st.st_mode)) {
nodev:
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Invalid audio output device"));
			error();
			return (FALSE);
		}
	}

	/* Copy new filename */
	Audio_name = malloc(strlen(arg) + 4);
	STRCPY(Audio_name, arg);

	/* Try to open the control device, for auto-release detection */
	/* XXX - there will be better ways to obtain this name */
	STRCAT(Audio_name, "ctl");
	Audioctl_fd = open(Audio_name, O_RDONLY, 0);
	(void) ioctl(Audioctl_fd, I_SETSIG, S_MSG);
	Audio_name[strlen(Audio_name) - 3] = '\0';
	return (TRUE);
}

#ifdef notyet
/* Parse a new audio output recording specification */
/* XXX - not really figured out yet */
int
record(
	char*		arg)
{
	struct stat	st;
	int		fd;

	/* If there's already an open record output, close it */
	/* XXX - should rewrite filesize */
	if (Record_fd >= 0) {
		(void) close(Record_fd);
		Record_fd = -1;
	}


	/* Stat the audio output file; save mode, if file exists */
	if ((stat(arg, &st) >= 0) && !S_ISREG(st.st_mode))
		goto nofile;

	if ((fd = open(arg, O_WRONLY | O_TRUNC | O_CREAT, 0666)) < 0) {
nofile:
		FPRINTF(stderr, "%s: %s\n", prog, arg,
		    T("Cannot open audio record file"));
		return (FALSE);
	}

	/* XXX - What's the output header? */
	if (audio_write_filehdr(fd, &Output_hdr, (char*)NULL, 0) !=
	    AUDIO_SUCCESS) {
		goto nofile;
	}
errret:
	if (fd >= 0)
		(void) close(fd);
	error();
	return (FALSE);
}
#endif


/*ARGSUSED*/
int
scan(char* arg)
{
	/* Clear out current station, if any */
	clearcurrent();

	Start_scan = TRUE;
	radio_cleartuner(&Rcv);
	return (TRUE);
}

int
release(char* arg)
{
	Release = is_on(arg);
	return (TRUE);
}

int
report(char* arg)
{
	Report = is_on(arg);
	return (TRUE);
}

/*ARGSUSED*/
int
start(char* arg)
{
	poweron();
	if (State == POWER_OFF)
		return (FALSE);
	return (TRUE);
}

/*ARGSUSED*/
int
stop(char* arg)
{
	if (State != POWER_OFF)
		poweroff();
	return (TRUE);
}

/* Quit command: set Quit flag */
/*ARGSUSED*/
int
quit(char* arg)
{
	if (Quit++ > 0)
		exit(1);
	return (TRUE);
}

int
address(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change address while receiving"));
		return (FALSE);
	}

	if (radio_set_address(arg) < 0) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Invalid multicast hostname or address"));
		return (FALSE);
	}
	return (TRUE);
}

int
service(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change service while receiving"));
		return (FALSE);
	}

	if (radio_set_service(arg) < 0) {
		if (errno == EPROTONOSUPPORT) {
			FPRINTF(stderr,
			    T("%s: No '%s' entry in NIS services map\n"),
			    prog, arg);
		} else {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Invalid service name or port number"));
		}
		return (FALSE);
	}
	return (TRUE);
}


/*
 * Process a data buffer, decoding data if necessary.
 * Stores output data address and length in outbuf/outsiz.
 * Returns outsiz, or -1 if error.
 */
int
convert_audio(
	char		type,
	unsigned char*	inbuf,
	unsigned	insiz,
	unsigned char**	outbuf,
	unsigned*	outsiz)
{
	int err;

	switch (type) {
#ifdef G721
	case RADIO_TYPE_G721:
		/* Initialize state, if necessary */
		if (Last_type != RADIO_TYPE_G721) {
			/* If previous data type was not g721, re-init */
			g721_init_state(&g721_state);
		}

		/* Allocate buffers, if necessary */
		if (g721_bufsiz < (2 * insiz)) {
			if (g721_buf != NULL)
				(void) free((char*)g721_buf);
			g721_bufsiz = 2 * insiz;
			g721_buf = (unsigned char*)malloc(g721_bufsiz);
		}

		/* Decode data into output buffer */
		err = g721_decode(inbuf, insiz, &Output_hdr, 
		    (void*)g721_buf, (int*)outsiz, &g721_state);
		if (err != AUDIO_SUCCESS) {
			FPRINTF(stderr, T("Error decompressing data\n"));
			outsiz = 0;
			return (-1);
		}

		*outbuf = g721_buf;
		break;
#endif /* G721 */

	case RADIO_TYPE_ULAW:
		/* No data conversion necessary */
		*outsiz = insiz;
		*outbuf = inbuf;
		break;
	default:
		outsiz = 0;
		return (-1);
	}
	Last_type = type;
	return (*outsiz);
}

/* Read audio data from input and broadcast it */
void
recv_data()
{
	Station_t*	sp;
	int		match;
	int		i;
	int*		timeout;
	unsigned char*	outbuf;
	unsigned	outsiz;

	/*
	 * If command mode, scan without waiting.
	 * If not command mode, wait forever the first time only.
	 */
	i = 0;
	timeout = Command_mode ? &i : NULL;
	if (radio_recv(&Rcv, timeout) < 0)
		goto norcv;

	/* Process this message and loop for more */
	do {
		/*
		 * Check the current station for a match.
		 * If scanning, cycle through the stations when data appears.
		 */
		if ((State == POWER_OFF) || (State == SCANNING) ||
		    (Rcv.tuner.callname[0] == '\0')) {
			match = FALSE;
		} else {
			match = radio_match_station(&Rcv);
		}

		/* Make sure this station is in the station list */
		sp = add_station(Rcv.hdr.callname);

		/* Dispatch according to the broadcast packet type */
		switch (Rcv.hdr.type & RADIO_TYPE_MASK) {

		case RADIO_TYPE_STATIONID:	/* Station Identification */
			/* If new id is different, update the id */
			if (!sp->idset ||
			    (memcmp((void*)&Rcv.msg.id, (void*)&sp->id,
			    sizeof (sp->id)) != 0)) {
				/*
				 * If two stations are using the same
				 * call letters, this may be nasty.
				 */
				if (sp->idset) {
					bogus(sp, &Rcv.msg.id);
					match = FALSE;
				} else {
					sp->id = Rcv.msg.id;
					sp->idset = TRUE;
					sp->idcnt++;

					/* If this is the first id, print it */
					if (match)
						report_id(sp);

					/* If scanning, station is eligible */
					if ((State == SCANNING) &&
					    (Current_station == NULL))
						next_scan();
				}
			} else {
				/* Mark old station as still on-the-air */
				sp->idcnt++;
			}

			/* If this is the current station, set the tuner */
			if (match) {
				Rcv.tuner = Rcv.msg.id;
				if (sp != Current_station) {
					clearcurrent();
					Current_station = sp;
				}

				/* Join group specified by frequency field */
				(void) join_datagroup(sp);
			}

			/* Poll for another radio broadcast */
			continue;

		case RADIO_TYPE_SIGNOFF:	/* Station Sign-Off */
			/* If it's the current station, turn off */
			if (match) {
				drain_audio();
				if (Command_mode) {
					close_audio();
					newstate(OFF_THE_AIR);
				} else {
					poweroff();
				}
			}

			/* Delete this station from the active list */
			remove_station(sp);

			/* Poll for another radio broadcast */
			continue;

		case RADIO_TYPE_ULAW:
#ifdef G721
		case RADIO_TYPE_G721:
#endif /* G721 */
			/*
			 * Check for duplicate packets.
			 */
			if ((Rcv.hdr.seqno.u <= sp->seqno) &&
			    ((sp->seqno - Rcv.hdr.seqno.u) < 0x7fffffff) &&
			    (Rcv.hdr.seqno.u != 0)) {
				return;
			}

			/* Update station statistics */
			sp->datacnt++;
			sp->timeout = 0;
			sp->seqno = Rcv.hdr.seqno.u;
			if (sp->idle) {
				sp->idle = FALSE;
				Station_listchanged = TRUE;
			}

			/* If power off, ignore the data */
			if (State == POWER_OFF)
				continue;

			/* If scanning, see if this station qualifies */
			if ((State == SCANNING) &&
			    ((Current_station == NULL) ||
			    (sp == Current_station))) {
				/*
				 * If the data is unplayable, skip it.
				 */
				if ((i = open_audio(Rcv.hdr.type)) != 0) {
					if ((i < 0) || (i > 1)) {
						next_scan();
						continue;
					}
				}

				/* If match, set tuner and report it */
				if (sp->idset) {
					Rcv.tuner = sp->id;
				} else {
					(void) memmove(Rcv.tuner.callname,
					    Rcv.hdr.callname,
					    sizeof (Rcv.tuner.callname));
				}
				if (Current_station == NULL)
					Current_station = sp;
				report_id(sp);
				match = TRUE;
			}

			/* If this station does not match, poll for more */
			if (!match)
				continue;

			sp->scanseen = TRUE;

			/* Make sure we can play this data type */
			i = open_audio(Rcv.hdr.type);
			if (i == 2) {
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Cannot play broadcast data format"));
				poweroff();
				continue;
			} else if (i != 0) {
				continue;
			}

			newstate(ON_THE_AIR);

			/* Process this data buffer, if necessary */
			i = convert_audio(Rcv.hdr.type,
			    Rcv.msg.data, Rcv.datasize, &outbuf, &outsiz);

			/* Write this data buffer out */
			if (i > 0) {
				(void) sigprocmask(SIG_BLOCK, &Timer_mask, 0);
				i = write(Audio_fd, outbuf, outsiz);
				(void) sigprocmask(SIG_UNBLOCK, &Timer_mask, 0);
			}

			if (i <= 0) {
				/* Error writing device */
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Error writing audio output device"));
				poweroff();
			}
			return;

		default:		/* unknown broadcast packet */
			continue;
		}

	/* Poll the receiver to see if any packets have arrived */
	} while (i = 0, (radio_recv(&Rcv, &i) >= 0));

norcv:
	if ((errno != ETIMEDOUT) && (errno != EINTR)) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Error reading radio broadcast"));
		Quit++;
	}
}

/* Command parse table */
Radio_cmdtbl	cmdlist[] = {
	{RADIOCMD_STATION,	station},
	{RADIOCMD_OUTPUT,	output},
	{RADIOCMD_SCAN,		scan},
	{RADIOCMD_RELEASE,	release},
	{RADIOCMD_START,	start},
	{RADIOCMD_STOP,		stop},
	{RADIOCMD_QUIT,		quit},
	{RADIOCMD_REPORT,	report},
	{RADIOCMD_ADDRESS,	address},
	{RADIOCMD_SERVICE,	service},
	{NULL,			(int(*)(char*)) NULL},
};

/* Main radio receiver loop */
static void
do_recv()
{
	/* Loop until there is a signal to quit */
	while (Quit == 0) {

		/* If station signed off and not command mode, quit now */
		if ((State == POWER_OFF) && !Command_mode)
			break;

		/* Print station list if it changed */
		report_stations();

		/* If SIGPOLL, check for auto-release */
		if (Checkrelease)
			release_audio();

		/* Read and queue some data (if any) */
		if (State != POWER_OFF)
			recv_data();

		/* Check cmd input stream; parse commands, if any */
		if (Command_mode) {
			fd_set		fdset;
			int		nfds;

			/* Wait for either radio or command input */
			nfds = radio_fdset(&Rcv, &fdset);
			FD_SET(fileno(stdin), &fdset);
			(void) select(nfds, &fdset, (fd_set*)NULL,
			    (fd_set*)NULL, (struct timeval*)NULL);

			/* Parse command input, quit on end-of-file */
			if (parse_input(fileno(stdin), cmdlist, TRUE, prog) < 0)
				break;
		}

		/* Start scanning, if necessary */
		if ((State != POWER_OFF) && Start_scan)
			newstate(SCANNING);

		/* Every second, check stations for timeout */
		if (Timer_flag > 0) {
			Station_t*	sp;
			Station_t*	tmpsp;

			/* Block timer interrupts during this */
			(void) sigprocmask(SIG_BLOCK, &Timer_mask, 0);

			/* Purge silent stations from the cache */
			tmpsp = Station_list.next;
			while (tmpsp != NULL) {
				sp = tmpsp;
				tmpsp = sp->next;

				/* If station is quiet, look for id */
				if ((sp->datacnt == 0) &&
				    (sp->timeout < (RADIO_TIMEOUT / 2)))
					sp->idcnt = 0;

				/* Check to see if the station is off-the-air */
				if ((sp->datacnt == 0) &&
				    ((sp->timeout += Timer_flag) >=
				    RADIO_TIMEOUT)) {
					/* If station is dead, remove it */
					if (sp->idcnt == 0) {
						remove_station(sp);
						continue;
					} else {
						/*
						 * Idle but on-the-air, and
						 * it is a UDP-broadcast
						 * station.
						 * Otherwise, we wouldn't
						 * really know if it's idle,
						 * since we wouldn't be in the
						 * station's multicast group.
						 */
						if (!sp->idle &&
						    (sp->id.freq[0] == '\0')) {
							sp->idle = TRUE;
							Station_listchanged =
							    TRUE;
						}
					}
				}

				/*
				 * If we're receiving data and this is the
				 * current station, check to see whether
				 * data is coming in.
				 */
				if (((State == ON_THE_AIR) ||
				    (State == WAITING)) &&
				    (memcmp(sp->id.callname, Rcv.tuner.callname,
				    RADIO_CALLNAME_SIZE) == 0)) {
					if (sp->datacnt == 0)
						newstate(QUIET);
				}

				/* Reset counters */
				sp->datacnt = 0;
			}

			/* If scanning, see if the current station is overdue */
			if ((State == SCANNING) && (Scan_timeout >= 0)) {
				Scan_timeout -= Timer_flag;
				if (Scan_timeout <= 0)
					next_scan();
			}

			/* Reset and enable the timer */
			Timer_flag = 0;
			(void) sigprocmask(SIG_UNBLOCK, &Timer_mask, 0);
		}
	}
}

/* Main entry point for radio transmitter */
main(
	int			argc,
	char*			argv[])
{
	int			i;
	int			err;
	struct itimerval	timer;

	/* get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;

	/* Parse start-up options */
	err = 0;
	while ((i = getopt(argc, argv, "C")) != EOF) switch (i) {
	case 'C':			/* read commands from stdin */
		Command_mode = TRUE;
		break;
	default:
		err++;
	}
	if (err > 0)
		usage();

	argc -= optind;			/* update arg pointers */
	argv += optind;

	/* Set up signal handlers to catch ^C, timers, and audio state change */
	(void) sigemptyset(&Timer_mask);
	(void) sigaddset(&Timer_mask, SIGALRM);
	(void) sigaddset(&Timer_mask, SIGPOLL);
	(void) sigset(SIGINT, quit_handler);
	(void) sigset(SIGALRM, timer_handler);
	(void) sigset(SIGPOLL, sigpoll_handler);
	if (Command_mode)
		(void) sigset(SIGPIPE, quit_handler);


	/* Set a timer to go off in a half-second and every interval after */
	(void) memset((void*)&timer, 0, sizeof (timer));
	timer.it_value.tv_usec = 500000;
	timer.it_interval.tv_sec = Timer_interval;
	if (setitimer(ITIMER_REAL, &timer, (struct itimerval*)NULL) < 0) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Error setting interval timer"));
		exit(1);
	}

	/* Parse the rest of the command line */
	if (parse_cmds(argv, cmdlist) != NULL)
		usage();

	/* Finish initialization if not command mode */
	if (!Command_mode) {
		/* If no station name set, start up scanning */
		if ((Station[0] == '\0') || (Station[0] == ' '))
			Start_scan = TRUE;

		/* If no output device name set, use the default */
		if ((Audio_name == NULL) && output(AUDIO_DEVICE))
			poweron();
	}

	/* Enter main processing loop */
	do_recv();

	/* Turn off the timer */
	(void) memset((void*)&timer, 0, sizeof (timer));
	(void) setitimer(ITIMER_REAL, &timer, (struct itimerval*)NULL);

	exit(0);
	return (0);
}
