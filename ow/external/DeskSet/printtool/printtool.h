/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#define CMD_ARG(s)	!strcmp(*argv, (s))
#define DOMAIN_NAME 	"SUNW_DESKSET_PRINTTOOL"
#define EMPTY	 	0
#define FULL		1
#define INTERVAL	10  /* number of seconds between print checks */
#define PROGNAME	"printtool"
#define NUM_FLASH   	2  /* # of flashes for errmsg or job done */
#define NUM_BEEP       	1  /* # of beeps for error messages or job done*/
#define BF           	1  /* beep/flash need if any */
#define NO_BF           0  /* no beep/flash need */


typedef struct Printtool_Misc_Struct {
   int           openmode;       /* icon mode */
   int           testing;        /* testing mode */
   int           check_interval; /* interval between printer status checks */
   int		 newprinter;	 /* new selected printer */
   Icon          current_icon;   /* current frame icon */
   char          *username;
   char		 *printer;	 /* printer preset by user */
   char		 print_method[MAXPATHLEN+1];   /* print method preset by user */
   char		 errfile[16];	 /* error file */
   Server_image  icon;
   Server_image  icon_mask;
   Server_image  noprint_icon;
   Server_image  noprint_icon_mask;
   Xv_font       fixedfont;     /* fixed font used in scrolling list */
   Xv_font       fixedttfont;   /* fixed font used for scrolling list title */

   CE_NAMESPACE  file_ns;       /* handle to file namespace (fns) */
   CE_ATTRIBUTE  file_type;     /* handle to fns type attribute */
   CE_NAMESPACE  type_ns;       /* handle to type namespace (tns) */
   CE_ATTRIBUTE  type_print;    /* handle to tns print attribute */
   CE_ATTRIBUTE  type_name;    /* handle to tns name attribute */
   int           ce_running;    /* Classing Engine Available? */

   int	 	 landscape;	/* portrait = FALSE, landscape = TRUE */
   int		 header;	/* w/o header = FALSE, w/  header = TRUE */
   int		 notify_beep;	/* no beep = FALSE, beep = TRUE */
   int		 notify_flash;  /* no flash = FALSE, flash = TRUE */
   int		 notify_mail;  	/* no mail = FALSE, mail = TRUE */
   int		 notify_console; /* no write in console=FALSE, console=TRUE */
   int		 override_pmethod;  /* not override = FALSE, override = TRUE */

   Display	 *dply;   	 /* graphics info for flashing/beeping */
   GC		 mp_gc;
   GC		 icon_gc;
   int		 foreground;
   int		 background;
   Drawable	 mp_xid;
   Drawable	 icon_xid;

   XrmDatabase	 rDB;		/* All resources database */
   XrmDatabase	 desksetDB;	/* Deskset resources database */
} Printtool_Mobject;

