#ifndef lint
static char sccsid[] = "@(#)dither.c	3.1 04/03/92 Copyright 1987-1990 Sun Microsystem, Inc." ;
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
 *
 * ------------------------------------------------------------------------
 *  This dither8to1() dithering code is taken from xloadimage. It uses
 *  error-diffusion dithering (floyd-steinberg).
 *
 *  Copyright 1989, 1990 Kirk L. Johnson
 *  Copyright 1989, 1990 Jim Frost and Steve Losen.
 *
 *  Permission to use, copy, modify, distribute, and sell this
 *  software and its documentation for any purpose is hereby granted
 *  without fee, provided that the above copyright notice appear in
 *  all copies and that both that copyright notice and this
 *  permission notice appear in supporting documentation. The
 *  author makes no representations about the suitability of this
 *  software for any purpose. It is provided "as is" without express
 *  or implied warranty.
 *
 *  THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 *  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 *  OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "snapshot.h"

/*  RGB intensity tables.
 *  red is (val * 0.30), green is (val * 0.59), blue is (val * .11),
 *  where val is intensity >> 8.
 *  These are used by the COLORINTENSITY() macro.
 */

static unsigned short red_ints[256] = {
      0,    76,   153,   230,   307,   384,   460,   537,
    614,   691,   768,   844,   921,   998,  1075,  1152,
   1228,  1305,  1382,  1459,  1536,  1612,  1689,  1766,
   1843,  1920,  1996,  2073,  2150,  2227,  2304,  2380,
   2457,  2534,  2611,  2688,  2764,  2841,  2918,  2995,
   3072,  3148,  3225,  3302,  3379,  3456,  3532,  3609,
   3686,  3763,  3840,  3916,  3993,  4070,  4147,  4224,
   4300,  4377,  4454,  4531,  4608,  4684,  4761,  4838,
   4915,  4992,  5068,  5145,  5222,  5299,  5376,  5452,
   5529,  5606,  5683,  5760,  5836,  5913,  5990,  6067,
   6144,  6220,  6297,  6374,  6451,  6528,  6604,  6681,
   6758,  6835,  6912,  6988,  7065,  7142,  7219,  7296,
   7372,  7449,  7526,  7603,  7680,  7756,  7833,  7910,
   7987,  8064,  8140,  8217,  8294,  8371,  8448,  8524,
   8601,  8678,  8755,  8832,  8908,  8985,  9062,  9139,
   9216,  9292,  9369,  9446,  9523,  9600,  9676,  9753,
   9830,  9907,  9984, 10060, 10137, 10214, 10291, 10368,
  10444, 10521, 10598, 10675, 10752, 10828, 10905, 10982,
  11059, 11136, 11212, 11289, 11366, 11443, 11520, 11596,
  11673, 11750, 11827, 11904, 11980, 12057, 12134, 12211,
  12288, 12364, 12441, 12518, 12595, 12672, 12748, 12825,
  12902, 12979, 13056, 13132, 13209, 13286, 13363, 13440,
  13516, 13593, 13670, 13747, 13824, 13900, 13977, 14054,
  14131, 14208, 14284, 14361, 14438, 14515, 14592, 14668,
  14745, 14822, 14899, 14976, 15052, 15129, 15206, 15283,
  15360, 15436, 15513, 15590, 15667, 15744, 15820, 15897,
  15974, 16051, 16128, 16204, 16281, 16358, 16435, 16512,
  16588, 16665, 16742, 16819, 16896, 16972, 17049, 17126,
  17203, 17280, 17356, 17433, 17510, 17587, 17664, 17740,
  17817, 17894, 17971, 18048, 18124, 18201, 18278, 18355,
  18432, 18508, 18585, 18662, 18739, 18816, 18892, 18969,
  19046, 19123, 19200, 19276, 19353, 19430, 19507, 19584
} ;

