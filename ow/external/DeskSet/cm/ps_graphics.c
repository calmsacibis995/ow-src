#ifndef lint
static  char sccsid[] = "@(#)ps_graphics.c 3.28 94/09/01 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*  ps_graphics.c */

/*
 *      Copyright (c) 1987, 1988, 1989 Sun Microsystems, Inc.
 *      All Rights Reserved.
 *
 *      Sun considers its source code as an unpublished, proprietary
 *      trade secret, and it is available only under strict license
 *      provisions.  This copyright notice is placed here only to protect
 *      Sun in the event the source is deemed a published work.  Dissassembly,
 *      decompilation, or other means of reducing the object code to human
 *      readable form is prohibited by the license agreement under which
 *      this code is provided to the user or company in possession of this
 *      copy.
 *
 *      RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *      Government is subject to restrictions as set forth in subparagraph
 *      (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *      clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *      NASA FAR Supplement.
 */

#include <stdio.h>
#include <limits.h>
#include <rpc/rpc.h>
#ifdef SVR4
#include <unistd.h>
#endif "SVR4"
#include <sys/stat.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/icon_load.h>
#include <xview/openmenu.h>
#include <xview/font.h>
#include "util.h"
#include "appt.h"
#include "table.h"
#include "graphics.h"
#include "calendar.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "ps_graphics.h"
#include "todo.h"
#include "cm_i18n.h"
#include "gettext.h"

#define BUFFERSIZE 512

int num_hours = 0;
int prolog_found = 0;
extern int debug;
static char ps_file[128];		/* name of output postscript file */
static char sched_bucket[96];		/* use to draw sched blocks */
static char timestamp[BUFFERSIZE];		/* use to print timestamp on printouts */
static void ps_week_sched_update();
static int nonascii_char = 0;
static int already_printed_warning = 0;
static void nonascii_string();
static int pages_printed = 0;

extern void print_std_month();
extern void get_time_str();
/* forward declaration */
char *fix_ps_string();

extern FILE *
ps_open_file ()
{
	FILE *fp;
	struct stat fstatus;
	Props *p = (Props *) calendar->properties;
	Tick t;
	struct tm *tm;

	if ( p->dest_choiceVAL == 1) 
	{
		/* redirect output to a file */
		char *dir  = p->dir_nameVAL;
		char *file = p->file_nameVAL;

		if (*dir == NULL || *file == NULL) {
			fprintf(stderr,
			EGET("ps_open_file(): need to specify dir or file\n"));
			return(NULL);
		}

		stat(dir, &fstatus);
		if ((fstatus.st_mode & S_IFMT) != S_IFDIR) {
			fprintf(stderr,
			EGET("ps_open_file(): Invalid Directory %s\n"),dir);
			return(NULL);
		}

		sprintf(ps_file,"%s/%s",dir,file);
	}
	else  {
		/* get a temporary filename to use */
		(void) cm_strcpy(ps_file, "/tmp/CMpXXXXXX");
		(void) mktemp(ps_file);
	}
		
	if ((fp = fopen(ps_file, "w+")) == NULL) {
		fprintf(stderr,
		EGET("ps_open_file(): unable to open file %s\n"), ps_file);
		return(NULL);
	}

	/* set up time stamp */
	/* STRING_EXTRACTION SUNW_DESKSET_CM_MSG
	 * The following string is the date/time format used in printing out
	 * calendar views.  This is the locale's date and time.  If this string
	 * is to be translated, please refer to the man pages for strftime()
	 * for various format strings.
	 */
	t = now();
	tm = localtime(&t);
	strftime(timestamp, BUFFERSIZE, MGET("%x %I:%M %p"), tm);

	nonascii_string(timestamp);

	return(fp);
}


extern void
ps_next_page(fp, page)
	FILE *fp;
        int  page;		/* page number */
{    
      fprintf (fp, "%%%%Page: %d %d\n", page, page); 
                                /* just start next page */
      pages_printed++;
}

extern void
ps_init_printer (fp, portrait)
	FILE *fp;
	short  portrait;		/* portrait or landscape mode? */
{    
	char prolog_file[MAXPATHLEN];
	int num_read = 0;
	char prolog_buf[BUFFERSIZE];
#ifdef OW_I18N
	char *locale;
#endif
	FILE *prolog_fp;
	double slm, srm, stm, sbm, tmp;
	int lm, rm, tm, bm;
	extern double atof();
	Props *p = (Props *) calendar->properties;

	/* setup margins for programs coordinate space */
	lm = INCH;
	rm = PRINTER_WIDTH-INCH;
	bm = INCH;
	tm = PRINTER_HEIGHT-INCH;
	/* 
	 * Get width & height settings from the printer props sheet.
	 * These will be used to scale the output to fit in the 
	 * area described in the printer props sheet.
	 */
	slm = (double)72.0 * (double)atof(p->xoffsetVAL);
	srm = (double)72.0 * (double)atof(p->widthVAL);
	sbm = (double)72.0 * (double)atof(p->yoffsetVAL);
	stm = (double)72.0 * (double)atof(p->heightVAL);

	if (!portrait) {
		rm = PRINTER_HEIGHT-INCH;	/* landscape mode */
		tm = PRINTER_WIDTH-INCH;
		tmp = srm;		/* exchange right & top margin */
		srm = stm;
		stm = tmp;
	}

	/* Use cm_printf to do numeric formatting to avoid locale conversion.
	 * This is to avoid printing (e.g.) 72,00 instead of 72.00 in the
	 * postscript file.
	 */

	fprintf (fp, "%%!PS-Adobe-2.0 EPSF-2.0\n"); 
	fprintf (fp, "%%%%Title: %s\n", ps_file); 
	fprintf (fp, "%%%%Creator: Mike Bundschuh & Nannette Simpson\n"); 
	fprintf (fp, "%%I18N printing: Wendy Mui\n");
	/* This page count may be wrong.  -welch */
	fprintf (fp, "%%%%Pages: (atend)\n");
	fprintf (fp, "%%%%BoundingBox ");
	fputs ((char *)(cm_printf(slm, 2)), fp); fputs (" ", fp);
	fputs ((char *)(cm_printf(sbm, 2)), fp); fputs (" ", fp);
	fputs ((char *)(cm_printf(srm, 2)), fp); fputs (" ", fp);
	fputs ((char *)(cm_printf(stm, 2)), fp);
	fprintf (fp, "\n%%%%EndComments\n"); 
	fprintf (fp, "%%\n");
	
/*  Add locale specific prolog file 
 *  Prolog file = $OPENWINHOME/lib/locale/<locale>/print/prolog.ps
 */
#ifdef OW_I18N
	locale = (char *)xv_get(calendar->frame, XV_LC_BASIC_LOCALE);
	if ( strcmp("C", locale) ) {
		ds_expand_pathname("$OPENWINHOME/lib/locale/", prolog_file);
		cm_strcat(prolog_file, locale);
		cm_strcat(prolog_file, "/print/prolog.ps");
		if ( (prolog_fp = fopen(prolog_file, "r")) == NULL ) {
			prolog_found = 0;
#ifdef DEBUG
			fprintf(stderr, "ps_init_printer(): cannot open postscript prolog file: %s\n", prolog_file);
#endif
			/* Need to define LC_<fontname> because prolog.ps is missing */
            fprintf (fp, "\n%% I18NL4 prolog.ps file not available\n");
            fprintf (fp, "/LC_Times-Bold\n");
            fprintf (fp, "    /Times-Bold findfont def\n");
            fprintf (fp, "/LC_Helvetica\n");
            fprintf (fp, "    /Helvetica findfont def\n");
            fprintf (fp, "/LC_Courier\n");
            fprintf (fp, "  /Courier findfont def\n");
            fprintf (fp, "/LC_Helvetica-BoldOblique\n");
            fprintf (fp, "  /Helvetica-BoldOblique findfont def\n");
            fprintf (fp, "/LC_Helvetica-Bold\n");
            fprintf (fp, "  /Helvetica-Bold findfont def\n");
            fprintf (fp, "/LC_Times-Italic\n");
            fprintf (fp, "  /Times-Italic findfont def\n");
            fprintf (fp, "/LC_Times-Roman\n");
            fprintf (fp, "  /Times-Roman findfont def\n\n");
		} else {
			prolog_found = 1;
		}
		if ( prolog_found ) {
			while ( (num_read = fread(prolog_buf, 1, BUFFERSIZE, prolog_fp)) ) {
				fwrite(prolog_buf, 1, num_read, fp);
			}
			fclose(prolog_fp);
		}
	}
#endif

#ifdef DEBUG
	fprintf (fp, "\n%% --- show rulers (for debugging) ---\n");
	fprintf (fp, "/showrulers { \n");
	fprintf (fp, "    /Times-Roman findfont 20 scalefont setfont\n");
	fprintf (fp, "    /pwid 8.5 72 mul def\n");
	fprintf (fp, "    /phgt 11  72 mul def\n");
	fprintf (fp, "    /gap  18 def\n");
	fprintf (fp, "    /xreps pwid gap div def\n");
	fprintf (fp, "    /yreps phgt gap div def\n");
	fprintf (fp, "    /linelen 20 def\n");
	fprintf (fp, "    /tmpstr 8 string def\n\n");

	fprintf (fp, "    0 0 moveto\n");
	fprintf (fp, "    /offset 0 def\n");
	fprintf (fp, "    xreps {\n");
	fprintf (fp, "        0 linelen rlineto gsave stroke grestore\n");
	fprintf (fp, "        offset 72 mod 0 eq\n");
	fprintf (fp, "        { offset 72 idiv tmpstr cvs gsave show grestore } if\n");
	fprintf (fp, "        gap linelen neg rmoveto\n");
	fprintf (fp, "        /offset gap offset add def\n");
	fprintf (fp, "    } repeat\n\n");

	fprintf (fp, "    0 0 moveto\n");
	fprintf (fp, "    /offset 0 def\n");
	fprintf (fp, "    yreps {\n");
	fprintf (fp, "        linelen 0 rlineto gsave stroke grestore\n");
	fprintf (fp, "        offset 72 mod 0 eq\n");
	fprintf (fp, "        { offset 72 idiv tmpstr cvs gsave show grestore } if\n");
	fprintf (fp, "        linelen neg gap rmoveto\n");
	fprintf (fp, "        /offset gap offset add def\n");
	fprintf (fp, "    } repeat\n");
	fprintf (fp, "} def\n");
	fprintf (fp, "\nshowrulers\t\t\t%% FOR DEBUGGING\n");
#endif

	if (!portrait)
		ps_landscape_mode(fp);
	else
	fprintf (fp, "\ngsave\t\t%% save initial graphics state\n");

	fprintf (fp, "\n%% --- fit into bounding box ---\n");

    fputs ((char *)(cm_printf(slm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)lm, 0)), fp);  fputs (" ", fp);
    fputs ((char *)(cm_printf(srm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)rm, 0)), fp);
    fputs (" div mul sub ", fp);
    fputs ((char *)(cm_printf(sbm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)bm, 0)), fp);  fputs (" ", fp);
    fputs ((char *)(cm_printf(stm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)tm, 0)), fp);
    fputs (" div mul sub translate\n", fp);

/*	fprintf (fp, "%.2f %d sub %.2f %d sub translate\n", slm, lm, sbm, bm);  */
/*	fprintf (fp, "%.2f %.2f translate\n", slm, sbm); */

#ifdef NEVER
fprintf (stderr, "slm=%.2f lm=%d sub sbm=%.2f bm=%d sub translate\n", slm, lm, sbm, bm);
#endif

	if (portrait)
		tm += (0.5*INCH);
	else
		rm += (0.5*INCH);

/*	fprintf (fp, "%.2f %.2f sub %d div %.2f %.2f sub %d div scale\n", 
		 srm, slm, rm, stm, sbm, tm); */

    fputs ((char *)(cm_printf(srm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)rm, 0)), fp);
    fputs (" div ", fp);
    fputs ((char *)(cm_printf(stm, 2)), fp); fputs (" ", fp);
    fputs ((char *)(cm_printf((double)tm, 0)), fp);
    fputs (" div scale\n", fp);

#ifdef NEVER
fprintf (stderr, "srm=%.2f slm=%.2f sub rm=%d div stm=%.2f sbm=%.2f sub tm=%d div scale\n", 
	 srm, slm, rm, stm, sbm, tm);
