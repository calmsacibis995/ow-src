/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audioplay.c	1.27	96/03/15 SMI"

/* Command-line audio play utility */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>
#include <limits.h>     /* All occurances of INT_MAX used to be ~0  (by MCA) */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <netinet/in.h>

#include <multimedia/libaudio.h>

/* localization stuff */
#ifndef SUNOS41
static char		*I18N_Message_File = I18N_DOMAIN;
#define MGET(s)		(char *) gettext(s)

#else /* 4.x */
#define MGET(s)		s
#define	textdomain(d)
#define	setlocale(a, b)
#endif /* 4.x */

#define	Error		(void) fprintf


/* Local variables */
char *prog;

char prog_opts[] = 	"VEiv:b:d:p:?";		/* getopt() flags */

char*		Stdin;

#define	MAX_GAIN		(100)		/* maximum gain */

#define LEFT_BAL		(-100)		/* min/max balance */
#define MID_BAL			(0)
#define RIGHT_BAL		(100)
/*
 * This defines the tolerable sample rate error as a ratio between the
 * sample rates of the audio data and the audio device.
 */
#define	SAMPLE_RATE_THRESHOLD	(.01)

#define BUFFER_LEN		10	/* seconds - for file i/o */
#define ADPCM_SIZE		(1000*8) /* adpcm conversion output buf size */
#define SWAP_SIZE		(8192) /* swap bytes conversion output buf size */

unsigned	Volume = INT_MAX;       /* output volume */
double		Savevol;		/* saved volume level */
unsigned int	Balance = INT_MAX;	/* output balance */
unsigned int	Savebal;		/* saved output balance */
unsigned int    Port = INT_MAX;		/* output port (spkr, line, hp-jack) */
unsigned int	Saveport = 0;		/* save prev. val so we can restore */
int		Verbose = FALSE;	/* verbose messages */
int		Immediate = FALSE;	/* don't hang waiting for device */
int		Errdetect = FALSE;	/* don't worry about underrun */
char*		Audio_dev = "/dev/audio";

int NetEndian = TRUE;		/* endian nature of the machine */

int		Audio_fd = -1;		/* file descriptor for audio device */
int		Audio_ctlfd = -1;	/* file descriptor for control device */
Audio_hdr	Save_hdr;		/* saved audio header for device */
Audio_hdr	Dev_hdr;		/* audio header for device */
char*		Ifile;			/* current filename */
Audio_hdr	File_hdr;		/* audio header for file */
unsigned	Decode = AUDIO_ENCODING_NONE;	/* decode type, if any */

unsigned char*	buf = NULL;		/* dynamically alloc'd */
unsigned	bufsiz = 0;		/* size of output buffer  */
unsigned char	adpcm_buf[ADPCM_SIZE + 32];	/* for adpcm conversion */
unsigned char	swap_buf[SWAP_SIZE + 32];	/* for byte swap conversion */
unsigned char*	inbuf;			/* current input buffer pointer */
unsigned	insiz;			/* current input buffer size */
struct audio_g72x_state	adpcm_state;	/* state structure for adpcm */

char*		Audio_path = NULL;	/* path to search for audio files */

/* Global variables */
extern int	getopt(int, char*const *, const char*);
extern int	optind;
extern char*	optarg;

/* Local functions  */
static int path_open(char* fname, int flags, mode_t mode, char* path);

usage()
{
	Error(stderr, MGET("Play an audio file -- usage:\n"
	    "\t%s [-iV] [-v vol] [-b bal]\n"
	    "\t%.*s [-p speaker|headphone|line] [-d dev] [file ...]\n"
	    "where:\n"
	    "\t-i\tDon't hang if audio device is busy\n"
	    "\t-V\tPrint verbose warning messages\n"
	    "\t-v\tSet output volume (0 - %d)\n"
	    "\t-b\tSet output balance (%d=left, %d=center, %d=right)\n"
	    "\t-p\tSpecify output port\n"
	    "\t-d\tSpecify audio device (default: /dev/audio)\n"
	    "\tfile\tList of files to play\n"
	    "\t\tIf no files specified, read stdin\n"),
	    prog, strlen(prog), "                    ",
	    MAX_GAIN, LEFT_BAL, MID_BAL, RIGHT_BAL);
#ifdef undocumented
 	Error(stderr, MGET("\t-E\tReport output underflow errors\n"));
#endif
	exit(1);
}

