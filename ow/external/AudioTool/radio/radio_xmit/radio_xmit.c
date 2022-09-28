/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)radio_xmit.c	1.19	92/06/29 SMI"

/* Radio Free Ethernet broadcast utility */

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

#include "audio_agc.h"
#include "radio.h"
#include "radio_network.h"


/* Macro to translate strings */
#ifndef SUNOS41
#define	T(str)	(dgettext("radio-labels", str))
extern char*	dgettext(char*, char*);
#else
#define	T(str)	str
#endif


/* Local variables */
char*	prog;					/* program name */

int	Quit = FALSE;				/* if TRUE, cleanup and quit */
int	Command_mode = FALSE;			/* if TRUE, read stdin cmds */
int	Timer_flag = 0;				/* incremented every second */
sigset_t Timer_mask;				/* signal mask */
int	Id_ctr;					/* station id time counter */
int	Id_flag = FALSE;			/* if TRUE, xmit station id */
int	Audio_fd = -1;				/* fd for audio input data */
int	Device_input;				/* FALSE, if file input */
Audio_hdr Input_hdr;				/* input data type */
unsigned char* Audio_buf = NULL;		/* input buffer */
unsigned Audio_bufsiz;				/* input buffer size */
char	Audio_type;				/* input data type */
int	Squelch_cnt;				/* count of silent segments */
int	Autostop_cnt;				/* count of silent seconds */
int	State = POWER_OFF;			/* current state */

struct radio_broadcast	Xmit;			/* Broadcast structure */
char	Station[RADIO_CALLNAME_SIZE + 1];	/* station */
char**	Network = NULL;				/* network interface list */
char*	Frequency = "AM";			/* broadcast frequency */
int	Squelch = TRUE;				/* squelch flag */
int	Autostop = FALSE;			/* auto shutoff flag */
int	Report = FALSE;				/* if TRUE, report status */
int	Range = RADIO_DEFAULT_RANGE;		/* packet hop count */
unsigned Dev_bufsiz = 0;			/* input buffer size */
char	Xmit_format = RADIO_TYPE_ULAW;		/* transmit data format */
char*	Audio_name;				/* audio input */

int		Agc = TRUE;			/* auto gain control flag */
Audio_agc*	Agc_state = NULL;		/* agc state structure */

#ifdef G721
/* Define structures for G.721 compression */
struct audio_g72x_state	g721_state;		/* compression state */
unsigned char*		g721_buf = NULL;	/* compress buffer */
unsigned		g721_bufsiz = 0;	/* compress buffer size */
#endif /* G721 */


/* Local routines */
int input(char* arg);


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
	Timer_flag++;

	/* Decrement time counter...if time to transmit station id, reset */
	if (--Id_ctr <= 0) {
		Id_flag = TRUE;
		Id_ctr = RADIO_ID_TIME;
	}

	/* If squelching, increment autostop counter */
	if (Squelch_cnt > 0)
		Autostop_cnt++;
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
	    T("Radio Transmitter -- usage:"), prog);
	FPRINTF(stderr, T("\t[-C] [commands]\nwhere:\n"));
	FPRINTF(stderr, T("\t-C\tRead commands from stdin\n\n"));
	FPRINTF(stderr, T("Valid commands are:\n"));
	FPRINTF(stderr, T("\t%s=[call letters]\n"), RADIOCMD_STATION);
	FPRINTF(stderr, T("\t%s=[audio file or device name]\n"),
	    RADIOCMD_INPUT);
#ifdef notyet
	FPRINTF(stderr, T("\t%s=[network interface]\n"), RADIOCMD_NETWORK);
