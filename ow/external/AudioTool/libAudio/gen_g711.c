/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident "@(#)gen_g711.c	1.3	92/06/09 SMI"

/*
 * file gen_g711.c
 *
 * generate g711.c file for u-law, A-law and linear PCM conversions.
 */
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static int seg_endpts_alaw[NSEGS] = {32, 64, 128, 256, 512, 1024, 2048, 4096};

/* copy from CCITT G.711 specifications. */
unsigned char _u2a[128] = {			/* Mu- to A-law conversions */
        1,      1,      2,      2,      3,      3,      4,      4,
        5,      5,      6,      6,      7,      7,      8,      8,
        9,      10,     11,     12,     13,     14,     15,     16,
        17,     18,     19,     20,     21,     22,     23,     24,
        25,     27,     29,     31,     33,     34,     35,     36,
        37,     38,     39,     40,     41,     42,     43,     44,
        46,     48,     49,     50,     51,     52,     53,     54,
        55,     56,     57,     58,     59,     60,     61,     62,
        64,     65,     66,     67,     68,     69,     70,     71,
        72,     73,     74,     75,     76,     77,     78,     79,
        81,     82,     83,     84,     85,     86,     87,     88,
        89,     90,     91,     92,     93,     94,     95,     96,
        97,     98,     99,     100,    101,    102,    103,    104,
        105,    106,    107,    108,    109,    110,    111,    112,
        113,    114,    115,    116,    117,    118,    119,    120,
        121,    122,    123,    124,    125,    126,    127,    128};

unsigned char _a2u[128] = {			/* A- to Mu-law conversions */
        1,      3,      5,      7,      9,      11,     13,     15,
        16,     17,     18,     19,     20,     21,     22,     23,
        24,     25,     26,     27,     28,     29,     30,     31,
        32,     32,     33,     33,     34,     34,     35,     35,
        36,     37,     38,     39,     40,     41,     42,     43,
        44,     45,     46,     47,     48,     48,     49,     49,
        50,     51,     52,     53,     54,     55,     56,     57,
        58,     59,     60,     61,     62,     63,     64,     64,
        65,     66,     67,     68,     69,     70,     71,     72,
        73,     74,     75,     76,     77,     78,     79,     79,
        80,     81,     82,     83,     84,     85,     86,     87,
        88,     89,     90,     91,     92,     93,     94,     95,
        96,     97,     98,     99,     100,    101,    102,    103,
        104,    105,    106,    107,    108,    109,    110,    111,
        112,    113,    114,    115,    116,    117,    118,    119,
        120,    121,    122,    123,    124,    125,    126,    127};

/*
 * audio_linear2alaw() - Convert a linear PCM value to A-law
 *
 * audio_linear2alaw() accepts a short integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
audio_linear2alaw(int pcm_val)		/* 2's complement (13-bit precision) */
{
	int		mask;
	int		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;
	} else {
		mask = 0x55;
		pcm_val = -pcm_val - 1;
	}

	/*
	 * Convert the scaled magnitude to segment number.
	 *
	 * Combine the sign, segment, and quantization bits to build
	 * the code word.
	 */
	for (seg = 0; seg < NSEGS; seg++)
		if (pcm_val < seg_endpts_alaw[seg]) {
			aval = seg << SEG_SHIFT;
			if (seg < 2)
				aval |= (pcm_val >> 1) & QUANT_MASK;
			else
				aval |= (pcm_val >> seg) & QUANT_MASK;
			return (aval ^ mask);
		}
	/* If magnitude is out of range, return maximum value. */
	return (0x7F ^ mask);
}

/*
 * audio_alaw2linear() - Convert an A-law value to linear PCM
 *
 * audio_alaw2linear() decodes the A-law code word by the inverse process.
 */
int
audio_alaw2linear(int a_val)
{
	int	t, seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 1;
	seg = (a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t++;
		break;
	case 1:
		t += 0x21;
		break;
	default:
		t += 0x21;
		t <<= seg - 1;
	}
	return (a_val & SIGN_BIT)? t : -t;
}

#define	BIAS		(33)		/* Bias for linear code. */

int	seg_endpts_mulaw[NSEGS] = {0x40, 0x80, 0x100, 0x200, 0x400, 0x800,
	    0x1000, 0x2000};