static unsigned short green_ints[256] = {
     0,   151,   302,   453,   604,   755,   906,  1057,
  1208,  1359,  1510,  1661,  1812,  1963,  2114,  2265,
  2416,  2567,  2718,  2869,  3020,  3171,  3322,  3473,
  3624,  3776,  3927,  4078,  4229,  4380,  4531,  4682,
  4833,  4984,  5135,  5286,  5437,  5588,  5739,  5890,
  6041,  6192,  6343,  6494,  6645,  6796,  6947,  7098,
  7249,  7400,  7552,  7703,  7854,  8005,  8156,  8307,
  8458,  8609,  8760,  8911,  9062,  9213,  9364,  9515,
  9666,  9817,  9968, 10119, 10270, 10421, 10572, 10723,
 10874, 11025, 11176, 11328, 11479, 11630, 11781, 11932,
 12083, 12234, 12385, 12536, 12687, 12838, 12989, 13140,
 13291, 13442, 13593, 13744, 13895, 14046, 14197, 14348,
 14499, 14650, 14801, 14952, 15104, 15255, 15406, 15557,
 15708, 15859, 16010, 16161, 16312, 16463, 16614, 16765,
 16916, 17067, 17218, 17369, 17520, 17671, 17822, 17973,
 18124, 18275, 18426, 18577, 18728, 18880, 19031, 19182,
 19333, 19484, 19635, 19786, 19937, 20088, 20239, 20390,
 20541, 20692, 20843, 20994, 21145, 21296, 21447, 21598,
 21749, 21900, 22051, 22202, 22353, 22504, 22656, 22807,
 22958, 23109, 23260, 23411, 23562, 23713, 23864, 24015,
 24166, 24317, 24468, 24619, 24770, 24921, 25072, 25223,
 25374, 25525, 25676, 25827, 25978, 26129, 26280, 26432,
 26583, 26734, 26885, 27036, 27187, 27338, 27489, 27640,
 27791, 27942, 28093, 28244, 28395, 28546, 28697, 28848,
 28999, 29150, 29301, 29452, 29603, 29754, 29905, 30056,
 30208, 30359, 30510, 30661, 30812, 30963, 31114, 31265,
 31416, 31567, 31718, 31869, 32020, 32171, 32322, 32473,
 32624, 32775, 32926, 33077, 33228, 33379, 33530, 33681,
 33832, 33984, 34135, 34286, 34437, 34588, 34739, 34890,
 35041, 35192, 35343, 35494, 35645, 35796, 35947, 36098,
 36249, 36400, 36551, 36702, 36853, 37004, 37155, 37306,
 37457, 37608, 37760, 37911, 38062, 38213, 38364, 38515
} ;

static unsigned short blue_ints[256] = {
     0,   28,   56,   84,  112,  140,  168,  197,
   225,  253,  281,  309,  337,  366,  394,  422,
   450,  478,  506,  535,  563,  591,  619,  647,
   675,  704,  732,  760,  788,  816,  844,  872,
   901,  929,  957,  985, 1013, 1041, 1070, 1098,
  1126, 1154, 1182, 1210, 1239, 1267, 1295, 1323,
  1351, 1379, 1408, 1436, 1464, 1492, 1520, 1548,
  1576, 1605, 1633, 1661, 1689, 1717, 1745, 1774,
  1802, 1830, 1858, 1886, 1914, 1943, 1971, 1999,
  2027, 2055, 2083, 2112, 2140, 2168, 2196, 2224,
  2252, 2280, 2309, 2337, 2365, 2393, 2421, 2449,
  2478, 2506, 2534, 2562, 2590, 2618, 2647, 2675,
  2703, 2731, 2759, 2787, 2816, 2844, 2872, 2900,
  2928, 2956, 2984, 3013, 3041, 3069, 3097, 3125,
  3153, 3182, 3210, 3238, 3266, 3294, 3322, 3351,
  3379, 3407, 3435, 3463, 3491, 3520, 3548, 3576,
  3604, 3632, 3660, 3688, 3717, 3745, 3773, 3801,
  3829, 3857, 3886, 3914, 3942, 3970, 3998, 4026,
  4055, 4083, 4111, 4139, 4167, 4195, 4224, 4252,
  4280, 4308, 4336, 4364, 4392, 4421, 4449, 4477,
  4505, 4533, 4561, 4590, 4618, 4646, 4674, 4702,
  4730, 4759, 4787, 4815, 4843, 4871, 4899, 4928,
  4956, 4984, 5012, 5040, 5068, 5096, 5125, 5153,
  5181, 5209, 5237, 5265, 5294, 5322, 5350, 5378,
  5406, 5434, 5463, 5491, 5519, 5547, 5575, 5603,
  5632, 5660, 5688, 5716, 5744, 5772, 5800, 5829,
  5857, 5885, 5913, 5941, 5969, 5998, 6026, 6054,
  6082, 6110, 6138, 6167, 6195, 6223, 6251, 6279,
  6307, 6336, 6364, 6392, 6420, 6448, 6476, 6504,
  6533, 6561, 6589, 6617, 6645, 6673, 6702, 6730,
  6758, 6786, 6814, 6842, 6871, 6899, 6927, 6955,
  6983, 7011, 7040, 7068, 7096, 7124, 7152, 7180
} ;

