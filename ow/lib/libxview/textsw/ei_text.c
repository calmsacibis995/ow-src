#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)ei_text.c 20.88 95/01/31";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Entity interpreter for ascii characters interpreted as plain text.
 */

#define USING_SETS
#include <xview_private/primal.h>

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>

#ifdef __STDC__ 
#ifndef CAT
#define CAT(a,b)        a ## b 
#endif 
#endif
#include <pixrect/memvar.h>

#include <pixrect/pixfont.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview/rect.h>
#include <xview/rectlist.h>
#include <xview/font.h>
#include <xview/defaults.h>
#include <xview/pixwin.h>
#include <xview_private/ev.h>
#include <xview/window.h>
#ifdef OW_I18N
#include <xview/server.h>
#include <euc.h>
#endif

Pkg_private Es_index es_backup_buf();

#define	NEWLINE	'\n'
#define	SPACE	' '
#define NB_SPACE 0240
#define	TAB	'\t'

typedef struct ei_plain_text_object {
#ifdef OW_I18N
    Xv_Font 	    font;
#else
    PIXFONT        *font;
#endif
    XFontStruct	   *x_font_info;	/* Perf. Cache font info */
    Pixfont	   *pf_font;
    unsigned        state;
    struct pr_pos   font_home;	/* == MIN all homes(font) */
    int             font_flags;
    int		    height;
    short           tab_width;	/* tab width in 'm's, used when */
    /* more tabs than num_tab_stops */
    short           tab_pixel;	/* tab width in pixels, used ... */
    short           tab_delta_y;/* actually ' ' delta y */
    short           num_tab_stops;
    short           max_tab_stops;	/* Never decreases */
    short          *tab_pixels;	/* tab stops in pixels */
    short          *tab_widths;	/* tab stops in 'm's */
#ifdef OW_I18N
    struct pixchar	mb_pc;	 /* for multibyte character */
    struct pixrect      mb_pr;	 /* for multibyte character */
    struct pixchar	kana_pc; /* for half size kana in ja locale */
    struct pixrect      kana_pr; /* for half size kana in ja locale */
    int			locale_is_ale;
#endif    
}               ei_plain_text_object;
typedef ei_plain_text_object *Eipt_handle;
#define	ABS_TO_REP(eih)	(Eipt_handle)eih->data
/*
 * Note: font_home, font_flags, tab_pixel, tab_pixels[], and tab_delta_y are
 * cached information which must be updated when the font changes.
 * Additionally, changes to tab_width require tab_pixel to be recomputed,
 * similarly tab_widths[] force new tab_pixels[].
 */
/* state values */
#define CONTROL_CHARS_USE_FONT	0x0000001
/* font_flags values */
#define FF_POSITIVE_X_ADVANCE	0x0000001
#define FF_UNIFORM_HEIGHT	0x0000002
#define FF_UNIFORM_HOME		0x0000004
#define FF_UNIFORM_X_ADVANCE	0x0000008
#define FF_UNIFORM_X_PR_ADVANCE	0x0000010
#define FF_ZERO_Y_ADVANCE	0x0000020
#define	FF_ALL			0x000003f
#define FF_EASY_Y		(FF_UNIFORM_HEIGHT|FF_UNIFORM_HOME| \
				 FF_ZERO_Y_ADVANCE)

#define EI_OP_CLEAR_ALL EI_OP_CLEAR_FRONT|EI_OP_CLEAR_INTERIOR|EI_OP_CLEAR_BACK

Pkg_private Ei_handle ei_plain_text_create();
Pkg_private int ei_plain_text_line_height();
static Ei_handle ei_plain_text_destroy();
static caddr_t  ei_plain_text_get();
static int      ei_plain_text_lines_in_rect();
static struct ei_process_result ei_plain_text_process();
int             ei_plain_text_set();
static void	ei_plain_text_set_mb_char();
static struct ei_span_result ei_plain_text_span_of_group();
static struct ei_process_result ei_plain_text_expand();

struct ei_ops   ei_plain_text_ops = {
    ei_plain_text_destroy,
    ei_plain_text_get,
    ei_plain_text_line_height,
    ei_plain_text_lines_in_rect,
    ei_plain_text_process,
    ei_plain_text_set,
    ei_plain_text_span_of_group,
    ei_plain_text_expand
};

/* Used in ei_plain_text_process. Init adv.y = 0 once and for all. */
static struct pixchar dummy_for_tab;

static int
ei_plain_text_set_tab_width(eih, tab_width)
    Ei_handle       eih;
    register int    tab_width;
{
    register Eipt_handle private = ABS_TO_REP(eih);

    private->tab_width = tab_width;
    if (private->x_font_info->per_char)  {
        private->tab_pixel =
	  private->x_font_info->per_char['m' - 
              private->x_font_info->min_char_or_byte2].width * tab_width;
    } else  {
        private->tab_pixel =
	    private->x_font_info->min_bounds.width * tab_width;
    }
    if (private->tab_pixel == 0)
	private->tab_pixel = 1;
}

static int
ei_plain_text_set_tab_widths(eih, widths, adjust_for_font)
    Ei_handle       eih;
    int            *widths;
    int             adjust_for_font;
/*
 * Passing widths == (int *)0 will remove the tab stops unless
 * adjust_for_font is TRUE, in which case the pixel version of the stops will
 * be recomputed from the existing widths and (new) font.
 */
{
    register Eipt_handle private = ABS_TO_REP(eih);
    register        i, factor;

    if (widths && widths[0] > 0) {
	/* Count the number of widths passed and ensure space for them. */
	for (i = 0; widths[i] > 0; i++) {
	}
	if (private->max_tab_stops < i) {
	    if (private->max_tab_stops > 0) {
		free((char *) private->tab_pixels);
		free((char *) private->tab_widths);
	    }
	    private->tab_pixels = (short *) malloc(i * sizeof(short));
	    private->tab_widths = (short *) malloc(i * sizeof(short));
	    if (private->tab_pixels == (short *) 0 ||
		private->tab_widths == (short *) 0) {
		/* We are out of malloc space. */
		private->max_tab_stops = private->num_tab_stops = 0;
		return (1);
	    } else {
		private->max_tab_stops = i;
	    }
	}
	private->num_tab_stops = i;
	for (i = 0; i < private->num_tab_stops; i++) {
	    private->tab_widths[i] = widths[i];
	}
    } else if (!adjust_for_font) {
	private->num_tab_stops = 0;
    }
    if (private->x_font_info->per_char)  {
        factor = private->x_font_info->per_char['m' - 
                   private->x_font_info->min_char_or_byte2].width;
    } else  {
        factor = private->x_font_info->min_bounds.width;
    }
    for (i = 0; i < private->num_tab_stops; i++) {
	private->tab_pixels[i] = factor * private->tab_widths[i];
    }
    return (0);
}

static void
ei_plain_text_set_mb_char(eih)
    Ei_handle       eih;
{
    register Eipt_handle private = ABS_TO_REP(eih);
    XRectangle	overall_ink_extents, overall_logical_extents;
    XFontSet	font_set;
    char	mb_str[3];
    wchar_t	wc_str[3];

    mb_str[0] = 0xa1; /* 0xa1a1: first full size character */
    mb_str[1] = 0xa1;
    mb_str[2] = 0;

    if (mbstowcs(wc_str, mb_str, 2) != 1) { /* non Asian locale */
	private->locale_is_ale = 0;
	return;
    }
    private->locale_is_ale = 1;

    /* Set a sample pixchar for multibyte character */
    font_set = (XFontSet)xv_get((Xv_opaque)private->font, FONT_SET_ID);
    (void) XwcTextExtents(font_set, wc_str, 1,
			  &overall_ink_extents, &overall_logical_extents);
    private->mb_pc.pc_pr = &private->mb_pr;
    private->mb_pc.pc_home.x = overall_logical_extents.x;
    private->mb_pc.pc_home.y = overall_logical_extents.y;
    private->mb_pc.pc_pr->pr_size.x = private->mb_pc.pc_adv.x = 
                                          overall_logical_extents.width;
    private->mb_pc.pc_pr->pr_size.y = private->mb_pc.pc_adv.y = 0;

    /* Set a sample pixchar for half size kana in japanese locale */
    mb_str[0] = 0x8e; /* SS2 */
    mb_str[1] = 0xa1;
    mb_str[2] = 0;
    if (mbstowcs(wc_str, mb_str, 2) == 1) { /* locale is ja */
	(void) XwcTextExtents(font_set, wc_str, 1,
			      &overall_ink_extents, &overall_logical_extents);
	private->kana_pc.pc_pr = &private->kana_pr;
	private->kana_pc.pc_home.x = overall_logical_extents.x;
	private->kana_pc.pc_home.y = overall_logical_extents.y;
	private->kana_pc.pc_pr->pr_size.x = private->kana_pc.pc_adv.x =
                               overall_logical_extents.width;
	private->kana_pc.pc_pr->pr_size.y = private->kana_pc.pc_adv.y = 0;
    }
    else { /* Set dummy value for some error conditions in other ale locales */
	private->kana_pc = private->pf_font->pf_char['m'];
    }
}

