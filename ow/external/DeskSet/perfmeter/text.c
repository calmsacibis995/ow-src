#ifndef lint
static  char sccsid[] = "@(#)text.c 1.7 93/03/29 Copyright (c) Sun Microsystems Inc." ;
#endif

/*  The DeskSet perfmeter - text initialisation.
 *
 *  Copyright (c) 1988-1990  Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in posesion of this copy.
 *
 *  RESTRICTED RIGHTS LEGEND:  Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include "perfmeter.h"

extern struct meter meters_init[] ;

/*  The following are all the static strings used by the perfmeter program.
 *  They are initialised in init_text() to the local language equivalents.
 */

char *cmdstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *hstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *mess[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL
} ;

char *nstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *perf_res[] = {
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

char *pmenu_help[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *sstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *ustrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *vstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;


void
init_cmdline_opts()      /* Initialise command line option strings. */
{
  cmdstr[(int) O_ALL]      = "-a" ;
  cmdstr[(int) O_TYPE]     = "-t" ;
  cmdstr[(int) O_DIAL]     = "-d" ;
  cmdstr[(int) O_GRAPH]    = "-g" ;
  cmdstr[(int) O_SOLID]    = "-S" ;
  cmdstr[(int) O_LINED]    = "-L" ;
  cmdstr[(int) O_HORIZ]    = "-H" ;
  cmdstr[(int) O_VERT]     = "-V" ;
  cmdstr[(int) O_HASLABEL] = "+Wn" ;
  cmdstr[(int) O_NOLABEL]  = "-Wn" ;
  cmdstr[(int) O_MAXS]     = "-M" ;
  cmdstr[(int) O_SAMPLE]   = "-s" ;
  cmdstr[(int) O_HRINTV]   = "-h" ;
  cmdstr[(int) O_MININTV]  = "-m" ;
  cmdstr[(int) O_LIM]      = "-C" ;
  cmdstr[(int) O_LOG]      = "-l" ;
  cmdstr[(int) O_LOGNAME]  = "-n" ;
  cmdstr[(int) O_PAGELEN]  = "-p" ;
  cmdstr[(int) O_NAME]     = "-name" ;
}


void
init_text()     /* Setup text strings depending upon language. */
{
  int i ;

/* Spot help messages. */

  hstr[(int) H_MCANVAS]    = DGET("perfmeter:MeterCanvas") ;
  hstr[(int) H_MFRAME]     = DGET("perfmeter:MeterFrame") ;
  hstr[(int) H_PROPS]      = DGET("perfmeter:BringProps") ;
  hstr[(int) H_PFRAME]     = DGET("perfmeter:PropsFrame") ;
  hstr[(int) H_VCHOICE]    = DGET("perfmeter:ViewChoice") ;
  hstr[(int) H_DIRCHOICE]  = DGET("perfmeter:DirectionChoice") ;
  hstr[(int) H_DISPCHOICE] = DGET("perfmeter:DisplayChoice") ;
  hstr[(int) H_GCHOICE]    = DGET("perfmeter:GraphChoice") ;
  hstr[(int) H_RCHOICE]    = DGET("perfmeter:RemoteChoice") ;
  hstr[(int) H_MNAME]      = DGET("perfmeter:MachineName") ;
  hstr[(int) H_SLEN]       = DGET("perfmeter:SampleLength") ;
  hstr[(int) H_HRHAND]     = DGET("perfmeter:HourHand") ;
  hstr[(int) H_DOLOG]      = DGET("perfmeter:SaveSamples") ;
  hstr[(int) H_MINHAND]    = DGET("perfmeter:MinuteHand") ;
  hstr[(int) H_LNAME]      = DGET("perfmeter:SampleName") ;
  hstr[(int) H_APPLY]      = DGET("perfmeter:ApplyButton") ;
  hstr[(int) H_RESET]      = DGET("perfmeter:ResetButton") ;
  hstr[(int) H_DEFS]       = DGET("perfmeter:DefaultButton") ;

/* Messages. */

/*  STRING_EXTRACTION perfmeter.message :
 *  "rstatd" is a unix command.  It's a kernel statistics server.
 */

  mess[(int) M_RSTAT] = MGET("perfmeter: rstatd on %s returned bad data\n") ;
  mess[(int) M_FONT]  = LGET("perfmeter: couldn't get the default font.") ;
  mess[(int) M_FRAME] = MGET("%s: can't create main window\n") ;
  mess[(int) M_CACHE] = MGET("perfmeter: cache size option no longer valid.\n") ;
  mess[(int) M_ICON]  = MGET("perfmeter: cannot read icon filename (%s)\n") ;

/* Label for perfmeter meters/graphs. */

/*  STRING_EXTRACTION perfmeter.label :
 *  The following 10 abbreviations stand for different system parameters.
 *  Namely: Central Processing Unit, Packets, Pages, Swap, Interrupts, Disk
 *  usage, Context switching, Load, Collisions and Errors.
 */

  meters_init[(int) CPU].m_name   = LGET("cpu") ;
  meters_init[(int) PKTS].m_name  = LGET("pkts") ;
  meters_init[(int) PAGE].m_name  = LGET("page") ;
  meters_init[(int) SWAP].m_name  = LGET("swap") ;
  meters_init[(int) INTR].m_name  = LGET("intr") ;
  meters_init[(int) DISK].m_name  = LGET("disk") ;
  meters_init[(int) CNTXT].m_name = LGET("cntxt") ;
  meters_init[(int) LOAD].m_name  = LGET("load") ;
  meters_init[(int) COLL].m_name  = LGET("colls") ;
  meters_init[(int) ERR].m_name   = LGET("errs") ;

/*  Names for each meter/graph. Used for command line comparisions.
 *  Not localised.
 */

  meters_init[(int) CPU].m_rname   = "cpu" ;
  meters_init[(int) PKTS].m_rname  = "pkts" ;
  meters_init[(int) PAGE].m_rname  = "page" ;
  meters_init[(int) SWAP].m_rname  = "swap" ;
  meters_init[(int) INTR].m_rname  = "intr" ;
  meters_init[(int) DISK].m_rname  = "disk" ;
  meters_init[(int) CNTXT].m_rname = "cntxt" ;
  meters_init[(int) LOAD].m_rname  = "load" ;
  meters_init[(int) COLL].m_rname  = "colls" ;
  meters_init[(int) ERR].m_rname   = "errs" ;

/* Notice messages. */

  nstrs[(int) N_NOTFOUND] =
    LGET("The host you specified could not be\nfound.  The perfmeter will be reset\nto your local host") ;
  nstrs[(int) N_BADLOG] =
    LGET("The logging filename could not be\nopened. Logging of samples has been\nturned off.") ;
  nstrs[(int) N_CONT]      = LGET("Continue") ;

/* Perfmeter resource strings. */

  perf_res[(int) R_DGRAPH]   = DGET("displayGraph") ;
  perf_res[(int) R_HRINT]    = DGET("hourInterval") ;
  perf_res[(int) R_MININT]   = DGET("minInterval") ;
  perf_res[(int) R_RESIZE]   = DGET("resizeGraphView") ;
  perf_res[(int) R_STIME]    = DGET("sampleTime") ;
  perf_res[(int) R_MON]      = DGET("monitor") ;
  perf_res[(int) R_MACHINE]  = DGET("machine") ;
  perf_res[(int) R_DVERT]    = DGET("displayVertical") ;
  perf_res[(int) R_GSOLID]   = DGET("graphSolid") ;
  perf_res[(int) R_CPUMAX]   = DGET("cpuMaxValues") ;
  perf_res[(int) R_PKTSMAX]  = DGET("pktsMaxValues") ;
  perf_res[(int) R_PAGEMAX]  = DGET("pageMaxValues") ;
  perf_res[(int) R_SWAPMAX]  = DGET("swapMaxValues") ;
  perf_res[(int) R_INTRMAX]  = DGET("intrMaxValues") ;
  perf_res[(int) R_DISKMAX]  = DGET("diskMaxValues") ;
  perf_res[(int) R_CNTXTMAX] = DGET("cntxtMaxValues") ;
  perf_res[(int) R_LOADMAX]  = DGET("loadMaxValues") ;
  perf_res[(int) R_COLLMAX]  = DGET("collMaxValues") ;
  perf_res[(int) R_ERRMAX]   = DGET("errMaxValues") ;
  perf_res[(int) R_LOCAL]    = DGET("showLocal") ;
  perf_res[(int) R_TITLE]    = DGET("hasTitle") ;
  perf_res[(int) R_LOG]      = DGET("saveSamples") ;
  perf_res[(int) R_LOGNAME]  = DGET("sampleFilename") ;
  perf_res[(int) R_PAGELEN]  = DGET("samplePageLength") ;
  perf_res[(int) R_CPUCOL]   = DGET("cpuColor") ;
  perf_res[(int) R_PKTSCOL]  = DGET("pktsColor") ;
  perf_res[(int) R_PAGECOL]  = DGET("pageColor") ;
  perf_res[(int) R_SWAPCOL]  = DGET("swapColor") ;
  perf_res[(int) R_INTRCOL]  = DGET("intrColor") ;
  perf_res[(int) R_DISKCOL]  = DGET("diskColor") ;
  perf_res[(int) R_CNTXTCOL] = DGET("cntxtColor") ;
  perf_res[(int) R_LOADCOL]  = DGET("loadColor") ;
  perf_res[(int) R_COLLCOL]  = DGET("collColor") ;
  perf_res[(int) R_ERRCOL]   = DGET("errColor") ;
  perf_res[(int) R_CPULIM]   = DGET("cpuCeiling") ;
  perf_res[(int) R_PKTSLIM]  = DGET("pktsCeiling") ;
  perf_res[(int) R_PAGELIM]  = DGET("pageCeiling") ;
  perf_res[(int) R_SWAPLIM]  = DGET("swapCeiling") ;
  perf_res[(int) R_INTRLIM]  = DGET("intrCeiling") ;
  perf_res[(int) R_DISKLIM]  = DGET("diskCeiling") ;
  perf_res[(int) R_CNTXTLIM] = DGET("cntxtCeiling") ;
  perf_res[(int) R_LOADLIM]  = DGET("loadCeiling") ;
  perf_res[(int) R_COLLLIM]  = DGET("collCeiling") ;
  perf_res[(int) R_ERRLIM]   = DGET("errCeiling") ;
  perf_res[(int) R_CEILCOL]  = DGET("ceilingColor") ;
  perf_res[(int) R_LFONT]    = DGET("labelFont") ;
  perf_res[(int) R_CSTYLE]   = DGET("ceilingStyleSolid") ;
  perf_res[(int) R_OBSCURE]  = DGET("collectWhenObscured") ;

/* Frame menu help values. */

  pmenu_help[(int) CPU]   = DGET("perfmeter:SetCPUView") ;
  pmenu_help[(int) PKTS]  = DGET("perfmeter:SetPacketView") ;
  pmenu_help[(int) PAGE]  = DGET("perfmeter:SetPageView") ;
  pmenu_help[(int) SWAP]  = DGET("perfmeter:SetSwapView") ;
  pmenu_help[(int) INTR]  = DGET("perfmeter:SetInterruptView") ;
  pmenu_help[(int) DISK]  = DGET("perfmeter:SetDiskView") ;
  pmenu_help[(int) CNTXT] = DGET("perfmeter:SetContextView") ;
  pmenu_help[(int) LOAD]  = DGET("perfmeter:SetLoadView") ;
  pmenu_help[(int) COLL]  = DGET("perfmeter:SetCollsView") ;
  pmenu_help[(int) ERR]   = DGET("perfmeter:SetErrsView") ;

/* Scales. */

  sstrs[(int) S_SMALL]      = LGET("small") ;
  sstrs[(int) S_MEDIUM]     = LGET("medium") ;
  sstrs[(int) S_LARGE]      = LGET("large") ;
  sstrs[(int) S_EXTRALARGE] = LGET("extra_large") ;

/* Usage message. */

  i = 0 ;

/*  STRING_EXTRACTION perfmeter.message :
 *  This is the version number of the tool.
 */

  ustrs[i++] = MGET("%s: version 3.2.%1d\n\n") ;

/*  STRING_EXTRACTION perfmeter.message :
 *  This shows users what command-line options are available.
 *  -h takes the number of seconds to set the hour-hand interval to.
 */

  ustrs[i++] = MGET("Usage: %s [-a] [-d] -[g] [-h hourhandintv]\n") ;
  ustrs[i++] = LGET("\t[-l] [-m minutehandintv] [-name app-name]\n") ;
  ustrs[i++] = LGET("\t[-n samplefile] [-p pagelength] [-s sampletime]\n") ;
  ustrs[i++] = LGET("\t[-t value] [-v] [-C value ceiling] [-M value curmax minmax maxmax]\n") ;
  ustrs[i++] = LGET("\t[-H] [-L] [-S] [-V] [-Wn] [+Wn] [-?] [hostname]\n\n") ;
  ustrs[i++] = LGET("\tvalue is one of:\n") ;
  ustrs[i++] = LGET("\nKeyboard accelerators:\n") ;
  ustrs[i++] = LGET("\td - toggle direction (horizontal/vertical).\n") ;
  ustrs[i++] = LGET("\tg - toggle graph type (dial/graph).\n") ;
  ustrs[i++] = LGET("\th - decrement hour hand interval by 1.\n") ;
  ustrs[i++] = LGET("\tH - increment hour hand interval by 1.\n") ;
  ustrs[i++] = LGET("\tm - decrement minute hand interval by 1.\n") ;
  ustrs[i++] = LGET("\tM - increment minute hand interval by 1.\n") ;
  ustrs[i++] = LGET("\tq - quit perfmeter.\n") ;
  ustrs[i++] = LGET("\ts - toggle graph style (solid/lined).\n") ;
  ustrs[i++] = LGET("\tS - toggle saving of samples to file.\n") ;
  ustrs[i++] = LGET("\tt - toggle monitoring mode (local/remote).\n") ;
  ustrs[i++] = LGET("\t1-9 - set sampletime to a range from 1-9 seconds.\n") ;

/* Various. */

  vstrs[(int) V_USAGE]   = DGET("\t\t%s\n") ;
  vstrs[(int) V_TRUE]    = DGET("true") ;
  vstrs[(int) V_FALSE]   = DGET("false") ;

/*  STRING_EXTRACTION perfmeter.label :
 *  Unix "localhost" variable.
 */

  vstrs[(int) V_LHOST]   = LGET("localhost") ;
  vstrs[(int) V_IADDR]   = DGET("127.0.0.1") ;
  vstrs[(int) V_SIGNAL]  = LGET("signal") ;

/*  STRING_EXTRACTION perfmeter.label :
 *  Unix process of "fork"ing another process.i.e starting another process.
 */

  vstrs[(int) V_FORK]    = LGET("fork") ;
  vstrs[(int) V_CLNTERR] = LGET("cpugetdata") ;
  vstrs[(int) V_PROPS]   = LGET("Properties...") ;

/*  STRING_EXTRACTION perfmeter.message :
 *  this is a font name from the font family screen, "r" for regular, then 
 *  comes the size of the font.
 */

  vstrs[(int) V_FNAME]   = MGET("screen.r.%1d") ;
  vstrs[(int) V_FLABEL]  = LGET("Perfmeter") ;
  vstrs[(int) V_ILABEL]  = LGET("perfmeter") ;
  vstrs[(int) V_MTITLE]  = LGET("Performance Meter") ;
  vstrs[(int) V_MALLOC]  = LGET("failed in malloc()\n") ;
}