/* This returns the (approximate) intensity of an RGB triple. */

#define  COLORINTENSITY(R, G, B)  (red_ints[(R)] + green_ints[(G)] + \
                                   blue_ints[(B)])

#define MAXGREY       32768        /* Limits on the grey levels used */
#define THRESHOLD     16384        /* in the dithering process */
#define MINGREY           0

static unsigned int tone_scale_adjust P((unsigned int)) ;
static void left_to_right P((int *, int *, int)) ;
static void right_to_left P((int *, int *, int)) ;


image_t *
compress(old)     /* Compress the colormap and adjust the image. */
image_t *old ;
{
  unsigned char *iptr ;
  unsigned long color ;
  unsigned long *pixel ;
  unsigned int *used ;
  unsigned int a, x, y ;
  image_t *new ;
 
  new                 = new_image() ;
  new->type           = old->type ;
  new->width          = old->width ;
  new->height         = old->height ;
  new->depth          = old->depth ;
  new->cmaptype       = old->cmaptype ;
  new->cmapused       = 0 ;
  new->bytes_per_line = old->bytes_per_line ;

  new->red      = (unsigned char *) malloc((unsigned int) old->cmapused) ;
  new->green    = (unsigned char *) malloc((unsigned int) old->cmapused) ;
  new->blue     = (unsigned char *) malloc((unsigned int) old->cmapused) ;
  new->data     = copy_imagedata(old) ;
 
  pixel = (unsigned long *) malloc(sizeof(unsigned long) * old->cmapused) ;
  used  = (unsigned int *)  malloc(sizeof(unsigned int)  * old->cmapused) ;
  iptr  = (unsigned char *) new->data ;
  for (x = 0; x < old->cmapused; x++) *(used + x) = NULL ;
 
  for (y = 0; y < old->height; y++)
    {
      for (x = 0; x < old->width; x++)
        {
          color = *iptr ;
          if (*(used + color) == 0)
            {
              for (a = 0; a < new->cmapused; a++)
                if ((*(new->red   + a) == *(old->red   + color)) &&
                    (*(new->green + a) == *(old->green + color)) &&
                    (*(new->blue  + a) == *(old->blue  + color)))
                  break ;
              *(pixel + color) = a ;
              *(used + color)  = 1 ;
              if (a == new->cmapused)
                {
                  *(new->red   + a) = *(old->red   + color) ;
                  *(new->green + a) = *(old->green + color) ;
                  *(new->blue  + a) = *(old->blue  + color) ;
                  new->cmapused++ ;
                }
            }
          *iptr = (unsigned char) pixel[(unsigned int) color] ;
          iptr++ ;
        }
      iptr += (old->bytes_per_line - old->width);
    }
  return(new) ;
}


/* Simple floyd-steinberg dither with serpentine raster processing. */

