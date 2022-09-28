/* Copyright (c) 1990 by Sun Microsystems, Inc. */
#ident	"@(#)gen_cosmult.c	1.5	92/06/09 SMI"

/*
 * gen_cosmult.c
 *
 * Description:
 *
 * Generates the cosine squared weighting function look up table used
 * by 'tsms()' and 'g721_tsms()'.  The table is actually a multiplication
 * table implements the function of multiplying a ulaw overlap region
 * sample with the corresponding value of cosine squared. More precisely
 * if the overlap region length is l, and we are at sample index, i, 
 * (relative to the start of the overlap region) and the sample at index
 * i, as a ulaw value of r, the function implemented by the table is:
 *
 * j = f2u(u2f(r)*(cos((PI/(2*(l + 1)))*i))^2)
 * 
 * Where f2u and u2f are ulaw to floating point coversions.
 *
 * For our implementation 'l', or 'olap' below (the only argument to
 * the generating program) is 64.  In this case the 'i' is gauranteed to
 * be a 6 bit or less number and 'r' is of course 8 bits.  So this table
 * replaces the above computation of j, for a fixed 'l' (specified at
 * the time of table generation) and given values for 'r' and 'i', by
 *
 * j = _cosmult64[(i<<8)|r]
 *
 */

#include <math.h>

#include "libaudio.h"

#define u2f(U) audio_s2d(audio_u2s(U))
#define f2u(F) audio_s2u(audio_d2s(F))

main(int argc, char **argv)
{
	unsigned int	i;
	unsigned char	j;
	unsigned char	l;
	double		freq;
	double		cos_sq;
	int		olap;
  
	if (argc > 1)
		sscanf(argv[1], "%d", &olap);
	else
		olap = 64;
  
	freq = M_PI_2 / (double)(olap + 1);
	printf("unsigned char\t_cosmult%d[%d] = {", olap, olap<<8);

	for (i = 1; i <= olap ; i++) {
		cos_sq = cos(i * freq);
		cos_sq *= cos_sq;
		for (l = 0; l < 255; l++) {
			j = f2u(u2f(l) * cos_sq);
			printf("%s%6d,", (l % 8)? " " : "\n\t", j);
		}
		j = f2u(u2f(l) * cos_sq);

		if (i < olap) {
			printf(" %6d,", j);
		} else {
			printf(" %6d", j);
		}
	}

	printf("\n};\n\n");

	return (0);
}