#endif
	FPRINTF(stderr, "\t%s=[YES|NO]\n", RADIOCMD_SQUELCH);
	FPRINTF(stderr, "\t%s=[YES|NO]\n", RADIOCMD_AGC);
	FPRINTF(stderr, "\t%s=[YES|NO]\n", RADIOCMD_AUTOSTOP);
	FPRINTF(stderr, "\t%s=[YES|NO]\n", RADIOCMD_REPORT);
	FPRINTF(stderr, "\t%s\n", RADIOCMD_START);
	FPRINTF(stderr, "\t%s\n", RADIOCMD_STOP);
	FPRINTF(stderr, "\t%s\n", RADIOCMD_QUIT);
	FPRINTF(stderr, T("\t%s=[COMPRESSED|UNCOMPRESSED]\n"),
	    RADIOCMD_FORMAT);
	FPRINTF(stderr, T("\t%s=[service name|port number]\n"),
	    RADIOCMD_SERVICE);
	FPRINTF(stderr, T("\t%s=[multicast hostname|ip address]\n"),
	    RADIOCMD_ADDRESS);
	FPRINTF(stderr, T("\t%s=[range (number of gateways)]\n"),
	    RADIOCMD_RANGE);
	FPRINTF(stderr, T("\t%s=[input buffer size in bytes]\n"),
	    RADIOCMD_BUFSIZ);
	exit(1);
}

void
error()
{
	if (Command_mode)
		return;
	exit(1);
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
		case QUIET:
			msg = RADIOCMD_SQUELCH; break;
		case ON_THE_AIR:
			msg = RADIOCMD_BROADCAST; break;
		case OFF_THE_AIR:
			msg = RADIOCMD_EOF; break;
		}
		FPRINTF(stderr, "%s: %s %s\n", prog,
		    RADIOCMD_STATUS, msg);
	}
}

/* If audio input open, close it */
void
close_audio()
{
	if (Audio_fd < 0)
		return;
	(void) close(Audio_fd);
	Audio_fd = -1;
}

/*
 * Tweak the audio input volume a little bit,
 * Input value specifies change:
 *	negative: lower volume
 *	positive: raise volume
 */
void
adjust_audio(
	int		val)
{
	double		gain;

	/* If agc disabled, or no input signal present, do nothing */
	if (!Agc || (val == 0) || (val > 5) || !Device_input || (Audio_fd < 0))
		return;

	audio_get_record_gain(Audio_fd, &gain);
	gain += (double)val * .02;
	if (gain <= 0.)
		gain = .02;
	if (gain > 1.)
		gain = 1.;
	audio_set_record_gain(Audio_fd, &gain);
}


/* Sign off a radio station */
void
signoff()
{
	struct itimerval	timer;

	/* If audio input open, close it */
	close_audio();

	/* If not broadcasting, just return */
	if (State == POWER_OFF)
		return;

	/* Transmit a sign-off message and shut down the network connection */
	radio_closebroadcast(&Xmit);

	/* Turn off the timer */
	(void) memset((void *)&timer, 0, sizeof (timer));
	(void) setitimer(ITIMER_REAL, &timer, (struct itimerval *)NULL);

	/* If there's an input buffer, free it */
	if (Audio_buf != NULL) {
		(void) free((char*)Audio_buf);
		Audio_buf = NULL;
	}

	/* If there's an agc state structure, free it */
	if (Agc_state != NULL) {
		audio_agcfree(Agc_state);
		Agc_state = NULL;
	}

	newstate(POWER_OFF);
}

/*
 * Sign on a radio station
 */
