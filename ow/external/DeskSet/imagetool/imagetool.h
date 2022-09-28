
/*
 * @(#)imagetool.h 1.30 94/03/14
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <stdio.h>
#include <sys/types.h>

#ifdef XGETTEXT
#define MSGFILE_ERROR	"SUNW_DESKSET_IMAGETOOL_ERR"
#define MSGFILE_LABEL	"SUNW_DESKSET_IMAGETOOL_LABEL"
#define MSGFILE_MESSAGE	"SUNW_DESKSET_IMAGETOOL_MSG"
#else
extern char 	        *MSGFILE_ERROR;
extern char 	        *MSGFILE_LABEL;
extern char 	        *MSGFILE_MESSAGE;
#endif

#define DGET(s)		 s
#define EGET(s)		 (char *) dgettext (MSGFILE_ERROR, s)
#define LGET(s)		 (char *) dgettext (MSGFILE_LABEL, s)
#define MGET(s)		 (char *) dgettext (MSGFILE_MESSAGE, s)

#define OFF		0
#define ON		1

#define FALSE		0
#define TRUE		1

#define Min(a,b)        ((a) > (b) ? (b) : (a))
#define Max(a,b)        ((a) > (b) ? (a) : (b))

#define MAXCOLORS       256

typedef struct mmap_handle {
  caddr_t       addr;
  int           len;
} mmap_handle_t;
 
typedef struct ENCODED_BM {
  unsigned int    sieve[256];
  unsigned char **strings;
  unsigned int   *lengths;
  unsigned int    len;
  unsigned int    nstrings;
} encoded_bm;

typedef struct {
	char	        *name;		/* Name of this application  	  */
	char	        *rel;		/* Release (from ds_relname  	  */
	char	        *directory;	/* Current working directory 	  */
	char	        *hostname;	/* Hostname where running    	  */
	char		*newsserver;	/* String used for ps_open_server */
	int		 dps;		/* True if using DPS server	  */
	int		 remote;	/* True if display is remote      */
	int		 news_opened;   /* True if opened news server     */
        int              xil_opened;    /* True if opened XIL	          */
	int		 frame_mapped;  /* Frame has been mapped	  */
	int              ce_okay;       /* CE initialized successfully    */
	int              sb_right;      /* True if scrolbars on on right  */
	int              def_ps_zoom;   /* Set default PS zoom factor     */
	int		 tt_started;	/* True if started with -tooltalk */
	char		*tt_sender;	/* Tooltalk sender id		  */
	int		 tt_load;	/* True if data from tt		  */
        int		 tt_timer_set;  /* True if imagetool sleeping     */
	char		*file_template; /* Template to use for tmp files  */
	char		*datafile;	/* File we wrote data into	  */
	char		*compfile;	/* Compressed file written here   */
	char		*uncompfile;	/* Uncompressed file written here */
	char		*printfile;	/* Print tmpfile name		  */
	char		*rashfile;	/* Rash output tmpfile name	  */
	char		*def_printer;	/* Default printer		  */
	int		 timeout;	/* timeout in sec		  */
	int		 verbose;	/* Print out additional info 	  */
	uid_t		 uid;
	gid_t 		 gid;
} ProgInfo;

extern	ProgInfo        *prog;

/* Function prototypes */
extern	char		*make_pathname (char *, char *);
extern  char		*basename (char *);
extern	char		*strip_filename (char *);
extern  int		 check_float_value (char *, double *);
extern  mmap_handle_t	*fast_read (char *);
extern  encoded_bm	*strbmencode (char **);
extern  char		*strbmexec (unsigned char *, int, encoded_bm *);

#endif