void
sigint(
	int		sig)
{
	/* flush output queues before exiting */
	if (Audio_fd >= 0) {
		(void) audio_flush_play(Audio_fd);

		/* restore saved parameters */
		if (Volume != INT_MAX)
			(void) audio_set_play_gain(Audio_fd, &Savevol);
		if (Balance != INT_MAX)
			(void) audio_set_play_balance(Audio_fd, &Savebal);
		if (Port != ~0)
			(void) audio_set_play_port(Audio_fd, &Saveport);
		if ((Audio_ctlfd >= 0) &&
		    (audio_cmp_hdr(&Save_hdr, &Dev_hdr) != 0)) {
			(void) audio_set_play_config(Audio_fd, &Save_hdr);
		}
	}
	exit(1);
}

/* Open the audio device and initalize it. */
void
open_audio()
{
	int		err;
	double		vol;

	/* Return if already open */
	if (Audio_fd >= 0)
		return;

	/* Try opening without waiting, first */
	Audio_fd = open(Audio_dev, O_WRONLY | O_NONBLOCK);
	if ((Audio_fd < 0) && (errno == EBUSY)) {
		if (Immediate) {
			Error(stderr, MGET("%s: %s is busy\n"),
			    prog, Audio_dev);
			exit(1);
		}
		if (Verbose) {
			Error(stderr, MGET("%s: waiting for %s..."), 
			    prog, Audio_dev);
			(void) fflush(stderr);
		}
		/* Now hang until it's open */
		Audio_fd = open(Audio_dev, O_WRONLY);
		if (Verbose)
			Error(stderr, (Audio_fd < 0) ? "\n" : MGET("open\n"));
	}
	if (Audio_fd < 0) {
		Error(stderr, MGET("%s: error opening "), prog);
		perror(Audio_dev);
		exit(1);
	}

	/* Clear the non-blocking flag (in System V it persists after open) */
	(void) fcntl(Audio_fd, F_SETFL,
	    (fcntl(Audio_fd, F_GETFL, 0) & ~(O_NDELAY | O_NONBLOCK)));

	/* Get the device output encoding configuration */
	if (audio_get_play_config(Audio_fd, &Dev_hdr) != AUDIO_SUCCESS) {
		Error(stderr, MGET("%s: %s is not an audio device\n"),
		    prog, Audio_dev);
		exit(1);
	}

	/* If -v flag, set the output volume now */
	if (Volume != INT_MAX) {
		vol = (double) Volume / (double) MAX_GAIN;
		(void) audio_get_play_gain(Audio_fd, &Savevol);
		err = audio_set_play_gain(Audio_fd, &vol);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set output volume for %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}

	if (Balance != INT_MAX) {
		(void) audio_get_play_balance(Audio_fd, &Savebal);
		err = audio_set_play_balance(Audio_fd, &Balance);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set output balance for %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}

	/* If -p flag, set the output port now */
	if (Port != INT_MAX) {
		(void) audio_get_play_port(Audio_fd, &Saveport);
		err = audio_set_play_port(Audio_fd, &Port);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: could not set output port %s\n"),
			    prog, Audio_dev);
			exit(1);
		}
	}
}

