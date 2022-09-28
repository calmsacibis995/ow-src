/*	@(#)charscreen.h 20.10 91/10/22 SMI	*/

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Definitions relating to physical screen image.
 */

/*
 * Macros to convert character coordinates to pixel coordinates.
 */
#define row_to_y(row)	((row)*chrheight)
#define col_to_x(col)	(((col)*chrwidth) + chrleftmargin)
#define y_to_row(y)	((y)/chrheight)
#define x_to_col(x)	((((x) >= chrleftmargin) ? \
			  ((x) - chrleftmargin) : 0)/chrwidth)

/*
 * Character dimensions (fixed width fonts only!)
 * and of screen in pixels.
 */
int	chrheight, chrwidth, chrbase;
int	winheightp, winwidthp;
int	chrleftmargin;

struct	pixfont *pixfont;

/*
 * If delaypainting, delay painting.  Set when clear screen.
 * When input will block then paint characters (! white space) of entire image
 * and turn delaypainting off.
 */
int	delaypainting;

#ifdef cplus
void	ttysw_pstring(char *s, int col, int row);
void	ttysw_pclearline(int fromcol, int tocol, int row);
void	ttysw_pcopyline(int fromcol, int tocol, int count, int row);
void	ttysw_pclearscreen(int fromrow, int torow, int count);
#endif
