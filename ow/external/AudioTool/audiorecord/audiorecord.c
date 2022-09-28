/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audiorecord.c	1.21	96/03/21 SMI"

/* Command-line audio record utility */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>     /* All occurances of INT_MAX used to be ~0  (by MCA) */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stropts.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>

/* localization stuff */
#ifndef SUNOS41
static char		*I18N_Message_File = I18N_DOMAIN;
#define MGET(s)		(char *) gettext(s)

#else /* 4.x */
#define MGET(s)		s
#define	textdomain(d)
#define	setlocale(a, b)
#endif /* 4.x */

/* Defined until I get a copy of the apropriate audioio.h file. */
#ifndef AUDIO_CD_IN 
#define AUDIO_CD_IN 0x04	/* input from the internal CD player */
#endif

#define	Error		(void) fprintf

/* Local variables */
char*		prog;
char		prog_opts[] 	= "aft:v:b:m:d:p:i:e:s:c:?"; /* getopt() flags */
char*		Stdout;

/* XXX - the input buffer size should depend on sample_rate */
#define AUDIO_BUFSIZ (1024 * 64)
unsigned char	buf[AUDIO_BUFSIZ];
char 			swapBuf[AUDIO_BUFSIZ];	/* for byte swapping */


#define	MAX_GAIN		(100)	/* maximum gain */

#define LEFT_BAL		(-100)	/* min/max balance */
#define MID_BAL			(0)
#define RIGHT_BAL		(100)

char*		Info = NULL;		/* pointer to info data */
unsigned	Ilen = 0;		/* length of info data */
unsigned	Volume = INT_MAX;	/* record volume */
double		Savevol;		/* saved  volume */
unsigned	Monvol = INT_MAX;	/* monitor volume */
double		Savemonvol;		/* saved monitor volume */
unsigned int	Balance = INT_MAX;	/* input balance */
unsigned int	Savebal;		/* saved input balance */
unsigned	Port = INT_MAX;		/* Input port (line, mic) */
unsigned	Saveport = 0;		/* restore value of input port */
unsigned	Sample_rate = 0;
unsigned	Channels = 0;
unsigned	Precision = 0;		/* based on encoding */
unsigned	Encoding = 0;

int		NetEndian = TRUE;	/* endian nature of the machines */

int		Append = FALSE;		/* append to output file */
int		Force = FALSE;		/* ignore rate differences on append */
double		Time = -1.;		/* recording time */
unsigned	Limit = AUDIO_UNKNOWN_SIZE;	/* recording limit */
char*		Audio_dev = "/dev/audio";

int		Audio_fd = -1;		/* file descriptor for audio device */
int		Audio_ctlfd = -1;	/* file descriptor for control device */
Audio_hdr	Dev_hdr;		/* audio header for device */
Audio_hdr	Save_hdr;		/* saved audio device header */
char*		Ofile;			/* current filename */
Audio_hdr	File_hdr;		/* audio header for file */
int		Cleanup = FALSE;	/* SIGINT sets this flag */
unsigned	Size = 0;		/* Size of output file */
unsigned	Oldsize = 0;		/* Size of input file, if append */

/* Global variables */
extern int getopt();
extern int optind;
extern char* optarg;


usage()
{
	Error(stderr, MGET("Record an audio file -- usage:\n"
	    "\t%s [-af] [-v vol] [-b bal] [-m monvol] [-p mic|line]\n"
	    "\t%.*s [-c channels] [-s rate] [-e encoding]\n"
	    "\t%.*s [-t time] [-i info] [-d dev] [file]\n"
	    "where:\n"
	    "\t-a\tAppend to output file\n"
	    "\t-f\tIgnore sample rate differences on append\n"
	    "\t-v\tSet record volume (0 - %d)\n"
	    "\t-b\tSet record balance (%d=left, %d=center, %d=right)\n"
	    "\t-m\tSet monitor volume (0 - %d)\n"
	    "\t-p\tSpecify input port\n"
	    "\t-c\tSpecify number to channels to record\n"
	    "\t-s\tSpecify rate in samples per second\n"
	    "\t-e\tSpecify encoding (ulaw | alaw | linear)\n"
	    "\t-t\tSpecify record time (hh:mm:ss.dd)\n"
	    "\t-i\tSpecify a file header information string\n"
	    "\t-d\tSpecify audio device (default: /dev/audio)\n"
	    "\tfile\tRecord to named file\n"
	    "\t\tIf no file specified, write to stdout\n"
	    "\t\tDefault audio encoding is ulaw, 8khz, mono\n"
	    "\t\tIf -t is not specified, record until ^C\n"),
	    prog,
	    strlen(prog), "                    ",
	    strlen(prog), "                    ",
	    MAX_GAIN, LEFT_BAL, MID_BAL, RIGHT_BAL, MAX_GAIN);
	exit(1);
}