/* Play a list of audio files. */
main(
	int		argc,
	char**		argv)
{
	int 	errorStatus = 0;
	int		i;
	int		cnt;
	int		outsiz;
	int		len;
	int		err;
	int		ifd;
	int		stdinseen;
	int		regular;
	int		bal;
	int		swapBytes = FALSE;
	int 	frame;
	char		*outbuf;
	caddr_t		mapaddr;
	struct stat	st;
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
	Stdin = MGET("(stdin)");

	/* Check AUDIODEV environment for audio device name */
	if (cp = getenv("AUDIODEV")) {
		Audio_dev = cp;
	}

	/* Parse the command line arguments */
	err = 0;
	while ((i = getopt(argc, argv, prog_opts)) != EOF) switch (i) {
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
	case 'd':
		Audio_dev = optarg;
		break;
	case 'V':
		Verbose = TRUE;
		break;
	case 'E':
		Errdetect = TRUE;
		break;
	case 'i':
		Immediate = TRUE;
		break;
	case 'p':
		/* a partial match is OK */
		if (strncmp(optarg, "speaker", strlen(optarg)) == 0) {
			Port = AUDIO_SPEAKER;
		} else if (strncmp(optarg, "headphone", strlen(optarg)) == 0) {
			Port = AUDIO_HEADPHONE;
		} else if (strncmp(optarg, "line", strlen(optarg)) == 0) {
			Port = AUDIO_LINE_OUT;
		} else {
			Error(stderr, MGET("%s: invalid value for -p\n"), prog);
			err++;
		}
		break;
	case '?':
		usage();
/*NOTREACHED*/
	}
	if (err > 0)
		exit(1);

	argc -= optind;		/* update arg pointers */
	argv += optind;

	/* Validate and open the audio device */
	err = stat(Audio_dev, &st);
	if (err < 0) {
		Error(stderr, MGET("%s: cannot stat "), prog);
		perror(Audio_dev);
		exit(1);
	}
	if (!S_ISCHR(st.st_mode)) {
		Error(stderr, MGET("%s: %s is not an audio device\n"), prog,
		    Audio_dev);
		exit(1);
	}

	/* This should probably use audio_cntl instead of open_audio */
	if ((argc <= 0) && isatty(fileno(stdin))) {
		if (Verbose) {
			Error(stderr,
		    MGET("%s: No files - setting audio device parameters.\n"),
			    prog);
		}
		open_audio();
		exit(0);
	}

	/* Check on the -i status now. */
	Audio_fd = open(Audio_dev, O_WRONLY | O_NONBLOCK);
	if ((Audio_fd < 0) && (errno == EBUSY)) {
		if (Immediate) {
			Error(stderr, MGET("%s: %s is busy\n"), prog, Audio_dev); 
			exit(1); 
		}
	}
	close(Audio_fd);
	Audio_fd = -1;

	/* Try to open the control device and save the current format */
	(void) sprintf(ctldev, "%sctl", Audio_dev);
	Audio_ctlfd = open(ctldev, O_RDWR);
	if (Audio_ctlfd >= 0) {
		/* 
		 * wait for the device to become available then get the controls.
		 * We want to save the format that is left when the device
		 * is in a quiescent state. So wait until then.
		*/
		Audio_fd = open(Audio_dev, O_WRONLY); close(Audio_fd);
		Audio_fd = -1;
		if (audio_get_play_config(Audio_ctlfd, &Save_hdr) 
		   != AUDIO_SUCCESS) {
			(void) close(Audio_ctlfd);
			Audio_ctlfd = -1;
		}
	}

	/* store AUDIOPATH so we don't keep doing getenv() */
	Audio_path = getenv("AUDIOPATH");

	/* Set up SIGINT handler to flush output */
	signal(SIGINT, sigint);

	/* Set the endian nature of the machine. */
	if( (u_long)1 != htonl((u_long)1)) {
		NetEndian = FALSE;
	}

	/* If no filenames, read stdin */
	stdinseen = FALSE;
	if (argc <= 0) {
		Ifile = Stdin;
	} else {
		Ifile = *argv++;
		argc--;
	}

	/* Loop through all filenames */
	do {
		/* Interpret "-" filename to mean stdin */
		if (strcmp(Ifile, "-") == 0)
			Ifile = Stdin;

		if (Ifile == Stdin) {
			if (stdinseen) {
				Error(stderr,
				    MGET("%s: stdin already processed\n"),
				    prog);
				goto nextfile;
			}
			stdinseen = TRUE;
			ifd = fileno(stdin);
		} else {
			if ((ifd = path_open(Ifile, O_RDONLY, 0, Audio_path))
			    < 0) {
				Error(stderr, MGET("%s: cannot open "), prog);
				perror(Ifile);
				errorStatus++;
				goto nextfile;
			}
		}

		/* Check to make sure this is an audio file */
		err = audio_read_filehdr(ifd, &File_hdr, (char*)NULL, 0);
		if (err != AUDIO_SUCCESS) {
			Error(stderr,
			    MGET("%s: %s is not a valid audio file\n"),
			    prog, Ifile);
			errorStatus++;
			goto closeinput;
		}

		/* If G.72X adpcm, set flags for conversion */
		if ((File_hdr.encoding == AUDIO_ENCODING_G721) &&
		    (File_hdr.samples_per_unit == 2) &&
		    (File_hdr.bytes_per_unit == 1)) {
			Decode = AUDIO_ENCODING_G721;
			File_hdr.encoding = AUDIO_ENCODING_ULAW;
			File_hdr.samples_per_unit = 1;
			File_hdr.bytes_per_unit = 1;
			g721_init_state(&adpcm_state);
		} else if ((File_hdr.encoding == AUDIO_ENCODING_G723) &&
		    (File_hdr.samples_per_unit == 8) &&
		    (File_hdr.bytes_per_unit == 3)) {
			Decode = AUDIO_ENCODING_G723;
			File_hdr.encoding = AUDIO_ENCODING_ULAW;
			File_hdr.samples_per_unit = 1;
			File_hdr.bytes_per_unit = 1;
			g723_init_state(&adpcm_state);
		} else {
			Decode = AUDIO_ENCODING_NONE;
		}

		/* Check the device configuration */
		open_audio();
		if (audio_cmp_hdr(&Dev_hdr, &File_hdr) != 0) {
			/*
			 * The device does not match the input file.
			 * Wait for any old output to drain, then attempt
			 * to reconfigure the audio device to match the
			 * input data.
			 */
			if (audio_drain(Audio_fd, FALSE) != AUDIO_SUCCESS) {
				Error(stderr, MGET("%s: "), prog);
				perror(MGET("AUDIO_DRAIN error"));
				exit(1);
			}
			if (!reconfig()) {
				errorStatus++;
				goto closeinput;
			}
		}


		/* try to do the mmaping - for regular files only ... */
		err = fstat(ifd, &st);
		if (err < 0) {
			Error(stderr, MGET("%s: cannot stat "), prog);
			perror(Ifile);
			exit(1);
		}
		regular = (S_ISREG(st.st_mode));


		/* If regular file, map it.  Else, allocate a buffer */
		mapaddr = 0;
		if (regular && 
			/* This sholud compare to MAP_FAILED not -1, can't find MAP_FAILED*/
			((mapaddr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, ifd, 0))	!= MAP_FAILED)) {

			(void) madvise(mapaddr, st.st_size, MADV_SEQUENTIAL);

			/* Skip the file header and set the proper size */
			cnt = lseek(ifd, 0, SEEK_CUR);
			if (cnt < 0) {
			  perror("lseek");
			  exit(1);
			}
			inbuf = (unsigned char*) mapaddr + cnt;
			len = cnt = st.st_size - cnt;
		} else {					/* Not a regular file, or map failed */

			/* mark is so. */
			mapaddr = 0;

			/* Allocate buffer to hold 10 seconds of data */
			cnt = BUFFER_LEN * File_hdr.sample_rate *
			    File_hdr.bytes_per_unit * File_hdr.channels;
			if (bufsiz != cnt) {
				if (buf != NULL)
					(void) free(buf);
				buf = (unsigned char*) malloc(cnt);
				if (buf == NULL) {
					Error(stderr,
				    MGET("%s: couldn't allocate %dK buf\n"),
					    prog, bufsiz / 1000);
					exit(1);
				}
				inbuf = buf;
				bufsiz = cnt;
			}
		}

		/* Set buffer sizes and pointers for conversion, if any */
		switch (Decode) {
		default:
		case AUDIO_ENCODING_NONE:
			insiz = bufsiz;
			outbuf = (char*)buf;
			break;
		case AUDIO_ENCODING_G721:
			insiz = ADPCM_SIZE / 2;
			outbuf = (char*)adpcm_buf;
			break;
		case AUDIO_ENCODING_G723:
			insiz = (ADPCM_SIZE * 3) / 8;
			outbuf = (char*)adpcm_buf;
			break;
		}

		/*
		 * Assume that all files are in Network byte order,
		 * swab the buffers if were on a non-network byte order machine
		 * and the device is a 16bit device. 
		 * Formats which bytes_per_unit > 2 will not work properly.
		 * This shouldn't be a problem for now.
		 *
		 * Note: Because the G.72X conersions produce 8bit output,
		 * they don't require a byte swap before display and so 
		 * this scheme works just fine. If a conversion is added
		 * that produces a 16 bit result and therefore requires  
		 * byte swapping before output, then a mechanism
		 * for chaining the two conversions will have to be built.
		*/
		if(swapBytes = (!NetEndian && (File_hdr.bytes_per_unit == 2))) { 
			/* Read in interal number of sample frames. */
			frame = File_hdr.bytes_per_unit * File_hdr.channels;
			insiz = (SWAP_SIZE / frame) * frame;
			/* make the output buffer  the swap buffer. */
			outbuf = (char *)swap_buf;
		}

  		/*
  		 * At this point, we're all ready to copy the data.
  		*/
		if (mapaddr == 0) {			/* Not mmapped, do it a buffer at a time. */
			inbuf = buf;
			while ((cnt = read(ifd, inbuf, insiz)) >= 0) {

				/* 
				 * If decoding adpcm, or swapping bytes do it now 
				 *
				 * We treat the swapping like a separate encoding
				 * here because the G.72X encodings decode to single
				 * byte output samples. If another encoding is added
				 * and it produces multi-byte output samples
				 * this will have to be changed.
				*/
				if (Decode == AUDIO_ENCODING_G721) {
					err = g721_decode(inbuf, cnt, &File_hdr,
					    (void*)outbuf, &cnt, &adpcm_state);
					if (err != AUDIO_SUCCESS) {
						Error(stderr, MGET(
					    "%s: error decoding g721\n"), prog);
						errorStatus++;
						break;
					}
				} else if (Decode == AUDIO_ENCODING_G723) {
					err = g723_decode(inbuf, cnt, &File_hdr,
					    (void*)outbuf, &cnt, &adpcm_state);
					if (err != AUDIO_SUCCESS) {
						Error(stderr, MGET(
					    "%s: error decoding g723\n"), prog);
						errorStatus++;
						break;
					}
				} else if( swapBytes) {
					swab((char *)inbuf, outbuf, cnt);
				}

				/* If input EOF, write an eof marker */
				err = write(Audio_fd, outbuf, cnt);

				if (err < 0) {
				  	perror ("write");
					errorStatus++;
					break;
				}
				else if (err != cnt) {
					Error(stderr,
					    MGET("%s: output error: "), prog);
					perror("");
					errorStatus++;
  					break;
  				}
				if (cnt == 0) {
					break;
				}
  			}
			if (cnt < 0) {
				Error(stderr, MGET("%s: error reading "), prog);
				perror(Ifile);
				errorStatus++;
			}
		} else {						/* We're mmaped */
			if ( (Decode != AUDIO_ENCODING_NONE) || swapBytes) {

				/* Transform data if we have to. */
				for (i = 0; i <= len; i += cnt) {
					cnt = insiz;
					if ((i + cnt) > len) {
						cnt = len - i;
					}
					if (Decode == AUDIO_ENCODING_G721) {
						err = g721_decode(inbuf, cnt,
						    &File_hdr, (void*)outbuf,
						    &outsiz, &adpcm_state);
						if (err != AUDIO_SUCCESS) {
							Error(stderr, MGET(
					    "%s: error decoding g721\n"), prog);
							errorStatus++;
							break;
						}
					} else if
					    (Decode == AUDIO_ENCODING_G723) {
						err = g723_decode(inbuf, cnt,
						    &File_hdr, (void*)outbuf,
						    &outsiz, &adpcm_state);
						if (err != AUDIO_SUCCESS) {
							Error(stderr, MGET(
					    "%s: error decoding g723\n"), prog);
							errorStatus++;
							break;
						}
					} else if (swapBytes) {
						swab((char*)inbuf, outbuf, cnt);
						outsiz = cnt;
					}
					inbuf += cnt;

					/* If input EOF, write an eof marker */
					err = write(Audio_fd, (char*)outbuf,
					    outsiz);
					if (err < 0) {
					        perror("write");
						errorStatus++;
					}
					else if (outsiz == 0) {
						break;
					}
					
				}
			} else {
				/* write the whole thing at once!  */
				err = write(Audio_fd, inbuf, len);
				if (err < 0) {
					perror("write");
					errorStatus++;
				}
				if (err != len) {
					Error(stderr,
					    MGET("%s: output error: "), prog);
					perror("");
					errorStatus++;
				}
				err = write(Audio_fd, inbuf, 0);
				if (err < 0) {
					perror("write");
					errorStatus++;
				}
			}
  		}
  
closeinput:;
		if (mapaddr != 0)
			munmap(mapaddr, st.st_size);
  		(void) close(ifd);		/* close input file */
		if (Errdetect) {
			cnt = 0;
			audio_set_play_error(Audio_fd, (unsigned int *)&cnt);
			if (cnt) {
				Error(stderr,
				    MGET("%s: output underflow in %s\n"),
				    Ifile, prog);
				errorStatus++;
			}
		}
nextfile:;
	} while ((argc > 0) && (argc--, (Ifile = *argv++) != NULL));

	/*
	 * Though drain is implicit on close(), it's performed here
	 * to ensure that the volume is reset after all output is complete.
	 */
	(void) audio_drain(Audio_fd, FALSE);
	if (Volume != INT_MAX)
		(void) audio_set_play_gain(Audio_fd, &Savevol);
	if (Balance != INT_MAX)
		(void) audio_set_play_balance(Audio_fd, &Savebal);
	if (Port != INT_MAX)
		(void) audio_set_play_port(Audio_fd, &Saveport);
	if ((Audio_ctlfd >= 0) && (audio_cmp_hdr(&Save_hdr, &Dev_hdr) != 0)) {
		(void) audio_set_play_config(Audio_fd, &Save_hdr);
	}
	(void) close(Audio_fd);			/* close output */
	exit(errorStatus);
}