static int
ei_plain_text_set_font(eih, font)
    Ei_handle       eih;
    register Xv_Font font;
{
    register Eipt_handle private = ABS_TO_REP(eih);
    Pixfont	*tempPf;
    struct pixchar *pc;
    register short  i, height, home, adv_x;
    int		max_char, min_char;
    XFontSetExtents	*font_set_extents;

    font_set_extents =
	XExtentsOfFontSet((XFontSet)xv_get((Xv_opaque)font, FONT_SET_ID));
    tempPf = (Pixfont *)xv_get((Xv_opaque)font, FONT_PIXFONT);
    pc = &tempPf->pf_char[SPACE];
    /* if Xfont does not have a glyph for space */
    if (pc->pc_pr->pr_size.x == 0 && pc->pc_pr->pr_size.y == 0)
	pc = &tempPf->pf_char['n'];
    private->font = font;
    private->font_home.x = 0;
    private->x_font_info = (XFontStruct *)xv_get((Xv_opaque)font, FONT_INFO);
    private->height = xv_get((Xv_opaque)font, FONT_DEFAULT_CHAR_HEIGHT);
    ei_plain_text_set_tab_width(eih, private->tab_width);
    ei_plain_text_set_tab_widths(eih, (int *) 0, TRUE);
    private->pf_font = tempPf;
    if (multibyte)
        ei_plain_text_set_mb_char(eih);
    height = font_set_extents->max_logical_extent.height;
    home = font_set_extents->max_logical_extent.y;
    private->tab_delta_y = home + height;
                   /* Assume that this is >= 0 */
    adv_x = font_set_extents->max_logical_extent.width; 
    private->font_flags = FF_ALL;
    max_char = MIN(255, private->x_font_info->max_char_or_byte2);
    min_char = MIN(255, private->x_font_info->min_char_or_byte2);

    for (i = min_char; i <= MIN(255, max_char); i++) {
	pc = &tempPf->pf_char[i];
	if (adv_x != pc->pc_adv.x) {
	    if (pc->pc_pr) {
		private->font_flags &=
		    ~(FF_UNIFORM_X_ADVANCE | FF_UNIFORM_X_PR_ADVANCE);
	    } else {
		private->font_flags &= ~FF_UNIFORM_X_ADVANCE;
	    }
	    if (adv_x < 0) {
		private->font_flags &= ~FF_POSITIVE_X_ADVANCE;
	    }
	}
	if (pc->pc_adv.y != 0) {
	    private->font_flags &= ~FF_ZERO_Y_ADVANCE;
	}
	if (pc->pc_pr) {
	    /* Home is meaningless unless pixrect exists for char. */
	    if (home != pc->pc_home.y) {
		private->font_flags &= ~FF_UNIFORM_HOME;
		/* Accumulate largest (magnitude, homes are < 0) home */
		if (home > pc->pc_home.y) {
		    home = pc->pc_home.y;
		}
	    }
	    if (height != pc->pc_pr->pr_size.y) {
		private->font_flags &= ~FF_UNIFORM_HEIGHT;
	    }
	}
    }
    private->font_home.y = home;
#ifdef DEBUG
    (void) fprintf(stderr, "Font_flags: %lx\n", private->font_flags);
#endif
}

Pkg_private     Ei_handle
ei_plain_text_create()
{
    register Ei_handle eih = NEW(struct ei_object);
    Eipt_handle     private;

    if (eih == 0)
	goto AllocFailed;
    private = NEW(ei_plain_text_object);
    if (private == 0) {
	free((char *) eih);
	goto AllocFailed;
    }
    eih->ops = &ei_plain_text_ops;
    eih->data = (caddr_t) private;
    private->tab_width = 8;
    return (eih);

AllocFailed:
    return (NULL);
}

static          Ei_handle
ei_plain_text_destroy(eih)
    Ei_handle       eih;
{
    register Eipt_handle private = ABS_TO_REP(eih);

    free((char *) eih);
    free((char *) private);
    return NULL;
}

Pkg_private int
ei_plain_text_line_height(eih)
    Ei_handle       eih;
{
    register Eipt_handle private = ABS_TO_REP(eih);
    XFontStruct	   *font_info;
    int		    max_char_height;
    int		    percent;
    int		    spacing;

    percent = defaults_get_integer("text.lineSpacing", "Text.LineSpacing", 0);
    if (percent > 0) {
	font_info = (XFontStruct *) xv_get((Xv_opaque) private->font,
	    FONT_INFO);
	max_char_height = font_info->max_bounds.ascent +
	    font_info->max_bounds.descent;
	spacing = max_char_height*percent/100;
	if ((max_char_height*percent)%100 > 0 || spacing == 0)
	    spacing++;  /* round up, or enforce a minimum of 1 pixel */
	return (max_char_height + spacing);
    } else {
 	return (private->height);
    }
}

static int
ei_plain_text_lines_in_rect(eih, rect)
    Ei_handle       eih;
    struct rect    *rect;
/*
 * Returns the number of complete lines that will fit in the rect. Any
 * partial line is ignored; call with one bit shorter rect to check if
 * partial exists.
 */
{
    register int    line_height = ei_line_height(eih);
    int             result = rect->r_height / line_height;

    return (result < 0 ? 0 : result);
}

static u_short  gray17_data[16] = {	/* really 16-2/3	 */
    0x8208, 0x2082, 0x0410, 0x1041, 0x4104, 0x0820, 0x8208, 0x2082,
    0x0410, 0x1041, 0x4104, 0x0820, 0x8208, 0x2082, 0x0410, 0x1041
};

mpr_static(gray17_pr, 12, 12, 1, gray17_data);

typedef struct run {
    void           *chars;	/* Pointer to characters to be painted */
    short           len;	/* Count of characters to be painted */
    short           x, y;	/* baseline origin to start painting at */
}               Run;

#define	INIT_RUN(_run, _x, _y)	\
	(_run)->chars = NULL; (_run)->len = 0; (_run)->x = _x; (_run)->y = _y;

static void
finalize_run(run, run_array, batch, batch_array)
Run *run, *run_array;
void *batch, *batch_array;
{
    if ( run == run_array )
            run->chars = batch_array;
    else 
        if (!multibyte)
            run->chars = (char *)((run-1)->chars) + (run-1)->len;
        else
            run->chars = (wchar_t *)((run-1)->chars) + (run-1)->len;
    if (!multibyte)
        run->len = (char *)batch - (char *)(run->chars);
    else
        run->len = (wchar_t *)batch - (wchar_t *)(run->chars);
}

/*
 * The following macros (suggested by JAG) make sure the compiler keeps all
 * the involved quantities as short's.
 */

#define	SAdd(_a, _b)	(short)((short)(_a) + (short)(_b))
#define	SSub(_a, _b)	(short)((short)(_a) - (short)(_b))
#define	SRect_edge(_a, _b)	\
			(short)((short)((short)(_a) + (short)(_b)) - (short)1)