image_t *
dither8to1(old)
image_t *old ;
{
  image_t *new ;
  unsigned int  *grey ;         /* Grey map for source image. */
  unsigned int   dll ;          /* Destination line length in bytes. */
  unsigned char *src ;          /* Source data. */
  unsigned char *dst ;          /* Destination data. */
  int           *curr ;         /* Current line buffer. */
  int           *next ;         /* Next line buffer. */
  int           *swap ;         /* For swapping line buffers. */
  unsigned long  color ;        /* Pixel color. */
  unsigned int   level ;        /* Grey level. */
  unsigned int   i, j ;         /* Loop counters. */
  int            src_rem ;      /* Source scanline remainder. */

  new           = new_image() ;
  new->type     = old->type ;
  new->width    = old->width ;
  new->height   = old->height ;
  new->depth    = 1 ;
  new->cmaptype = RMT_NONE ;
  new->cmapused = 0 ;

  new->bytes_per_line = dll = linelen (new->width, new->depth) ;
  src_rem = linelen (old->width, old->depth) - (old->width * old->depth / 8) ;

  new->data = (unsigned char *) ck_zmalloc((size_t) (dll * old->height)) ;
  src       = old->data ;
  dst       = new->data ;

/*  Compute the grey level for each entry and store it in grey[]. */

  grey = (unsigned int *) malloc(sizeof(unsigned int) * old->cmapused) ;
  for (i = 0; i < old->cmapused; i++)
    grey[i] = (((int) COLORINTENSITY(old->red[i],
                                     old->green[i], old->blue[i])) >> 1) ;

  for (i = 0; i < old->cmapused; i++) grey[i] = tone_scale_adjust(grey[i]) ;

/* Dither setup. */

  curr  = (int *) malloc(sizeof(int) * (old->width + 2)) ;
  next  = (int *) malloc(sizeof(int) * (old->width + 2)) ;
  curr += 1 ;
  next += 1 ;
  for (j = 0; j < old->width; j++) curr[j] = next[j] = 0 ;

/* Primary dither loop. */

  for (i = 0; i < old->height; i++)
    {

/* Copy the row into the current line. */

      for (j = 0; j < old->width; j++)
        {
          color    = *src++ ;
          level    = grey[color] ;
          curr[j] += level ;
        }

/* Dither the current line */

      if (i & 1) right_to_left(curr, next, old->width) ;
      else       left_to_right(curr, next, old->width) ;

/* Copy the dithered line to the destination image. */

      for (j = 0; j < old->width; j++)
        if (curr[j] < THRESHOLD) dst[j / 8] |= 1 << (7 - (j & 7)) ;
      src += src_rem ;
      dst += dll ;
    
/* Circulate the line buffers */

      swap = curr ;
      curr = next ;
      next = swap ;
      for (j = 0; j < old->width; j++) next[j] = 0 ;
    }

/* Clean up. */

  if (grey != NULL) FREE(grey) ;
  FREE(curr-1) ;
  FREE(next-1) ;
  return((image_t *) new) ;
}


/*  A _very_ simple tone scale adjustment routine. provides a piecewise
 *  linear mapping according to the following:
 *
 *      input:          output:
 *     0 (MINGREY)    0 (MINGREY)
 *     THRESHOLD      THRESHOLD/2
 *     MAXGREY        MAXGREY
 * 
 *  This should help things look a bit better on most displays.
 */

static unsigned int
tone_scale_adjust(val)
unsigned int val ;
{
  unsigned int rslt ;
  
  if (val < THRESHOLD) rslt = val / 2 ;
  else
    rslt = (((val - THRESHOLD) * (MAXGREY -(THRESHOLD / 2))) /
            (MAXGREY - THRESHOLD)) + (THRESHOLD / 2) ;
  return(rslt) ;
}


/* Dither a line from left to right. */

static void
left_to_right(curr, next, width)
int *curr, *next, width ;
{
  int idx, error, output ;

  for (idx = 0; idx < width; idx++)
    {
      output       = (curr[idx] > THRESHOLD) ? MAXGREY : MINGREY ;
      error        = curr[idx] - output ;
      curr[idx]    = output ;
      next[idx-1] += error * 3 / 16 ;
      next[idx]   += error * 5 / 16 ;
      next[idx+1] += error * 1 / 16 ;
      curr[idx+1] += error * 7 / 16 ;
    }
}


/* Dither a line from right to left. */