/*
 * Try to reconfigure the audio device to match the file encoding.
 * If this fails, we should attempt to make the input data match the
 * device encoding.  For now, we give up on this file.
 *
 * Returns TRUE if successful.  Returns FALSE if not.
 */
reconfig()
{
	int	err;
	char	msg[AUDIO_MAX_ENCODE_INFO];

	Dev_hdr = File_hdr;
	err = audio_set_play_config(Audio_fd, &Dev_hdr);

	switch (err) {
	case AUDIO_SUCCESS:
		return (TRUE);

	case AUDIO_ERR_NOEFFECT:
		/*
		 * Couldn't change the device.
		 * Check to see if we're nearly compatible.
		 * audio_cmp_hdr() returns >0 if only sample rate difference.
		 */
		if (audio_cmp_hdr(&Dev_hdr, &File_hdr) > 0) {
			double	ratio;

			ratio = (double) abs((int)
			    (Dev_hdr.sample_rate - File_hdr.sample_rate)) /
			    (double) File_hdr.sample_rate;
			if (ratio <= SAMPLE_RATE_THRESHOLD) {
				if (Verbose) {
					Error(stderr,
			MGET("%s: WARNING: %s sampled at %d, playing at %d\n"),
					    prog, Ifile, File_hdr.sample_rate,
					    Dev_hdr.sample_rate);
				}
				return (TRUE);
			}
			Error(stderr,
			    MGET("%s: %s sample rate %d not available\n"),
			    prog, Ifile, File_hdr.sample_rate);
			return (FALSE);
		}
		(void) audio_enc_to_str(&File_hdr, msg);
		Error(stderr, MGET("%s: %s encoding not available: %s\n"),
		    prog, Ifile, msg);
		return (FALSE);

	default:
		Error(stderr,
		    MGET("%s: %s audio encoding type not available\n"),
		    prog, Ifile);
		exit(1);
	}
}