#endif

	fprintf (fp, "\n%% --- init vars, fonts, procedures, etc. ---\n");

	fprintf (fp, "/WHITE   1.0  def\n");		/* set print tones */
	fprintf (fp, "/GRAY    0.5  def\n");
	fprintf (fp, "/LT_GRAY 0.95 def\n");
	fprintf (fp, "/BLACK   0.0  def\n");

	fprintf (fp, "/TM %d def\n", tm);		/* set margins */
	fprintf (fp, "/BM %d def\n", bm);
	fprintf (fp, "/LM %d def\n", lm);
	fprintf (fp, "/RM %d def\n", rm);

	fprintf (fp, "/line_width 1 def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");

	if (prolog_found) {
		fprintf (fp, "%%\n");
		fprintf (fp, "/MonthFont\n");
		fprintf (fp, "    /LC_Times-Bold findfont def\n");
		fprintf (fp, "/DayFont\n");
		fprintf (fp, "    /LC_Times-Bold findfont def\n");
		fprintf (fp, "/DateFont\n");
		fprintf (fp, "    /LC_Helvetica findfont def\n");
		fprintf (fp, "/TimeFont\n");
		fprintf (fp, "    /LC_Helvetica-Bold findfont def\n");
		fprintf (fp, "/ApptFont\n");
		fprintf (fp, "    /LC_Helvetica findfont def\n");
		fprintf (fp, "/StampFont\n");
		fprintf (fp, "    /LC_Times-Bold findfont def\n");
		fprintf (fp, "%%\n");
	}
else {
		fprintf (fp, "%%\n");
		fprintf (fp, "/ISOLatin1_header {\n");
		fprintf (fp, "%% Use ISOLatin1Encoding\n");
		fprintf (fp, "/Helvetica findfont\n");
		fprintf (fp, "\tdup length dict begin\n");
		fprintf (fp, "\t{1 index /FID ne {def} {pop pop} ifelse} forall\n");
		fprintf (fp, "\t/Encoding ISOLatin1Encoding def\n");
		fprintf (fp, "\tcurrentdict\n");
		fprintf (fp, "\tend\n");
		fprintf (fp, "/Helvetica-ISOLatin1 exch definefont pop\n");
		fprintf (fp, "%%\n");
		fprintf (fp, "/Times-Bold findfont\n");
		fprintf (fp, "\tdup length dict begin\n");
		fprintf (fp, "\t{1 index /FID ne {def} {pop pop} ifelse} forall\n");
		fprintf (fp, "\t/Encoding ISOLatin1Encoding def\n");
		fprintf (fp, "\tcurrentdict\n");
		fprintf (fp, "\tend\n");
		fprintf (fp, "/Times-Bold-ISOLatin1 exch definefont pop\n");
		fprintf (fp, "%%\n");

		fprintf (fp, "/MonthFont\n");
		fprintf (fp, "    /Times-Bold-ISOLatin1 findfont def\n");
		fprintf (fp, "/DayFont\n");
		fprintf (fp, "    /Times-Bold-ISOLatin1 findfont def\n");
		fprintf (fp, "/DateFont\n");
		fprintf (fp, "    /Helvetica-ISOLatin1 findfont def\n");
		fprintf (fp, "/TimeFont\n");
		fprintf (fp, "    /Times-Bold-ISOLatin1 findfont def\n");
		fprintf (fp, "/ApptFont\n");
		fprintf (fp, "    /Helvetica-ISOLatin1 findfont def\n");
		fprintf (fp, "/StampFont\n");
		fprintf (fp, "    /Times-Bold-ISOLatin1 findfont def\n");
		fprintf (fp, "} def    %% ISOLatin1_header\n");

		fprintf (fp, "%%\n");
		fprintf (fp, "/regular_header {\n");
		fprintf (fp, "/MonthFont\n");
		fprintf (fp, "    /Times-Bold findfont def\n");
		fprintf (fp, "/DayFont\n");
		fprintf (fp, "    /Times-Bold findfont def\n");
		fprintf (fp, "/DateFont\n");
		fprintf (fp, "    /Helvetica findfont def\n");
		fprintf (fp, "/TimeFont\n");
		fprintf (fp, "    /Times-Bold findfont def\n");
		fprintf (fp, "/ApptFont\n");
		fprintf (fp, "    /Helvetica findfont def\n");
		fprintf (fp, "/StampFont\n");
		fprintf (fp, "    /Times-Bold findfont def\n");
		fprintf (fp, "} def    %% regular_header\n");
		fprintf (fp, "%%\n");
		fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
		fprintf (fp, "{ISOLatin1_header} {regular_header} ifelse\n");
		fprintf (fp, "%%\n");
}
	fprintf (fp, "/MonthFont.scale 16 def\n");
	fprintf (fp, "/DayFont.scale   12 def\n");
	fprintf (fp, "/DateFont.scale  12 def\n");
	fprintf (fp, "/TimeFont.scale  12 def\n");
	fprintf (fp, "/ApptFont.scale  10 def\n");
	fprintf (fp, "/StampFont.scale  9 def\n");

	fprintf (fp, "\n%% --- inch macro ---\n");
	fprintf (fp, "/inch { %% x => -\n");
	fprintf (fp, "    72 mul\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- make a rect path ---\n");
	fprintf (fp, "/rect { %% x y w h => -\n");
	fprintf (fp, "    4 2 roll moveto\n");
	fprintf (fp, "    exch dup 3 1 roll\n");
	fprintf (fp, "    0 rlineto\n");
	fprintf (fp, "    0 exch rlineto\n");
	fprintf (fp, "    neg 0 rlineto\n");
	fprintf (fp, "    closepath\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- fill a square with black ---\n");
	fprintf (fp, "/make_black { %% x y w h => -\n");
	fprintf (fp, "    rect\n");
	fprintf (fp, "    fill \n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- fill a square with gray ---\n");
	fprintf (fp, "/make_gray { %% x y w h => -\n");
	fprintf (fp, "    GRAY setgray\n");
	fprintf (fp, "    rect\n");
	fprintf (fp, "    fill \n");
	fprintf (fp, "    BLACK setgray\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- fill a square with light gray ---\n");
	fprintf (fp, "/make_lt_gray { %% x y w h => -\n");
	fprintf (fp, "    LT_GRAY setgray\n");
	fprintf (fp, "    rect\n");
	fprintf (fp, "    fill \n");
	fprintf (fp, "    BLACK setgray\n");
	fprintf (fp, "} def\n");
	  
	fprintf (fp, "\n%% --- draw a box & set line width ---\n");
	fprintf (fp, "/draw_box { %% x y w h linew => - \n");
	fprintf (fp, "    setlinewidth\n");
	fprintf (fp, "    rect\n");
	fprintf (fp, "    stroke\n");
	fprintf (fp, "    1 setlinewidth\n");
	fprintf (fp, "} def\n");
	    
	fprintf (fp, "\n%% --- draw a box with gray lines ---\n");
	fprintf (fp, "/draw_gray_box { %% x y w h linew => - \n");
	fprintf (fp, "    GRAY setgray\n");
	fprintf (fp, "    draw_box\n");
	fprintf (fp, "    BLACK setgray\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- draw a box with white lines ---\n");
	fprintf (fp, "/dissolve_box { %% x y w h linew => - \n");
	fprintf (fp, "    WHITE setgray\n");
	fprintf (fp, "    draw_box\n");
	fprintf (fp, "    BLACK setgray\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- clear the inside of a box ---\n");
	fprintf (fp, "/clear_box { %% x y w h => -\n");
	fprintf (fp, "    WHITE setgray\n");
	fprintf (fp, "    rect\n");
	fprintf (fp, "    fill \n");
	fprintf (fp, "    BLACK setgray\n");
	fprintf (fp, "} def\n");
	      
	fprintf (fp, "\n%% --- fill a square with gray with white edges ---\n");
	fprintf (fp, "/gray_box { %% x y w h => -\n");
	fprintf (fp, "    6 sub 4 1 roll\n");
	fprintf (fp, "    6 sub 4 1 roll\n");
	fprintf (fp, "    3 add 4 1 roll\n");
	fprintf (fp, "    3 add 4 1 roll\n");
	fprintf (fp, "    make_gray\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- print center-justified text ---\n");
	fprintf (fp, "/ctr_text { %% x1 y x2 str => -\n");
	fprintf (fp, "    dup stringwidth pop 2 div\n");
	fprintf (fp, "    5 2 roll\n");
	fprintf (fp, "    3 -1 roll dup 3 1 roll sub\n");
	fprintf (fp, "    2 div add\n");
	fprintf (fp, "    3 -1 roll sub\n");
	fprintf (fp, "    exch moveto\n");
	fprintf (fp, "    show\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- print left-justified text ---\n");
	fprintf (fp, "/left_text { %% x y str => -\n");
	fprintf (fp, "   3 -2 roll moveto\n");
	fprintf (fp, "   show\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- print right-justified text ---\n");
	fprintf (fp, "/right_text { %% x y str => -\n");
	fprintf (fp, "   dup stringwidth pop\n");
	fprintf (fp, "   4 -1 roll exch sub\n");
	fprintf (fp, "   3 -1 roll moveto show\n"); 
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- set font, keep track of font width & height ---\n");
	fprintf (fp, "/font_set { %% font scale => - \n");
	fprintf (fp, "    /font.x exch def\n");
	fprintf (fp, "    font.x scalefont\n");
	fprintf (fp, "    setfont\n");
	fprintf (fp, "   /font.y (M) stringwidth pop def\n");
	fprintf (fp, "} def\n");
		   
	fprintf (fp, "\n%% --- line feed ---\n");
	fprintf (fp, "/LF { %% - => - \n");
	fprintf (fp, "    /y1 y1 font.x sub def\n");
	fprintf (fp, "    x1 y1 moveto\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- carriage return ---\n");
	fprintf (fp, "/CR { %% - => - \n");
	fprintf (fp, "    /y1 y1 font.x sub font.x 4 div sub def\n");
	fprintf (fp, "    /x1 LM def\n");
	fprintf (fp, "    x1 y1 moveto\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- Sun Logo ---\n");
	fprintf (fp, "/Sunlogo { %% x y size = -\n");
	fprintf (fp, "3 1 roll        %% s x y\n");
	fprintf (fp, "gsave translate %% s\n");
	fprintf (fp, "dup scale       %% -\n");
	fprintf (fp, "45 rotate\n");
	fprintf (fp, "    /Uchar {\n");
	fprintf (fp, "        newpath\n");
	fprintf (fp, "        -.1 0 moveto\n");
	fprintf (fp, "        0 0 .1 180 360 arc\n");
	fprintf (fp, "        0 2.9 rlineto\n");
	fprintf (fp, "        .8 0 rlineto\n");
	fprintf (fp, "        0 -2.9 rlineto\n");
	fprintf (fp, "        0 0 .9 0 180 arcn\n");
	fprintf (fp, "        0 2.9 rlineto\n");
	fprintf (fp, "        .8 0 rlineto\n");
	fprintf (fp, "        closepath\n");
	fprintf (fp, "        fill\n");
	fprintf (fp, "        } def\n");
	fprintf (fp, "    /2Uchar { \n");
	fprintf (fp, "        Uchar gsave 4 4 translate Uchar grestore\n");
	fprintf (fp, "        } def\n");
	fprintf (fp, "    4 { 2Uchar 6 0 translate 90 rotate } repeat\n");
	fprintf (fp, "grestore\n");
	fprintf (fp, "} def\n");

	fprintf (fp, "\n%% --- get y coord to center between TM & BM of a rect ---\n");
	fprintf (fp, "/center_y { %% y boxh lines => y_delta\n");
	fprintf (fp, "    font.y mul 2 div 3 1 roll\n");
	fprintf (fp, "    2 div add\n");
	fprintf (fp, "    add font.y sub\n");
	fprintf (fp, "} def\n");

/*
 *     Wrap up the prolog
*/
	fprintf (fp, "%%%%EndProlog\n");
	fprintf (fp, "%%%%Page: 1 1\n");
	pages_printed = 1;
}

extern void
ps_set_font(fp, font_type, font)
    FILE *fp;
    char *font_type, *font;
{
    fprintf (fp, "/%s /%s findfont def\n", font_type, font);
}

extern void 
ps_set_fontsize (fp, font_type, size)
    FILE *fp; 
    char *font_type; 
    int  size;
{ 
    fprintf (fp, "/%s.scale %d def\n", font_type, size);
} 