int
signon()
{
	struct itimerval	timer;

	/* If already initialized, just return */
	if (State != POWER_OFF)
		return (TRUE);

	/* If no audio file open, try to open the last name */
	if (Audio_fd < 0) {
		if (Audio_name != NULL) {
			(void) input(Audio_name);
		} else {
			if (!Command_mode) {
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("No audio input specified"));
			} else {
				/* Force a STOP message */
				State = 0;
				newstate(POWER_OFF);
			}
		}
		if (Audio_fd < 0)		/* if input() failed */
			return (FALSE);
	}

	/* If null station id, use hostname (unless command mode) */
	if ((Station[0] == ' ') || (Station[0] == '\0')) {
		if (Command_mode) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("No station call letters specified"));
			return (FALSE);
		} else {
			char	hostname[MAXHOSTNAMELEN + 1];

			if (gethostname(hostname, sizeof (hostname)) >= 0) {
				radio_callname_copy(hostname, Station);
				Station[RADIO_CALLNAME_SIZE] = '\0';
			} else {
				STRCPY(Station, "????");
			}
		}
	}

	/* Initialize for broadcast and send a station id */
	if (radio_initbroadcast(Network, Station, Range, &Xmit) < 0) {
		if (errno == EPROTONOSUPPORT)
			FPRINTF(stderr,
			    T("%s: No '%s' entry in NIS services map\n"),
			    prog, RADIO_DEFAULT_SERVICE);
		else
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Cannot initialize radio transmitter"));
		return (FALSE);
	}

	/* Init timer variables */
	Id_ctr = 0;
	Timer_flag = 1;
	Squelch_cnt = 0;
	Autostop_cnt = 0;

	/* Set transmitting state */
	newstate(ON_THE_AIR);

	/* Set a timer to go off in a half-second and every second after */
	(void) memset((void *)&timer, 0, sizeof (timer));
	timer.it_value.tv_usec = 500000;
	timer.it_interval.tv_sec = 1;
	if (setitimer(ITIMER_REAL, &timer, (struct itimerval *)NULL) < 0) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Error setting interval timer"));
		signoff();
		return (FALSE);
	}
	return (TRUE);
}



/* Input command routines */

/* Station command: set new call letters */
int
station(char* arg)
{
	if (strcmp(arg, Station) == 0)
		return (TRUE);			/* no change to station id */

	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change station callname while transmitting"));
		return (FALSE);
	}

	/* Copy callname, padding out with spaces and null-terminate */
	radio_callname_copy(arg, Station);
	Station[RADIO_CALLNAME_SIZE] = '\0';
	return (TRUE);
}

int
squelch(char* arg)
{
	Squelch = is_on(arg);
	return (TRUE);
}

int
agc(char* arg)
{
	Agc = is_on(arg);
	return (TRUE);
}

int
autostop(char* arg)
{
	Autostop = is_on(arg);
	return (TRUE);
}


/* Parse a new audio input source */
int
input(char* arg)
{
	struct stat	st;
	int		fd;

	/* If there's already an open audio input, close it */
	close_audio();
	fd = -1;

	/* If there's an input filename, free it */
	if (Audio_name != NULL) {
		(void) free(Audio_name);
		Audio_name = NULL;
	}

	/* If there's an input buffer, free it */
	if (Audio_buf != NULL) {
		(void) free((char*)Audio_buf);
		Audio_buf = NULL;
	}

	/* If there's an agc state structure, free it */
	if (Agc_state != NULL) {
		audio_agcfree(Agc_state);
		Agc_state = NULL;
	}
	Squelch_cnt = 0;
	Autostop_cnt = 0;

	/* Stat the audio input file */
	if (stat(arg, &st) < 0) {
		FPRINTF(stderr, "%s: %s %s\n", prog, arg,
		    T("does not exist"));
		goto errret;
	}

	/* If character device, assume it is supposed to be an audio device */
	if (S_ISCHR(st.st_mode)) {
		fd = open(arg, O_RDONLY | O_NONBLOCK, 0);
		if (fd < 0) {
			if (errno == EBUSY)
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Audio input device is already in use"));
			else
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Cannot open audio input device"));
			goto errret;
		}

		/* Get the input data format */
		if (audio_get_record_config(fd, &Input_hdr) != AUDIO_SUCCESS) {
			FPRINTF(stderr, "%s: %s %s\n", prog, arg,
			    T("is not an audio device"));
			(void) close(fd);
			goto errret;
		}
		/* Make sure non-blocking flags are clear */
		(void) fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL, 0) &
		    ~(O_NONBLOCK | O_NDELAY)));
		Device_input = TRUE;

	} else if (S_ISREG(st.st_mode)) {
		fd = open(arg, O_RDONLY, 0);
		if (fd < 0) {
			FPRINTF(stderr, "%s: %s %s\n", prog, arg,
			    T("is not a readable audio file"));
			goto errret;
		}
		if (audio_read_filehdr(fd, &Input_hdr, (char *)NULL, 0) !=
		    AUDIO_SUCCESS) {
			FPRINTF(stderr, "%s: %s %s\n", prog, arg,
			    T("is not an audio file"));
			goto errret;
		}
		Device_input = FALSE;

	} else {
		FPRINTF(stderr, "%s: %s %s\n", prog, arg,
		    T("is not an audio device"));
		goto errret;
	}

	/* Set the type according to the input data */
	switch (Input_hdr.encoding) {
	case AUDIO_ENCODING_ULAW:
		/* Check for valid data encoding */
		if ((Input_hdr.sample_rate != RADIO_ULAW_RATE) ||
		    (Input_hdr.channels != 1))
			goto badenc;

		Audio_type = RADIO_TYPE_ULAW;

#ifdef G721
		/* If compressed broadcast, init output format */
		if (Xmit_format == RADIO_TYPE_G721) {
			g721_init_state(&g721_state);
		}
#endif /* G721 */
		break;

#ifdef G721
	case AUDIO_ENCODING_G721:
		/* Check for valid data encoding */
		if ((Input_hdr.sample_rate != RADIO_ULAW_RATE) ||
		    (Input_hdr.channels != 1))
			goto badenc;

		Audio_type = RADIO_TYPE_G721;
		break;
#endif /* G721 */

	default:
badenc:
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot broadcast audio data format"));
		goto errret;
	}

	/* Copy new filename */
	Audio_name = malloc(strlen(arg) + 1);
	STRCPY(Audio_name, arg);
	Audio_fd = fd;
	Timer_flag = 0;		/* Reset ctr, in case this is a file */
	if (State == OFF_THE_AIR)
		newstate(ON_THE_AIR);
	return (TRUE);