/* Parse an unsigned integer */
parse_unsigned(
	char*		str,
	unsigned	*dst,
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
 * Search for fname in path and open. Ignore path not opened O_RDONLY. 
 * Note: in general path can be a list of ':' separated paths to search
 * through.
*/
static int
path_open(
	char*		fname,
	int		flags,
	mode_t		mode,
	char*		path)
{
	char		fullpath[MAXPATHLEN]; 	/* full path of file */
/*	char		buf[MAXPATHLEN];*/	/* tmp buf to parse path */
	char 		*buf;			/* malloc off the tmp buff */
	char		*cp;
	struct stat	st;

	if (!fname) {		/* bogus */
		return (-1);
	}

	/*
	 * cases where we don't bother checking path:
	 *      - no path
	 *	- file not opened O_RDONLY
	 *	- not a relative path (i.e. starts with /, ./, or ../).
	 */

	if ((!path) || (flags != O_RDONLY)
	    || (*fname == '/')
	    || (strncmp(fname, "./", strlen("./")) == 0)
	    || (strncmp(fname, "../", strlen("../")) == 0)) {
		return (open(fname, flags, mode));
	}

	/*
	 * Malloc off a buffer to hold the path variable.
	 * This is NOT limited to MAXPATHLEN characters as
	 * it may contain multiple paths.
	*/
	buf = malloc( strlen(path) + 1);

	/* 
	 * if first character is ':', but not the one following it,
	 * skip over it - or it'll be interpreted as "./". it's OK
	 * to have "::" since that does mean "./".
	 */

	if ((path[0] == ':') && (path[1] != ':')) {
		strncpy(buf, path+1, strlen(path));
	} else {
		strncpy(buf, path, strlen(path));
	}
	
	for (path = buf; path && *path; ) {
		if (cp = strchr(path, ':')) {
			*cp++ = NULL; /* now pts to next path element */
		}

		/* the safest way to create the path string :-) */
		if (*path) {
			strncpy(fullpath, path, MAXPATHLEN);
			strncat(fullpath, "/", MAXPATHLEN);
		} else {
			/* a NULL path element means "./" */
			strncpy(fullpath, "./", MAXPATHLEN);
		}
		strncat(fullpath, fname, MAXPATHLEN);

		/* see if there's a match */
		if (stat(fullpath, &st) >= 0) {
			if (S_ISREG(st.st_mode)) {
				/* got a match! */
				if (Verbose) {
					Error(stderr,
				    MGET("%s: Found %s in path at %s\n"),
					    prog, fname, fullpath);
				}
				return (open(fullpath, flags, mode));
			}
		}

		/* go on to the next one */
		path = cp;
	}

	/*
	 * if we fall through with no match, just do a normal file open
	 */
	return (open(fname, flags, mode));
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
	return ((((double)g / AUDIO_RIGHT_BALANCE) * (RIGHT_BAL - LEFT_BAL)) 
	       - (RIGHT_BAL));
}