extern void
ps_translate (fp, x, y)
    FILE *fp;
    int  x, y;	/* where to move the origin of the coord system */
{
    fprintf (fp, "%d %d translate \t\t%% move coord system \n", x, y);
}

extern void 
ps_scale (fp, x, y) 
    FILE *fp; 
    float  x, y;  /* x & y dimension scaling factors */ 
{
    fputs ((char *)cm_printf((double)x, 2), fp); fputs (" ", fp);
    fputs ((char *)cm_printf((double)y, 2), fp);
    fputs (" scale \t\t%% move coord system\n", fp);
}

extern void 
ps_rotate (fp, angle) 
    FILE *fp; 
    int  angle;  /* rotate the coord axes angle degrees counter-clockwise */ 
{
    fprintf (fp, "%d rotate \t\t%% rotate coord system\n", angle);
}
 
extern void 
ps_landscape_mode (fp) 	/* set to print in landscape mode */
    FILE *fp; 
{
	/* set to landscape by rotating 90 degrees and moving the   */
	/* origin back the width of the paper                       */
	fprintf (fp, "\n\n%% --- set to landscape mode ---\n");
	fprintf (fp, "\ngsave\t\t%% save initial graphics state\n");
	ps_rotate(fp, 90);
	ps_translate(fp, 0, -612);	/* 8.5" * 72 pts per inch */
}
extern void
ps_init_list (fp)
	FILE *fp;
{    
	fprintf (fp, "\n%% --- init for todo print ---\n");

	fprintf (fp, "/boxw RM LM sub 2 div def\n");
	fprintf (fp, "/boxh LM def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
	fprintf (fp, "/x2 x1 boxw add def\n");
	fprintf (fp, "/y2 TM def\n");
	fprintf (fp, "/line1 0 def\n");
	fprintf (fp, "/line2 0 def\n");
	fprintf (fp, "/tab0 0 def\n");
	fprintf (fp, "/tab1 0 def\n");
	fprintf (fp, "/tab2 0 def\n");
	fprintf (fp, "/tab3 0 def\n");

	ps_set_fontsize (fp, "TimeFont",  10);
	ps_set_fontsize (fp, "ApptFont",  10);

}
 
extern void
ps_init_day (fp)
	FILE *fp;
{    
	fprintf (fp, "\n%% --- init for day print ---\n");


	fprintf (fp, "/boxw RM LM sub 2 div def\n");
	fprintf (fp, "/boxh LM def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
	fprintf (fp, "/x2 x1 boxw add def\n");
	fprintf (fp, "/y2 TM def\n");
	fprintf (fp, "/line1 0 def\n");
	fprintf (fp, "/line2 0 def\n");
	fprintf (fp, "/tab1 0 def\n");
	fprintf (fp, "/tab2 0 def\n");

	ps_set_fontsize (fp, "TimeFont",  10);
	ps_set_fontsize (fp, "ApptFont",  10);
}

extern void
ps_init_month (fp)
	FILE *fp;
{    
	fprintf (fp, "\n%% --- init for month view print ---\n");

	fprintf (fp, "/boxw RM LM sub 2 div def\n");
	fprintf (fp, "/boxh LM def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
	fprintf (fp, "/x2 x1 boxw add def\n");
	fprintf (fp, "/y2 TM def\n");
	fprintf (fp, "/line1 0 def\n");
	fprintf (fp, "/line2 0 def\n");
	fprintf (fp, "/tab1 0 def\n");
	fprintf (fp, "/tab2 0 def\n");

	if (prolog_found) {
		ps_set_font(fp, "TimeFont", "LC_Helvetica");
		ps_set_font(fp, "DateFont", "LC_Times-Bold");
		ps_set_font(fp, "DayFont",  "LC_Times-Bold");
	}
	else {
		fprintf (fp, "/ISO_fonts {\n");
		ps_set_font(fp, "TimeFont", "Helvetica-ISOLatin1");
		ps_set_font(fp, "DateFont", "Times-Bold-ISOLatin1");
		ps_set_font(fp, "DayFont",  "Times-Bold-ISOLatin1");
		fprintf (fp, "} def\n");
		fprintf (fp, "/Reg_fonts {\n");
		ps_set_font(fp, "TimeFont", "Helvetica");
		ps_set_font(fp, "DateFont", "Times-Bold");
		ps_set_font(fp, "DayFont",  "Times-Bold");
		fprintf (fp, "} def\n");
		fprintf (fp, "%%\n");
		fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
		fprintf (fp, "{ISO_fonts} {Reg_fonts} ifelse\n");
		fprintf (fp, "%%\n");
	}

	ps_set_fontsize (fp, "MonthFont", 30);
	ps_set_fontsize (fp, "DateFont",  20);
	ps_set_fontsize (fp, "DayFont",   14);
	ps_set_fontsize (fp, "TimeFont",   8);
	ps_set_fontsize (fp, "ApptFont",   8);
}


extern void
ps_finish_printer (fp)
	FILE *fp;
{
	char *str;

	fprintf (fp, "\n\n%% --- final cmds to printer ---\n");
	fprintf (fp, "showpage\n");
	fprintf (fp, "%%%%Trailer\n");
	fprintf (fp, "%%%%Pages: %d\n",pages_printed);
	
	if ( !prolog_found ) {
		if ( nonascii_char && !already_printed_warning ) {
			already_printed_warning = 1;
			fprintf (fp, "%%\n");
			fprintf (fp, "90 rotate\n");
			fprintf (fp, "0 -612 translate\n");
			fprintf (fp, "/Helvetica-Bold findfont 18 scalefont setfont\n");
			fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
			fprintf (fp, "{ }\n");
			str = euc_to_octal(MGET("Calendar Manager warning:"));
			fprintf (fp, "{ 72 470 moveto (%s) show\n", str);
			str = euc_to_octal(MGET("Your calendar contains characters that"));
			fprintf (fp, "  72 435 moveto (%s) show\n", str);
			str = euc_to_octal(MGET("your printer does not support."));
			fprintf (fp, "  72 410 moveto (%s) show\n", str);
			str = euc_to_octal(MGET("This will result in erroneous characters being printed."));
			fprintf (fp, "  72 385 moveto (%s) show showpage }\n", str);
			fprintf (fp, "ifelse\n");
			fprintf (fp, "%%\n");
		}
	}

	fprintf (fp, "grestore\t\t%% restore initial graphics state\n"); 
}
extern void
ps_print_file ()
{
	Props *p = (Props *) calendar->properties;
	char command[BUFSIZ];
	char *def_pr=NULL;
	char printer[81];

	nonascii_char = 0;
	already_printed_warning = 0;
	/* check to see if print to printer (0) or file (1) */
	if (p->dest_choiceVAL == 0) 
	{

		if (p->printer_nameVAL[0] == '\0') {
			def_pr = (char*)ds_def_printer();
#ifdef SVR4
			sprintf(printer,"-d%s", def_pr);
		}
		else
			sprintf(printer,"-d%s", p->printer_nameVAL);
		sprintf(command, "lp -s -Tpostscript -c %s -n%s %s %s", printer, 
			p->copiesVAL, p->optionVAL, ps_file);
#else
			sprintf(printer,"-P%s", def_pr);
		}
		else
			sprintf(printer,"-P%s", p->printer_nameVAL);
		sprintf(command, "lpr -r %s -#%s %s %s", printer, 
			p->copiesVAL, p->optionVAL, ps_file);
#endif
		system(command);
		if (def_pr != NULL)
			free(def_pr);
#ifdef SVR4
		unlink(ps_file);
#endif
	}
}
extern void
ps_print_header (fp, buf)
	FILE *fp;
	char *buf;
{
	char *str;

	fprintf (fp, "\n%% --- print header info at top ---\n");

	fprintf (fp, "StampFont StampFont.scale font_set\n"); 

	/* print timestamp at left margin */ 
	fprintf (fp, "LM y1 font.y sub (%s) left_text\n", timestamp); 

	/* print user@host stamp at right margin */ 
	fprintf (fp, "RM y1 font.y sub (%s) right_text\n", calendar->view->current_calendar); 

	fprintf (fp, "MonthFont MonthFont.scale font_set\n"); 

	nonascii_string(buf);
	str = euc_to_octal(buf);
	fprintf (fp, "LM y1 RM (%s) ctr_text \n", str); 

#ifdef NEVER
	/* print sun logo at left margin */ 
	fprintf (fp, "LM 20 add y1 4 Sunlogo\n"); 
#endif

}

extern void
ps_day_header (fp, timeslots, num_page, total_pages)
    FILE *fp;
    int timeslots, num_page, total_pages;
{
	char *str, str2[BUFFERSIZE];

	if (total_pages == 0)
		total_pages++;

	fprintf (fp, "CR CR \n");

	/* print creation notice at bottom */ 
	fprintf (fp, "StampFont StampFont.scale font_set\n"); 

	str = MGET("of");
	nonascii_string(str);
        cm_strcpy(str2, MGET("Page"));
        cm_strcpy(str2, euc_to_octal(str2));
     	fprintf (fp, "LM BM font.y sub (%s %d %s %d) left_text \n",
                        str2, num_page, str, total_pages);

	str = MGET("Day view by Calendar Manager");
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str);

	fprintf (fp, "\n%% --- print Morning/Afternoon boxes ---\n");
	fprintf (fp, "DayFont DayFont.scale font_set\n");
	fprintf (fp, "/tab1 font.y 2 mul def\n");
	fprintf (fp, "/boxh font.x 2 mul def\n");
	fprintf (fp, "/y2 y1 def\n");

	fprintf (fp, "x1 y1 boxw boxh make_lt_gray\n");
	fprintf (fp, "x2 y2 boxw boxh make_lt_gray\n");

	fprintf (fp, "x1 y1 tab1 boxh make_gray\n");
	fprintf (fp, "x2 y2 tab1 boxh make_gray\n");

	fprintf (fp, "x1 y1 boxw boxh line_width draw_box\n");
	fprintf (fp, "x2 y2 boxw boxh line_width draw_box\n");

	fprintf (fp, "x1 y1 tab1 boxh line_width draw_box\n");
	fprintf (fp, "x2 y2 tab1 boxh line_width draw_box\n");

	str = MGET("Morning");
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "x1 tab1 add y1 boxh 1 center_y x2 (%s)   ctr_text\n", str);
	str = MGET("Afternoon");
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "x2 tab1 add y2 boxh 1 center_y RM (%s) ctr_text\n", str);

	fprintf (fp, "/boxh y1 BM sub %d div def\n", timeslots);
	fprintf (fp, "/y2 y1 def\n");	/* save pos for 2nd column */
}

extern void
ps_day_timeslots(fp, i, more)
    FILE *fp;
    int i;
    Boolean more;
{
    char buf[6];

    fprintf (fp, "\n%% --- print hourly boxes for appt entries ---\n");

    if (i == 12)
       /* reset to next col */
	fprintf (fp, "/x1 LM boxw add def  /y1 y2 def \n");

    fprintf (fp, "/y1 y1 boxh sub def\n");

    fprintf (fp, "x1 y1 boxw boxh line_width draw_box\n");
    fprintf (fp, "x1 y1 tab1 boxh make_lt_gray\n");
    fprintf (fp, "x1 y1 tab1 boxh line_width draw_box\n");

    fprintf (fp, "TimeFont TimeFont.scale font_set\n");
    sprintf (buf, "%d", morning(i) ? i : i==12 ? i : (i-12) );
    fprintf (fp, "x1 y1 boxh 2 center_y x1 tab1 add (%s) ctr_text\n", buf);

    sprintf (buf, "%s", morning(i) ? MGET("am") : MGET("pm"));   
    fprintf (fp, 
        "x1 y1 boxh 2 center_y font.y sub x1 tab1 add (%s) ctr_text\n", buf);
    if (more)
	fprintf(fp, "-9 -12 rmoveto (%s) show\n", "*");
    fprintf (fp, "/line1 y1 boxh add font.y sub 1 sub def\n");
}
extern void 
ps_todo_outline(fp, t, appt_type) 
    	FILE *fp; 
	Todo *t;
	Event_Type appt_type;
{
	char *str;
	char str2[BUFFERSIZE];

	fprintf (fp, "StampFont StampFont.scale font_set\n");
	if (appt_type == toDo) {
		str = MGET("To Do List by Calendar Manager");
	} else {
		str = MGET("Appt List by Calendar Manager");
	}
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str);
	cm_strcpy(str2, MGET("Page"));
	str = MGET("of");
	cm_strcpy(str2, euc_to_octal(str2));
	fprintf (fp, "LM BM font.y sub (%s %d %s %d) left_text \n",
			str2, t->curr_page, str, t->num_pages);
	t->curr_page++;
	fprintf (fp, "/line_width 2 def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 BM def\n");
	fprintf (fp, "/boxw RM LM sub def\n");
	fprintf (fp, "/boxh TM BM sub 15 sub def\n");

	fprintf (fp, "x1 y1 boxw boxh make_lt_gray\n");

	fprintf (fp, "x1 y1 boxw 10 make_gray\n");
	fprintf (fp, "x1 y1 10 boxh make_gray\n");
	fprintf (fp, "x1 y1 boxh add 10 sub boxw 10 make_gray\n");
	fprintf (fp, "x1 boxw add 10 sub y1 10 boxh make_gray\n");

	fprintf (fp, "x1 y1 boxw boxh line_width draw_box\n");
	fprintf (fp, "x1 10 add y1 10 add boxw 20 sub boxh 20 sub line_width draw_box\n");
	/* init for drawing appts */
	fprintf (fp, "/line1 TM 50 sub def\n");
	fprintf (fp, "/tab0 25 def\n");
	fprintf (fp, "/tab1 50 def\n");
	fprintf (fp, "/tab2 70 def\n");
	fprintf (fp, "/tab3 84 def\n");
}