/*
 * audio_linear2ulaw() - Convert a linear PCM value to u-law
 *
 * audio_linear2ulaw() accepts a short integer and encodes it as u-law data.
 *
 * U-law encoding uses a non-linear function to compress the input signal
 * and then quantizes it so that successively larger input signal intervals
 * are compressed into constant length quantization intervals. It serves
 * to increase the dynamic range at the expense of the signal-to-noise
 * ratio for large amplitude signals. The u-law characteristic is given
 * by:
 *
 *		       ln (1 + u |x|)
 *	F (x) = sgn(x) --------------
 *	 u	       ln (1 + u)
 *
 * where x is the input signal (-1 <= x <= 1), sgn(x) is the polarity of x,
 * and u is a parameter used to define the amount of compression (u = 255).
 * The expansion characteristic for the u-law compander is given by:
 *
 *	   -1			      |y|
 *	F    (y) = sgn(y)(1/u)[(1 + u)   -1)
 *	 u
 *
 * When u is 255, the companding characteristic is closely approximated
 * by a set of eight straight line segments with the slope of each
 * successive segment being exactly one half of the slope of the previous
 * segment.
 *
 * The algorithm used here implements a u255 PCM encoder using a 14 bit
 * uniform sample as input. The polarity is saved, and the magnitude of
 * the sample is encoded into 3 bits representing the segment number and
 * 4 bits representing the quantization interval within the segment.
 * The resulting code word has the form PSSSQQQQ. In order to simplify
 * the encoding process, the original linear magnitude is biased by adding
 * 33 which shifts the encoding range from (0 - 8158) to (33 - 8191). The
 * result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. (In our case we find the segment number by testing
 * against the value of the segment endpoint, which is adjusted for the bias).
 * The quantization interval is directly available as the four bits wxyz.
 * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
audio_linear2ulaw(int pcm_val)	/* 2's complement (14-bit precision) */
{
	int		sign;
	int		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = BIAS - pcm_val;
		sign = SIGN_BIT;
	} else {
		pcm_val += BIAS;
		sign = 0;
	}

	/*
	 * Convert the scaled magnitude to segment number.
	 *
	 * Combine the sign, segment, and quantization bits to build
	 * the code word.  Complement the code word to make it fit for
	 * SPARCstation audio and the ISDN world.
	 */
	for (seg = 0; seg < NSEGS; seg++) {
		if (pcm_val < seg_endpts_mulaw[seg]) {
			uval = (seg << SEG_SHIFT) |
			    ((pcm_val >> (seg + 1)) & QUANT_MASK);
			if (uval == 0)
				return 0xFF;
			else
				return (~(sign | uval));
		}
	}

	/* If magnitude is out of range, return maximum value. */
	return (~(sign | SEG_MASK | QUANT_MASK));
}

/*
 * audio_ulaw2linear() - Convert a u-law value to linear PCM
 *
 * audio_ulaw2linear() decodes the u-law code word by the inverse process.
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with SPARCstation and ISDN
 * conventions.
 */
int
audio_ulaw2linear(int u_val)
{
	int	t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits.
	 * Shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 1) + BIAS;
	t <<= ((u_val & SEG_MASK) >> SEG_SHIFT);

	if (u_val & SIGN_BIT)
		return (BIAS - t);
	else
		return (t - BIAS);
}
/*
 * Create linear PCM <-> A-law conversion lookup tables
 */
main(void)
{
	int	i;

	printf("/* Copyright (c) 1992 by Sun Microsystems, Inc. */\n\n");

	/* A-law to PCM */
	printf("/* 8-bit A-law to 16-bit signed PCM */\n");
	printf("short\t_alaw2linear[256] = {");

	for (i = 0; i < 0x100; i++)
		printf("%s%6d,", (i % 8) ? " ": "\n\t",
		    audio_alaw2linear(i) << 3);

	printf("\n};\n\n");

	/* PCM to A-law */
	printf("/* 13-bit unsigned PCM to inverted 8-bit A-law */\n");
	printf("unsigned char\t_ulinear2alaw[0x2000] = {");

	/* Loop through 13-bit space */
	for (i = -0x1000; i < 0x1000; i++)
		printf("%s0x%02x,", ((i + 4) % 12) ? " ": "\n\t",
		    audio_linear2alaw(i));
	printf("\n};\n\n");

	printf("/* 13-bit signed PCM to inverted 8-bit A-law */\n");
	printf("unsigned char\t*_linear2alaw = _ulinear2alaw + 0x1000;\n");

/*
 * Create linear PCM <-> u-law conversion lookup tables
 */

	/* u-law to PCM */
	printf("/* Inverted 8-bit u-law to 16-bit signed PCM */\n");
	printf("short\t_ulaw2linear[256] = {");

	for (i = 0; i < 0x100; i++)
		printf("%s%6d,", (i % 8) ? " ": "\n\t",
		    audio_ulaw2linear(i) << 2);

	printf("\n};\n\n");

	/* PCM to u-law */
	printf("/* 14-bit unsigned PCM to inverted 8-bit u-law */\n");
	printf("unsigned char\t_ulinear2ulaw[0x4000] = {");

	/* Loop through 14-bit space */
	for (i = -0x2000; i < 0x2000; i++)
		printf("%s0x%02x,", ((i + 4) % 12) ? " ": "\n\t",
		    audio_linear2ulaw(i));
	printf("\n};\n\n");

	printf("/* 14-bit signed PCM to inverted 8-bit u-law */\n");
	printf("unsigned char\t*_linear2ulaw = _ulinear2ulaw + 0x2000;\n");

/*
 * Create A-law <-> Mu-law conversion lookup tables
 */
	/* A-law to Mu-law */
	printf("/* A-law to Mu-law conversion table */\n");
	printf("unsigned char\t_alaw2ulaw[256] = {");

        for (i = 0; i < 0x80; i++) {            /* plus side */
                printf("%s%6d,", ( i & 7)? " " : "\n\t", 0xFF ^ _a2u[i ^ 0x55]);        }
        for (; i < 0x100; i++) {                /* minus side */
                printf("%s%6d,", ( i & 7)? " " : "\n\t", 0x7F ^ _a2u[i ^ 0xD5]);        }
	printf("};\n\n");

	/* Mu-law to A-law */
	printf("/* Mu-law to A-law conversion table */\n");
	printf("unsigned char\t_ulaw2alaw[256] = {");
        for (i = 0; i < 0x80; i++)              /* minus side */
                printf("%s%6d,", (i & 7) ? " " : "\n\t",
                    0xD5 ^ (_u2a[0x7F ^ i] - 1));
        for (; i < 0x100; i++)                  /* plus side */
                printf("%s%6d,", (i & 7) ? " " : "\n\t",
                    0x55 ^ (_u2a[0xFF ^ i] - 1));
	printf("};\n");

	exit(0);
}
