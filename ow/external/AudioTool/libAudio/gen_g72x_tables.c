/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ident "@(#)gen_g72x_tables.c	1.3	92/06/09 SMI"

#define	UPLIM	(0x7FFF)

main(void)
{
	int	i, level, exp;
	int	j, l, m;

	/*
	 * gen_g72x_tables.c combined the old gen_fmultanexp.c gen_fmultwanmant.c
	 * gen_quani.c and gen_g723quani.c
	 *
	 * gen_fmultanexp.c
	 *
	 * Description:
	 *
	 * Generates the '_fmultanexp[]' lookup table used by our
	 * CCITT G721 and G723 coding algorithm implementation. The i'th
	 * entry of the table is the greatest integer smaller than the
	 * the log to the base 2 of i (truncation to integer).
	 */

	printf("unsigned char\t_fmultanexp[%5d] = {", UPLIM + 1);

	/* Loop through UPLIM possible values for i. */
	exp = 0;
	level = 1;
	for (i = 0; i < UPLIM; i++) {
		if (i >= level) {
			exp++;
			level <<= 1;
		}
		printf("%s%6d,", (i & 7) ? " ": "\n\t", exp);
	}
	/* Don't print a comma after the last entry. */
	if (i >= level)
		exp++;
	printf("%s%6d};\n", (i & 7) ? " ": "\n\t", exp);
	/*
	 * gen_fmultwanmant.c
	 *
	 * Description:
	 *
	 * Generates the '_fmultwanmant[]' lookup table used by the CCITT 
	 * G.721/G.723 coding algorithm. The i'th entry in the table contains:
	 *
	 *	j = ((i & 63) * (i >> 6) + 48) >> 4
	 *
	 * The table is used to speed the computation of the truncated
	 * biased product of the 6 bit mantissas of the floating point
	 * representations of the filter coefficient and delayed signal sample
	 * in the routines '_g721_fmult()' in 'g721.c', 'g723.c'.
	 * To compute the truncated biased product, j, of
	 * two 6 bit integers, w and l, simply do:
	 *
	 *	j = _fmultwanmant[(w << 6) | l].
	 */

	printf("unsigned char\t_fmultwanmant[4096] = {");
	for (m = 0; m < 64; m++) {
		j = 48 - m;
		for (l = 0; l < 64; l++) {
			j += m;			/* j = (l * m) + 48 */
			printf("%s%6d,", (l & 7)? " " : "\n\t", j >> 4);
		}
	}
	printf("};\n");

	/*
	 * gen_quani.c
	 *
	 * Description:
	 *
	 * Generates the table in the QUAN module description
	 * in CCITT Recommendation G.721.
	 */

	printf("unsigned char\t_quani[0x1000] = {");
	for (i = 0; i < 80; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 1);
	for (; i < 178; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 2);
	for (; i < 246; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 3);
	for (; i < 300; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 4);
	for (; i < 349; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 5);
	for (; i < 400; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 6);
	for (; i < 2048; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 7);
	for (; i < 3972; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 0);
	for (; i < 0x1000; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 1);
	printf("\n};\n");

	/*
	 * gen_g723quani.c
	 *
	 * Description:
	 *
	 * Generates the table in the QUAN module description
	 * in CCITT Recommendation G.723.
	 */

	printf("unsigned char\t_g723quani[0x1000] = {");
	for (i = 0; i < 8; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 0);
	for (; i < 218; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 1);
	for (; i < 331; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 2);
	for (; i < 2048; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 3);
	for (; i < 0x1000; i++)
		printf("%s%6d,", (i & 7) ? " ": "\n\t", 0);
	printf("};\n");
	exit (0);
}

