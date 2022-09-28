/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _RADIO_H
#define	_RADIO_H

#ident	"@(#)radio.h	1.8	92/03/19 SMI"

/* Definitions and structures for Radio Free Ethernet */


/* Default audio device name */
#define	AUDIO_DEVICE		"/dev/audio"

/* Default name of state file */
#define	RADIO_RCFILE		".radiorc"

/* Define command strings for communication between programs */
#define	RADIOCMD_STATION	"STATION"	/* station call letters */
#define	RADIOCMD_REPORT		"REPORT"	/* enable status reports */
#define	RADIOCMD_START		"START"
#define	RADIOCMD_STOP		"STOP"
#define	RADIOCMD_QUIT		"QUIT"
#define	RADIOCMD_SERVICE	"SERVICE"	/* service name or port num */
#define	RADIOCMD_ADDRESS	"ADDRESS"	/* BROADCAST or IP base addr */

/* return status */
#define	RADIOCMD_STATUS		"STATUS:"
#define	RADIOCMD_POWEROFF	"OFF"
#define	RADIOCMD_BROADCAST	"BROADCAST"

/* radio_xmit commands */
#define	RADIOCMD_INPUT		"INPUT"
#define	RADIOCMD_SQUELCH	"SQUELCH"
#define	RADIOCMD_AGC		"AGC"
#define	RADIOCMD_AUTOSTOP	"AUTOSTOP"
#define	RADIOCMD_BUFSIZ		"BUFFER"
#define	RADIOCMD_FORMAT		"FORMAT"
#define	RADIOCMD_RANGE		"RANGE"
#define	RADIOCMD_NETWORK	"NETWORK"

/* radio_xmit return status */
#define	RADIOCMD_EOF		"DONE"

/* radio_recv commands */
#define	RADIOCMD_OUTPUT		"OUTPUT"
#define	RADIOCMD_SCAN		"SCAN"
#define	RADIOCMD_RELEASE	"RELEASE"

/* radio_recv return status */
#define	RADIOCMD_STATIONREPORT	"ACTIVE:"
#define	RADIOCMD_IDREPORT	"STATIONID:"
#define	RADIOCMD_SCANNING	"SCANNING"
#define	RADIOCMD_SIGNOFF	"SIGNOFF"


/* Limiting values for input buffersize */
/* XXX - this should be defined in terms of time */
#define	RADIO_MIN_BUFSIZ	(128)
#define	RADIO_MAX_BUFSIZ	(8000)


/* Values for program state */
#define	POWER_OFF	(1)
#define	SCANNING	(2)	/* radio only */
#define	QUIET		(3)
#define	ON_THE_AIR	(4)
#define	OFF_THE_AIR	(5)
#define	WAITING		(6)	/* radio only */
#define	BACKOFF		(7)	/* radio only */
#define	SIGN_ON		(8)	/* xmit only */
#define	SIGN_OFF	(9)	/* xmit only */

extern char*	sys_errlist[];

/* macros for various libc routines */
#define	STRCPY		(void) strcpy
#define	STRNCPY		(void) strncpy
#define	STRCAT		(void) strcat
#define	STRNCAT		(void) strncat
#define	FPRINTF		(void) fprintf
#define	SPRINTF		(void) sprintf
#define	PERROR(msg)	FPRINTF(stderr, "%s: %s: %s\n",	\
			    prog, msg, sys_errlist[errno]);

/* Why aren't these stupid values defined in a standard place?! */
#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif

typedef void*	ptr_t;


/* Command parse table */
typedef struct {
	char*		keyword;		/* Command keyword */
	int		(*func)(char*);		/* Handler function */
	int		seen;			/* Flag for .radiorc writes */
} Radio_cmdtbl;


/* Routines defined in radio_subr.c */
extern char*	copy_string(char*);
extern void	replace_string(char**, char*);
extern int	input_ready(int);
extern int	parse_input(int, Radio_cmdtbl*, int, char*);
extern char**	lex_line(int);
extern char**	lex_multiline(int);
extern int	is_on(char*);
extern char*	parse_cmds(char**, Radio_cmdtbl*);

/* Routines defined in radio_rcfile.c */
extern int	radiorc_open(int);
extern void	radiorc_close(int);
extern void	radiorc_putstr(int, Radio_cmdtbl*, char*);
extern void	radiorc_putval(int, Radio_cmdtbl*, int);
extern void	radiorc_putcomment(int, char**);
extern void	radiorc_putblank(int);

#endif /* !_RADIO_H */