static void
right_to_left(curr, next, width)
int *curr, *next, width ;
{
  int idx, error, output ;

  for (idx = (width-1); idx >= 0; idx--)
    {
      output       = (curr[idx] > THRESHOLD) ? MAXGREY : MINGREY ;
      error        = curr[idx] - output ;
      curr[idx]    = output ;
      next[idx+1] += error * 3 / 16 ;
      next[idx]   += error * 5 / 16 ;
      next[idx-1] += error * 1 / 16 ;
      curr[idx-1] += error * 7 / 16 ;
    }
}


/*  I should point out that the actual fractions we used were, assuming
 *  you are at X, moving left to right:
 *
 *                X     7/16
 *         3/16   5/16  1/16    
 *
 *  Note that the error goes to four neighbors, not three.  I think this
 *  will probably do better (at least for black and white) than the
 *  3/8-3/8-1/4 distribution, at the cost of greater processing.  I have
 *  seen the 3/8-3/8-1/4 distribution described as "our" algorithm before,
 *  but I have no idea who the credit really belongs to.
 *
 *  Also, I should add that if you do zig-zag scanning, it is sufficient
 *  (but not quite as good) to send half the error one pixel ahead (e.g.
 *  to the right on lines you scan left to right), and half one pixel
 *  straight down. Again, this is for black and white; I've not tried it
 *  with color.
 *
 *          Lou Steinberg
 */

#define  FS_GETPIX  rval = *thisptr++ ;                         \
                    gval = *thisptr++ ;                         \
                    bval = *thisptr++ ;                         \
                                                                \
                    r2 = rval * nr / 255 ;                      \
                    g2 = gval * ng / 255 ;                      \
                    b2 = bval * nb / 255 ;                      \
                    if (r2 > nr) r2 = nr ;                      \
                    if (g2 > ng) g2 = ng ;                      \
                    if (b2 > nb) b2 = nb ;                      \
                    tmp = (((g2 * num_b) + b2) * num_r) + r2 ;  \
                    *dst++ = tmp ;                              \
                                                                \
                    rval -= new->red[tmp] ;                     \
                    gval -= new->green[tmp] ;                   \
                    bval -= new->blue[tmp]

