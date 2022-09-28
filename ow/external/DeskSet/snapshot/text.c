#ifndef lint
static char sccsid[] = "@(#)text.c	3.6 02/03/93 Copyright 1987-1990 Sun Microsystem, Inc." ;
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

/*  The following are all the static strings used by the snapshot program.
 *  They are initialised in init_text() to the local language equivalents.
 */

char *cmdstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL
} ;


char *hstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *mstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *mpstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *ostr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *Pstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;


void
init_text()     /* Setup text strings depending upon language. */
{

/* Command line options (for saving). */

  cmdstr[(int) CMD_DEFDIR]    = DGET("-d") ;
  cmdstr[(int) CMD_DEFFILE]   = DGET("-f") ;
  cmdstr[(int) CMD_GRAY]      = DGET("-g") ;
  cmdstr[(int) CMD_LOAD]      = DGET("-l") ;
  cmdstr[(int) CMD_NOCONFIRM] = DGET("-n") ;

/* Message strings. */

  mstr[(int) M_GETRECT] = (char *)
    MGET("SELECT-Position Rectangle. ADJUST-Snap Image. MENU-Cancel.") ;
  mstr[(int) M_GETWIN] = (char *)
    MGET("SELECT - Select Window.  ADJUST or MENU - Cancel.") ;
  mstr[(int) M_ADELAY] = (char *)
    MGET("Timer adjusted to guarantee correct operation.") ;
 
  mstr[(int) M_ABORT]    = (char *)
    MGET("Snap aborted.") ;
  mstr[(int) M_OUTMEM]   = (char *)
    MGET("Out of memory!") ;
  mstr[(int) M_BACKIN8]  = (char *)
    MGET("Be back in 8 seconds...") ;
  mstr[(int) M_BEBACK]   = (char *)
    MGET("Be right back...") ;
  mstr[(int) M_COPYING]  = (char *)
    MGET("Copying Screen...") ;
  mstr[(int) M_NOWIN]    = (char *)
    MGET("No window under mouse.") ;
  mstr[(int) M_PRINTING] = (char *)
    MGET("Printing...") ;
  mstr[(int) M_CPRINT]   = (char *)
    MGET("Print Complete") ;
  mstr[(int) M_FPRINT]   = (char *)
    MGET("Print Failed") ;
  mstr[(int) M_NODIR]    = (char *)
    MGET("Not a directory") ;
  mstr[(int) M_BADPATH]  = (char *)
    MGET("Pathname incorrect") ;
  mstr[(int) M_NOOPEN]   = (char *)
    MGET("Cannot open snap file!") ;
  mstr[(int) M_TAKING]   = (char *)
    MGET("Taking snapshot...") ;
  mstr[(int) M_SAVEFAIL] = (char *)
    MGET("Save failed.") ;
  mstr[(int) M_SNAPSUCS] = (char *)
    MGET("Snap succeeded.") ;
  mstr[(int) M_SAVESUCS] = (char *)
    MGET("Save succeeded.") ;
  mstr[(int) M_CANCEL]   = (char *)
    MGET("Cancelled.") ;
  mstr[(int) M_SCANCEL]  = (char *)
    MGET("Screen Capture Cancelled.") ;
  mstr[(int) M_REDUCE]   = (char *)
    MGET("Reducing to mono image...") ;
  mstr[(int) M_LOADING]  = (char *)
    MGET("Loading...") ;
  mstr[(int) M_NOTRAST]  = (char *)
    MGET("Not a Sun rasterfile") ;
  mstr[(int) M_BADCOL]   = (char *)
    MGET("Bad colormap!") ;
  mstr[(int) M_NOREAD]   = (char *)
    MGET("Couldn't read image!") ;
  mstr[(int) M_LOADSING] = (char *)
    MGET("Load succeeding.") ;
  mstr[(int) M_LOADSUCS] = (char *)
    MGET("Load Succeeded") ;
  mstr[(int) M_COLOR]    = (char *)
    MGET("Couldn't allocate full colormap") ;
  mstr[(int) M_NOSIMAGE] = (char *)
    MGET("No image to save.") ;
  mstr[(int) M_NOVIMAGE] = (char *)
    MGET("No image to view.") ;
  mstr[(int) M_NOPIMAGE] = (char *)
    MGET("No image to print.") ;
  mstr[(int) M_COMPRESS] = (char *)
    MGET("Compressing colormap") ;
  mstr[(int) M_ADDCOLS]  = (char *)
    MGET("Adding new colors to colormap") ;
  mstr[(int) M_ADJUST]   = (char *)
    MGET("Creating adjusted image") ;
  mstr[(int) M_DITHER24] = (char *)
    MGET("Dithering image (24bit to 8bit)...") ;
  mstr[(int) M_DITHER8]  = (char *)
    MGET("Dithering image (8bit to 1bit)...") ;
  mstr[(int) M_EXPAND24] = (char *)
    MGET("Expanding image (8bit to 24bit)...") ;
  mstr[(int) M_EXPAND8]  = (char *)
    MGET("Expanding image (1bit to 8bit)...") ;
  mstr[(int) M_GRAY]     = (char *)
    MGET("Converting colormap to grayscale") ;
  mstr[(int) M_RASTCHK]  = (char *)
    MGET("Checking if this is a Sun rasterfile") ;
  mstr[(int) M_GIFCHK]   = (char *)
    MGET("Checking if this is a GIF file") ;
  mstr[(int) M_UNRECOG]   = (char *)
    MGET("Unrecognized file type") ;
  mstr[(int) M_NOSNAP]   = (char *)
    MGET("No region to snap.") ;
  mstr[(int) M_DELAY]   = (char *)
    MGET("8 sec. delay required for Hide option.") ;
  mstr[(int) M_ISDIR]   = (char *)
    MGET("Cannot load directory.") ;
  mstr[(int) M_ISNOTREG]   = (char *)
    MGET("Not a regular file - cannot load.");
  mstr[(int) M_NOACCESS]   = (char *)
    MGET("File access denied.");
  mstr[(int) M_NOTOPEN] = (char *)
    MGET("Cannot open temp file!") ;
  mstr[(int) M_TFAIL] = (char *)
    MGET("Cannot write to temp file!") ;
  mstr[(int) M_LAUNCH] = (char *)
    MGET("Starting Image Tool...") ;
  mstr[(int) M_NOLAUNCH] = (char *)
    MGET("Couldn't connect to Image Tool");

/* Main and print panel strings, which have an associated panel item. */

  mpstrs[(int) PI_BELL]     = (char *)
    LGET("Beep During Countdown") ;
  mpstrs[(int) PI_DELAY]    = (char *)
    LGET("Snap Delay:") ;
  mpstrs[(int) PI_LSDIR]    = (char *)
    LGET("Directory:") ;
  mpstrs[(int) PI_LSFILE]   = (char *)
    LGET("File:") ;
  mpstrs[(int) PI_LSBUT]    = (char *)
    LGET("Save") ;
  mpstrs[(int) PI_HIDEWIN]  = (char *)
    LGET("Hide Window During Capture") ;
  mpstrs[(int) PI_STYPE]    = (char *)
    LGET("Snap Type:") ;
  mpstrs[(int) PI_DEST]     = (char *)
    LGET("Destination:") ;
  mpstrs[(int) PI_DBITS]    = (char *)
    LGET("Double Size:") ;
  mpstrs[(int) PI_HEIGHTT]  = (char *)
    LGET("inches") ;
  mpstrs[(int) PI_HEIGHTV]  = (char *)
    LGET("Height:") ;
  mpstrs[(int) PI_PDIR]     = (char *)
    LGET("Directory:") ;
  mpstrs[(int) PI_PFILE]    = (char *)
    LGET("File:") ;
  mpstrs[(int) PI_POSITION] = (char *)
    LGET("Position:") ;
  mpstrs[(int) PI_PORIENT]  = (char *)
    LGET("Orientation:") ;
  mpstrs[(int) PI_PNAME]    = (char *)
    LGET("Printer:") ;
  mpstrs[(int) PI_SCALE]    = (char *)
    LGET("Scale to:") ;
  mpstrs[(int) PI_WIDTHT]   = (char *)
    LGET("inches") ;
  mpstrs[(int) PI_WIDTHV]   = (char *)
    LGET("Width:") ;

/* Other panel strings. */

  Pstrs[(int) DELAY0]       = (char *) LGET("0") ;
  Pstrs[(int) DELAY2]       = (char *) LGET("2") ;
  Pstrs[(int) DELAY4]       = (char *) LGET("4") ;
  Pstrs[(int) DELAY8]       = (char *) LGET("8") ;
  Pstrs[(int) DELAY16]      = (char *) LGET("16") ;
  Pstrs[(int) TIMER]        = (char *) LGET("seconds") ;
  Pstrs[(int) SAVE_SNAP]    = (char *) LGET("Save...") ;
  Pstrs[(int) LOAD_SNAP]    = (char *) LGET("Load...") ;
  Pstrs[(int) PRINT_SNAP]   = (char *) LGET("Print") ;
  Pstrs[(int) ST_SCREEN]    = (char *) LGET("Screen") ;
  Pstrs[(int) ST_REGION]    = (char *) LGET("Region") ;
  Pstrs[(int) ST_WINDOW]    = (char *) LGET("Window") ;
  Pstrs[(int) DO_SNAP]      = (char *) LGET("Snap") ;
  Pstrs[(int) VIEW_SNAP]    = (char *) LGET("View...") ;
  Pstrs[(int) P_LABEL]      = (char *) LGET("Snapshot: Print Options") ;
  Pstrs[(int) L_LABEL]      = (char *) LGET("Snapshot: Load Options") ;
  Pstrs[(int) S_LABEL]      = (char *) LGET("Snapshot: Save Options") ;
  Pstrs[(int) DEST1]        = (char *) LGET("Printer") ;
  Pstrs[(int) DEST2]        = (char *) LGET("File") ;
/* STRING_EXTRACTION SUNW_DESKSET_SNAPSHOT_LABEL
 * "lp" stands for laser writer which is basically the printer.
 */  
  Pstrs[(int) DEF_PRINTER]  = (char *) LGET("lp") ;
  Pstrs[(int) ORIENTU]      = (char *) LGET("Upright") ;
  Pstrs[(int) ORIENTS]      = (char *) LGET("Sideways") ;
  Pstrs[(int) CENTER]       = (char *) LGET("Center") ;
  Pstrs[(int) SPECIFY]      = (char *) LGET("Specify") ;
  Pstrs[(int) DEF_LEFTP]    = (char *) LGET("0.25") ;
  Pstrs[(int) PLPOS]        = (char *) LGET("inches from left ") ;
  Pstrs[(int) DEF_BOTP]     = (char *) LGET("0.25") ;
  Pstrs[(int) PBPOS]        = (char *) LGET("inches from bottom") ;
  Pstrs[(int) SCALE1]       = (char *) LGET("Actual Size") ;
  Pstrs[(int) SCALE2]       = (char *) LGET("Width") ;
  Pstrs[(int) SCALE3]       = (char *) LGET("Height") ;
  Pstrs[(int) SCALE4]       = (char *) LGET("Both") ;
  Pstrs[(int) DEF_WIDTH]    = (char *) LGET("8") ;
  Pstrs[(int) DEF_HEIGHT]   = (char *) LGET("10") ;
  Pstrs[(int) DBITS1]       = (char *) LGET("no") ;
  Pstrs[(int) DBITS2]       = (char *) LGET("yes") ;
  Pstrs[(int) MONO]         = (char *) LGET("Monochrome Printer") ;
  Pstrs[(int) PBUTTON]      = (char *) LGET("Print") ;

/* Help messages. */

  hstr[(int) H_MENUOPT]     = DGET("snapshot:PrintOpt") ;
  hstr[(int) H_LSDIR]       = DGET("snapshot:LoadSaveDirectory") ;
  hstr[(int) H_LSFILE]      = DGET("snapshot:LoadSaveFile") ;
  hstr[(int) H_DELAY]       = DGET("snapshot:SelfTimer") ;
  hstr[(int) H_BELL]        = DGET("snapshot:TimerBell") ;
  hstr[(int) H_HIDEWIN]     = DGET("snapshot:Hide") ;
  hstr[(int) H_SCREEN]      = DGET("snapshot:SScreen") ;
  hstr[(int) H_REGION]      = DGET("snapshot:SRegion") ;
  hstr[(int) H_WINDOW]      = DGET("snapshot:SWindow") ;
  hstr[(int) H_SOURCE]      = DGET("snapshot:SourceDrag") ;
  hstr[(int) H_STYPE]       = DGET("snapshot:SnapType") ;
  hstr[(int) H_SAVE]        = DGET("snapshot:Save") ;
  hstr[(int) H_LOAD]        = DGET("snapshot:Load") ;
  hstr[(int) H_PRINT]       = DGET("snapshot:Print") ;
  hstr[(int) H_SNAP]        = DGET("snapshot:Snap") ;
  hstr[(int) H_VIEW]        = DGET("snapshot:View") ;
  hstr[(int) H_DEST]        = DGET("snapshot:Destination") ;
  hstr[(int) H_PNAME]       = DGET("snapshot:Printer") ;
  hstr[(int) H_PFILE]       = DGET("snapshot:PrintFile") ;
  hstr[(int) H_ORIENT]      = DGET("snapshot:Orientation") ;
  hstr[(int) H_POS]         = DGET("snapshot:Position") ;
  hstr[(int) H_APPEAR]      = DGET("snapshot:Appearance") ;
  hstr[(int) H_DOUBLE]      = DGET("snapshot:Double") ;
  hstr[(int) H_MONO]        = DGET("snapshot:Mono") ;
  hstr[(int) H_PBUTTON]     = DGET("snapshot:PrintBut") ;
  hstr[(int) H_LSBUTTON]    = DGET("snapshot:LoadSaveBut") ;

/* Other strings. */

  ostr[(int) O_CONTINUE]    = (char *) LGET("Continue") ;
  ostr[(int) O_CANCEL]      = (char *) LGET("Cancel") ;
  ostr[(int) O_POPT]        = (char *) LGET("Print Options") ;
  ostr[(int) O_PMITEM]      = (char *) LGET("Print Snap") ;
  ostr[(int) O_OMITEM]      = (char *) LGET("Options...") ;
  ostr[(int) O_VIEW]        = (char *) LGET("Snapshot: View") ;
  ostr[(int) O_CMSNAME]     = (char *) LGET("new_colors") ;

  ostr[(int) O_FOVERWRITE]  = (char *)
    MGET("File exists, Overwrite?") ;
  ostr[(int) O_LOVERWRITE]  = (char *)
    MGET("Previous image hasn't been saved") ;
  ostr[(int) O_SOVERWRITE]  = (char *)
    MGET("Snapshot hasn't been saved") ;

  ostr[(int) O_YES]         = (char *) MGET("Yes") ;
  ostr[(int) O_NO]          = (char *) MGET("No") ;

  ostr[(int) O_ALERT1] = (char *)
    MGET("Couldn't put up alert, check your console.") ;
  ostr[(int) O_ALERT2] = (char *)
    MGET("Couldn't put up alert to request confirmation") ;
  ostr[(int) O_ALERT3] = (char *)
    MGET(" for overwriting the snap file.\n") ;
  ostr[(int) O_ALERT4] = (char *)
    MGET("Proceeding by assuming that this operation ") ;
  ostr[(int) O_ALERT5] = (char *)
    MGET("was NOT confirmed\n") ;

  ostr[(int) O_FONT]    = (char *)
    MGET("%s: can't open default font\n") ;
  ostr[(int) O_ONEFILE] = (char *)
    MGET("Snapshot may only accept one file at a time.") ;
  ostr[(int) O_NOSRC]   = (char *)
    MGET("There is no image to drag.") ;
  ostr[(int) O_OERR]    = (char *)
    MGET("Error opening image file") ;
  ostr[(int) O_NORAST]  = (char *)
    MGET("Not Sun Rasterfile.\n") ;
/* STRING_EXTRACTION SUNW_DESKSET_SNAPSHOT_MSG
 * This is the full path name of the file.  /tmp is the directory.
 */
  ostr[(int) O_RNDNAME] = (char *)
    MGET("/tmp/SNAP%s") ;
/* STRING_EXTRACTION SUNW_DESKSET_SNAPSHOT_MSG
 * This is the full path to the program "rasfilter8to1".
 */
  ostr[(int) O_RAST8T1] = (char *)
    MGET("/bin/rasfilter8to1 %s %s") ;
  ostr[(int) O_REDUCE]  = (char *)
    MGET("Could not reduce to 1 bit deep.\n") ;
  ostr[(int) O_DEPTH]   = (char *)
    MGET("Depth too great for display.\n") ;
  ostr[(int) O_CMAPERR] = (char *)
    MGET("Error loading colormap.\n") ;
  ostr[(int) O_LERR]    = (char *)
    MGET("Error loading image file.\n") ;
  ostr[(int) O_NOWIN]   = (char *)
    MGET("Couldn't create window.") ;
  ostr[(int) O_FLABEL]  = (char *)
    LGET("snapshot") ;
  ostr[(int) O_NOAPP]   = (char *)
    MGET("Unable to create application\n") ;
  ostr[(int) O_CONOVER] = (char *)
    MGET("File already exists, confirm overwrite.") ;
  ostr[(int) O_TOOL]    = (char *)
    LGET("Snapshot") ;
/* STRING_EXTRACTION SUNW_DESKSET_SNAPSHOT_MSG
 * This is the full path to the script that does the printing.
 */
  ostr[(int) O_PDEST]   = (char *)
    MGET("%s/bin/rash ") ;

#ifdef SVR4
  ostr[(int) O_PRINT] = (char *)
    MGET("lp -Tpostscript");
#else /* SVR4 */
  ostr[(int) O_PRINT] = (char *)
    MGET("lpr ");
#endif /* SVR4 */

  ostr[(int) O_NOCDIR]  = (char *)
    MGET("Unable to change directory to: %s.") ;
  ostr[(int) O_SETNAME] = (char *)
    MGET("%s %s");
  ostr[(int) O_SETNAMEFILE] = (char *)
    MGET("%s %s - %s");
  ostr[(int) O_BADIDEP] = (char *)
    MGET("Cannot handle image depth of %d") ;
  ostr[(int) O_BADSDEP] = (char *)
    MGET("Cannot handle screen depth of %d") ;
/* STRING_EXTRACTION SUNW_DESKSET_SNAPSHOT_LABEL
 * This is the default file name used to store the image.
 */
  ostr[(int) O_DEFF]    = (char *) LGET("snapshot.rs") ;
  ostr[(int) O_LOAD]    = (char *) LGET("Load") ;
  ostr[(int) O_SAVE]    = (char *) LGET("Save") ;

  ostr[(int) O_LDISCARD] = (char *)
    MGET("Discard loaded image") ;
  ostr[(int) O_LCANCEL]  = (char *)
    MGET("Cancel load operation") ;
  ostr[(int) O_SDISCARD] = (char *)
    MGET("Discard snapshot") ;
  ostr[(int) O_SCANCEL]  = (char *)
    MGET("Cancel snap operation") ;
  ostr[(int) O_DCANCEL] = (char *)
    MGET("Cancel drag and drop operation") ;
  ostr[(int) O_QLDISCARD] = (char *)
    MGET("Discard loaded image and quit");
  ostr[(int) O_QSDISCARD] = (char *)
    MGET("Discard snapshot and quit");
  ostr[(int) O_QCANCEL] = (char *)
    MGET("Cancel quit operation");

  ostr[(int) O_VERSION] = (char *)
    MGET("%s: version 3.0.%1d\n\n") ;
  ostr[(int) O_USAGE1]  = (char *)
    MGET("Usage: %s [ -d def-directory ] [ -f def-filename ]\n") ;
  ostr[(int) O_USAGE2]  = (char *)
    MGET("\t[ -g ] [ -l filename ] [ -n ] [tool args]\n") ;
}