void
sigint(
	int		sig)
{
	/* If this is the first ^C, set a flag for the main loop */
	if (!Cleanup && (Audio_fd >= 0)) {
		/* flush input queues before exiting */
		Cleanup = TRUE;
		if (audio_pause_record(Audio_fd) == AUDIO_SUCCESS)
			return;
		Error(stderr, MGET("%s: could not flush input buffer\n"), prog);
	}

	/* If double ^C, really quit */
	if (Audio_fd >= 0) {
		if (Volume != INT_MAX)
			(void) audio_set_record_gain(Audio_fd, &Savevol);
		if (Balance != INT_MAX)
			(void) audio_set_record_balance(Audio_fd, &Savebal);
		if (Monvol != INT_MAX)
			(void) audio_set_monitor_gain(Audio_fd, &Savemonvol);
		if (Port != INT_MAX)
			(void) audio_set_record_port(Audio_fd, &Saveport);
		if ((Audio_ctlfd >= 0) &&
		    (audio_cmp_hdr(&Save_hdr, &Dev_hdr) != 0)) {
			(void) audio_set_record_config(Audio_fd, &Save_hdr);
		}
	}
	exit(1);
}

/*
 * Record from the audio device to a file.
 */
main(
	int		argc,
	char**		argv)
{
	int		i;
	int		cnt;
	int		err;
	int		ofd;
	int 	swapBytes;
	double		vol;
	int		bal;
	struct stat	st;
	struct pollfd	pfd;
	char		*cp;
	char		ctldev[MAXPATHLEN];

	setlocale(LC_ALL, "");
	textdomain(I18N_Message_File);

	/* Get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;
	Stdout = MGET("(stdout)");

	/* first check AUDIODEV environment for audio device name */
	if (cp = getenv("AUDIODEV")) {
		Audio_dev = cp;
	}

	/* Set the endian nature of the machine */
	if( (u_long)1 != htonl((u_long)1)) {
		NetEndian = FALSE;
	}

	err = 0;
	while ((i = getopt(argc, argv, prog_opts)) != EOF)  switch (i) {
	case 'v':
		if (parse_unsigned(optarg, &Volume, "-v")) {
			err++;
		} else if (Volume > MAX_GAIN) {
			Error(stderr, MGET("%s: invalid value for -v\n"), prog);
			err++;
		}
		break;
	case 'b':
		bal = atoi(optarg);
		if ((bal > RIGHT_BAL) || (bal < LEFT_BAL)) {
			Error(stderr, MGET("%s: invalid value for -b\n"), prog);
			err++;
		} else {
			Balance = (unsigned) scale_balance(bal);
		}
		break;
	case 'm':
		if (parse_unsigned(optarg, &Monvol, "-m")) {
			err++;
		} else if (Monvol > MAX_GAIN) {
			Error(stderr, MGET("%s: invalid value for -m\n"), prog);
			err++;
		}
		break;
	case 't':
		Time = audio_str_to_secs(optarg);
		if ((Time == HUGE_VAL) || (Time < 0.)) {
			Error(stderr, MGET("%s: invalid value for -t\n"), prog);
			err++;
		}
		break;
	case 'd':
		Audio_dev = optarg;
		break;
	case 'p':
		/* a partial match is OK */
		if (strncmp(optarg, "microphone", strlen(optarg)) == 0) {
			Port = AUDIO_MICROPHONE;
		} else if (strncmp(optarg, "line", strlen(optarg)) == 0) {
			Port = AUDIO_LINE_IN;
		} else if (strncmp(optarg, "cd", strlen(optarg)) == 0) {
			Port = AUDIO_CD_IN;
		} else {
			Error(stderr, MGET("%s: invalid value for -p\n"), prog);
			err++;
		}
		break;
	case 'f':
		Force = TRUE;
		break;
	case 'a':
		Append = TRUE;
		break;
	case 'i':
		Info = optarg;		/* set information string */
		Ilen = strlen(Info);
		break;
	case 's':
		if (parse_sample_rate(optarg, &Sample_rate)) {
			err++;
		}
		break;
	case 'c':
		if (strncmp(optarg, "mono", strlen(optarg)) == 0) {
			Channels = 1;
		} else if (strncmp(optarg, "stereo", strlen(optarg)) == 0) {
			Channels = 2;
		} else if (parse_unsigned(optarg, &Channels, "-c")) {
			err++;
		} else if ((Channels != 1) && (Channels != 2)) {
			Error(stderr, "%s: invalid value for -c\n", prog);
			err++;
		}
		break;
	case 'e':
		if (strncmp(optarg, "ulaw", strlen(optarg)) == 0) {
			Encoding = AUDIO_ENCODING_ULAW;
			Precision = 8;
		} else if (strncmp(optarg, "alaw", strlen(optarg)) == 0) {
			Encoding = AUDIO_ENCODING_ALAW;
			Precision = 8;
		} else if ((strncmp(optarg, "linear", strlen(optarg)) == 0)
			   || (strncmp(optarg, "pcm", strlen(optarg)) == 0)) {
			Encoding = AUDIO_ENCODING_LINEAR;
			Precision = 16;
		} else {
			Error(stderr, MGET("%s: invalid value for -e\n"), prog);
			err++;
		}
		break;
	case '?':
		usage();
/*NOTREACHED*/
	}
	if (Append && (Info != NULL)) {
		Error(stderr, MGET("%s: cannot specify -a and -i\n"), prog);
		err++;
	}
	if (err > 0)
		exit(1);

	argc -= optind;		/* update arg pointers */
	argv += optind;

	/* Open the output file */
	if (argc <= 0) {
		Ofile = Stdout;
	} else {
		Ofile = *argv++;
		argc--;

		/* Interpret "-" filename to mean stdout */
		if (strcmp(Ofile, "-") == 0)
			Ofile = Stdout;
	}

	if (Ofile == Stdout) {
		ofd = fileno(stdout);
		Append = FALSE;
	} else {
		ofd = open(Ofile,
		    (O_RDWR | O_CREAT | (Append ? 0 : O_TRUNC)), 0666);
		if (ofd < 0) {
			Error(stderr, MGET("%s: cannot open "), prog);
			perror(Ofile);
			exit(1);
		}
		if (Append) {
			/*
			 * Check to make sure we're appending to an audio file.
			 * It must be a regular file (if zero-length, simply
			 * write it from scratch).  Also, its file header
			 * must match the input device configuration.
			 */
			if ((fstat(ofd, &st) < 0) || (!S_ISREG(st.st_mode))) {
				Error(stderr,
				    MGET("%s: %s is not a regular file\n"),
				    prog, Ofile);
				exit(1);
			}
			if (st.st_size == 0) {
				Append = FALSE;
				goto openinput;
			}

			err = audio_read_filehdr(ofd, &File_hdr,
			    (char *)NULL, 0);

			if (err != AUDIO_SUCCESS) {
				Error(stderr,
				    MGET("%s: %s is not a valid audio file\n"),
				    prog, Ofile);
				exit(1);
			}

			/*
			 * Set the format state to the format
			 * in the file header.
			 */
			Sample_rate = File_hdr.sample_rate;
			Channels = File_hdr.channels;

			/* Precision is based on encoding. */
			Encoding = File_hdr.encoding;
			switch( Encoding) {

			case AUDIO_ENCODING_ULAW: 
			case AUDIO_ENCODING_ALAW:
				Precision = 8;
				break;
			case AUDIO_ENCODING_LINEAR:
				Precision = 16;
				break;		
			default: {
				char	msg[AUDIO_MAX_ENCODE_INFO];
				(void) audio_enc_to_str(&File_hdr, msg);
				Error(stderr,
		    	 	    MGET( "%s: Append is not supported for "),
				    prog);
				Error(stderr,
				    MGET( "this file encoding:\n\t[%s]\n"),
				    msg);
				exit(1);
		   	    }
			}

			/* Get the current size, if possible */
			Oldsize = File_hdr.data_size;
			if ((Oldsize == AUDIO_UNKNOWN_SIZE) &&
			    ((err = (int)lseek(ofd, 0L, SEEK_CUR)) >= 0)) {
		          if (err < 0) {
			  Error(stderr,
				MGET("%s: %s is not a valid audio file\n"),
				      prog, Ofile);
				exit(1);
			  }
			  Oldsize = st.st_size - err;
			}
			/* Seek to end to start append */
			if ((int)lseek(ofd, st.st_size, SEEK_SET) < 0) {
				Error(stderr,
				    MGET("%s: cannot find end of %s\n"),
				    prog, Ofile);
				exit(1);
			}
		}
	}
openinput:
	/* Validate and open the audio device */
	err = stat(Audio_dev, &st);
	if (err < 0) {
		Error(stderr, MGET("%s: cannot open "), prog);
		perror(Audio_dev);
		exit(1);
	}
	if (!S_ISCHR(st.st_mode)) {
		Error(stderr, MGET("%s: %s is not an audio device\n"), prog,
		    Audio_dev);
		exit(1);
	}

	/* Try to open the control device and save the current format */
	(void) sprintf(ctldev, "%sctl", Audio_dev);
	Audio_ctlfd = open(ctldev, O_RDWR);
	if (Audio_ctlfd >= 0) {
		if (audio_get_record_config(Audio_ctlfd, &Save_hdr) !=
		    AUDIO_SUCCESS) {
			(void) close(Audio_ctlfd);
			Audio_ctlfd = -1;
		}
	}

	Audio_fd = open(Audio_dev, O_RDONLY | O_NONBLOCK);
	if (Audio_fd < 0) {
		if (errno == EBUSY) {
			Error(stderr, MGET("%s: %s is busy\n"),
			    prog, Audio_dev);
		} else {
			Error(stderr, MGET("%s: error opening "), prog);
			perror(Audio_dev);
		}
		exit(1);
	}
	if (audio_pause_record(Audio_fd) != AUDIO_SUCCESS) {
		Error(stderr, MGET("%s: not able to pause recording\n"), prog);
		exit(1);
	}
	if (audio_flush_record(Audio_fd) != AUDIO_SUCCESS) {
		Error(stderr, MGET("%s: not able to flush recording\n"), prog);
		exit(1);
	}
	if (audio_get_record_config(Audio_fd, &Dev_hdr) != AUDIO_SUCCESS) {
		Error(stderr, MGET("%s: %s is not an audio device\n"),
		    prog, Audio_dev);
		exit(1);
	}

	if ((Sample_rate != 0) || (Channels != 0) ||
		(Precision != 0) || (Encoding != 0)) {
		if (Sample_rate != 0) {
			Dev_hdr.sample_rate = Sample_rate;
		}
		if (Channels != 0) {
			Dev_hdr.channels = Channels;
		}
		if (Precision != 0) {
			Dev_hdr.bytes_per_unit = Precision / 8;
		}
		if (Encoding != 0) {
			Dev_hdr.encoding  = Encoding;
		}
	}

	if (audio_set_record_config(Audio_fd, &Dev_hdr) != AUDIO_SUCCESS) {
		Error(stderr, MGET(
		    "%s: Audio format not supported by the audio device\n"),
		    prog);
		exit(1);
	}
  
	if (audio_resume_record(Audio_fd) != AUDIO_SUCCESS) {
		Error(stderr, MGET("%s: not able to resume recording\n"), prog);
		exit(1);
	}

	/* If appending to an existing file, check the configuration */
	if (Append) {
		char	msg[AUDIO_MAX_ENCODE_INFO];

		switch (audio_cmp_hdr(&Dev_hdr, &File_hdr)) {
		case 0:			/* configuration matches */
			break;
		case 1:			/* all but sample rate matches */
			if (Force) {
				Error(stderr,
		MGET("%s: WARNING: appending %.3fkHz data to %s (%.3fkHz)\n"),
				    prog,
				    ((double)Dev_hdr.sample_rate / 1000.),
				    Ofile,
				    ((double)File_hdr.sample_rate / 1000.));
				break;
			}		/* if not -f, fall through */

		default:		/* encoding mismatch */
			(void) audio_enc_to_str(&Dev_hdr, msg);
			Error(stderr,
			    MGET("%s: device encoding [%s]\n"), prog, msg);
			(void) audio_enc_to_str(&File_hdr, msg);
			Error(stderr,
			    MGET("\tdoes not match file encoding [%s]\n"), msg);
			exit(1);
		}
	} else if (!isatty(ofd)) {
		if (audio_write_filehdr(ofd, &Dev_hdr, Info, Ilen) !=
		    AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: error writing header for \n"), prog);
			perror(Ofile);
			exit(1);
		}
	}

	/* 
	 * We always write files in Network Endian byte order.
	 * If we're reading 16 bit values on LittleEndian machine
	 * we want to swap the bytes on the output.
	*/
	 swapBytes = !NetEndian && (Dev_hdr.bytes_per_unit == 2);

	/* If -v flag, set the record volume now */
	if (Volume != INT_MAX) {
		vol = (double) Volume / (double) MAX_GAIN;
		(void) audio_get_record_gain(Audio_fd, &Savevol);
		err = audio_set_record_gain(Audio_fd, &vol);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set record volume for %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}

	if (Balance != INT_MAX) {
		(void) audio_get_record_balance(Audio_fd, &Savebal);
		err = audio_set_record_balance(Audio_fd, &Balance);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set record balance for %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}

	/* If -m flag, set monitor volume now */
	if (Monvol != INT_MAX) {
		vol = (double) Monvol / (double) MAX_GAIN;
		(void) audio_get_monitor_gain(Audio_fd, &Savemonvol);
		err = audio_set_monitor_gain(Audio_fd, &vol);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set monitor volume for %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}

	/* If -p flag, set the input port */
	if (Port != INT_MAX) {
		(void) audio_get_record_port(Audio_fd, &Saveport);
		err = audio_set_record_port(Audio_fd, &Port);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			      MGET("%s: could not set input port %s\n"),
			      prog, Audio_dev);
			exit(1);
		}
	}

	if (isatty(ofd)) {
#ifdef notdef
		/* XXX - maybe if we add a Verbose flag? ... */
		Error(stderr,
		    MGET("%s: No files - setting audio device parameters.\n"),
		    prog);
#endif
		exit(0);
	}

	/* Set up SIGINT handler so that final buffers may be flushed */
	signal(SIGINT, sigint);

	/*
	 * At this point, we're (finally) ready to copy the data.
	 * Init a poll() structure, to use when there's nothing to read.
	 */
	if (Time > 0)
		Limit = audio_secs_to_bytes(&Dev_hdr, Time);
	pfd.fd = Audio_fd;
	pfd.events = POLLIN;
	while ((Limit == AUDIO_UNKNOWN_SIZE) || (Limit != 0)) {
		/* Fill the buffer or read to the time limit */
		cnt = read(Audio_fd, (char *)buf,
		    ((Limit != AUDIO_UNKNOWN_SIZE) && (Limit < sizeof (buf)) ?
		    (int)Limit : sizeof (buf)));

		if (cnt == 0)		/* normally, eof can't happen */
			break;

		/* If error, probably have to wait for input */
		if (cnt < 0) {
			if (Cleanup)
				break;		/* done if ^C seen */
			switch (errno) {
			case EAGAIN:
#ifdef SUNOS41	/* same on SVr4 */
			case EWOULDBLOCK:
#endif
				(void) poll(&pfd, 1L, -1);
				break;
			case EOVERFLOW:  /* Possibly a Large File */
				Error(stderr, MGET("%s: error reading"), prog);
				perror("Large File");
				exit(1);
			default:
				Error(stderr, MGET("%s: error reading"), prog);
				perror(Audio_dev);
				exit(1);
			}
			continue;
		}

		/* Swab the output if required. */
		if( swapBytes) {
			swab((char *)buf, swapBuf, cnt);
			err = write( ofd, swapBuf, cnt);
		} else {
			err = write(ofd, (char *)buf, cnt);
		}
		if (err < 0) {
			Error(stderr, MGET("%s: error writing "), prog);
			perror(Ofile);
			exit(1);
		}
		if (err != cnt) {
			Error(stderr, MGET("%s: error writing "), prog);
			perror(Ofile);
			break;
		}
		Size += cnt;
		if (Limit != AUDIO_UNKNOWN_SIZE)
			Limit -= cnt;
	}

	/* Attempt to rewrite the data_size field of the file header */
	if (!Append || (Oldsize != AUDIO_UNKNOWN_SIZE)) {
		if (Append)
			Size += Oldsize;
		(void) audio_rewrite_filesize(ofd, Size);
	}

	(void) close(ofd);			/* close input file */


	/* Check for error during record */
	if (audio_get_record_error(Audio_fd, (unsigned *)&err) != AUDIO_SUCCESS)
		Error(stderr, MGET("%s: error reading device status\n"), prog);
	else if (err)
		Error(stderr, MGET("%s: WARNING: Data overflow occurred\n"),
		    prog);

	/* Reset record volume, balance, monitor volume, port, encoding */
	if (Volume != INT_MAX)
		(void) audio_set_record_gain(Audio_fd, &Savevol);
	if (Balance != INT_MAX)
		(void) audio_set_record_balance(Audio_fd, &Savebal);
	if (Monvol != INT_MAX)
		(void) audio_set_monitor_gain(Audio_fd, &Savemonvol);
	if (Port != INT_MAX)
		(void) audio_set_record_port(Audio_fd, &Saveport);
	if ((Audio_ctlfd >= 0) && (audio_cmp_hdr(&Save_hdr, &Dev_hdr) != 0)) {
		(void) audio_set_record_config(Audio_fd, &Save_hdr);
	}
	(void) close(Audio_fd);
	exit(0);
}