image_t *
dither24to8(old)
image_t *old ;
{
  unsigned char *src, *dst ;
  unsigned char current_red, current_blue, current_green, current_color;
  short *thisline, *nextline, *tmpptr ;
  short *tline, *nline;
  int num_r, num_g, num_b ;
  int nr, ng, nb ;
  int rval, gval, bval ;
  register short *thisptr, *nextptr ;
  register int i, j, k;
  int cmap_size ;
  int bytes_per_pixel ;
  int linebytes, dll ;
  int min_r, max_r, min_g, max_g, min_b, max_b ;
  int use_cmap, dst_rem, src_rem, tmp ;
  int r2, g2, b2 ;
  int ir, ig, ib, lr, lg, lb, ptr ;
  int colors_ok = 1;
  int found;

  image_t *new ;

  num_r = 8 ;
  num_g = 8 ;
  num_b = 4 ;
  min_r = min_g = min_b = 0 ;
  max_r = max_g = max_b = 255 ;
  nr    = num_r - 1 ;
  ng    = num_g - 1 ;
  nb    = num_b - 1 ;

  cmap_size = num_r * num_g * num_b ;

  new           = new_image() ;
  new->type     = RT_STANDARD ;
  new->width    = old->width ;
  new->height   = old->height ;
  new->depth    = 8 ;
  new->cmaptype = RMT_EQUAL_RGB ;
  new->cmapused = cmap_size ;
  new->red      = (unsigned char *) malloc((unsigned) cmap_size) ;
  new->green    = (unsigned char *) malloc((unsigned) cmap_size) ;
  new->blue     = (unsigned char *) malloc((unsigned) cmap_size) ;

  use_cmap = (old->type == RMT_EQUAL_RGB) && (old->cmapused > 0) ;

  switch (old->depth)
    {
      case 24 : bytes_per_pixel = 3 ;
                break ;
      case 32 : bytes_per_pixel = 4 ;
    }
  linebytes = old->width * bytes_per_pixel ;
  linebytes += linebytes % 2 ;

  dll                 =  old->width ;
  dll                += dll % 2 ;
  new->bytes_per_line = dll ;
  src_rem = linelen (old->width, old->depth) - (old->width * old->depth / 8) ;
  dst_rem = linelen (new->width, new->depth) - (new->width * new->depth / 8) ;
 
  new->data = (unsigned char *) ck_zmalloc((size_t) (dll * old->height)) ;
  src       = old->data ;
  dst       = new->data ;
 
/*
 * Check the image and see how many colors we have.
 * If > 256 then continue.
 * If <= 256 then we can skip this.
 */
 
  if (old->depth == 32)
     src++;
  current_red = *src;
  current_green = *(src+1);
  current_blue = *(src+2);
  new->red [0] = current_red;
  new->green [0] = current_green;
  new->blue [0] = current_blue;
  current_color = 0;
  if (old->depth == 32)
     src--;
 
  for (i = 0; i < old->height && colors_ok; i++) {
      for (j = 0; j < old->width && colors_ok; j++) {
	  if (old->depth == 32)
	     src++;
          if ((*src == current_red) && (*(src+1) == current_green) &&
              (*(src+2) == current_blue)) {
             *dst++ = current_color;
             src += 3;
             }
          else {
             found = 0;
             for (k = 0; k <= (int) current_color && !found ; k++)
                 if ((*src == new->red[k]) && (*(src+1) == new->green[k]) &&
                     (*(src+2) == new->blue[k])) {
                    *dst++ = (unsigned char) k;
                    src += 3;
                    found = 1;
                    }
             if (!found) {
                if ( (int) (current_color + 1) == cmap_size)
                   colors_ok = 0;
                else {
                   current_red = *src++;
                   current_green = *src++;
                   current_blue = *src++;
                   current_color++;
                   new->red [current_color] = current_red;
                   new->green [current_color] = current_green;
                   new->blue [current_color] = current_blue;
                   *dst++ = current_color;
                   }
                }
             }
          }
      src += src_rem ;
      dst += dst_rem ;
      }
 
  if (colors_ok) {
     new->cmapused = current_color + 1;
     return (new);
     }
 
  memset ( (char *) new->data, 0, dll * old->height);
  lr = max_r - min_r ;
  lg = max_g - min_g ;
  lb = max_b - min_b ;
  nr = num_r - 1 ;
  ng = num_g - 1 ;
  nb = num_b - 1 ;

  ptr = 0 ;
  for (ig = 0; ig < num_g; ++ig)
    for (ib = 0; ib < num_b; ++ib)
      for (ir = 0; ir < num_r; ++ir)
        {
          new->red[ptr]   = min_r + ir * lr / nr ;
          new->green[ptr] = min_g + ig * lg / ng ;
          new->blue[ptr]  = min_b + ib * lb / nb ;
          ++ptr ;
        }

/*
  dll                 =  old->width ;
  dll                += dll % 2 ;
  new->bytes_per_line = dll ;
  src_rem = linelen (old->width, old->depth) - (old->width * old->depth / 8) ;
  dst_rem = linelen (new->width, new->depth) - (new->width * new->depth / 8) ;

  new->data = (unsigned char *) ck_zmalloc((size_t) (dll * old->height)) ;
*/
  src       = old->data ;
  dst       = new->data ;

  tline = (short *)
             LINT_CAST(malloc((unsigned) (old->width * 3 * sizeof(short)))) ;
  nline = (short *)
             LINT_CAST(malloc((unsigned) (old->width * 3 * sizeof(short)))) ;

  thisline = tline;
  nextline = nline;
  nextptr = nextline ;                  /* Get first line. */
  for (j = 0; j < old->width; ++j)
    {
      switch (old->depth)
        {
          case 8  : rval = gval = bval = *src++ ;
                    break ;
          case 32 : ++src ;
          case 24 : rval = *src++ ;
                    gval = *src++ ;
                    bval = *src++ ;
        }
      if (use_cmap)
        {
          rval = *(old->red   + rval) ;
          gval = *(old->green + gval) ;
          bval = *(old->blue  + bval) ;
        }
      *nextptr++ = rval ;
      *nextptr++ = gval ;
      *nextptr++ = bval ;
    }

  src += src_rem ;
  dst += dst_rem ;

  for (i = 0; i < old->height-1; ++i)
    {
      tmpptr   = thisline ;
      thisline = nextline ;
      nextline = tmpptr ;
      nextptr = nextline ;
      for (j = 0; j < old->width; ++j)
        {
          switch (old->depth)
            {
              case 8  : rval = gval = bval = *src++ ;
                        break ;
              case 32 : ++src ;
              case 24 : rval = *src++ ;
                        gval = *src++ ;
                        bval = *src++ ;
                        break ;
            }
          if (use_cmap)
            {
              rval = *(old->red   + rval) ;
              gval = *(old->green + gval) ;
              bval = *(old->blue  + bval) ;
            }
          *nextptr++ = rval ;
          *nextptr++ = gval ;
          *nextptr++ = bval ;
        }
      src += src_rem ;
      dst += dst_rem ;

      thisptr = thisline ;
      nextptr = nextline ;

      FS_GETPIX ;                    /* Special case: first pixel on line. */

/*
 * The following 8 lines used to be ordered such that it looked 
 * like this function expected the data to be ordered in bgr format
 * rather than in rgb format. However, we've taken care of that already 
 * in the `adjust_image' function, so the pixel values are already in 
 * rgb format. 
 */

      thisptr[0] += rval * 7 / 16 ;
      thisptr[1] += gval * 7 / 16 ;
      thisptr[2] += bval * 7 / 16 ;
      nextptr[0] += rval * 5 / 16 ;
      nextptr[1] += gval * 5 / 16 ;
      nextptr[2] += bval * 5 / 16 ;
      nextptr[3] += rval     / 16 ;
      nextptr[4] += gval     / 16 ;
      nextptr[5] += bval     / 16 ;
      nextptr += 3 ;

/* Next case: most of the rest of the line */

      for (j = 1; j < old->width-1; ++j)
        {
          FS_GETPIX ;

/*
 * Again, these used to be ordered differently.. (looked like it expected
 * the data to be in bgr format, not rgb).
 */

          thisptr[0]  += rval * 7 / 16 ;
          thisptr[1]  += gval * 7 / 16 ;
          thisptr[2]  += bval * 7 / 16 ;
          nextptr[-3] += rval * 3 / 16 ;
          nextptr[-2] += gval * 3 / 16 ;
          nextptr[-1] += bval * 3 / 16 ;
          nextptr[0]  += rval * 5 / 16 ;
          nextptr[1]  += gval * 5 / 16 ;
          nextptr[2]  += bval * 5 / 16 ;
          nextptr[3]  += rval     / 16 ;
          nextptr[4]  += gval     / 16 ;
          nextptr[5]  += bval     / 16 ;
          nextptr += 3 ;
        }

      FS_GETPIX ;                      /* Last case: last pixel on line */

/*
 * Same here (bgr vs. rgb).
 */

      nextptr[-3] += rval * 3 / 16 ;
      nextptr[-2] += gval * 3 / 16 ;
      nextptr[-1] += bval * 3 / 16 ;
      nextptr[0]  += rval * 5 / 16 ;
      nextptr[1]  += gval * 5 / 16 ;
      nextptr[2]  += bval * 5 / 16 ;
    }

  thisline = nextline ;                /* Special case: last line: */
  thisptr = thisline ;

  for (j = 0; j < old->width-1; ++j)   /* First case: most of the line */
    {
      FS_GETPIX ;

/*
 * And here (bgr vs. rgb).
 */

      thisptr[0] += rval * 7 / 16 ;
      thisptr[1] += gval * 7 / 16 ;
      thisptr[2] += bval * 7 / 16 ;
    }

  FS_GETPIX ;                          /* Last case: last pixel on line */

  FREE((short *) tline) ;
  FREE((short *) nline) ;
  return(new) ;
}


