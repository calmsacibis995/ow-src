#ifndef lint
static char sccsid[] = "@(#)snapshot.c	3.10 11/20/96 Copyright 1987-1990 Sun Microsystem, Inc." ;
#endif

/*  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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

#include "snapshot.h"
#include "patchlevel.h"
#include <errno.h>
#include <string.h>
#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <sys/stat.h>
#include <dstt.h>
#include "xdefs.h"
#include "math.h"

Vars v ;              /* Snapshot variables and options. */
extern XVars X ;

extern Panel_item	load_button;
extern Panel_item	save_button;
extern Panel_item	print_button;

extern char *cmdstr[] ;
extern char *mstr[] ;
extern char *ostr[] ;

static void usage P((char *)) ;

int SNAPSHOT_DB_HW = 0;   /* Workaround flag for ESC 502957, BUGID 1222996 */

void
tool_info (vend, name, ver)
char    **vend;
char    **name;
char    **ver;
{
    *vend = MGET("Sun Microsystems");
    *name = v->tool_name;
    *ver = (char *) ds_relname ();
}

int
main(argc, argv)
int argc ;
char  *argv[] ;
{
  v = (Vars) LINT_CAST(malloc(sizeof(SnapVars))) ;
  do_snapshot(argc, argv) ;
  exit(0) ;
/*NOTREACHED*/
}


int
check_overwrite(current_op)
int current_op;
{
  if (v->noconfirm == FALSE && v->image != NULL && v->image->data != NULL)
    {
      switch (v->ls_status)
        {
          case ISNONE : return(TRUE) ;
          case ISSNAP : return(do_overwrite(O_SOVERWRITE, current_op)) ;
          case ISLOAD : return(TRUE) ;
	  case ISDRAG : return(do_overwrite(O_LOVERWRITE, current_op)) ;
	  case ISQUIT : return(do_overwrite(O_SOVERWRITE, current_op)) ;
        }
    }
  return(TRUE) ;
}