errret:
	if (fd >= 0)
		(void) close(fd);
	signoff();
	error();
	return (FALSE);
}

int
buffersize(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change buffer size while transmitting"));
		return (FALSE);
	}

	/* Convert value and do range checking (errors restore default) */
	Dev_bufsiz = atoi(arg);
	if ((Dev_bufsiz != 0) && (Dev_bufsiz < RADIO_MIN_BUFSIZ)) {
		Dev_bufsiz = RADIO_MIN_BUFSIZ;
	}

	if (Dev_bufsiz > RADIO_MAX_BUFSIZ)
		Dev_bufsiz = RADIO_MAX_BUFSIZ;
	return (TRUE);
}

int
format(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change broadcast format while transmitting"));
		return (FALSE);
	}

	/* Decode value */
#ifdef G721
	if ((arg[0] == 'C') || (arg[0] == 'c'))
		Xmit_format = RADIO_TYPE_G721;
	else
#endif /* G721 */
		Xmit_format = RADIO_TYPE_ULAW;
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
	return (signon());
}

/*ARGSUSED*/
int
stop(char* arg)
{
	signoff();
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

/*ARGSUSED*/
int
network(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change network interface while transmitting"));
		return (FALSE);
	}
	/* XXX - unimplemented */
	return (TRUE);
}

int
range(char* arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change range while transmitting"));
		return (FALSE);
	}
	Range = atoi(arg);
	if (Range <= 0)
		Range = 1;
	return (TRUE);
}

int
address(char *arg)
{
	/* Cannot be broadcasting when this command is received */
	if (State != POWER_OFF) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Cannot change address while transmitting"));
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
		    T("Cannot change service while transmitting"));
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