image_t *
expand1to8(old)
image_t *old ;
{
  unsigned char *dst, *linep, *src ;
  int dll, i, j, linesize, mask, dst_rem ;
  image_t *new ;

  new           = new_image() ;
  new->type     = old->type ;
  new->width    = old->width ;
  new->height   = old->height ;
  new->depth    = 8 ;
  new->cmaptype = RMT_EQUAL_RGB ;
  new->cmapused = 2 ;
  new->red      = (unsigned char *) malloc((unsigned) new->cmapused) ;
  new->green    = (unsigned char *) malloc((unsigned) new->cmapused) ;
  new->blue     = (unsigned char *) malloc((unsigned) new->cmapused) ;

  dll	      = linelen (new->width, new->depth);
  new->bytes_per_line = dll;
  new->data   = (unsigned char *) ck_zmalloc((size_t) (dll * new->height)) ;
  memset((char *) new->data, 0, dll * old->height) ;

  new->red[0] = new->green[0] = new->blue[0] = 255 ;    /* White. */
  new->red[1] = new->green[1] = new->blue[1] = 0 ;      /* Black. */

  linep       = old->data ;
  dst         = new->data ;
  linesize    = linelen (old->width, old->depth) ;

  dst_rem = linelen (new->width, new->depth) - (new->width * new->depth / 8) ;

  for (i = 0; i < old->height; i++)
    {
      src = linep ;
      mask = 0x80 ;
      for (j = 0; j < old->width; j++, dst++)
        {
          if (mask == 0)
            {
              src++ ;
              mask = 0x80 ;
            }
          if (*src & mask) *dst = 1 ;
          mask >>= 1 ;
        }
      linep += linesize ;
      dst += dst_rem ;
    }
  return(new) ;
}