void
status_callback (ttmsg, status, vendor, toolname, version, msgid, domain)
Tt_message  ttmsg;
char       *status;
char       *vendor;
char       *toolname;
char       *version;
char       *msgid;
char       *domain;
{

    char	*sender = tt_message_sender (ttmsg);

/*
 * If v->ttid is NULL, and we got a dstt_status_req_rec status,
 * then imagetool was started OK.
 */

    if (v->ttid == (char *) NULL) {
       if (strcmp (status, 
		 (char *) dstt_set_status (NULL, dstt_status_req_rec)) == 0) {
          v->ttid = strdup (sender);
          xv_set (load_button, PANEL_INACTIVE, TRUE, NULL);
          xv_set (save_button, PANEL_INACTIVE, TRUE, NULL);
          xv_set (print_button, PANEL_INACTIVE, TRUE, NULL);
          xv_set (X->pitems [(int) PI_DRAG], PANEL_INACTIVE, TRUE, NULL);
          message(" ");
          setactive ();
          return;
          }
       else
	  return;
       }

/*
 * Now check ttid, if not the same then we don't care.
 */

    if (strcmp (v->ttid, tt_message_sender (ttmsg)) != 0)
       return;

/*
 * Check status. If it's quit or discontinued service, then
 * imagetool is going away. 
 */

    if (strcmp (status, 
	(char *) dstt_set_status (NULL, dstt_status_user_request_cancel)) 
		== 0) {
       v->no_imagetool = TRUE;
       free (v->ttid);
       v->ttid = (char *) NULL;

/*
 * Turn back on the panel items. 
 */
 
       xv_set (load_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (save_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (X->pitems [(int) PI_DRAG], PANEL_INACTIVE, FALSE, NULL);

       }

/*
 * Else, we got another request received (probably we just displayed
 * another image), so make frame active again.
 */

    else if (strcmp (status, 
		 (char *) dstt_set_status (NULL, dstt_status_req_rec)) == 0) {
       message(" ");
       setactive ();
       return;
       }

}

/*
 * Imagetool is quitting.
 */

void
quit_tt (ttmsg, silent, force, msgID)
Tt_message       ttmsg;
int              silent;
int              force;
char            *msgID;
{
    v->no_imagetool = TRUE;
    free (v->ttid);
    v->ttid = (char *) NULL;

/*
 * Turn back on the panel items. 
 */
 
    xv_set (load_button, PANEL_INACTIVE, FALSE, NULL);
    xv_set (save_button, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print_button, PANEL_INACTIVE, FALSE, NULL);
    xv_set (X->pitems [(int) PI_DRAG], PANEL_INACTIVE, FALSE, NULL);
}

void
saved_file (ttmsg, type, filename)
Tt_message	 ttmsg;
char		*type;
char		*filename;
{
    if (strcmp (type, DGET ("File")) != 0)
       return;

    if (filename != (char *) NULL) {
       strcpy (v->path, filename);
       v->havename = TRUE ;
       set_namestripe ();
       v->ls_status = ISNONE;
       }
}
    
void create_gamma_table()
{
	int i;
	double new;

	for (i = 0; i < 256; i++) {
		new = 255.0 * pow((i / 255.0), 1.0 / v->gamma_val);
		v->gamma_tbl[i] = (int) floor (new + 0.5);
	}
}

void
do_snapshot(argc, argv)
int argc ;
char *argv[] ;
{
  dstt_check_startup (tool_info, &argc, &argv);
  init_graphics(&argc, argv) ;   /* Make connection to graphics system. */
  init_text() ;                  /* Set text strings to local language. */
  v->tool_name    = ostr[(int) O_TOOL] ;

  init_vars() ;                  /* Setup initial values for variables. */
  init_pad () ;			 /* Initialize padding info */
  v->default_file = ostr[(int) O_DEFF] ;
  make_icon() ;                  /* Create snapshot icon (plus mask). */
  make_frame() ;                 /* Create main snapshot frame. */
  make_cursors() ;               /* Build the cursors we need. */
  make_images() ;                /* Create drag-n-drop server images. */

  init_multivis () ;		 /* See if we're dealing with > 1 visuals */

  while (--argc)
    {
      ++argv ;
      if (argv[0][0] == '-') 
        {
          switch (argv[0][1])
            {
              case 'c' : v->gamma_val = atof(argv[1]);
                         ++argv ;
                         --argc ;
                         break ;
              case 'd' : v->default_dir = argv[1] ;
                         ++argv ;
                         --argc ;
                         break ;
              case 'f' : v->default_file = argv[1] ;
                         ++argv ;
                         --argc ;
                         break ;
              case 'g' : v->isgray = TRUE ;
                         break ;
              case 'l' : v->autoload = TRUE ;
                         if (argv[1] != NULL)
                           STRCPY(v->path, argv[1]) ;
                         ++argv ;
                         --argc ;
                         break ;
              case 'n' : v->noconfirm = TRUE ;
                         break ;
              default  : usage(v->tool_name) ;
            }
        }
      else usage(v->tool_name) ;
    }

  create_gamma_table();
  set_namestripe() ;
  make_main_panel() ;    /* Create main snapshot panel plus items. */

  dragdrop_init () ;	 /* Create interesting dnd objects. */

  if (v->autoload == FALSE) CLIPBOARD_NAME(v->path) ;

  v->file_pos = (char *) strrchr(v->path, '/') ;

  if (v->file_pos == NULL)
    {
      GETCWD(v->directory_name, MAXPATHLEN) ;
      STRCPY(v->file_name, v->path) ;
    }
  else
    { 
      STRCPY(v->file_name, (v->file_pos + 1)) ;
      *(v->file_pos) = '\0' ;
      STRCPY(v->directory_name, v->path) ;
      SPRINTF(v->path, "%s/%s", v->directory_name, v->file_name) ;
    }
  if (v->autoload == TRUE)
    {
      v->havename = TRUE ;
      SPRINTF(v->path, "%s/%s", v->directory_name, v->file_name) ;
      if (loadfile(v->path, R_FILE) == TRUE) {
         v->ls_status = ISLOAD ;
         viewfile() ;
 	 }
      else 
	 message (mstr [(int) M_UNRECOG]);
    }
  write_cmdline() ;      /* Setup and save snapshot command line options. */
  dstt_xview_desktop_callback (X->frames [(int) F_MAIN], 
				QUIT,	quit_tt,
				NULL);
  dstt_xview_desktop_register (X->frames [(int) F_MAIN], 
				QUIT,	TRUE,
				NULL);
  dstt_notice_callback (NULL, STATUS, status_callback, 
			      SAVED,  saved_file, NULL);
  dstt_notice_register (NULL, STATUS, TRUE, 
			      SAVED,  TRUE,
			      NULL);
  start_tool() ;         /* Setup event handlers and start application. */
}


void
do_delay()
{
  static struct timeval stall = { 0, 50000 } ;
  int bells = 5 ;

  if (v->delay_count > 0)
    {
      if (get_int_val(PI_BELL)) sound_bell(stall) ;  /* Ring keyboard bell. */
      v->delay_count--;
    }
  else
    { 
      disable_timer() ;
      while (bells > 0)     /* Screendump pending, ring more frequently. */
        {

/* Pause between bells */

          SELECT(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &stall) ;
          bells-- ;
          if (get_int_val(PI_BELL)) sound_bell(stall) ;
        }
      copyrect() ;          /* Complete the screendump */
    }
}


void
do_destination(value)
int value ;
{
  if (value == 0)
    {
      show_item(PI_PDIR,  FALSE) ;
      show_item(PI_PFILE, FALSE) ;
      show_item(PI_PNAME, TRUE) ;
    } 
  else
    { 
      show_item(PI_PNAME, FALSE) ;
      show_item(PI_PDIR,  TRUE) ;
      show_item(PI_PFILE, TRUE) ;
    }
}


void
do_position(value)
int value ;
{
  switch (value)
    {
      case 0  : activate_item(PI_LEFTPOSV,  FALSE) ;
                activate_item(PI_BOTPOSV,  FALSE) ;
	        activate_item(PI_LEFTPOSM, FALSE) ;
	        activate_item(PI_BOTPOSM, FALSE) ;
                break ;
      case 1  : activate_item(PI_LEFTPOSV,  TRUE) ;
                activate_item(PI_BOTPOSV,  TRUE) ;
	        activate_item(PI_LEFTPOSM, TRUE) ;
	        activate_item(PI_BOTPOSM, TRUE) ;
                break ;
    }
}


void
do_scaleto(value)
int value ;
{
  switch (value)
    {
      case 0  : activate_item(PI_WIDTHV,  FALSE) ;
                activate_item(PI_WIDTHT,  FALSE) ;
                activate_item(PI_HEIGHTV, FALSE) ;
                activate_item(PI_HEIGHTT, FALSE) ;
                break ;
      case 1  : activate_item(PI_WIDTHV,  TRUE) ;
                activate_item(PI_WIDTHT,  TRUE) ;
                activate_item(PI_HEIGHTV, FALSE) ;
                activate_item(PI_HEIGHTT, FALSE) ;
                break ;
      case 2  : activate_item(PI_HEIGHTV, TRUE) ;
                activate_item(PI_HEIGHTT, TRUE) ;
                activate_item(PI_WIDTHV,  FALSE) ;
                activate_item(PI_WIDTHT,  FALSE) ;
                break ;
      case 3  : activate_item(PI_WIDTHV,  TRUE) ;
                activate_item(PI_WIDTHT,  TRUE) ;
                activate_item(PI_HEIGHTV, TRUE) ;
                activate_item(PI_HEIGHTT, TRUE) ;
    }
}


void
do_rect_dump()
{
  int width, height ;

  if (check_overwrite(ISSNAP) == FALSE) return ;
  message(mstr[(int) M_GETRECT]) ;
     
  do_sync(F_MAIN) ;
 
  if (get_area(D_RECT, &width, &height) == 1)
    {
      free_fullscreen() ;

      if (get_int_val(PI_HIDEWIN))
        {
          message(mstr[(int) M_BACKIN8]) ;
          frame_show(F_ALL, FALSE) ;
        }
 
      if (v->snapshot_delay > 0) start_timer() ;
      else                       copyrect() ;
    }
}


void
do_screen_dump()
{
  int height, width ;

  if (check_overwrite(ISSNAP) == FALSE) return ;
  message("") ;

  if (get_int_val(PI_HIDEWIN))
    {
      message(mstr[(int) M_BEBACK]) ;
      frame_show(F_ALL, FALSE) ;
    }
  message(mstr[(int) M_COPYING]) ;

  do_sync(F_MAIN) ;
  make_fullscreen(D_SCREEN) ;

  if (get_area(D_SCREEN, &width, &height) == 1)
    {
      free_fullscreen() ;
 
      if (v->snapshot_delay > 0) start_timer() ;
      else                       copyrect() ;
    }
}


void
do_window_dump()
{
  int reply, width, height ;
 
  if (check_overwrite(ISSNAP) == FALSE) return ;
  message(mstr[(int) M_GETWIN]) ;
 
  do_sync(F_MAIN) ;
 
  if ((reply = get_area(D_WINDOW, &width, &height)) == 1)
    { 
      free_fullscreen() ;

      if (v->snapshot_delay > 0) start_timer() ;
      else {
	 sleep(1);
         copyrect() ;
	 }
    }
  else if (reply == -1)
    {
      free_fullscreen() ;
      if (get_int_val(PI_HIDEWIN)) frame_show(F_ALL, TRUE) ;
      message(mstr[(int) M_NOWIN]) ;
    }
}


void
init_vars()       /* Setup initial values for certain variables. */
{
  v->ttid	     = (char *) NULL;
  v->no_imagetool    = FALSE ;   /* We don't know yet if this is TRUE. */
  v->autoload        = FALSE ;
  v->debug_on        = 0 ;
  v->havename        = FALSE ;
  v->filetype	     = RASTERFILE;
  v->hide            = 0 ;
  v->isgray          = FALSE ;
  v->ispopen         = FALSE ;
  v->ls_status       = ISNONE ;          /* No initial load/snap. */
  v->noconfirm       = FALSE ;           /* Prompt user before overwriting. */
  v->snapshot_delay  = DEFAULT_DELAY ;   /* Seconds to delay */
  v->timer_bell_on   = 0 ;               /* Keep state for graying out. */
  v->gamma_val       = 1.0 ;             /* Gamma correction factor */

  v->grasps[0][0]    = NW ;
  v->grasps[0][1]    = N ;
  v->grasps[0][2]    = NE,
  v->grasps[1][0]    = W ;
  v->grasps[1][1]    = MID ;
  v->grasps[1][2]    = E,
  v->grasps[2][0]    = SW ;
  v->grasps[2][1]    = S ;
  v->grasps[2][2]    = SE ;

  v->default_dir     = NULL ;
  v->fp              = NULL ;
  v->rbuf            = NULL ;
  v->image           = NULL ;

  /* Set workaround flag for ESC 502957, BUGID 1222996 */
  SNAPSHOT_DB_HW = (getenv("SNAPSHOT_DB_HW") == NULL) ? 0 : 1;
}


void
print_snapfile()
{
  char buf[120], cmd[2048], tmpname[MAXPATHLEN] ;
  char tmppath [MAXPATHLEN];

  if (v->image == NULL)
    {
      message(mstr[(int) M_NOPIMAGE]) ;
      return;
    }
	      
/*
 * Check directory that was entered first... (if saving to a file)
 * to determine if we can really save to that directory.
 */

  if (get_int_val(PI_DEST) == 1) 
     if (!verify_paths(TRUE, PI_PDIR, PI_PFILE, tmppath))
	return;

  SPRINTF(tmpname, "/tmp/snapshot%s.image", mktemp("XXXXXX")) ;
  message(mstr[(int) M_PRINTING]) ;
  
  SPRINTF(cmd, ostr[(int) O_PDEST], getenv("OPENWINHOME")) ;

/* Set up rotate option. */

  if (get_int_val(PI_PORIENT) == 1)
    {
      SPRINTF(buf, "-%c ", ROTATE_OPT) ;
      STRCAT(cmd, buf) ;
    }

/* Set up position option. */

  if (get_int_val(PI_POSITION) == 1)
    {
      SPRINTF(buf, "-%c %s%s %s%s ",
	      POSITION_OPT, 
	      get_char_val(PI_LEFTPOSV), "in", get_char_val(PI_BOTPOSV), "in") ;
      STRCAT(cmd, buf) ;
    }

/* Set up double bits option. */

  if (get_int_val(PI_DBITS) == 1)
    {
      SPRINTF(buf, "-%c %d%s %d%s ", SCALE_OPT,
	      2 * v->image->width, "pt", 2 * v->image->height, "pt") ;
      STRCAT(cmd, buf) ;
    }
  else
/* Set up scale option.  Not valid if Double bits is on */
  if (get_int_val(PI_SCALE) == 0)
    {
      SPRINTF(buf, "-%c %d%s %d%s ", SCALE_OPT,
	      v->image->width, "pt", v->image->height, "pt") ;
      STRCAT(cmd, buf) ;
    }
  else if (get_int_val(PI_SCALE) == 1)
    {
      SPRINTF(buf, "-%c %s%s ", WIDTH_OPT, get_char_val(PI_WIDTHV), "in") ;
      STRCAT(cmd, buf) ;
    }
  else if (get_int_val(PI_SCALE) == 2)
    {
      SPRINTF(buf, "-%c %s%s ", HEIGHT_OPT, get_char_val(PI_HEIGHTV), "in") ;
      STRCAT(cmd, buf) ;
    }
  else if (get_int_val(PI_SCALE) == 3)
    {
      SPRINTF(buf, "-%c %s%s %s%s ", SCALE_OPT,
	      get_char_val(PI_WIDTHV), "in", get_char_val(PI_HEIGHTV), "in") ;
      STRCAT(cmd, buf) ;
    }

/* Create a temporary file containing the image, and append this name to
 *  the print command.
 */

  if (get_int_val(PI_MONO) == 1)
    {
      SPRINTF(buf, "-%c ", MONO_OPT) ;
      STRCAT(cmd, buf) ;
    }

  savefile(tmpname, R_FILE, 1) ;
  STRCAT(cmd, tmpname) ;

/* Set up printer or destination. */

  if (get_int_val(PI_DEST) == 0) {
    char *tmp_name = get_char_val (PI_PNAME);
    if ((tmp_name != (char *) NULL)  &&
	(tmp_name [0] != (char) NULL)) 
       SPRINTF (buf, " | %s -s -%c %s ", ostr [(int) O_PRINT], PRINTER_OPT, 
			tmp_name);
    else
       SPRINTF (buf, " | %s -s ", ostr [(int) O_PRINT]);
    STRCAT(cmd, buf) ;
  }
  else
    {
      SPRINTF(buf, " > %s ", tmppath) ;
      STRCAT(cmd, buf) ;
    }

  if (system(cmd) == 0) message(mstr[(int) M_CPRINT]) ;
  else                  message(mstr[(int) M_FPRINT]) ;

  UNLINK(tmpname) ;     /* Remove temporary print file. */
}


static void
usage(name)
char *name ;
{
  FPRINTF(stderr, ostr[(int) O_VERSION], name, PATCHLEVEL) ;
  FPRINTF(stderr, ostr[(int) O_USAGE1], name) ;
  FPRINTF(stderr, ostr[(int) O_USAGE2]) ;
  exit(1) ;
}


/* Return pointer to longest suffix not beginning with '/'. */

char *
base_name(full_name)
char *full_name ;
{
  register char *temp ;

  if ((temp = strrchr(full_name, '/')) == NULL) return(full_name) ;
  return(temp + 1) ;
}


int
change_dir()
{
  struct stat stat_buf ;
  char new_dir[MAXPATHLEN], msg_buf[256] ;

  expand_path(get_char_val(PI_LSDIR), new_dir) ;
  if (!strcmp(v->cdir, new_dir)) return(0) ;
  if ((stat(new_dir, &stat_buf) == 0) && (chdir(new_dir) == 0))
    return (0);
  else
    { 
      SPRINTF(msg_buf, ostr[(int) O_NOCDIR], new_dir) ;
      message(msg_buf) ;
      return(-1) ;
    }
  return(0) ;
}


int
check_directory(old, new)
char *old, *new;
{
  struct stat stat_buf ;
 
  expand_path(old, new) ;
  if (stat(new, &stat_buf) == 0) return(0) ;
  return(-1) ;
}


int
check_pathname(pathname)
char *pathname ;
{
  struct stat stat_buf ;
 
  if (stat(pathname, &stat_buf) == 0) return(0) ;
  return(-1) ;
}


char *
clipboard_name(buffer)
char *buffer ;
{
  char cwd[MAXPATHLEN] ;

  if (v->default_dir != NULL) STRCPY(cwd, v->default_dir) ;
  else                        GETCWD(cwd, MAXPATHLEN) ;
  SPRINTF(buffer, "%s/%s", cwd, v->default_file) ;
  return(buffer) ;
}


/* Save the current image to file or buffer. Redisplay the tool if hidden. */

void
savefile(file, rtype, printflag)
char *file ;
enum rasio_type rtype ;
int printflag;
{
  int rastersize ;

  if (rtype == R_FILE)
    {
      if ((v->fp = fopen(file, "w")) == NULL)
        {
          message(mstr[(int) M_NOOPEN]) ;
          return ;
        }
    }    
  else
    { 
      if (v->rbuf != NULL) {
	 FREE(v->rbuf) ;
	 v->rbuf = NULL;
	 }
      rastersize = get_raster_len(v->image) ;
      v->rbuf = (unsigned char *) malloc((unsigned int) rastersize) ;
    }

  if (dump_rasterfile(v->image, rtype) < 0)
    {
      if (rtype == R_FILE) message(mstr[(int) M_SAVEFAIL]) ;
    }
  else
    { 
      if ((rtype == R_FILE) && (printflag == 0))
 	{
	  message(mstr[(int) M_SAVESUCS]) ;
          v->havename = TRUE ;
	  set_namestripe ();
	  v->ls_status = ISNONE;
	}
    }

  if (rtype == R_FILE) FCLOSE(v->fp) ;
}


void
set_namestripe()                  /* Set the namestripe. */
{
  static char namestripe[MAXPATHLEN] ;

  if ((v->havename) && (v->path != (char *) NULL))
     SPRINTF(namestripe, ostr[(int) O_SETNAMEFILE], v->tool_name, ds_relname(),
				base_name(v->path));
  else
     SPRINTF(namestripe, ostr[(int) O_SETNAME], v->tool_name, ds_relname());
  set_label(F_MAIN, namestripe) ;
}


void
start_timer()            /* Call the notifier to do a timed screendump. */
{
  v->tv.it_interval.tv_usec = 0 ;  /* Setup the timer. */
  v->tv.it_interval.tv_sec  = 1 ;
  v->tv.it_value            = v->tv.it_interval ;
  v->delay_count            = v->snapshot_delay ;

  set_time_delay(&v->tv) ;         /* Register callback with the notifier. */
}


int
verify_paths(check, dir, file, path)
int check ;
enum pi_type dir, file ;
char path [MAXPATHLEN];
{
  struct stat stat_buf ;

  if (check_directory(get_char_val(dir), path))
    {
      message(mstr[(int) M_NODIR]) ;
      return(FALSE) ;
    }

  STRCAT(path, "/") ;
  STRCAT(path, get_char_val(file)) ;
  if (check == FALSE) {
     if (check_pathname(path))
       {
         message(mstr[(int) M_BADPATH]) ;
         return(FALSE) ;
       }
     }
  else {
     if (stat(path, &stat_buf) == NULL && v->noconfirm == FALSE)
        return(do_overwrite(O_FOVERWRITE, ISNONE)) ;
     }

  return(TRUE) ;
}


void
write_cmdline()
{
  int argc ;
  char *argv[10], ddir[MAXPATHLEN], dfile[MAXPATHLEN], lfile[MAXPATHLEN] ;

  argc = 0 ;
  if (v->default_dir != NULL)
    {
      argv[argc++] = cmdstr[(int) CMD_DEFDIR] ;
      SPRINTF(ddir, "%s", v->default_dir) ;
      argv[argc++] = ddir ;
    }

  argv[argc++] = cmdstr[(int) CMD_DEFFILE] ;
  SPRINTF(dfile, "%s", v->default_file) ;
  argv[argc++] = dfile ;

  if (v->isgray)    argv[argc++] = cmdstr[(int) CMD_GRAY] ;
  if (v->noconfirm) argv[argc++] = cmdstr[(int) CMD_NOCONFIRM] ;

  if (v->autoload)
    {
      argv[argc++] = cmdstr[(int) CMD_LOAD] ;
      SPRINTF(lfile, "%s", v->path) ;
      argv[argc++] = lfile ;
    }
  save_cmdline(argc, argv) ;
}