/* Read audio data from input and broadcast it */
void
xmit_data()
{
	int		i;
	int		aval;
	int		err;
	unsigned char*	xbuf;
	char		xtype;

	/* If end-of-file on input, do nothing */
	if (State == OFF_THE_AIR)
		return;

	/* If reading from a file, wait until the timer goes off */
	if (!Device_input) {
		while (Timer_flag == 0)
			pause();

		/* Decrement the timer flag safely */
		(void) sigprocmask(SIG_BLOCK, &Timer_mask, 0);
		Timer_flag--;
		(void) sigprocmask(SIG_UNBLOCK, &Timer_mask, 0);
	}

	/* If no input buffer allocated, grab one now */
	if (Audio_buf == NULL) {
		/*
		 * Set the input buffer size:
		 *  if device input and buffersize specified, use that.
		 *  if device input, round 1 second up to a packetsize multiple.
		 *  otherwise, use 1 second buffer size.
		 */
		if (Device_input && (Dev_bufsiz != 0)) {
			if (Dev_bufsiz < Xmit.maxpkt)
				Audio_bufsiz = Dev_bufsiz;
			else
				Audio_bufsiz = Xmit.maxpkt;
		} else {
			Audio_bufsiz = (Input_hdr.sample_rate *
			    Input_hdr.bytes_per_unit * Input_hdr.channels)
			    / Input_hdr.samples_per_unit;
			if (Device_input) {
				/* Round to a multiple of packetsize */
				i = Xmit.maxpkt - (Audio_bufsiz % Xmit.maxpkt);
				Audio_bufsiz += i;
			}
		}

		Audio_buf = (unsigned char *)malloc(Audio_bufsiz);
		if (Audio_buf == NULL) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Cannot broadcast audio data density"));
			signoff();
			return;
		}
	}

	/* Read a buffer's worth (1 second) */
	if (Device_input)
		(void) sigprocmask(SIG_BLOCK, &Timer_mask, 0);
	i = read(Audio_fd, (char *)Audio_buf, Audio_bufsiz);
	if (Device_input)
		(void) sigprocmask(SIG_UNBLOCK, &Timer_mask, 0);

	if (i <= 0) {
		/* Error reading device or normal end-of-file? */
		if (Device_input) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Error reading audio input device"));
			signoff();
		} else {
			if (Command_mode) {
				newstate(OFF_THE_AIR);
				close_audio();
			} else {
				/* Normal end-of-file */
				signoff();
			}
		}
		return;
	}

	/* Process this data buffer, if necessary and possible */
	if ((Agc || Squelch || Autostop) &&
	    (i == Audio_bufsiz) &&
	    Device_input && (Audio_type == RADIO_TYPE_ULAW)) {
		/* Init agc state, if first time */
		if (Agc_state == NULL) {
			Agc_state = audio_agcinit(Audio_bufsiz / 4,
			    Input_hdr.sample_rate);
		}

		/* If init succeeded */
		if (Agc_state != NULL) {
			aval = audio_agc(Agc_state, Audio_buf);
			adjust_audio(aval);
			if (aval <= 0) {
				/* Input is plenty loud */
				Squelch_cnt = 0;
				Autostop_cnt = 0;
			} else {
				/* Input is too soft */
				if (aval > 5) {
					/* No signal detected */
					Squelch_cnt++;
				} else {
					Squelch_cnt = 0;
					Autostop_cnt = 0;
				}
			}
		}
	}

	/* If this buffer is silent, check squelch and autostop */
	if (Squelch_cnt > 0) {
		/* If autostop, shut off after a timeout */
		if (Autostop && (Autostop_cnt > 60)) {
			if (Command_mode) {
				newstate(OFF_THE_AIR);
				close_audio();
			} else {
				signoff();
			}
			Autostop_cnt = 0;
			Squelch_cnt = 0;
			return;
		}

		/* If quiet for a while, skip transmitting quiet packets */
		if (Squelch && (Squelch_cnt
		    > (2 * Input_hdr.sample_rate / Audio_bufsiz))) {
			newstate(QUIET);
			return;
		}
	}

	/* Set address and format of buffer to transmit */
	xbuf = Audio_buf;
	xtype = Audio_type;