/*ARGSUSED*/
image_t *
expand8to24(old)
image_t *old ;
{
  unsigned char *dst, *linep, *src ;
  int dll, i, j, linesize;
  image_t *new ;

  new           = new_image() ;
  new->type     = old->type ;
  new->width    = old->width ;
  new->height   = old->height ;
  new->depth    = 24 ;
  new->cmaptype = RMT_NONE;
  new->cmapused = 0 ;
  new->red      = (unsigned char *) NULL;
  new->green    = (unsigned char *) NULL;
  new->blue     = (unsigned char *) NULL;
 
  dll         =  old->width * 4 ;
  new->data   = (unsigned char *) ck_zmalloc((size_t) (dll * old->height)) ;
  memset((char *) new->data, 0, dll * old->height) ;
 
  linep       = old->data ;
  dst         = new->data ;
  linesize    = linelen (old->width, old->depth) ;
                          
  for (i = 0; i < old->height; i++) {
      src = linep;
      for (j = 0; j < old->width; j++) {
	  *dst++ = 0;
          *dst++ = old->blue [*src];
          *dst++ = old->green [*src];
          *dst++ = old->red [*src];
	  src++;
	  }
      linep += linesize ;
      }
  return(new) ;
}


image_t *
gray(old)
image_t *old ;
{
  int i ;
  image_t *new ;
  unsigned short intensity ;
 
  new           = new_image() ;
  new->type     = old->type ;
  new->width    = old->width ;
  new->height   = old->height ;
  new->depth    = old->depth ;
  new->bytes_per_line = old->bytes_per_line ;
  new->cmaptype = old->cmaptype ;
  new->cmapused = old->cmapused ;
  new->red      = (unsigned char *) malloc((unsigned) new->cmapused) ;
  new->green    = (unsigned char *) malloc((unsigned) new->cmapused) ;
  new->blue     = (unsigned char *) malloc((unsigned) new->cmapused) ;
  new->data     = copy_imagedata(old) ;

  for (i = 0; i < old->cmapused; i++)
    {
      intensity     = ((int) (COLORINTENSITY(old->red[i], old->green[i],
                                             old->blue[i])) >> 8) ;
      new->red[i]   = (unsigned char) intensity ;
      new->green[i] = (unsigned char) intensity ;
      new->blue[i]  = (unsigned char) intensity ;
    }
  return((image_t *) new) ;
}