extern void 
ps_month_daynames(fp, rows, num_page, total_pages) 
    FILE *fp; 
    int rows;
    int num_page;
    int total_pages;
{ 
	char *str, str2[BUFFERSIZE];
	char *day_of_week[7];
	int k;

	if (total_pages == 0)
		total_pages++;

	/* print creation notice at bottom */ 
	fprintf (fp, "StampFont StampFont.scale font_set\n"); 
	str = MGET("Month view by Calendar Manager");
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str); 

	fprintf (fp, "StampFont StampFont.scale font_set\n"); 
	str = MGET("of");
	nonascii_string(str);
        cm_strcpy(str2, MGET("Page"));
        cm_strcpy(str2, euc_to_octal(str2));
     	fprintf (fp, "LM BM font.y sub (%s %d %s %d) left_text \n",
                        str2, num_page, str, total_pages);

	fprintf (fp, "\n%% --- print days of the week with boxes ---\n"); 
	fprintf (fp, "DayFont DayFont.scale font_set\n");
	fprintf (fp, "/boxh font.x 2 mul def\n"); 
	fprintf (fp, "/boxw RM LM sub 7 div def\n");
	fprintf (fp, "/x1 LM def\n"); 
	fprintf (fp, "/y1 y1 boxh sub 15 sub def\n");

	day_of_week[0] = MGET("Sun");
	nonascii_string(day_of_week[0]);
	day_of_week[1] = MGET("Mon");
	nonascii_string(day_of_week[1]);
	day_of_week[2] = MGET("Tue");
	nonascii_string(day_of_week[2]);
	day_of_week[3] = MGET("Wed");
	nonascii_string(day_of_week[3]);
	day_of_week[4] = MGET("Thu");
	nonascii_string(day_of_week[4]);
	day_of_week[5] = MGET("Fri");
	nonascii_string(day_of_week[5]);
	day_of_week[6] = MGET("Sat");
	nonascii_string(day_of_week[6]);
	for ( k = 0;  k < 7;  k++ ) {
		str = euc_to_octal(day_of_week[k]);	
		day_of_week[k] = (char *)malloc(sizeof(char) * cm_strlen(str) + 1);
		cm_strcpy(day_of_week[k], str);
	}
	fprintf (fp, "[(%s) (%s) (%s) (%s) (%s) (%s) (%s)] {\n", day_of_week[0], 
			 day_of_week[1], day_of_week[2], day_of_week[3], day_of_week[4],
			 day_of_week[5], day_of_week[6]);
	fprintf (fp, "    x1 y1 boxw boxh make_gray\n");
	fprintf (fp, "    x1 y1 boxw boxh line_width draw_box\n");
	fprintf (fp, "    WHITE setgray\n");
	fprintf (fp, "    x1 y1 boxh 1 center_y x1 boxw add\n");
	fprintf (fp, "    4 -1 roll ctr_text\n");
	fprintf (fp, "    BLACK setgray\n"); 
	fprintf (fp, "    /x1 x1 boxw add def\n");
	fprintf (fp, "} forall\n"); 
	fprintf (fp, "/boxh y1 BM sub %d div def\n", rows);
	fprintf (fp, "/y1 y1 boxh sub def\n");

	fprintf (fp, "\n%% --- make monthly grid ---\n\n");
	fprintf (fp, "/y2 y1 def\n"); 
	fprintf (fp, "%d {\n", rows);
	fprintf (fp, "    /x1 LM def\n"); 
	fprintf (fp, "    7 { \n");
	fprintf (fp, "        x1 y1 boxw boxh make_lt_gray\n");
	fprintf (fp, "        x1 y1 boxw boxh line_width draw_box\n");
	fprintf (fp, "        /x1 x1 boxw add def\n");
	fprintf (fp, "    } repeat  \n");
	fprintf (fp, "    /y1 y1 boxh sub def\n");
	fprintf (fp, "} repeat\n");
	fprintf (fp, "/y1 y2 def\n"); 

	for ( k = 0;  k < 7;  k++ ) {
		free((char *)day_of_week[k]);
	}
}

extern void 
ps_month_timeslots(fp, date, dow, more) 
    FILE *fp; 
    int date, dow; 
    Boolean more;
{ 
    fprintf (fp, "\n%% --- print daily boxes for appt entries ---\n"); 

    /* if weekday==0 && not 1st day of month then start on next line */
    fprintf (fp, "%d 0 eq %d 1 ne and { /y1 y1 boxh sub def } if\n",dow,date);
    fprintf (fp, "/x1 boxw %d mul LM add def\n", dow);
    fprintf (fp, "x1 y1 boxw boxh clear_box\n");
    fprintf (fp, "x1 y1 boxw boxh line_width draw_box\n");

    fprintf (fp, "DateFont DateFont.scale font_set\n");
    fprintf (fp, "/tab1 3 def\n");
    fprintf (fp, "/line1 y1 boxh add font.y sub tab1 sub def\n");
    fprintf (fp, "x1 tab1 add line1 (%d) left_text\n", date);
    if (more)
	fprintf(fp, "6 0 rmoveto (%s) show\n", "*");
    fprintf (fp, "/line1 line1 tab1 sub def\n");

    fprintf (fp, "TimeFont TimeFont.scale font_set\n");
    fprintf (fp, "/line1 line1 font.y sub 1 sub def\n");
    fprintf (fp, "/tab2 font.x 3 mul tab1 add def\n");
}

extern void 
ps_month_cont(fp, date) 
    FILE *fp; 
    int date; 
{ 
    fprintf (fp, "\n%% --- print daily boxes for appt entries ---\n"); 

    /* if weekday==0 && not 1st day of month then start on next line */
    fprintf (fp, "%d 0 eq %d 1 ne and { /y1 y1 boxh sub def } if\n",dow,date);
    fprintf (fp, "/x1 boxw %d mul LM add def\n", dow);
    fprintf (fp, "x1 y1 boxw boxh clear_box\n");
    fprintf (fp, "x1 y1 boxw boxh line_width draw_box\n");

    fprintf (fp, "DateFont DateFont.scale font_set\n");
    fprintf (fp, "/tab1 6 def\n");
    fprintf (fp, "/line1 y1 boxh add font.y sub tab1 sub def\n");
    fprintf (fp, "x1 tab1 add line1 (%s) right_text\n", "(...)");
    fprintf (fp, "/line1 line1 tab1 sub def\n");

    fprintf (fp, "TimeFont TimeFont.scale font_set\n");
    fprintf (fp, "/line1 line1 font.y sub 1 sub def\n");
    fprintf (fp, "/tab2 font.x 3 mul tab1 add def\n");
}

extern void
ps_print_time (fp, str, view) 
	FILE *fp;
	char *str;
	Glance view;
{

	fprintf (fp, "line1 2 sub y1 gt {\n");
	/* time and text must be same font for multi-pages */
	fprintf (fp, "    ApptFont ApptFont.scale font_set\n");

	if (view == dayGlance) {
                fprintf (fp, "    /tab2 tab1 font.x 2 mul add def\n");
                fprintf (fp, "    x1 tab1 add 3 add line1 (%s) left_text\n",
str);
                fprintf (fp, "    /line1 line1 font.y sub 1 sub def\n");
        }
	else if (view == weekGlance) {
                fprintf (fp, "    /tab2 tab1 2 mul def\n");
                fprintf (fp, "    x1 tab1 add line1 (%s) left_text\n", str);
                fprintf (fp, "    /line1 line1 font.y sub 1 sub def\n");
	}
	else if (view == monthGlance) {
		fprintf (fp, "    /tab2 font.x 3 mul tab1 add def\n");
		fprintf (fp, "    x1 tab2 add line1 (%s) right_text\n", str);
	}

	fprintf (fp, "} if\n\n");

}