#ifdef G721
	/* Compress the outgoing data if requested and possible */
	if ((Xmit_format == RADIO_TYPE_G721) &&
	    (Audio_type == RADIO_TYPE_ULAW)) {
		/* Allocate a state structure if necessary */
		if (g721_bufsiz < (i / 2)) {
			if (g721_buf != NULL)
				(void) free((char*)g721_buf);
			g721_bufsiz = (Audio_bufsiz + 1) / 2;
			g721_buf = (unsigned char*)malloc(g721_bufsiz);
		}
		err = g721_encode(Audio_buf, i, &Input_hdr, g721_buf, &i,
		    &g721_state);
		if (err != AUDIO_SUCCESS) {
			FPRINTF(stderr, "%s: %s\n", prog,
			    T("Error compressing audio data"));
			signoff();
			return;
		}
		/* Reset buffer address and format */
		xbuf = g721_buf;
		xtype = Xmit_format;
	}
#endif /* G721 */

	/* Broadcast this data buffer */
	if (radio_broadcastdata(&Xmit, xbuf, i, xtype) < 0) {
		FPRINTF(stderr, "%s: %s\n", prog,
		    T("Error broadcasting audio data"));
		signoff();
		return;
	}
	newstate(ON_THE_AIR);
}


/* Command parse table */
Radio_cmdtbl	cmdlist[] = {
	{RADIOCMD_STATION,	station},
	{RADIOCMD_SQUELCH,	squelch},
	{RADIOCMD_AGC,		agc},
	{RADIOCMD_AUTOSTOP,	autostop},
	{RADIOCMD_INPUT,	input},
	{RADIOCMD_BUFSIZ,	buffersize},
	{RADIOCMD_FORMAT,	format},
	{RADIOCMD_REPORT,	report},
	{RADIOCMD_START,	start},
	{RADIOCMD_STOP,		stop},
	{RADIOCMD_QUIT,		quit},
	{RADIOCMD_NETWORK,	network},
	{RADIOCMD_RANGE,	range},
	{RADIOCMD_ADDRESS,	address},
	{RADIOCMD_SERVICE,	service},
	{NULL,			(int(*)()) NULL},
};

/* Main radio transmitter loop */
void
xmit()
{
	/* Loop until there is a signal to quit */
	while (Quit == 0) {
		(void) fflush(stderr);

		/* If now transmitting, move some data if possible */
		if (State != POWER_OFF)
			xmit_data();

		/* If it is time for station identification, send it now */
		if (Id_flag && (State != POWER_OFF)) {
			Id_flag = FALSE;
			if (radio_broadcastid(&Xmit) < 0) {
				FPRINTF(stderr, "%s: %s\n", prog,
				    T("Error broadcasting station id"));
				signoff();
			}
		}

		/* If not transmitting and not command mode, quit now */
		if (!Command_mode && (State == POWER_OFF))
			break;

		/*
		 * If not transmitting and command mode, wait for a command.
		 * If transmitting and command mode, check cmd input stream.
		 */
		(void) fflush(stderr);
		if (Command_mode) {
			int	transmit;

			transmit = ((State != POWER_OFF) &&
			    (State != OFF_THE_AIR));

			/* Parse command input, quit on end-of-file */
			if (parse_input(fileno(stdin),
			    cmdlist, transmit, prog) < 0)
				break;
		}
	}
	signoff();
}

/* Main entry point for radio transmitter */
main(
	int		argc,
	char*		argv[])
{
	int		i;
	int		err;

	/* get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;
	err = 0;

	/* Parse start-up options */
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

	/* Set up signal handlers to catch ^C and timers */
	(void) sigemptyset(&Timer_mask);
	(void) sigaddset(&Timer_mask, SIGALRM);
	(void) sigset(SIGINT, quit_handler);
	(void) sigset(SIGALRM, timer_handler);
	if (Command_mode)
		(void) sigset(SIGPIPE, quit_handler);

	/* Parse the rest of the command line */
	if (parse_cmds(argv, cmdlist) != NULL)
		usage();

	/* Finish initialization if not command mode */
	if (!Command_mode) {
		/* If no audio input specified, use the default */
		if (Audio_fd < 0)
			input(AUDIO_DEVICE);
		(void) signon();
	}

	/* Start broadcasting */
	xmit();
	exit(0);
	return (0);
}