#define MAX_PER_BATCH 1024
static struct ei_process_result
eiPlainTextProcessSB(eih, op, esbuf, x, y, rop, pw, rect, tab_origin)
    Ei_handle       eih;
    int             op;
    Es_buf_handle   esbuf;
    int             x, y, rop;
    Xv_Window       pw;
    register struct rect *rect;
    int             tab_origin;
{
    register Eipt_handle private = ABS_TO_REP(eih);
    register struct pixchar *pc;
    register short  c;
    register short  temp;
    register Es_index esi;
    char          *buf_rep = (char *) esbuf->buf;
    struct ei_process_result result;
    register short  bounds_right, rects_right;
    short           bounds_bottom, rects_bottom;
    short           in_white_space = 0, special_char = -1;
    register int    check_vert_bounds = TRUE;

    Run             run_array[MAX_PER_BATCH + 1];
    Run            *run;
    char            batch_array[MAX_PER_BATCH + 1];
    char           *batch;
    int             ii;
    Pixfont	   *tempPf;

    temp = (short) x;
    result.bounds.r_left = temp;
    bounds_right = temp;
    result.pos.x = SSub(temp, private->font_home.x);
    result.bounds.r_width = 0;
    temp = (short) y;
    result.bounds.r_top = temp;

    bounds_bottom = SSub(SAdd(temp, private->height), 1);
    result.pos.y = SSub(temp, private->font_home.y);
    rects_right = SRect_edge(rect->r_left, rect->r_width);
    rects_bottom = SRect_edge(rect->r_top, rect->r_height);
    result.break_reason = EI_PR_BUF_EMPTIED;

    batch = batch_array;
    run = run_array;
    INIT_RUN(run, result.pos.x, result.pos.y);
    tempPf = private->pf_font;
	if (private->x_font_info == (XFontStruct *)NULL)
	    private->x_font_info = 
                (XFontStruct *)xv_get((Xv_opaque)private->font, FONT_INFO);
    
    for (esi = esbuf->first; esi < esbuf->last_plus_one; esi++) {
	c = (unsigned char) (*buf_rep++);

    if (c > private->x_font_info->max_char_or_byte2)
	c = SPACE;

Rescan:
	if ( (c == SPACE) || (c == NB_SPACE) ) {
	    in_white_space = 1;
	    pc = &tempPf->pf_char[SPACE];
	    /* if Xfont does not have glyph for space */
	    if (pc->pc_pr->pr_size.x == 0
		&& pc->pc_pr->pr_size.y == 0)
		pc = &tempPf->pf_char['n'];
	} else if (c == TAB) {
	    op |= EI_OP_CLEAR_INTERIOR;
	    in_white_space = 1;
	    pc = &dummy_for_tab;
	    /*
	     * dummy_for_tab.pc_adv.y = 0 implicitly due to declaring
	     * dummy_for_tab as a static. Don't set pc->pc_home as it is
	     * never examined
	     */
	    pc->pc_pr = 0;
	    /*
	     * Find the next tab stop (or standard tab if beyond the tab
	     * stops), and compute the advance to that position. If painting,
	     * start, or extend delta of, new run.
	     */
	    temp = SSub(result.pos.x, tab_origin);
	    for (ii = 0;
		 ii < private->num_tab_stops &&
		 private->tab_pixels[ii] <= temp;
		 ii++) {
	    }
	    if (ii < private->num_tab_stops) {
		pc->pc_adv.x = private->tab_pixels[ii] - temp;
	    } else {
		pc->pc_adv.x = temp % private->tab_pixel;
		pc->pc_adv.x = private->tab_pixel - pc->pc_adv.x;
	    }
	    if (!(op & EI_OP_MEASURE)) {
		if ((run == run_array && batch == batch_array) ||
		    (run > run_array &&
		     (run[-1].len == (batch - (char *)(run[-1].chars))))) {
		    /* Current run does not have any chars yet */
		} else {
		    /* Need to end current run and start a new run */
		    finalize_run(run++, run_array, batch, batch_array);
		    INIT_RUN(run, result.pos.x, result.pos.y);
		}
		run->x += pc->pc_adv.x;
	    }
	    /*
	     * Explicitly test right and bottom boundary hits, as we will
	     * skip over the standard tests.
	     */
	    temp = (short) pc->pc_adv.x - (short) 1;
	    temp += result.pos.x;
	    if (temp > bounds_right) {
		if (temp > rects_right) {
		    result.break_reason = EI_PR_HIT_RIGHT;
		    break;
		}
		bounds_right = temp;
	    }
	    if (check_vert_bounds) {
		temp = (short) private->tab_delta_y;
		temp += result.pos.y;
		if (--temp > bounds_bottom) {
		    if (temp > rects_bottom) {
			result.break_reason = EI_PR_HIT_BOTTOM;
			break;
		    }
		    bounds_bottom = ++temp;
		}
		if (private->font_flags & FF_EASY_Y)
		    check_vert_bounds = FALSE;
	    }
	    goto Skip_pc_pr_tests;
	} else if (c == NEWLINE) {
	    in_white_space = 0;
	    pc = &tempPf->pf_char[SPACE];
	    /* if Xfont does not have glyph for space */
	    if (pc->pc_pr->pr_size.x == 0 &&
		pc->pc_pr->pr_size.y == 0)
		pc = &tempPf->pf_char['n'];
	} else {
	    in_white_space = 0;
	    pc = &tempPf->pf_char[c];
	}
	if (pc->pc_pr &&
	    (!((128 <= c && c <= 159) || (iscntrl(c))) 
                 || in_white_space || c == NEWLINE ||
	     (private->state & CONTROL_CHARS_USE_FONT))) {
	    *batch = (c == NEWLINE) ? ' ' : c;
	    temp = result.pos.x;
	    temp += pc->pc_pr->pr_width - 1;
	    if (temp > bounds_right) {
		if (temp > rects_right) {
		    result.break_reason = EI_PR_HIT_RIGHT;
		    if (special_char < -1) {
			batch--;
			result.bounds.r_width = -2 - special_char;
		    }
		    break;
		}
		bounds_right = temp;
	    }
	    if (check_vert_bounds) {
		temp = (short) pc->pc_home.y;
		temp += result.pos.y;
		if (temp < result.bounds.r_top) {
		    if (temp < rect->r_top) {
			result.break_reason = EI_PR_HIT_TOP;
			break;
		    }
		    result.bounds.r_top = temp;
		}
		temp += pc->pc_pr->pr_size.y - 1;
		if (temp > bounds_bottom) {
		    if (temp > rects_bottom) {
			result.break_reason = EI_PR_HIT_BOTTOM;
			break;
		    }
		    bounds_bottom = temp;
		}
		if (private->font_flags & FF_EASY_Y)
		    check_vert_bounds = FALSE;
	    }
	    batch++;
	} else {
	    if (c == 127)
		special_char = '?';
	    else
		special_char = c + 64;
	    c = '^';
	    goto Rescan;
	}
Skip_pc_pr_tests:
	/* Accumulate advances for caller and ourselves. */
	result.pos.x += pc->pc_adv.x;
	result.pos.y += pc->pc_adv.y;
	if (special_char != -1) {
	    if (special_char >= 0) {
		c = special_char;
		special_char = -2 - result.bounds.r_width;
		goto Rescan;
	    } else
		special_char = -1;
	}
	if (c == NEWLINE)
	    break;
    }
    if (c == NEWLINE)
	/* Note following overrides possible EI_PR_HIT_RIGHT above. */
	result.break_reason = EI_PR_NEWLINE;
    result.last_plus_one = esi;
    result.bounds.r_width = bounds_right - result.bounds.r_left + 1;
    result.bounds.r_height = bounds_bottom - result.bounds.r_top + 1;
    result.considered = esi;

    if (!(op & EI_OP_MEASURE)) {
	int             run_length;

	finalize_run(run++, run_array, batch, batch_array);
	run_length = run - run_array;	/* C does "/ sizeof(struct)" */
	paint_batch(op, rop, pw, rect,
		    run_array, run_length, &result.bounds,
		    private->font);
    }
    result.pos.x += private->font_home.x;
    result.pos.y += private->font_home.y;
    return (result);
}

static struct ei_process_result
eiPlainTextProcessWC(eih, op, esbuf, x, y, rop, pw, rect, tab_origin)
    Ei_handle       eih;
    int             op;
    Es_buf_handle   esbuf;
    int             x, y, rop;
    Xv_Window       pw;
    register struct rect *rect;
    int             tab_origin;