extern void
ps_print_text (fp, str, indented, view)
	FILE *fp;
	char *str;
	int indented;
	Glance view;
{
	fprintf (fp, "line1 2 sub y1 gt {\n");
	fprintf (fp, "    ApptFont ApptFont.scale font_set\n");
	fprintf (fp, "    gsave\n");

	fprintf (fp, "    x1 y1 boxw font.x 2 div sub boxh rect clip newpath\n");
	nonascii_string(str);
	str = fix_ps_string(str);

	str = euc_to_octal(str);
	if (view == dayGlance) {
		fprintf (fp, "    /tab2 tab1 font.x 2 mul add def\n");
		if (indented)
			fprintf (fp, "    x1 tab2 add line1 (%s) left_text\n", str);
		else
			fprintf (fp, "    x1 tab1 add 3 add line1 (%s) left_text\n", str);
	}

	else if (view == weekGlance) {
		if (indented) {
			fprintf (fp, "    /tab2 tab1 3 mul def\n");
			fprintf (fp, "    x1 tab2 add line1 (%s) left_text\n", str);
		}
		else {
			fprintf (fp, "    /tab2 tab1 2 mul def\n");
			fprintf (fp, "    x1 tab1 add line1 (%s) left_text\n", str);
		}
	}

	else if (view == monthGlance) {
		if (indented)
			fprintf (fp, "    x1 tab2 add 1 add line1 (%s) left_text\n", str);
		else
			fprintf (fp, "    x1 font.x add line1 (%s) left_text\n", str);
	}

	fprintf (fp, "    grestore\n");
	fprintf (fp, "    /line1 line1 font.y sub 1 sub def\n");
	fprintf (fp, "} if\n");
}
static void
ps_print_todo_text (fp, str1, as, appt_type, items)
	FILE *fp;
	char *str1;
	Appt_Status as;
	Event_Type appt_type;
	int items;
{
	char buf[20];
	char *str;

	fprintf (fp, "line1 20 sub y1 gt {\n");
	fprintf (fp, "    ApptFont ApptFont.scale font_set\n");
	fprintf (fp, "    gsave\n");

	fprintf (fp, "    x1 y1 boxw font.x 2 div sub boxh rect clip newpath\n");

        if (items > 9)
		sprintf(buf, "%d.", items);
	else
		sprintf(buf, " %d.", items);
    	fprintf (fp, "TimeFont TimeFont.scale font_set\n");
	fprintf (fp, "    x1 tab0 add line1 (%s) left_text\n", buf);

	fprintf (fp, "    ApptFont ApptFont.scale font_set\n");
	if (appt_type == toDo) {
		/* draw check box */
		fprintf (fp, "    x1 tab1 add line1 10 10 1 draw_box\n");
		/* add check to box */
		if (as == completed) {
			fprintf (fp, "    x1 tab1 add 3 add line1 6 add moveto\n");
			fprintf (fp, "    x1 tab1 add 6 add line1 1 add lineto\n");
			fprintf (fp, "    x1 tab1 add 14 add line1 10 add lineto\n");
			fprintf (fp, "    1 setlinewidth");
			fprintf (fp, "    stroke");
		}
	}
	else 
		fprintf (fp, "    /tab2 tab1 def\n");
	
	nonascii_string(str1);
	str1 = fix_ps_string(str1);
	str = euc_to_octal(str1);

	fprintf (fp, "    x1 tab2 add line1 (%s) left_text\n", str);
	fprintf (fp, "    grestore\n");
	fprintf (fp, "    /line1 line1 font.y 2 mul sub def\n");
	fprintf (fp, "} if\n");
}
extern void
ps_month_line(fp)
	FILE *fp;
{
	fprintf (fp, "    2 setlinewidth \n");
	fprintf (fp, "    /yval 213 def\n");
	fprintf (fp, "x1 10 add yval boxw 20 sub 11 2 draw_box\n");
	fprintf (fp, "x1 8 add yval boxw 15 sub 11 make_gray\n");
}
extern void
ps_print_todo_months(fp)
	FILE *fp;
{
	int m_wid, m_hgt, mon, yr;
	Calendar *c = calendar;

	mon = month(c->view->date);
	yr = year(c->view->date);

	m_wid = (PRINTER_WIDTH-INCH-INCH-20) / 3;
        m_hgt = (PRINTER_HEIGHT-3*INCH-20) / 4;

	ps_translate(fp, INCH+6, 184); 

	if ((mon-1) == 0) 
		print_std_month(fp, 12, yr-1, m_wid, m_hgt);
	else 
		print_std_month(fp, mon-1, yr, m_wid, m_hgt);


	ps_translate(fp, m_wid, 0);
	print_std_month(fp, mon, yr, m_wid, m_hgt);

	ps_translate(fp, m_wid, 0);
	if ((mon+1) > 12) 
		print_std_month(fp, 1, yr+1, m_wid, m_hgt);
	else 
		print_std_month(fp, mon+1, yr, m_wid, m_hgt);

	ps_translate(fp, -(INCH+6+m_wid+m_wid), -184);

	fprintf (fp, "230 213 moveto\n");
	fprintf (fp, "230 82 lineto\n");
	fprintf (fp, "    stroke\n");
	fprintf (fp, "380 213 moveto\n");
	fprintf (fp, "380 82 lineto\n");
	fprintf (fp, "    stroke\n");

}
extern void
ps_print_todo (fp, a, appt_type, glance) 	/* print out times & appts */
	FILE *fp;
	Abb_Appt *a; 
	Event_Type appt_type;
	Glance glance;
{
	Calendar	*c = calendar;
	Props		*p = (Props*)c->properties;
	int      	max_chars=75, items_per_page = 0;
	char            buf3[100], buf2[50], buf1[80];
	Todo 		*t = (Todo*)calendar->todo;
	static		int total_items;
	int 		meoval; 
 
	meoval = p->meoVAL;
	buf1[1] = buf2[0] = buf3[0] = '\0';
	while (a != NULL) {
		switch(a->privacy) {
			case public:
				if ((meoval == 2) || (meoval == 4) || (meoval == 6)) {
					a=a->next;
					continue;
				}
				break;
			case semiprivate:
				if ((meoval == 1) || (meoval == 4) || (meoval == 5)) {
					a=a->next;
					continue;
				}
				break;
			case private:
				if ((meoval == 1) || (meoval == 2) || (meoval == 3)) {
					a=a->next;
					continue;
				}
				break;
			default:
				break;
		}
		if (appt_type == toDo) {
			if (a->tag->tag != toDo) {
				a=a->next;
				continue;
			}
		}
		else if (appt_type == appointment) 
			if (a->tag->tag != appointment) {
				a=a->next;
				continue;
			}
		format_maxchars(a, buf1, max_chars, p->default_disp_VAL);
		if (glance != dayGlance) {
			format_date3(a->appt_id.tick, p->ordering_VAL,
				p->separator_VAL, buf2);
			sprintf(buf3, "%s  %s", buf2, buf1);
		}
		else
			cm_strcpy(buf3, buf1);
		if (buf3 != NULL) {
			total_items++;
			ps_print_todo_text (fp, buf3, a->appt_status,
					appt_type, total_items);
			if (++items_per_page > MAX_TODO) {
				fprintf (fp, "showpage\n");
				ps_next_page(fp, t->curr_page);
				ps_init_list(fp);
				ps_print_header(fp, t->header);
				ps_todo_outline(fp, t, appt_type);
				ps_print_todo(fp, a->next, appt_type, glance);
				return;
			}
		}
		a = a->next;
	}

	if (items_per_page > MAX_TODO_LP) {
		fprintf (fp, "showpage\n");
		ps_next_page(fp, t->curr_page);
		ps_init_list(fp);
		ps_print_header(fp, t->header);
		ps_todo_outline(fp, t, appt_type);
	}

	ps_month_line(fp);
	ps_print_todo_months(fp);
	total_items = 0;
}