/* Parse an unsigned integer */
parse_unsigned(
	char*		str,
	unsigned*	dst,
	char*		flag)
{
	char		x;

	if (sscanf(str, "%u%c", dst, &x) != 1) {
		Error(stderr, MGET("%s: invalid value for %s\n"), prog, flag);
		return (1);
	}
	return (0);
}

/*
 * set the sample rate. assume anything is ok. check later on to make sure 
 * the sample rate is valid.
 */
int
parse_sample_rate(
	char*		s,
	unsigned*	rate)
{
	char*		cp;
	double		drate;

	/* 
	 * check if it's "cd" or "dat" or "voice". these also set 
	 * the precision and encoding, etc.
	 */
	if (strcasecmp(s, "dat") == 0) {
		drate = 48000.0;
	} else if (strcasecmp(s, "cd") == 0) {
		drate = 44100.0;
	} else if (strcasecmp(s, "voice") == 0) {
		drate = 8000.0;
	} else {
		/* just do an atof */
		drate = atof(s);

		/* 
		 * if the first non-digit is a "k" multiply by 1000, 
		 * if it's an "h", leave it alone. anything else, 
		 * return an error.
		 */

		/* 
		 * XXX bug alert: could have multiple "." in string
		 * and mess things up.
		 */
		for(cp = s; *cp && (isdigit(*cp) || (*cp == '.')); cp++)
			;
		if (*cp != NULL) {
			if ((*cp == 'k') || (*cp == 'K')) {
				drate *= 1000.0;
			} else if ((*cp != 'h') || (*cp != 'H')) {
				/* bogus! */
				Error(stderr,
				    MGET("invalid sample rate: %s\n"), s);
				return(1);
			}
		}

	}

	*rate = irint(drate);
	return(0);
}

/* Convert local balance into device parameters */
int
scale_balance(int g)
{
	return (((g + RIGHT_BAL) / (double) (RIGHT_BAL - LEFT_BAL)) *
		(double)AUDIO_RIGHT_BALANCE);
}

/* Convert device bal into the local scaling factor */
int
unscale_balance(int g)
{
	return((((double)g / AUDIO_RIGHT_BALANCE) * (RIGHT_BAL - LEFT_BAL)) 
	       - (RIGHT_BAL));
}