/*
 * Arguments are: eih	handle of the entity interpreter whose ei_process op
 * mapped to this routine. op		see EI_OP_* in entity_interpreter.h.
 * esbuf	chars to be painted/measured. sizeof_buf	number of
 * characters in buffer. buf		the characters themselves. esh andle
 * of entity stream they came from. first		Es_index into esh.
 * last_plus_one	Es_index into esh. x		position to start
 * painting from. y		position of the largest ascender's top, NOT
 * the baseline. Probably always == rect.r_top. rop	raster op, usually
 * either PIX_SRC or PIX_SRC|PIX_DST. pw		pixwin to paint into.
 * rect	rectangle to paint into, indicates where to stop with
 * result.break_reason = EI_HIT_RIGHT, or whether to do nothing due to
 * result.break_reason = EI_HIT_BOTTOM. r_left only needs to be different
 * from x if we are starting to paint later than the beginning of the line,
 * and op specifies EI_OP_CLEAR_FRONT. tab_origin	x position of zeroth
 * tab stop on the line.
 * 
 * WARNING!  This code has been extensively hand tuned to make sure that the
 * compiler generates good code.  Seemingly trivial changes can impact the
 * generated code.  If you change this code, make sure you look at the actual
 * assembly code both before and after.
 */
{
    register Eipt_handle private = ABS_TO_REP(eih);
    register struct pixchar *pc;
    register wchar_t   c;
    register short  temp;
    register Es_index esi;
    wchar_t          *buf_rep = (wchar_t *) esbuf->buf;
    struct ei_process_result result;
    register short  bounds_right, rects_right;
    short           bounds_bottom, rects_bottom;
    short           in_white_space = 0, special_char = -1;
    register int    check_vert_bounds = TRUE;

    Run             run_array[MAX_PER_BATCH + 1];
    Run            *run;
    wchar_t         batch_array[MAX_PER_BATCH + 1];
    wchar_t        *batch;
    int             ii;
    Pixfont	   *tempPf;
    wchar_t	    wcs[2];

    wcs[1] = NULL;

    temp = (short) x;
    result.bounds.r_left = temp;
    bounds_right = temp;
    result.pos.x = SSub(temp, private->font_home.x);
    result.bounds.r_width = 0;
    temp = (short) y;
    result.bounds.r_top = temp;
    /*
     * BUG ALERT! The following is not completely correct, as it assumes that
     * the result.bounds.r_top is at the top of the current line, not just
     * the current "ink" for the batch. Make sure that clear|invert|pattern
     * in paint_batch affects the entire height of the line.
     */
    bounds_bottom = SSub(SAdd(temp, private->height), 1);
    result.pos.y = SSub(temp, private->font_home.y);
    rects_right = SRect_edge(rect->r_left, rect->r_width);
    rects_bottom = SRect_edge(rect->r_top, rect->r_height);
    result.break_reason = EI_PR_BUF_EMPTIED;
    /*
     * Construct the run items for the characters to be displayed. Except at
     * the end of the routine, run-1 is the last complete run, while run is
     * empty except for possibly containing a non-zero delta due to tabs.  At
     * the end, run's chars and len fields are updated from batch. During the
     * run construction, result.pos accumulates the advances along the
     * baseline, and is an absolute position. After building the run, it is
     * offset by font_home, thereby being the accumulation of the advances
     * applied to the original x,y args. 
     */
    batch = batch_array;
    run = run_array;
    INIT_RUN(run, result.pos.x, result.pos.y);
    tempPf = private->pf_font;
    
    for (esi = esbuf->first; esi < esbuf->last_plus_one; esi++) {
	c = (wchar_t) (*buf_rep++);

Rescan:
	if ( (c == SPACE) || (c == NB_SPACE) ) {
	    in_white_space = 1;
	    if (!check_vert_bounds)  
		   /* Ascii is shorter than kanji, so clean up first */
		    op |= EI_OP_CLEAR_INTERIOR;
	    pc = &tempPf->pf_char[SPACE];
	    /* if Xfont does not have glyph for space */
	    if (pc->pc_pr->pr_size.x == 0
		&& pc->pc_pr->pr_size.y == 0)
		pc = &tempPf->pf_char['n'];
	} else if (c == TAB) {
	    op |= EI_OP_CLEAR_INTERIOR;
	    in_white_space = 1;
	    pc = &dummy_for_tab;
	    /*
	     * dummy_for_tab.pc_adv.y = 0 implicitly due to declaring
	     * dummy_for_tab as a static. Don't set pc->pc_home as it is
	     * never examined
	     */
	    pc->pc_pr = 0;
	    /*
	     * Find the next tab stop (or standard tab if beyond the tab
	     * stops), and compute the advance to that position. If painting,
	     * start, or extend delta of, new run.
	     */
	    temp = SSub(result.pos.x, tab_origin);
	    for (ii = 0;
		 ii < private->num_tab_stops &&
		 private->tab_pixels[ii] <= temp;
		 ii++) {
	    }
	    if (ii < private->num_tab_stops) {
		pc->pc_adv.x = private->tab_pixels[ii] - temp;
	    } else {
		pc->pc_adv.x = temp % private->tab_pixel;
		pc->pc_adv.x = private->tab_pixel - pc->pc_adv.x;
	    }
	    if (!(op & EI_OP_MEASURE)) {
		if ((run == run_array && batch == batch_array) ||
		    (run > run_array &&
		     (run[-1].len == (batch - (wchar_t *)(run[-1].chars))))) {
		    /* Current run does not have any chars yet */
		} else {
		    /* Need to end current run and start a new run */
		    finalize_run(run++, run_array, batch, batch_array);
		    INIT_RUN(run, result.pos.x, result.pos.y);
		}
		run->x += pc->pc_adv.x;
	    }
	    /*
	     * Explicitly test right and bottom boundary hits, as we will
	     * skip over the standard tests.
	     */
	    temp = (short) pc->pc_adv.x - (short) 1;
	    temp += result.pos.x;
	    if (temp > bounds_right) {
		if (temp > rects_right) {
		    result.break_reason = EI_PR_HIT_RIGHT;
		    break;
		}
		bounds_right = temp;
	    }
	    if (check_vert_bounds) {
		temp = (short) private->tab_delta_y;
		temp += result.pos.y;
		if (--temp > bounds_bottom) {
		    if (temp > rects_bottom) {
			result.break_reason = EI_PR_HIT_BOTTOM;
			break;
		    }
		    bounds_bottom = ++temp;
		}
		if (private->font_flags & FF_EASY_Y)
		    check_vert_bounds = FALSE;
	    }
	    goto Skip_pc_pr_tests;
	} else if (c == NEWLINE) {
	    if (!check_vert_bounds)  
		    op |= EI_OP_CLEAR_INTERIOR;
	    in_white_space = 0;
	    pc = &tempPf->pf_char[SPACE];
	    /* if Xfont does not have glyph for space */
	    if (pc->pc_pr->pr_size.x == 0 &&
		pc->pc_pr->pr_size.y == 0)
		pc = &tempPf->pf_char['n'];
	} else {
	    in_white_space = 0;
            if (iswascii(c)) {
		pc = &tempPf->pf_char[c];
		if (!check_vert_bounds)  
		    op |= EI_OP_CLEAR_INTERIOR;
	    } else if (private->locale_is_ale) {
	        wcs[0] = c;
	        if (wscol(wcs) == 2) /* This char occupy two col */
		    pc = &private->mb_pc;            
	        else {
	            pc = &private->kana_pc;
		    if (!check_vert_bounds)  
			op |= EI_OP_CLEAR_INTERIOR;
		}
	    } else { 
	        pc = &tempPf->pf_char['n'];
	    }
	}
	if (pc->pc_pr &&
	    (!((c >= 0) && iswcntrl(c)) || in_white_space || c == NEWLINE ||
	     (private->state & CONTROL_CHARS_USE_FONT))) {
	    *batch = (c == NEWLINE) ? ' ' : c;
	    temp = result.pos.x;
	    temp += pc->pc_pr->pr_size.x - 1;       
	    if (temp > bounds_right) {
		if (temp > rects_right) {
		    result.break_reason = EI_PR_HIT_RIGHT;
		    if (special_char < -1) {
			batch--;
			result.bounds.r_width = -2 - special_char;
		    }
		    break;
		}
		bounds_right = temp;
	    }
	    if (check_vert_bounds) {
		temp = (short) pc->pc_home.y;
		temp += result.pos.y;
		if (temp < result.bounds.r_top) {
		    if (temp < rect->r_top) {
			result.break_reason = EI_PR_HIT_TOP;
			break;
		    }
		    result.bounds.r_top = temp;
		}
		temp += pc->pc_pr->pr_size.y - 1;
		if (temp > bounds_bottom) {
		    if (temp > rects_bottom) {
			result.break_reason = EI_PR_HIT_BOTTOM;
			break;
		    }
		    bounds_bottom = temp;
		}
		if (private->font_flags & FF_EASY_Y)
		    check_vert_bounds = FALSE;
	    }
	    batch++;
	} else {
	    if (c == 127)
		special_char = '?';
	    else
		special_char = c + 64;
	    c = '^';
	    goto Rescan;
	}
Skip_pc_pr_tests:
	/* Accumulate advances for caller and ourselves. */
	result.pos.x += pc->pc_adv.x;
	result.pos.y += pc->pc_adv.y;
	if (special_char != -1) {
	    if (special_char >= 0) {
		c = special_char;
		special_char = -2 - result.bounds.r_width;
		goto Rescan;
	    } else
		special_char = -1;
	}
	if (c == NEWLINE)
	    break;
    }
    if (c == NEWLINE)
	/* Note following overrides possible EI_PR_HIT_RIGHT above. */
	result.break_reason = EI_PR_NEWLINE;
    result.last_plus_one = esi;
    result.bounds.r_width = bounds_right - result.bounds.r_left + 1;
    result.bounds.r_height = bounds_bottom - result.bounds.r_top + 1;
    result.considered = esi;
    if (!(op & EI_OP_MEASURE)) {
	int             run_length;

	finalize_run(run++, run_array, batch, batch_array);
	run_length = run - run_array;	/* C does "/ sizeof(struct)" */
	paint_batch(op, rop, pw, rect,
		    run_array, run_length, &result.bounds,
		    private->font);
    }
    result.pos.x += private->font_home.x;
    result.pos.y += private->font_home.y;
    return (result);
}

static struct ei_process_result
ei_plain_text_process(eih, op, esbuf, x, y, rop, pw, rect, tab_origin)
    Ei_handle       eih;
    int             op;
    Es_buf_handle   esbuf;
    int             x, y, rop;
    Xv_Window       pw;
    register struct rect *rect;
    int             tab_origin;