extern Boolean
ps_print_month_appts (fp, a, num_page, hi_tm, lines_per_box, view)
	FILE *fp;
	Abb_Appt *a;
	int num_page;
	int hi_tm;
	int lines_per_box;
	Glance view;
{
	/*
	 * This routine is used to print appointments for day, week 
	 * and month views.  The parm "view" is used to differentiate
	 * who is printing, and personalize the print for that view.
	 */

	int      	indented, multlines=TRUE;
	Lines		*lines, *lp;
    	char     	buf1[128];
	Calendar	*c = calendar;
	Props		*pr = (Props*)c->properties;
	int	        i, meoval, start, pos = 1, line_counter = 0;	
 
	buf1[0] = 0;
	meoval = pr->meoVAL; /* my eyes only or private appt: 0 = include */
	start = ((num_page - 1) * lines_per_box) + 1;

	while ((a!=NULL) && (a->appt_id.tick < hi_tm)) {
               switch(a->privacy) {
                        case public:
                                if ((meoval == 2) || (meoval == 4) || (meoval == 6)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case semiprivate:
                                if ((meoval == 1) || (meoval == 4) || (meoval == 5)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case private:
                                if ((meoval == 1) || (meoval == 2) || (meoval == 3)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        default:
                                break;
                }

                if (pos < start) {
                        a=a->next;
                        pos++;
                        continue;
                }

		get_time_str (a, buf1);

		indented = (*buf1 != NULL);


                lp = lines = text_to_lines(a->what, multlines ? 10 : 1);
                line_counter++;
                if ((line_counter > lines_per_box) && (lines != NULL))
                        return(FALSE);

		/* only print if appt text found */ 
		if (lines != NULL && lines->s != NULL) {
			if (indented)  	/* time found so print it */
				ps_print_time (fp, buf1, view);
			ps_print_text (fp, lines->s, indented, view);

		}
		destroy_lines(lp);
		a = a->next;
	}
	return(TRUE);
}

extern Boolean   
ps_print_multi_appts (fp, a, num_page, hi_tm, view)
        FILE *fp;
        Abb_Appt *a;
        int num_page;
        int hi_tm;  
        Glance view;
{
        /*
         * This routine is used to print appointments for day, week
         * and month views.  The parm "view" is used to differentiate
         * who is printing, and personalize the print for that view.
         */
 
        int             indented, multlines=TRUE;
        Lines           *lines, *lp;    
        char            buf1[128], buf2[257];
        Calendar        *c = calendar;  
        Props           *pr = (Props*)c->properties;
        int             i, meoval, start, maxlines, pos = 1, line_counter = 0;
	Boolean		new_appt = FALSE, time = FALSE;
 
	buf1[0] = buf2[0] = 0;
        if (view == weekGlance)
                maxlines = 22;
        else if (view == dayGlance)
                maxlines = get_lines_per_page(pr);
 
        meoval = pr->meoVAL; /* my eyes only or private appt: 0 = include */
        start = ((num_page - 1) * maxlines) + 1;
 
        while ((a!=NULL) && (a->appt_id.tick < hi_tm)) {
               switch(a->privacy) {
                        case public:
                                if ((meoval == 2) || (meoval == 4) || (meoval == 6)) {                                
                                        a=a->next;
                                        continue;
                                }
                                break;   
                        case semiprivate:
                                if ((meoval == 1) || (meoval == 4) || (meoval == 5)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case private:
                                if ((meoval == 1) || (meoval == 2) || (meoval == 3)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        default:
                                break;
                }
 
                lp = lines = text_to_lines(a->what, multlines ? 10 : 1);
		new_appt = TRUE;

                /* skip past lines already printed */
                if (pos < start) {
                        if (a->tag->showtime) {
				if (new_appt)
					for (i = 1; i <= num_page; i++)
						if (pos == (maxlines * i))
							start--;
				pos++;
			}
                        while ((lines != NULL) && (pos < start)) {
                        	pos++;
				new_appt = FALSE;
                        	lines = lines->next;
                        }
                        if (pos < start) {
                        	a=a->next;
                        	continue;
                        }
                }
 
		/* skip last line if it's a time */
		if (a->tag->showtime) {
			if (line_counter == (maxlines - 1))
				return(FALSE);
		}

                /* HH:MM xm - HH:MM xm format */
                format_line2(a, buf1, buf2, pr->default_disp_VAL);

                indented = (*buf1 != NULL);
                /* only print if appt text found */
                if (lines != NULL && lines->s != NULL) {
                        if ((indented) && (new_appt)) {
                                line_counter++;
                                if ((line_counter > maxlines) && (lines != NULL))
                                	return(FALSE);
                                ps_print_time (fp, buf1, view);
                        }
 
                        ps_print_text (fp, lines->s, indented, view);
                        line_counter++;
                        if ((line_counter > maxlines) && (lines != NULL))
                                return(FALSE);
 
                        lines = lines->next;
                        while (lines != NULL) {
                        	line_counter++;
                        	if ((line_counter > maxlines) && (lines != NULL))
                        		return(FALSE);
                        	ps_print_text (fp, lines->s, indented, view);
                        	lines = lines->next;
                        }
                }
                destroy_lines(lp);
                if (view == weekGlance)
                        ps_week_sched_update(a, pr);
		new_appt = FALSE;
		time = FALSE;
                a = a->next;
        }
        return(TRUE);
}


extern void
ps_print_little_months (fp, tick) 
	FILE *fp;
	Tick  tick;
{
	int m = month(tick);		/* current month just printed */
	int y = year(tick);		/* year of month just printed */
	int fday = fdom(tick);		/* first day of month, 0=Sun ... */

	int nm, nmy, pm, pmy;

	int ndays = monthlength(tick);
	int nrows = numwks(tick);
	int inch = 72;
	int dx, dy;

	/* 
	 * Print out miniature prev & next month on month grid.
	 * Check if there is enough room at end;  if not then
	 * print at beguinning of grid.
	 *
	 * This is a massive kludge, as we are eyeballing where to place
	 * the miniatures.  Oh well... redo right later...
	 */
	if (nrows == 4) nrows++;

	fprintf (fp, "\n%% --- print miniature months ---\n");

	if ((fday+ndays+2) <= (nrows*7))
	{
		dx = 564;		/* print at bottom */
		dy = 140;
		if (nrows == 6)
			dy = 130;
	}
	else
	{
		dx = 75;		/* print at top */
		dy = 478;
	}

	if (m == 12)
	{
		nm = 1;
		pm = m - 1;
		nmy = y + 1;
		pmy = y;
	}
	else if (m == 1)
	{
		nm = m + 1;
		pm = 12;
		nmy = y;
		pmy = y - 1;
	}
	else
	{
		nm = m + 1;
		pm = m - 1;
		nmy = pmy = y;
	}

	fprintf (fp, "\ngsave\n\n");
	ps_translate (fp, dx, dy);
	ps_scale (fp, 0.6, 0.6);
	print_std_month(fp, pm, pmy, 2*inch, 2*inch);

	ps_translate (fp, 165, 0);
	print_std_month(fp, nm, nmy, 2*inch, 2*inch);
	fprintf (fp, "\ngrestore\n\n");
}


/* 
 * 	print week routines
 */

ps_init_week(fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- init for week view print ---\n");

	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
	fprintf (fp, "/boxw RM LM sub 5 div def\n");

	ps_set_fontsize (fp, "MonthFont", 16);
	ps_set_fontsize (fp, "DayFont",   12);
	ps_set_fontsize (fp, "TimeFont",   9);
	ps_set_fontsize (fp, "ApptFont",   9);
}


ps_week_header(fp, buf, num_page, total_pages)
	FILE *fp;
	char *buf;
	int num_page;
	int total_pages;
{
	char *str, str2[BUFFERSIZE];

	if (total_pages == 0)
		total_pages++;

	nonascii_string(buf);
	fprintf (fp, "\n%% --- print week header centered at top ---\n");
	fprintf (fp, "MonthFont MonthFont.scale font_set CR\n"); 
	str = euc_to_octal(buf);
	fprintf (fp, "LM y1 RM (%s) ctr_text\n", str); 

#ifdef NEVER
	/* print sun logo at left margin */ 
	fprintf (fp, "LM 20 add y1 4 Sunlogo\n", buf); 
#endif

	fprintf (fp, "StampFont StampFont.scale font_set\n"); 

	str = euc_to_octal(timestamp);
	/* print timestamp at left margin */ 
	fprintf (fp, "LM y1 font.y sub (%s) left_text\n", str); 

	/* print user@host stamp at right margin */ 
	fprintf (fp, "RM y1 font.y sub (%s) right_text\n", calendar->view->current_calendar); 

       	str = MGET("of");
        nonascii_string(str);
        cm_strcpy(str2, MGET("   Page"));
        cm_strcpy(str2, euc_to_octal(str2));
        fprintf (fp, "LM BM font.y sub (%s %d %s %d) left_text \n",
                        str2, num_page, str, total_pages);

	/* print creation notice at bottom */ 
	str = MGET("Week view by Calendar Manager");
	nonascii_string(str);
	str = euc_to_octal(str);
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str); 

	/* save current position of y1 */
	fprintf (fp, "CR\n");
	fprintf (fp, "/off_y y1 2 sub def\n");

	/* determine box height (takes into account size of month font) */
	fprintf (fp, "/boxh off_y BM sub 2 div def\n");
}


ps_week_appt_boxes(fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- print week appt boxes ---\n");

	/* save to place box around weekday string */
	fprintf (fp, "/tab_y DayFont.scale 1.5 mul def\n");

	/* print gray background then bold weekdays box */
	fprintf (fp, "/line_width 2 def\n");
	fprintf (fp, "/x2 LM def\n");
	fprintf (fp, "/y2 off_y boxh sub def\n");
	fprintf (fp, "x2 off_y tab_y sub boxw 5 mul tab_y make_lt_gray\n");
	fprintf (fp, "x2 y2 boxw 5 mul boxh line_width draw_box\n");

	/* print gray background then weekends box */
	fprintf (fp, "/x2 LM boxw 3 mul add def\n");
	fprintf (fp, "/y2 off_y boxh 2 mul sub def\n");
	fprintf (fp, "x2 y2 boxh add tab_y sub boxw 2 mul tab_y make_lt_gray\n");
	fprintf (fp, "x2 y2 boxw 2 mul boxh line_width draw_box\n");

	/* print internal lines for weekday box */
	fprintf (fp, "/x2 LM def\n");
	fprintf (fp, "/y2 off_y boxh sub def\n");
	fprintf (fp, "/line_width 1 def\n");
	fprintf (fp, "5 {\n");
	fprintf (fp, "    x2 y2 boxw boxh line_width draw_box\n");
	fprintf (fp, "    /x2 x2 boxw add def\n");
	fprintf (fp, "} repeat\n");

	/* print horizontal internal line for weekday box */
	fprintf (fp, "LM off_y tab_y sub moveto\n");
	fprintf (fp, "RM off_y tab_y sub lineto\n");
	fprintf (fp, "stroke\n");

	/* print internal lines for weekend box */
	fprintf (fp, "/x2 LM boxw 3 mul add def\n");
	fprintf (fp, "/y2 off_y boxh 2 mul sub def\n");
	fprintf (fp, "2 {\n");
	fprintf (fp, "    x2 y2 boxw boxh line_width draw_box\n");
	fprintf (fp, "    /x2 x2 boxw add def\n");
	fprintf (fp, "} repeat\n");

	/* print horizontal internal line for weekend box */
	fprintf (fp, "LM boxw 3 mul add off_y boxh sub tab_y sub moveto\n");
	fprintf (fp, "RM off_y boxh sub tab_y sub lineto\n");
	fprintf (fp, "stroke\n");
}


ps_week_sched_boxes(fp)
	FILE *fp;
{
	char *str;
	char *day_of_week[7];
	int malloc_memory = 0;
	int k;
	Props *p = (Props *)calendar->properties;
	int reset_font = 0;
	int begin_hr = 0;
	char *current_locale;

	fprintf (fp, "\n%% --- print week sched boxes ---\n");

	/* figure out number of partitions in sched box */
	num_hours = p->end_slider_VAL - p->begin_slider_VAL;
	/* determine offset of sched box from left margin */
	fprintf (fp, "DayFont DayFont.scale font_set\n");
	fprintf (fp, "/tab (12) stringwidth pop def\n");
	fprintf (fp, "/x2 LM tab add def\n");
	fprintf (fp, "/y2 off_y boxh sub DayFont.scale 2 mul sub def\n");
	fprintf (fp, "/off_y1 y2 5 sub def\n");

	/* divide by number of partitions in schedule box */
	fprintf (fp, "/s_boxh off_y1 BM sub %d div def\n", num_hours);
	fprintf (fp, "/s_boxw 3 boxw mul tab sub tab sub 7 div def\n");

	current_locale = (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG);
	if (strcmp("C", current_locale)) {
		day_of_week[0] = MGET("M");
		nonascii_string(day_of_week[0]);
		day_of_week[1] = MGET("T");
		nonascii_string(day_of_week[1]);
		day_of_week[2] = MGET("W");
		nonascii_string(day_of_week[2]);
		day_of_week[3] = MGET("R");
		nonascii_string(day_of_week[3]);
		day_of_week[4] = MGET("F");
		nonascii_string(day_of_week[4]);
		day_of_week[5] = MGET("Sa");
		nonascii_string(day_of_week[5]);
		day_of_week[6] = MGET("S");
		nonascii_string(day_of_week[6]);

		for ( k = 0;  k < 7;  k++ ) {
			str = euc_to_octal(day_of_week[k]);
			day_of_week[k] = (char *)malloc(sizeof(char) * cm_strlen(str) + 1);
			cm_strcpy(day_of_week[k], str);
		}
		malloc_memory = 1;
	} else {
		day_of_week[0] = MGET("M");
		day_of_week[1] = MGET("T");
		day_of_week[2] = MGET("W");
		day_of_week[3] = MGET("T");
		day_of_week[4] = MGET("F");
		day_of_week[5] = MGET("S");
		day_of_week[6] = MGET("S");
	}

	/* print abbreviated weekdays on top */
	fprintf (fp, "[(%s) (%s) (%s) (%s) (%s) (%s) (%s)] {\n", day_of_week[0],
			 day_of_week[1], day_of_week[2], day_of_week[3], day_of_week[4],
			 day_of_week[5], day_of_week[6]); 
	fprintf (fp, "    x2 y2 x2 s_boxw add 4 -1 roll ctr_text\n");
	fprintf (fp, "    /x2 x2 s_boxw add def\n");
	fprintf (fp, "} forall\n"); 

	/* print times at left of box */
	fprintf (fp, "/x2 LM tab add 3 sub def\n");
	fprintf (fp, "/y2 off_y1 def\n");
	/* adjust font scale according to number of partitions in sched box */
	if ( num_hours > 12  &&  num_hours <= 15 ) {
		reset_font = 1;
		fprintf (fp, "/DayFont.scale.old DayFont.scale def\n");
		fprintf (fp, "/DayFont.scale 10 def\n");
		fprintf (fp, "DayFont DayFont.scale font_set\n");
	} else if ( num_hours > 15 ) {
		reset_font = 1;
		fprintf (fp, "/DayFont.scale.old DayFont.scale def\n");
		fprintf (fp, "/DayFont.scale 9 def\n");
		fprintf (fp, "DayFont DayFont.scale font_set\n");
	}
	/* set up the hour(s) array */
	fprintf (fp, "[");
	begin_hr = p->begin_slider_VAL;
	for ( k = 0;  k <= num_hours;  k++ ) {
		/* 24 hr clock -- 24th hr = 0 */
		if ( p->default_disp_VAL == hour12  &&  begin_hr > 12 ) {
			begin_hr = 1;
		} else if ( p->default_disp_VAL == hour24  &&  begin_hr > 24 ) {
			begin_hr = 1;
		}
		fprintf (fp, "(%d)", begin_hr++);
	}
	fprintf (fp, "] {\n");
	fprintf (fp, "    x2 y2 3 -1 roll right_text\n");
	fprintf (fp, "    /y2 y2 s_boxh sub def\n");
	fprintf (fp, "} forall\n"); 
	if ( reset_font ) {    /* reset font scale to old value */
		fprintf (fp, "/DayFont.scale DayFont.scale.old def\n");
		fprintf (fp, "DayFont DayFont.scale font_set\n");
		reset_font = 0;
	}

	/* print horizontal internal lines (1/2 linewidth) */
	fprintf (fp, "BLACK setgray\n");
	fprintf (fp, "0.25 setlinewidth\n");
	fprintf (fp, "/x2 LM tab add def\n");
	fprintf (fp, "/y2 BM s_boxh add def\n");
	fprintf (fp, "%d {\n", num_hours - 1);   /* number of lines drawn */
	fprintf (fp, "    x2 y2 moveto\n");
	fprintf (fp, "    s_boxw 7 mul 0 rlineto\n");
	fprintf (fp, "    stroke\n");
	fprintf (fp, "    /y2 y2 s_boxh add def\n");
	fprintf (fp, "} repeat\n");

	/* print vertical internal lines */
	fprintf (fp, "1.0 setlinewidth\n");
	fprintf (fp, "/x2 LM tab add s_boxw add def\n");
	fprintf (fp, "/y2 BM def\n");
	fprintf (fp, "6 {\n");
	fprintf (fp, "    x2 BM moveto\n");
	fprintf (fp, "    x2 off_y1 lineto\n");
	fprintf (fp, "    stroke\n");
	fprintf (fp, "    /x2 x2 s_boxw add def\n");
	fprintf (fp, "} repeat\n");

	/* print bold box */
	fprintf (fp, "/x2 LM tab add def\n");
	fprintf (fp, "/y2 BM def\n");
	fprintf (fp, "/line_width 2 def\n");
	fprintf (fp, "x2 y2 s_boxw 7 mul s_boxh %d mul line_width draw_box\n", num_hours);

	/* set for ps_week_daynames() call (saves repeating code) */
	fprintf (fp, "\n%% --- print weekday strings ---\n");
	fprintf (fp, "/x2 LM def\n");
	fprintf (fp, "/y2 off_y tab_y sub 0.40 DayFont.scale mul add def\n");

	if ( malloc_memory ) {
		for ( k = 0;  k < 7;  k++ ) {
			free((char *)day_of_week[k]);
		}
	}
}

ps_week_daynames(fp, buf, more)
	FILE *fp;
	char *buf;
	Boolean more;
{
	char *str;
	char *buf_lower;
	int saturday;
	int sunday;

	/* STRING_EXTRACTION SUNW_DESKSET_CM_MSG
     * Note that the strings "Sat" and "Sun" begin with a capital letter.
     * Please translate accordingly.  If it is not the local custom to
     * capitalize these strings, then just use lower case.
	 */
    /*  Convert strings to lower case before making comparison so that
     *  the application is not dependent on case sensitive translation.
     */
	buf_lower = (char *)strdup(buf);
	lowercase(buf_lower);
	str = (char *)strdup(MGET("Sat"));
	lowercase(str);
	saturday = (strncmp(buf_lower, str, cm_strlen(str))==0);
	free(str);
	str = (char *)strdup(MGET("Sun"));
	lowercase(str);
	sunday   = (strncmp(buf_lower, str, cm_strlen(str))==0);
	free(buf_lower);
	free(str);

	if (saturday)
	{
		/* reset for weekend line */
		fprintf (fp, "\n%% --- print weekend strings ---\n");
		fprintf (fp, "/x2 LM boxw 3 mul add def\n");
		fprintf (fp, "/y2 y2 boxh sub def\n");
	}

	fprintf (fp, "DayFont DayFont.scale font_set\n");
	str = euc_to_octal(buf);
	fprintf (fp, "x2 y2 x2 boxw add (%s) ctr_text\n", str);
	fprintf (fp, "/x2 x2 boxw add def\n");

	fprintf (fp, "\n%% --- setup for printing appts ---\n");
	fprintf (fp, "/x1 x2 boxw sub def\n");
	if (saturday || sunday) {
		fprintf (fp, "/y1 BM def\n");
		fprintf (fp, "/line1 off_y tab_y sub font.y sub boxh sub def\n");
	}
	else {
		fprintf (fp, "/y1 off_y boxh sub def\n");
		fprintf (fp, "/line1 off_y tab_y sub font.y sub def\n");
	}
    	if (more)
		fprintf(fp, "4 0 rmoveto (%s) show\n", "*");

	fprintf (fp, "/tab1 5 def\n");
	fprintf (fp, "/tab2 tab1 def\n");
}

extern void
ps_week_sched_init()
{
	/* initialize the sched array */
	register int i;

	for (i = 0; i < 96; i++)
		sched_bucket[i] = 0;
}

static void
ps_week_sched_update(a, p)
	Abb_Appt *a;
	Props *p;
{
	/*
	 * In order to draw appts in the sched box,  all appts for a day are
	 * mapped into a array, and then drawn later.  The array has
	 * 96 elements, and represents the 95,  15 minute segements
	 * available between the day boundaries
	 */
	Tick	tick = a->appt_id.tick;
	register int i, start, end ;
	int end_slider;

	/*
	 * Determine the last element in the sched_buckets array which
	 * corresponds to this time range.
	 */

	end_slider = (p->end_slider_VAL - p->begin_slider_VAL) * 4;

	/* 
	 * Figure where the begin and end times should be in the array.
	 */

	start = ((hour(tick) - p->begin_slider_VAL) * 4) + (minute(tick) / 15);
	end   = start + ((a->duration) * 4 / 3600);

	/*
	 * Make sure that the appointment starts/ends within the visible
	 * time range.
	 */

	if (start < 0) start = 0;
	if (end < 0) end = 0;
	if (start > end_slider) start = end_slider;
	if (end > end_slider) end = end_slider;

	/* 
	 * Only map if some portion of time is between day boundaries.
	 */

	if ((start < 0 && end < 0) || 
	    (start >= end_slider && end >= end_slider))
		return;

	/* 
	 * Mark the blocks of time affected in the array.
	 */

	for (i = start; i < end; i++)
		sched_bucket[i]++;
}

extern void
ps_week_sched_draw(fp, wd)
	FILE *fp;
	int  wd; 	/* 1 = Mon, etc */
{
	/*
	 * Draw the sched appt blocks for the day
	 */
	register int i, last_color=0, curr_color=0;

	fprintf (fp, "\n%% --- filling in sched box for appt ---\n");

	/*
	 * The following calculations assume: 
	 *
	 * 	s_boxw, s_boxh (width and height of sched boxes)
	 * 	off_y1         (y coord of top left corner of * sched box) 
	 *
	 * are defined previously in the program.
	 */

	/*
	 * The way this is done:
         *   Position x,y to the top left corner of the appt grid.
         *   Check sched_buckets to see if we should be drawing anything.
         *   If not (sched_buckets is 0), then just move down one 15 minute
         *     increment on the grid (each element in the sched_buckets
         *     array represents one 15 minute increment).
         *   If sched_buckets is 1 (or 2), then we should begin shading
         *     in the area. Each appt is shaded in, in 15 minute increments.
         *   The shaded area is drawn by having the upper left x,y
         *     coordinate, the width and the height. You'll see in the
         *     postscript that is generated that the height is made negative
         *     since we are drawing from top to bottom, and we always have
         *     the upper left x,y coordinate.
         *   Outline the shaded region just drawn.
         */

	/* determine x coord 
         * NOTE: the 0.5 added to the x coordinate was determined mainly
         * by trial and error. This is definetely not the way this should
         * have been done, but it's too late to do it write and totally
         * rewrite the postscript that is generated.
	 */

	fprintf(fp, "/s_x LM tab add s_boxw %d mul add 0.5 add def\n", wd-1);

	/* determine y coord */
	fprintf(fp, "/s_y off_y1 def\n");

	/* determine width of box (minus borders) 
         * NOTE: Again, the 1.1 subtracted from the box width was
         * determined mainly by trial and error.
	 */
	fprintf(fp, "/s_width s_boxw 1.1 sub def\n");

	/* determine height of 15 minute box */
	fprintf(fp, "/s_height s_boxh 4 div def\n");

	/* 
	 * Fill in appt blocks for the day.  Remember to draw a line if
	 * there is a change in color from the previous draw.  This ends
	 * up in outlining the sched boxes with lines.
	 */
	fprintf (fp, "0.25 setlinewidth\n");
	for (i=0; i < 96; i++)
	{
		switch (sched_bucket[i])
		{
		case 0 :/* no appts - no appts to draw */
			curr_color = 0;
			break;

		case 1 :/* 1 appt - no time intersections */
			fprintf(fp, "s_x s_y s_width s_height neg make_gray\n");
			curr_color = 1;
			break;

		default:/* 2 or more appts - time intersects discovered */
			fprintf(fp, "s_x s_y s_width s_height neg 0.10 setgray rect fill BLACK setgray\n");
			curr_color = 2;
			break;

		}
		if (curr_color != last_color)
		{	/* draw line of demarcation */
			fprintf (fp, "s_x 1 sub s_y moveto\n");
			fprintf (fp, "s_boxw 0 rlineto\n");
			fprintf (fp, "stroke\n");
			last_color = curr_color;
		}
		fprintf(fp, "/s_y s_y s_height sub def\n");
	}

	/* redraw surrounding black box  & horiz lines */
	fprintf (fp, "BLACK setgray\n");
	fprintf (fp, "0.25 setlinewidth\n");
	fprintf (fp, "/s_x LM tab add def\n");
	fprintf (fp, "/s_y BM s_boxh add def\n");
	fprintf (fp, "%d {\n", num_hours - 1);   
	fprintf (fp, "    s_x s_y moveto\n");
	fprintf (fp, "    s_boxw 7 mul 0 rlineto\n");
	fprintf (fp, "    stroke\n");
	fprintf (fp, "    /s_y s_y s_boxh add def\n");
	fprintf (fp, "} repeat\n");

	fprintf (fp, "/s_x LM tab add def\n");
	fprintf (fp, "/s_y BM def\n");
	fprintf (fp, "/line_width 2 def\n");
	fprintf (fp, "s_x s_y s_boxw 7 mul s_boxh %d mul line_width draw_box\n", num_hours);
}


/* 
 * 	print standard year routines
 */

extern void
ps_init_std_year (fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- init for std year ---\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM 0.5 inch sub def\n");
}

extern void
ps_std_year_name (fp, year)
	FILE *fp;
	int  year; 
{
	char *str;

#ifdef NEVER
	/* print sun logo at left margin */ 
	fprintf (fp, "LM 20 add y1 4 Sunlogo\n"); 
#endif
	fprintf (fp, "StampFont StampFont.scale font_set\n"); 

	/* print timestamp at left margin */ 
	fprintf (fp, "LM 8 add y1 font.y sub (%s) left_text\n", timestamp); 

	/* print user@host stamp at right margin */ 
	fprintf (fp, "RM y1 font.y sub (%s) right_text\n", calendar->view->current_calendar); 

	str = MGET("Year view by Calendar Manager");
	nonascii_string(str);
	str = euc_to_octal(str);

	/* print creation notice at bottom */ 
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str); 

	/* set scale to 2x, y normal */
	ps_scale(fp, 2.0, 1.0);

	fprintf (fp, "\n%% --- std print month name centered at top ---\n");
	fprintf (fp, "MonthFont MonthFont.scale font_set\n"); 

	/* Since we're scaling 2x in x dim, set LM & RM accordingly */
	fprintf (fp, "LM 2 div y1 RM 2 div (%d) ctr_text \n", year); 
	
	/* Set scaling back */
	ps_scale(fp, 1.0/2.0, 1.0);

	fprintf (fp, "0 font.x 2 div neg translate\n"); 
}

extern void
ps_init_std_month (fp, wid, hgt)
	FILE *fp;
	int  wid, hgt;
{
	fprintf (fp, "\n%% --- init vars for std month ---\n");
	fprintf (fp, "/old_TM TM def\n");
	fprintf (fp, "/old_LM LM def\n");
	fprintf (fp, "/old_RM RM def\n");
	fprintf (fp, "/old_BM BM def\n");
	fprintf (fp, "/old_x1 x1 def\n");
	fprintf (fp, "/old_y1 y1 def\n\n");

	fprintf (fp, "/TM 5 def\n");
	fprintf (fp, "/LM 5 def\n");
	fprintf (fp, "/RM %d def\n", wid-5);
	fprintf (fp, "/BM %d def\n", hgt-5);
	fprintf (fp, "/Datewidth RM LM sub 7 div def\n");
	fprintf (fp, "/Monwidth  RM LM sub def\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
}

extern void
ps_finish_std_month (fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- finish std month - restore old vars  ---\n");
	fprintf (fp, "/TM old_TM def\n");
	fprintf (fp, "/LM old_LM def\n");
	fprintf (fp, "/RM old_RM def\n");
	fprintf (fp, "/BM old_BM def\n");
	fprintf (fp, "/x1 old_x1 def\n");
	fprintf (fp, "/y1 old_y1 def\n");
}

extern void
ps_std_month_name (fp, mon)
	FILE *fp;
	char *mon; 
{
	char *str;

	fprintf (fp, "\n%% --- print month name centered at top ---\n");
	fprintf (fp, "MonthFont MonthFont.scale font_set\n"); 
	fprintf (fp, "/dx font.x 0.3 mul def\n");
	fprintf (fp, "/dy font.y 0.3 mul def\n");
	fprintf (fp, "x1 dx add y1 dy sub Monwidth font.y dy add make_lt_gray\n");
	nonascii_string(mon);
	str = euc_to_octal(mon);
	fprintf (fp, "LM y1 RM (%s) ctr_text \n", str); 
}

extern void
ps_std_month_weekdays (fp)
	FILE *fp;
{
	char *str;
	char *day_of_week[7];
	int k;
	char *current_locale;
	int malloc_memory = 0;

	current_locale = (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG);
	if (strcmp("C", current_locale)) {
		day_of_week[0] = MGET("S");
		nonascii_string(day_of_week[0]);
		day_of_week[1] = MGET("M");
		nonascii_string(day_of_week[1]);
		day_of_week[2] = MGET("T");
		nonascii_string(day_of_week[2]);
		day_of_week[3] = MGET("W");
		nonascii_string(day_of_week[3]);
		day_of_week[4] = MGET("R");
		nonascii_string(day_of_week[4]);
		day_of_week[5] = MGET("F");
		nonascii_string(day_of_week[5]);
		day_of_week[6] = MGET("Sa");
		nonascii_string(day_of_week[6]);
		for ( k = 0;  k < 7;  k++ ) {
			str = euc_to_octal(day_of_week[k]);
			day_of_week[k] = (char *)malloc(sizeof(char) * cm_strlen(str) + 1);
			cm_strcpy(day_of_week[k], str);
		}
		malloc_memory = 1;
	} else {
		day_of_week[0] = MGET("S");
		day_of_week[1] = MGET("M");
		day_of_week[2] = MGET("T");
		day_of_week[3] = MGET("W");
		day_of_week[4] = MGET("T");
		day_of_week[5] = MGET("F");
		day_of_week[6] = MGET("S");
	}

	fprintf (fp, "\n%% --- print std month weekdays ---\n");
	fprintf (fp, "DayFont DayFont.scale font_set\n"); 
	fprintf (fp, "CR /t%% do a carriage return with new font size\n");
	fprintf (fp, "[(%s) (%s) (%s) (%s) (%s) (%s) (%s)] {\n", day_of_week[0],
			 day_of_week[1], day_of_week[2], day_of_week[3], day_of_week[4],
			 day_of_week[5], day_of_week[6]); 
	fprintf (fp, "    /x1 x1 Datewidth add def\n");
	fprintf (fp, "    x1 y1 3 -1 roll right_text\n");
	fprintf (fp, "} forall CR\n"); 

	if ( malloc_memory ) {
		for ( k = 0;  k < 7;  k++ ) {
			free((char *)day_of_week[k]);
		}
	}
}

extern void
ps_std_month_dates (fp, fdom, monlen) 
	FILE *fp;
	int  fdom;	/* first day of month, 0 = Sun */
	int  monlen;	/* length of month */
{
	fdom++;
	fprintf (fp, "\n%% --- print std month dates ---\n");
	fprintf (fp, "DateFont DateFont.scale font_set\n"); 
	fprintf (fp, "/dow %d def\n",fdom); 
	fprintf (fp, "/date 1 def\n"); 
	fprintf (fp, "/numstr 2 string def\n"); 

	/* be sure to incr x offset by dow */
	fprintf (fp, "/x1 Datewidth dow mul x1 add def\n");

	fprintf (fp, "%d { \n", monlen);
	fprintf (fp, "    x1 y1 date numstr cvs right_text\n");
	fprintf (fp, "    dow 7 eq\n");
	fprintf (fp, "    { /dow 1 def CR }\t%% start on next line\n");
	fprintf (fp, "    { /dow dow 1 add def } ifelse \n");
	fprintf (fp, "    /x1 x1 Datewidth add def\n");
	fprintf (fp, "    /date date 1 add def\n"); 
	fprintf (fp, "} repeat\n\n");
}


/* 
 * 	print alternate year routines
 */

extern void
ps_init_alt_year (fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- init for alt year ---\n");
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
}

extern void
ps_alt_year_name (fp, year)
	FILE *fp;
	int  year; 
{
	char *str;

#ifdef NEVER
	/* print sun logo at left margin */ 
	fprintf (fp, "LM 20 add y1 4 Sunlogo\n"); 
#endif
	fprintf (fp, "StampFont StampFont.scale font_set\n"); 

	/* print timestamp at left margin */ 
	fprintf (fp, "LM 45 add y1 font.y sub (%s) left_text\n", timestamp); 

	/* print user@host stamp at right margin */ 
	fprintf (fp, "RM y1 font.y sub (%s) right_text\n", calendar->view->current_calendar); 

	str = MGET("Year view by Calendar Manager");
	nonascii_string(str);
	str = euc_to_octal(str);
	/* print creation notice at bottom */ 
	fprintf (fp, "RM BM font.y sub (%s) right_text \n", str); 

	/* set scale to 4x, y normal */
	ps_scale(fp, 4.0, 1.0);

	fprintf (fp, "\n%% --- print year name centered at top ---\n");
	fprintf (fp, "MonthFont MonthFont.scale font_set\n"); 

	/* Since we're scaling 4x in x dim, set LM & RM accordingly */
	fprintf (fp, "LM 4 div y1 RM 4 div (%d) ctr_text CR \n", year); 
	
	/* Set scaling back */
	ps_scale(fp, 1.0/4.0, 1.0);
}

extern void
ps_init_alt_month (fp, wid, hgt)
	FILE *fp;
	int  wid, hgt;
{
	fprintf (fp, "\n%% --- init vars for alt month ---\n");
	fprintf (fp, "/old_TM TM def\n");
	fprintf (fp, "/old_LM LM def\n");
	fprintf (fp, "/old_RM RM def\n");
	fprintf (fp, "/old_BM BM def\n");
	fprintf (fp, "/old_x1 x1 def\n");
	fprintf (fp, "/old_y1 y1 def\n\n");

	fprintf (fp, "/TM 0 def\n");
	fprintf (fp, "/LM 0 def\n");
	fprintf (fp, "/RM %d def\n", wid);
	fprintf (fp, "/BM %d def\n", hgt);
	fprintf (fp, "/x1 LM def\n");
	fprintf (fp, "/y1 TM def\n");
}

extern void
ps_finish_alt_month (fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- finish alt month - restore old vars  ---\n");
	fprintf (fp, "/TM old_TM def\n");
	fprintf (fp, "/LM old_LM def\n");
	fprintf (fp, "/RM old_RM def\n");
	fprintf (fp, "/BM old_BM def\n");
	fprintf (fp, "/x1 old_x1 def\n");
	fprintf (fp, "/y1 old_y1 def\n");
}

extern void
ps_alt_month_name (fp, mon)
	FILE *fp;
	char *mon; 
{
	char *str;

	fprintf (fp, "\n%% --- print month name on left side ---\n");
	fprintf (fp, "MonthFont MonthFont.scale font_set\n"); 
	fprintf (fp, "/y_offset BM TM sub 2 div font.y 2 div add def \n", mon); 
	nonascii_string(mon);
	str = euc_to_octal(mon);
	fprintf (fp, "x1 y1 y_offset sub (%s) left_text \n", str); 
	
}

extern void
ps_alt_month_weekdays (fp)
	FILE *fp;
{
	char *str;
	char *day_of_week[7];
	int k;
	char *current_locale;
	int malloc_memory = 0;

	current_locale = (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG);
	if (strcmp("C", current_locale)) { 
		day_of_week[0] = MGET("S");
		nonascii_string(day_of_week[0]);
		day_of_week[1] = MGET("M");
		nonascii_string(day_of_week[1]);
		day_of_week[2] = MGET("T");
		nonascii_string(day_of_week[2]);
		day_of_week[3] = MGET("W");
		nonascii_string(day_of_week[3]);
		day_of_week[4] = MGET("R");
		nonascii_string(day_of_week[4]);
		day_of_week[5] = MGET("F");
		nonascii_string(day_of_week[5]);
		day_of_week[6] = MGET("Sa");
		nonascii_string(day_of_week[6]);
		for ( k = 0;  k < 7;  k++ ) {
			str = euc_to_octal(day_of_week[k]);
			day_of_week[k] = (char *)malloc(sizeof(char) * cm_strlen(str) + 1);
			cm_strcpy(day_of_week[k], str);
		}
		malloc_memory = 1;
	} else {
		day_of_week[0] = MGET("S");
		day_of_week[1] = MGET("M");
		day_of_week[2] = MGET("T");
		day_of_week[3] = MGET("W");
		day_of_week[4] = MGET("T");
		day_of_week[5] = MGET("F");
		day_of_week[6] = MGET("S");
	}

	fprintf (fp, "\n%% --- set some vars for printing weekdays ---\n");
	/* set x_offset based on month font */
	/* This needs to be calculated more cleverly.  Right now it is making
	 * assumption about Asian font size.  But better than nothing.
	 */
	if ( prolog_found ) {
		fprintf (fp, "/x_offset font.x 3.5 mul LM add def\n"); 
	} else {
		fprintf (fp, "/x_offset font.x 2.5 mul LM add def\n"); 
	}
	fprintf (fp, "DayFont DayFont.scale font_set\n"); 
	fprintf (fp, "/box_wid RM x_offset sub 37 div def\n"); 
	fprintf (fp, "/box_hgt TM BM sub def\n"); 
	fprintf (fp, "/x1 x_offset def\n"); 
	fprintf (fp, "/DayList \n"); 
	fprintf (fp, "    [(%s)(%s)(%s)(%s)(%s)(%s)(%s)] def \n", day_of_week[0],
			 day_of_week[1], day_of_week[2], day_of_week[3], day_of_week[4],
			 day_of_week[5], day_of_week[6]); 	/* zero based */
	fprintf (fp, "/day 0 def \n"); 

	fprintf (fp, "\n%% --- draw weekdays on top ---\n");
	fprintf (fp, "37 {\n");
	fprintf (fp, "    x1  y1 2 add x1 box_wid add DayList day 7 mod get ctr_text\n"); 
	fprintf (fp, "    /x1 x1 box_wid add def\n"); 
	fprintf (fp, "    /day day 1 add def\n"); 
	fprintf (fp, "} repeat\n");

	if ( malloc_memory ) {
		for ( k = 0;  k < 7;  k++ ) {
			free((char *)day_of_week[k]);
		}
	}
}

extern void
ps_alt_month_boxes (fp)
	FILE *fp;
{
	fprintf (fp, "\n%% --- set some vars for printing boxes ---\n");
	fprintf (fp, "/x1 x_offset def\n"); 

	fprintf (fp, "\n%% --- draw boxes to right of month name  ---\n");
	fprintf (fp, "37 {\n");
	fprintf (fp, "    x1 y1 box_wid box_hgt make_gray \n"); 
	fprintf (fp, "    x1 y1 box_wid box_hgt 0.5 draw_box \n"); 
	fprintf (fp, "    /x1 x1 box_wid add def\n"); 
	fprintf (fp, "} repeat\n");
}

extern void
ps_alt_month_dates (fp, fdom, monlen)
	FILE *fp;
	int  fdom;	/* first day of month, 0 = SUN, 1 = MON, ... */
	int  monlen;	/* length of month */
{
	fprintf (fp, "\n%% --- print month dates in boxes ---\n");
	fprintf (fp, "DateFont DateFont.scale font_set\n"); 
	fprintf (fp, "/by y1 0.5 sub def \n"); 
	fprintf (fp, "/bw box_wid 1 sub def \n");
	fprintf (fp, "/bh box_hgt 1 add def \n"); 
	fprintf (fp, "/border 2 def \n"); 
	fprintf (fp, "/y1 y1 font.y sub border sub def \n"); 

	/* remember to offset by dow also */
	fprintf (fp, "/x1 x_offset border add box_wid %d mul add def\n",fdom); 
	fprintf (fp, "/bx x_offset 0.5 add box_wid %d mul add def \n",fdom); 
	fprintf (fp, "/dom %d def \n", fdom++); 
	fprintf (fp, "/date 1 def \n"); 
	fprintf (fp, "/nstr 2 string def \n"); 

	fprintf (fp, "%d { \n", monlen); 
	/* if dow == sun or sat */
	fprintf (fp, "    dom 7 mod 6 eq dom 7 mod 0 eq or\n"); 
	fprintf (fp, "    { bx by bw bh make_lt_gray }\n");
	fprintf (fp, "    { bx by bw bh clear_box } ifelse\n");

	fprintf (fp, "    x1 y1 date nstr cvs left_text \n"); 
	fprintf (fp, "    /x1 x1 box_wid add def\n"); 
	fprintf (fp, "    /bx bx box_wid add def\n"); 
	fprintf (fp, "    /dom dom 1 add def\n"); 
	fprintf (fp, "    /date date 1 add def\n"); 
	fprintf (fp, "} repeat \n"); 
}


int token_count(str)
char *str;
{
	int count = 0;

	while (*str) {
		if ( *str == '\\' ) {
			switch (*(str+1)) {
				case 'n':
				case 'r':
				case 't':
				case 'b':
				case 'f':
				case '(':
				case ')':
				case '\\':
					count++;	
					break;	
				case '0': case '1': case '2': case '3': case '4': case '5':
				case '6': case '7':
					if (*(str+2) >= '0' && *(str+1) < '8' &&
						*(str+3) >= '0' && *(str+2) < '8' &&
						*(str+3)) {
						count++;
					}
			}
		} else if ( *str == '('  ||  *str == ')' ) {
			count++;
		}
		str++;
	}
	return count;
}


/* Put '\' in front of escape sequences */

char *fix_ps_string(str)
char *str;
{
	int num_occur;
	char *fixed_str;
	int fixed_str_len = 0;
	int str_len = 0;
	int str_pos = 0;
	int fixed_pos = 0;

	num_occur = token_count(str);
	if (num_occur) {
		str_len = cm_strlen(str);
		fixed_str_len = CHAR_SIZE * str_len + num_occur;
		fixed_str = (char *)malloc(fixed_str_len + 1);
		for (str_pos = 0; str_pos < str_len; str_pos++, fixed_pos++) {
			if ( *(str+str_pos) == '\\' ) {
				switch (*(str+str_pos+1)) { 
					case 'n':
					case 'r':
					case 't':
					case 'b':
					case 'f':
					case '(':
					case ')':
					case '\\':
						*(fixed_str+fixed_pos) = '\\';
						fixed_pos += 1;
						break;	
					case '0': case '1': case '2': case '3': case '4': case '5':
					case '6': case '7':
						if (*(str+str_pos+2) >= '0' && *(str+str_pos+2) < '8' &&
							*(str+str_pos+3) >= '0' && *(str+str_pos+3) < '8' &&
							str_pos + 3 < str_len) {
							*(fixed_str+(fixed_pos++)) = '\\';
							*(fixed_str+(fixed_pos++)) = *(str+(str_pos++));
							*(fixed_str+(fixed_pos++)) = *(str+(str_pos++));
							*(fixed_str+(fixed_pos++)) = *(str+(str_pos++));
						}
						break;
				}
			} else if ( *(str+str_pos) == '(' || *(str+str_pos) == ')' ) {
				/* Prevent mismatch parentheses */
				*(fixed_str+fixed_pos) = '\\';
				fixed_pos += 1;
			}
			*(fixed_str+fixed_pos) = *(str+str_pos);
		}
		*(fixed_str+fixed_pos) = '\0';
		return fixed_str;
	}
	return str;
}


static void
nonascii_string(str)
char *str;
{
	int str_len;
	int k;

	if ( !nonascii_char ) {
		str_len = cm_strlen(str);
		for ( k = 0;  k < str_len;  k++ ) {
			if ( !isascii(str[k]) ) {
				nonascii_char = 1;
				break;
			}
		}
	}
}