/*
 * Arguments are: eih	handle of the entity interpreter whose ei_process op
 * mapped to this routine. op		see EI_OP_* in entity_interpreter.h.
 * esbuf	chars to be painted/measured. sizeof_buf	number of
 * characters in buffer. buf		the characters themselves. esh andle
 * of entity stream they came from. first		Es_index into esh.
 * last_plus_one	Es_index into esh. x		position to start
 * painting from. y		position of the largest ascender's top, NOT
 * the baseline. Probably always == rect.r_top. rop	raster op, usually
 * either PIX_SRC or PIX_SRC|PIX_DST. pw		pixwin to paint into.
 * rect	rectangle to paint into, indicates where to stop with
 * result.break_reason = EI_HIT_RIGHT, or whether to do nothing due to
 * result.break_reason = EI_HIT_BOTTOM. r_left only needs to be different
 * from x if we are starting to paint later than the beginning of the line,
 * and op specifies EI_OP_CLEAR_FRONT. tab_origin	x position of zeroth
 * tab stop on the line.
 * 
 */
{
    if (!multibyte)
        return(eiPlainTextProcessSB
                 (eih, op, esbuf, x, y, rop, pw, rect, tab_origin));
    else
        return(eiPlainTextProcessWC
                 (eih, op, esbuf, x, y, rop, pw, rect, tab_origin));
}

static
paint_batch(op, rop, pw, rect, run, run_length, bounds, font)
    int             op, rop;
    Xv_Window       pw;
    struct rect    *rect;
    Run            *run;
    Xv_Font         font;    
    int             run_length;
    struct rect    *bounds;
{
    int temp;
    extern int      xv_textsw_doing_refresh;

    if ((op & EI_OP_CLEAR_ALL) &&
	((op & EI_OP_INVERT) || !xv_textsw_doing_refresh)) {
	int bounds_right = bounds->r_left + bounds->r_width;

	/* these are only (?) needed for textedit operations now.. */
	if (op & EI_OP_CLEAR_FRONT)
	    (void) tty_background(pw,
				  rect->r_left, bounds->r_top,
			          bounds_right - rect->r_left,
                                  bounds->r_height,
				  PIX_CLR);
	if (op & EI_OP_CLEAR_INTERIOR)
	    (void) tty_background(pw,
				  bounds->r_left, bounds->r_top,
				  bounds->r_width, bounds->r_height,
				  PIX_CLR);
	if (op & EI_OP_CLEAR_BACK)
	    (void) tty_background(pw,
				  bounds_right, bounds->r_top,
 				  rect->r_left + rect->r_width - bounds_right,
				  bounds->r_height,
				  PIX_CLR);

    }

    /* this outputs all the stuff! */
    for (temp = 0; temp < run_length; temp++, run++)
	tty_newtext(pw, run->x, run->y, rop, font, run->chars, run->len);

    if (op & EI_OP_LIGHT_GRAY)
	(void) xv_replrop(pw,
			  bounds->r_left, bounds->r_top,
			  bounds->r_width, bounds->r_height,
			  PIX_SRC | PIX_DST, &gray17_pr, 0, 0);
    if (op & EI_OP_STRIKE_UNDER) {
	int bottom = rect_bottom(bounds);
	(void) pw_vector(pw,
		     bounds->r_left, bottom, rect_right(bounds), bottom,
			 PIX_SET, 0);
    }
    if (op & EI_OP_STRIKE_THRU) {
	int middle = bounds->r_top + bounds->r_height / 2;
	(void) pw_vector(pw,
		     bounds->r_left, middle, rect_right(bounds), middle,
			 PIX_SET, 0);
    }
    if (op & EI_OP_INVERT)
	 (void) tty_background(pw,
			       bounds->r_left, bounds->r_top,
			       bounds->r_width, bounds->r_height,
		               PIX_NOT(PIX_DST));
}

static          caddr_t
ei_plain_text_get(eih, attribute)
    Ei_handle       eih;
    Ei_attribute    attribute;
{
    register Eipt_handle private = ABS_TO_REP(eih);

    switch (attribute) {
      case EI_CONTROL_CHARS_USE_FONT:
	return ((caddr_t) (private->state & CONTROL_CHARS_USE_FONT));
      case EI_FONT:
	return ((caddr_t) private->font);
      case EI_TAB_WIDTH:
	return ((caddr_t) (private->tab_width));
      case EI_LOCALE_IS_ALE:
	return ((caddr_t) (private->locale_is_ale));
      case EI_LINE_SPACE:
	return ((caddr_t) (private->height));
      default:
	return (0);
    }
}

ei_plain_text_set(eih, attributes)
    Ei_handle       eih;
    Attr_attribute  *attributes;
{
    register Eipt_handle private = ABS_TO_REP(eih);

    while (*attributes) {
	switch ((Ei_attribute) * attributes) {
	  case EI_CONTROL_CHARS_USE_FONT:
	    if (attributes[1]) {
		private->state |= CONTROL_CHARS_USE_FONT;
	    } else {
		private->state &= ~CONTROL_CHARS_USE_FONT;
	    }
	    break;
	  case EI_FONT:
	    if (attributes[1]) {
		ei_plain_text_set_font(eih,
				       (struct pixfont *) attributes[1]);
	    } else {
		return (1);
	    }
	    break;
	  case EI_TAB_WIDTH:
	    ei_plain_text_set_tab_width(eih, (int) attributes[1]);
	    break;
	  case EI_TAB_WIDTHS:
	    ei_plain_text_set_tab_widths(eih, &attributes[1], FALSE);
	    break;
	  default:
	    break;
	}
	attributes = attr_next(attributes);
    }
    return (0);
}

#define EI_IS_LINE_CHAR(char)						\
	((char) == '\n')

#define	EI_WORD_CLASS		0
#define	EI_PATH_NAME_CLASS	1
#define	EI_SP_AND_TAB_CLASS	2
#define	EI_CLIENT1_CLASS	3
#define	EI_CLIENT2_CLASS	4
#define	EI_NUM_CLASSES		5	/* number of character classes */

static SET      ei_classes[EI_NUM_CLASSES];	/* character classes */
static short    ei_classes_initialized;	/* = 0 (implicit init for cc -A-R) */

/*	this is the array of those characters that are 
 *	standard delimiters used by default in the system. 
 *	this list is from the Panel Code implementation in 
 *	SunView (1.80 - I believe)
 *
 *	there is a lot of different ways this could be done,
 *	however using the ctype() routines isn't currently
 *	something that can be supported by all the OSes out
 *	there so this is a hardcoded stab at the problem that
 *	gets the job done and provides the user some flexibility
 *	at the same time. [jcb 5/15/90]
 */
#define	DELIMITERS	 " \t,.:;?!\'\"`*/-+=(){}[]<>\\|~@#$%^&"

static wchar_t	delims_wcs[DEFAULTS_MAX_VALUE_SIZE];
static int	delims_class_value;

#define	word_type(c)	((delims_wcs[0]) ? (wschr(delims_wcs, c) ? 0 : 1) \
					 : wchar_kind(c))

static void
ei_classes_initialize()
{
    register SET   *setp;	/* character class of interest */
    char	*delims;
    char	delim_chars[256];

    if (multibyte) {

        delims = (char*)defaults_get_string("text.delimiterChars",
					"Text.DelimiterChars", "");

        if (*delims) {
	    (void) mbstowcs(delims_wcs, delims, DEFAULTS_MAX_VALUE_SIZE - 1);
	    delims_class_value = 0;
        } else {
	    delims_wcs[0] = NULL;
	    delims_class_value = wchar_kind(L' '); /* SPACE */
        }
    } else {

        delims = (char*)defaults_get_string("text.delimiterChars",
					"Text.DelimiterChars", DELIMITERS);

        /*
         changing the logic to use the delimiters that are in the array rather
         than those characters that are simply isalnum(). this is so the
         delimiters can be expanded to include those which are in the ISO latin
         specification from the user defaults.
        */

        /* WORD is alpha-numeric characters only */
        setp = &ei_classes[EI_WORD_CLASS];

        sprintf( delim_chars, delims );

        /* set all and then remove the delimiters specified */
        FILL_SET(setp);
        for( delims = delim_chars; *delims; delims++ ) {
	    REMOVE_ELEMENT(setp, *delims );
        }
    }

    /* PATH_NAME is non-white & non-null chars */
    setp = &ei_classes[EI_PATH_NAME_CLASS];
    FILL_SET(setp);
    REMOVE_ELEMENT(setp, ' ');
    REMOVE_ELEMENT(setp, '\t');
    REMOVE_ELEMENT(setp, '\n');
    REMOVE_ELEMENT(setp, '\0');

    /* SP_AND_TAB is exactly that */
    setp = &ei_classes[EI_SP_AND_TAB_CLASS];
    CLEAR_SET(setp);
    ADD_ELEMENT(setp, ' ');
    ADD_ELEMENT(setp, '\t');

    /* CLIENT1/2 are initially empty */
    setp = &ei_classes[EI_CLIENT1_CLASS];
    CLEAR_SET(setp);
    setp = &ei_classes[EI_CLIENT2_CLASS];
    CLEAR_SET(setp);

    ei_classes_initialized = 1;
}


static struct ei_span_result
eiPlainTextSpanOfGroupSB(eih, esbuf, group_spec, index)
    Ei_handle       eih;	/* Currently unused */
    register Es_buf_handle esbuf;
    register int    group_spec;
    Es_index        index;
{
    register Es_index esi = index;
    register int    i, in_class;
    struct ei_span_result result;
    /*
     * WARNING: the code below that uses the SET macros, must not have
     * characters with the 8-th bit on be sign extended when converted to
     * larger storage classes.  Thus, buf_rep[i] and c must both be variables
     * of type unsigned char.
     */
    register unsigned char *buf_rep = (unsigned char *)esbuf->buf;
    register unsigned char c;
   

    /*
     * Invariants of this routine: i == esi - esbuf->first during left scan,
     * c == esbuf->buf[(--i)], (i++) during right
     */
#define ESTABLISH_I_INVARIANT	i = esi - esbuf->first
#define ESTABLISH_C_INVARIANT_LEFT	c = buf_rep[(--esi, --i)]
#define ESTABLISH_C_INVARIANT_RIGHT	c = buf_rep[(esi++, i++)]

    result.flags = 0;

    if (group_spec & EI_SPAN_LEFT_ONLY) {
	/*
	 * For a LEFT_ONLY span, the entity after esi should not be
	 * considered, requiring us to backup esi before starting.
	 */
	if (esi <= 0) {
	    goto ErrorReturn;
	} else
	    esi--;
    }
    if (esi < esbuf->first || esi >= esbuf->last_plus_one) {
	if (es_make_buf_include_index(esbuf, esi, esbuf->sizeof_buf / 4)) {
	    goto ErrorReturn;
	    /*
	     * BUG ALERT: this always fails at the end of the stream unless
	     * EI_SPAN_LEFT_ONLY has already backed up esi.
	     */
	}
    }
    result.first = esi;
    result.last_plus_one = esi + 1;
    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_CHAR)
	goto Return;

    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_DOCUMENT) {
	result.first = 0;
	result.last_plus_one = es_get_length(esbuf->esh);
	goto Return;
    };

    ESTABLISH_I_INVARIANT;
    c = buf_rep[i];
    /* treat LINE class special */
    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_LINE) {
	if ((in_class = EI_IS_LINE_CHAR(c)) == 0) {
	    result.flags |= EI_SPAN_NOT_IN_CLASS;
	    if (group_spec & EI_SPAN_IN_CLASS_ONLY)
		goto ErrorReturn;
	} else {
	    if (group_spec & EI_SPAN_NOT_CLASS_ONLY)
		goto ErrorReturn;
	    if (group_spec & EI_SPAN_LEFT_ONLY) {
		result.first++;
		goto DoneLineScanLeft;
	    }
	}
	if ((group_spec & EI_SPAN_RIGHT_ONLY) == 0) {
	    while (esi > 0) {	/* Scan left. */
		if (i == 0) {
		    esi = es_backup_buf(esbuf);
		    if (esi == ES_CANNOT_SET) {
			goto DoneLineScanLeft;
		    }
		    esi++;	/* ... because i is pre-decremented */
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_LEFT;
		if (EI_IS_LINE_CHAR(c)) {
		    break;
		} else
		    result.first = esi;
	    }
    DoneLineScanLeft:		/* Fix the buffer up for the scan right */
	    esi = index;
	    if (esi < esbuf->last_plus_one) {
		ESTABLISH_I_INVARIANT;
	    }
	}
	esi++;
	i++;
	if ((group_spec & EI_SPAN_LEFT_ONLY) == 0)
	    for (; !in_class;) {
		if (esi >= esbuf->last_plus_one) {
		    esbuf->last_plus_one = esi;
		    es_set_position(esbuf->esh, esbuf->last_plus_one);
		    if (es_advance_buf(esbuf))
			goto Return;
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_RIGHT;
		in_class = EI_IS_LINE_CHAR(c);
		result.last_plus_one = esi;
	    }
    } else {			/* Handle other classes uniformly */
	SET            *setp;	/* character class of interest */

	if (!ei_classes_initialized)
	    ei_classes_initialize();
	switch (group_spec & EI_SPAN_CLASS_MASK) {
	  case EI_SPAN_WORD:
	    setp = &ei_classes[EI_WORD_CLASS];
	    break;
	  case EI_SPAN_PATH_NAME:
	    setp = &ei_classes[EI_PATH_NAME_CLASS];
	    break;
	  case EI_SPAN_SP_AND_TAB:
	    setp = &ei_classes[EI_SP_AND_TAB_CLASS];
	    break;
	  case EI_SPAN_CLIENT1:
	    setp = &ei_classes[EI_CLIENT1_CLASS];
	    break;
	  case EI_SPAN_CLIENT2:
	    setp = &ei_classes[EI_CLIENT2_CLASS];
	    break;
	  default:
	    goto ErrorReturn;
	}
	if ((in_class = IN(setp, c)) == 0) {
	    result.flags |= EI_SPAN_NOT_IN_CLASS;
	    if (group_spec & EI_SPAN_IN_CLASS_ONLY)
		goto ErrorReturn;
	} else {
	    if (group_spec & EI_SPAN_NOT_CLASS_ONLY)
		goto ErrorReturn;
	}
	if ((group_spec & EI_SPAN_RIGHT_ONLY) == 0) {
	    while (esi > 0) {	/* Scan left. */
		if (i == 0) {
		    esi = es_backup_buf(esbuf);
		    if (esi == ES_CANNOT_SET) {
			goto DoneClassScanLeft;
		    }
		    esi++;	/* ... because i is pre-decremented */
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_LEFT;
		if (in_class != IN(setp, c)) {
		    break;
		    /* Here we assume LINE is the next level for this class */
		} else if (EI_IS_LINE_CHAR(c)) {
		    result.flags |= EI_SPAN_LEFT_HIT_NEXT_LEVEL;
		    break;
		} else
		    result.first = esi;
	    }
    DoneClassScanLeft:		/* Fix the buffer up for the scan right */
	    esi = index;
	    if (esi < esbuf->last_plus_one) {
		ESTABLISH_I_INVARIANT;
	    }
	}
	esi++;
	i++;
	if ((group_spec & EI_SPAN_LEFT_ONLY) == 0)
	    for (;;) {
		if (esi >= esbuf->last_plus_one) {
		    esbuf->last_plus_one = esi;
		    es_set_position(esbuf->esh, esbuf->last_plus_one);
		    if (es_advance_buf(esbuf))
			goto Return;
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_RIGHT;
		if (in_class != IN(setp, c)) {
		    break;
		    /* Here we assume LINE is the next level for this class */
		} else if (EI_IS_LINE_CHAR(c)) {
		    result.flags |= EI_SPAN_RIGHT_HIT_NEXT_LEVEL;
		    break;
		} else
		    result.last_plus_one = esi;
	    }
    }
    goto Return;
ErrorReturn:
    result.first = result.last_plus_one = ES_CANNOT_SET;
Return:
    return (result);
}

static struct ei_span_result
eiPlainTextSpanOfGroupWC(eih, esbuf, group_spec, index)
    Ei_handle       eih;	/* Currently unused */
    register Es_buf_handle esbuf;
    register int    group_spec;
    Es_index        index;
{
    register Es_index esi = index;
    register int    i, in_class;
    struct ei_span_result result;
    /*
     * WARNING: the code below that uses the SET macros, must not have
     * characters with the 8-th bit on be sign extended when converted to
     * larger storage classes.  Thus, buf_rep[i] and c must both be variables
     * of type unsigned char.
     */
    register wchar_t *buf_rep = (wchar_t *)esbuf->buf;
    register wchar_t c;
   

    /*
     * Invariants of this routine: i == esi - esbuf->first during left scan,
     * c == esbuf->buf[(--i)], (i++) during right
     */
#define ESTABLISH_I_INVARIANT	i = esi - esbuf->first
#define ESTABLISH_C_INVARIANT_LEFT	c = buf_rep[(--esi, --i)]
#define ESTABLISH_C_INVARIANT_RIGHT	c = buf_rep[(esi++, i++)]

    result.flags = 0;

    if (group_spec & EI_SPAN_LEFT_ONLY) {
	/*
	 * For a LEFT_ONLY span, the entity after esi should not be
	 * considered, requiring us to backup esi before starting.
	 */
	if (esi <= 0) {
	    goto ErrorReturn;
	} else
	    esi--;
    }
    if (esi < esbuf->first || esi >= esbuf->last_plus_one) {
	if (es_make_buf_include_index(esbuf, esi, esbuf->sizeof_buf / 4)) {
	    goto ErrorReturn;
	    /*
	     * BUG ALERT: this always fails at the end of the stream unless
	     * EI_SPAN_LEFT_ONLY has already backed up esi.
	     */
	}
    }
    result.first = esi;
    result.last_plus_one = esi + 1;
    result.flags = 0;
    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_CHAR)
	goto Return;

    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_DOCUMENT) {
	result.first = 0;
	result.last_plus_one = es_get_length(esbuf->esh);
	goto Return;
    };

    ESTABLISH_I_INVARIANT;
    c = buf_rep[i];
    /* treat LINE class special */
    if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_LINE) {
	if ((in_class = EI_IS_LINE_CHAR(c)) == 0) {
	    result.flags |= EI_SPAN_NOT_IN_CLASS;
	    if (group_spec & EI_SPAN_IN_CLASS_ONLY)
		goto ErrorReturn;
	} else {
	    if (group_spec & EI_SPAN_NOT_CLASS_ONLY)
		goto ErrorReturn;
	    if (group_spec & EI_SPAN_LEFT_ONLY) {
		result.first++;
		goto DoneLineScanLeft;
	    }
	}
	if ((group_spec & EI_SPAN_RIGHT_ONLY) == 0) {
	    while (esi > 0) {	/* Scan left. */
		if (i == 0) {
		    esi = es_backup_buf(esbuf);
		    if (esi == ES_CANNOT_SET) {
			goto DoneLineScanLeft;
		    }
		    esi++;	/* ... because i is pre-decremented */
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_LEFT;
		if (EI_IS_LINE_CHAR(c)) {
		    break;
		} else
		    result.first = esi;
	    }
    DoneLineScanLeft:		/* Fix the buffer up for the scan right */
	    esi = index;
	    if (esi < esbuf->last_plus_one) {
		ESTABLISH_I_INVARIANT;
	    }
	}
	esi++;
	i++;
	if ((group_spec & EI_SPAN_LEFT_ONLY) == 0)
	    for (; !in_class;) {
		if (esi >= esbuf->last_plus_one) {
		    esbuf->last_plus_one = esi;
		    es_set_position(esbuf->esh, esbuf->last_plus_one);
		    if (es_advance_buf(esbuf))
			goto Return;
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_RIGHT;
		in_class = EI_IS_LINE_CHAR(c);
		result.last_plus_one = esi;
	    }
	} else if ((group_spec & EI_SPAN_CLASS_MASK) == EI_SPAN_WORD) {
	    int	charType;

	    if (!ei_classes_initialized)
		ei_classes_initialize();
	    charType = word_type(c);
	    if (charType == delims_class_value) {
		result.flags |= EI_SPAN_NOT_IN_CLASS;
		if (group_spec & EI_SPAN_IN_CLASS_ONLY)
		    goto ErrorReturn;
	    } else {
		if (group_spec & EI_SPAN_NOT_CLASS_ONLY)
		    goto ErrorReturn;
	    }
	    if ((group_spec & EI_SPAN_RIGHT_ONLY) == 0) {
		while (esi > 0) {	/* Scan left. */
		    if (i == 0) {
			esi = es_backup_buf(esbuf);
			if (esi == ES_CANNOT_SET) {
			    goto DoneWordScanLeft;
			}
			esi++;		/* ... because i is pre-decremented */
			ESTABLISH_I_INVARIANT;
		    }
		    ESTABLISH_C_INVARIANT_LEFT;
		    if (word_type(c) != charType) {
			break;
		    /* Here we assume LINE is the next level for this class */
		    } else if (EI_IS_LINE_CHAR(c)) {
			result.flags |= EI_SPAN_LEFT_HIT_NEXT_LEVEL;
			break;
		    } else result.first = esi;
		}
DoneWordScanLeft:		/* Fix the buffer up for the scan right */
		esi = index;
		if (esi < esbuf->last_plus_one) {
		    ESTABLISH_I_INVARIANT;
		}
	    }
	    esi++; i++;
	    if ((group_spec & EI_SPAN_LEFT_ONLY) == 0) for (;;) {
		if (esi+2 >= esbuf->last_plus_one) {
		    esbuf->last_plus_one = esi;
		    es_set_position(esbuf->esh, esbuf->last_plus_one);
		    if (es_advance_buf(esbuf))
			goto Return;
		    ESTABLISH_I_INVARIANT;
		}
		/* ESTABLISH_C_INVARIANT_RIGHT; */
		c = buf_rep[i];
		if (word_type(c) != charType) {
		    break;
		/* Here we assume LINE is the next level for this class */
		} else if (EI_IS_LINE_CHAR(c)) {
		    result.flags |= EI_SPAN_RIGHT_HIT_NEXT_LEVEL;
		    break;
		}
	        esi++; i++;
		result.last_plus_one = esi;
	    }
    } else {			/* Handle other classes uniformly */
	SET            *setp;	/* character class of interest */

	if (!ei_classes_initialized)
	    ei_classes_initialize();
	switch (group_spec & EI_SPAN_CLASS_MASK) {
	  case EI_SPAN_PATH_NAME:
	    setp = &ei_classes[EI_PATH_NAME_CLASS];
	    break;
	  case EI_SPAN_SP_AND_TAB:
	    setp = &ei_classes[EI_SP_AND_TAB_CLASS];
	    break;
	  case EI_SPAN_CLIENT1:
	    setp = &ei_classes[EI_CLIENT1_CLASS];
	    break;
	  case EI_SPAN_CLIENT2:
	    setp = &ei_classes[EI_CLIENT2_CLASS];
	    break;
	  default:
	    goto ErrorReturn;
	}
	if ((in_class = IN(setp, c)) == 0) {
	    result.flags |= EI_SPAN_NOT_IN_CLASS;
	    if (group_spec & EI_SPAN_IN_CLASS_ONLY)
		goto ErrorReturn;
	} else {
	    if (group_spec & EI_SPAN_NOT_CLASS_ONLY)
		goto ErrorReturn;
	}
	if ((group_spec & EI_SPAN_RIGHT_ONLY) == 0) {
	    while (esi > 0) {	/* Scan left. */
		if (i == 0) {
		    esi = es_backup_buf(esbuf);
		    if (esi == ES_CANNOT_SET) {
			goto DoneClassScanLeft;
		    }
		    esi++;	/* ... because i is pre-decremented */
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_LEFT;
		if (in_class != IN(setp, c)) {
		    break;
		    /* Here we assume LINE is the next level for this class */
		} else if (EI_IS_LINE_CHAR(c)) {
		    result.flags |= EI_SPAN_LEFT_HIT_NEXT_LEVEL;
		    break;
		} else
		    result.first = esi;
	    }
    DoneClassScanLeft:		/* Fix the buffer up for the scan right */
	    esi = index;
	    if (esi < esbuf->last_plus_one) {
		ESTABLISH_I_INVARIANT;
	    }
	}
	esi++;
	i++;
	if ((group_spec & EI_SPAN_LEFT_ONLY) == 0)
	    for (;;) {
		if (esi >= esbuf->last_plus_one) {
		    esbuf->last_plus_one = esi;
		    es_set_position(esbuf->esh, esbuf->last_plus_one);
		    if (es_advance_buf(esbuf))
			goto Return;
		    ESTABLISH_I_INVARIANT;
		}
		ESTABLISH_C_INVARIANT_RIGHT;
		if (in_class != IN(setp, c)) {
		    break;
		    /* Here we assume LINE is the next level for this class */
		} else if (EI_IS_LINE_CHAR(c)) {
		    result.flags |= EI_SPAN_RIGHT_HIT_NEXT_LEVEL;
		    break;
		} else
		    result.last_plus_one = esi;
	    }
    }
    goto Return;
ErrorReturn:
    result.first = result.last_plus_one = ES_CANNOT_SET;
Return:
    return (result);
}

static struct ei_span_result
ei_plain_text_span_of_group(eih, esbuf, group_spec, index)
    Ei_handle       eih;	/* Currently unused */
    register Es_buf_handle esbuf;
    register int    group_spec;
    Es_index        index;
{
    if (!multibyte)
        return(eiPlainTextSpanOfGroupSB(eih, esbuf, group_spec, index));
    else
        return(eiPlainTextSpanOfGroupWC(eih, esbuf, group_spec, index));    
}

static struct ei_process_result
eiPlainTextExpandSB(eih, esbuf, rect, x, out_buf, out_buf_len, tab_origin)
    Ei_handle       eih;
    register Es_buf_handle esbuf;
    Rect           *rect;
    int             x;
    char           *out_buf;
    int             out_buf_len;
    int             tab_origin;
/*
 * Returns number of expanded chars in result.last_plus_one.
 * Result.break_reason = EI_PR_END_OF_STREAM if exhausted the entity stream.
 * EI_PR_BUF_EMPTIED if exhausted out_buf_len or esbuf. EI_PR_NEWLINE if a
 * character in the newline class was encountered. EI_PR_HIT_RIGHT if scan
 * reached right edge of rect.
 */
{
    Eipt_handle     private = ABS_TO_REP(eih);
    struct ei_process_result result;
    struct ei_process_result process_result;
    Es_buf_object   process_esbuf;
    unsigned char           *cp, *op, *in_buf = (unsigned char *)esbuf->buf;

    result.last_plus_one = 0;
    result.break_reason = EI_PR_BUF_EMPTIED;
    if (!in_buf || !out_buf)
	return (result);
    process_esbuf = *esbuf;
    for (cp = in_buf, op = (unsigned char *)out_buf;
	 esbuf->first < esbuf->last_plus_one &&
	 cp < in_buf + esbuf->sizeof_buf &&
	 result.last_plus_one < out_buf_len;
	 cp++, esbuf->first++) {

	if (*cp == TAB) {
	    /*
	     * Measure to just after the tab. (x corresponds to start) If
	     * HIT_RIGHT or NEWLINE, return only enough chars in
	     * result.last_plus_one to get to right edge. If BUF_EMPTIED,
	     * then tab must be on the same line as esbuf->first, and
	     * process_result.pos.x tells where it ends. Save
	     * process_result.pos.x. Then measure to just before the tab.
	     * Now, old process_result.pos.x - process_result.pos.x is the
	     * width of the tab in pixels. Divide by the width of the space
	     * to get number of spaces, and, if there is enough room, copy
	     * them into out_buf.
	     */
	    int             tmp_x, spaces_in_tab, i;

	    process_esbuf.last_plus_one = esbuf->first + 1;
	    process_result = ei_plain_text_process(
			 eih, EI_OP_MEASURE, &process_esbuf, x, rect->r_top,
				  PIX_SRC, (Xv_Window) 0, rect, tab_origin);
	    switch (process_result.break_reason) {
	      case EI_PR_HIT_RIGHT:
	      case EI_PR_NEWLINE:
		/*
		 * Following is a cop-out instead of returning only enough
		 * chars in result.last_plus_one to get to right edge.
		 */
		*op++ = SPACE;
		result.last_plus_one++;
		break;
	      default:
		tmp_x = process_result.pos.x;
		process_esbuf.last_plus_one--;
		process_result = ei_plain_text_process(
			 eih, EI_OP_MEASURE, &process_esbuf, x, rect->r_top,
				  PIX_SRC, (Xv_Window) 0, rect, tab_origin);
		if (private->x_font_info->per_char)  {
		    spaces_in_tab = (tmp_x - process_result.pos.x) /
		        private->x_font_info->per_char['m' -
                            private->x_font_info->min_char_or_byte2].width;
		}
		else  {
		    spaces_in_tab = (tmp_x - process_result.pos.x) /
		        private->x_font_info->min_bounds.width;
		}
		if (result.last_plus_one
		    >= (out_buf_len - spaces_in_tab)) {
		    result.break_reason = EI_PR_BUF_FULL;
		    break;
		}
		for (i = 0; i < spaces_in_tab; i++) {
		    *op++ = ' ';
		    result.last_plus_one++;
		}
	    }
	    if (result.break_reason == EI_PR_BUF_FULL)
		break;
	} else if (*cp == NEWLINE) {
	    *op++ = SPACE;
	    result.last_plus_one++;
	} else if (!((128 <= *cp && *cp <= 159) || (iscntrl(*cp)))
		   || private->state & CONTROL_CHARS_USE_FONT) {
	    *op++ = *cp;
	    result.last_plus_one++;
	} else {
	    if (result.last_plus_one < (out_buf_len - 2)) {
		*op++ = '^';
		*op++ = ((128 <= *cp && *cp <= 159) 
                          || (iscntrl(*cp))) ? *cp + 64 : '?';
		result.last_plus_one += 2;
	    } else {
		result.break_reason = EI_PR_BUF_FULL;
		break;
	    }
	}
    }
    return (result);
}

static struct ei_process_result
eiPlainTextExpandWC(eih, esbuf, rect, x, out_buf, out_buf_len, tab_origin)
    Ei_handle       eih;
    register Es_buf_handle esbuf;
    Rect           *rect;
    int             x;
    wchar_t        *out_buf;
    int             out_buf_len;
    int             tab_origin;
{
    Eipt_handle     private = ABS_TO_REP(eih);
    struct ei_process_result result;
    struct ei_process_result process_result;
    Es_buf_object   process_esbuf;
    wchar_t           *cp, *op, *in_buf = (wchar_t*)esbuf->buf;

    result.last_plus_one = 0;
    result.break_reason = EI_PR_BUF_EMPTIED;
    if (!in_buf || !out_buf)
	return (result);
    process_esbuf = *esbuf;
    for (cp = in_buf, op = out_buf;
	 esbuf->first < esbuf->last_plus_one &&
	 cp < in_buf + esbuf->sizeof_buf &&
	 result.last_plus_one < out_buf_len;
	 cp++, esbuf->first++) {

	if (*cp == TAB) {
	    /*
	     * Measure to just after the tab. (x corresponds to start) If
	     * HIT_RIGHT or NEWLINE, return only enough chars in
	     * result.last_plus_one to get to right edge. If BUF_EMPTIED,
	     * then tab must be on the same line as esbuf->first, and
	     * process_result.pos.x tells where it ends. Save
	     * process_result.pos.x. Then measure to just before the tab.
	     * Now, old process_result.pos.x - process_result.pos.x is the
	     * width of the tab in pixels. Divide by the width of the space
	     * to get number of spaces, and, if there is enough room, copy
	     * them into out_buf.
	     */
	    int             tmp_x, spaces_in_tab, i;

	    process_esbuf.last_plus_one = esbuf->first + 1;
	    process_result = ei_plain_text_process(
			 eih, EI_OP_MEASURE, &process_esbuf, x, rect->r_top,
				  PIX_SRC, (Xv_Window) 0, rect, tab_origin);
	    switch (process_result.break_reason) {
	      case EI_PR_HIT_RIGHT:
	      case EI_PR_NEWLINE:
		/*
		 * Following is a cop-out instead of returning only enough
		 * chars in result.last_plus_one to get to right edge.
		 */
		*op++ = SPACE;
		result.last_plus_one++;
		break;
	      default:
		tmp_x = process_result.pos.x;
		process_esbuf.last_plus_one--;
		process_result = ei_plain_text_process(
			 eih, EI_OP_MEASURE, &process_esbuf, x, rect->r_top,
				  PIX_SRC, (Xv_Window) 0, rect, tab_origin);
		if (private->x_font_info->per_char)  {
		    spaces_in_tab = (tmp_x - process_result.pos.x) /
		        private->x_font_info->per_char['m' - 
                          private->x_font_info->min_char_or_byte2].width;
		}
		else  {
		    spaces_in_tab = (tmp_x - process_result.pos.x) /
		        private->x_font_info->min_bounds.width;
		}
		if (result.last_plus_one
		    >= (out_buf_len - spaces_in_tab)) {
		    result.break_reason = EI_PR_BUF_FULL;
		    break;
		}
		for (i = 0; i < spaces_in_tab; i++) {
		    *op++ = ' ';
		    result.last_plus_one++;
		}
	    }
	    if (result.break_reason == EI_PR_BUF_FULL)
		break;
	} else if (*cp == NEWLINE) {
	    *op++ = SPACE;
	    result.last_plus_one++;
	} else if (!((*cp >= 0) && iswcntrl(*cp))
		   || private->state & CONTROL_CHARS_USE_FONT) {
	    *op++ = *cp;
	    result.last_plus_one++;
	} else {
	    if (result.last_plus_one < (out_buf_len - 2)) {
		*op++ = '^';
		*op++ = ((*cp >= 0) && iswcntrl(*cp)) ? *cp + 64 : '?';
		result.last_plus_one += 2;
	    } else {
		result.break_reason = EI_PR_BUF_FULL;
		break;
	    }
	}
    }
    return (result);
}

static struct ei_process_result
ei_plain_text_expand(eih, esbuf, rect, x, out_buf, out_buf_len, tab_origin)
    Ei_handle       eih;
    register Es_buf_handle esbuf;
    Rect           *rect;
    int             x;
    void           *out_buf;
    int             out_buf_len;
    int             tab_origin;
/*
 * Returns number of expanded chars in result.last_plus_one.
 * Result.break_reason = EI_PR_END_OF_STREAM if exhausted the entity stream.
 * EI_PR_BUF_EMPTIED if exhausted out_buf_len or esbuf. EI_PR_NEWLINE if a
 * character in the newline class was encountered. EI_PR_HIT_RIGHT if scan
 * reached right edge of rect.
 */
{
    if (!multibyte) 
        return(eiPlainTextExpandSB
              (eih, esbuf, rect, x, (char *)out_buf, out_buf_len, tab_origin));
    else
        return(eiPlainTextExpandWC
        (eih, esbuf, rect, x, (wchar_t *)out_buf, out_buf_len, tab_origin));
}
