#ifdef lint
#ifdef sccs
static char sccsid[]="@(#)msort.c	3.1 04/03/92 Copyright 1987-1990 Sun Microsystems, Inc.";
#endif   
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
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

/*
 * msort() is a list-merge sort routine generalized from Knuth (5.2.4)
 * Algorithm L.  This routine requires 2 artificial records: R0 and
 * Rn+1 where n = number of elements "nel".  "offset" is the byte-offset
 * of the "link" field.  "width" is the size of each record.  "base" is
 * the base address of the starting record (i.e. R0.)
 */

#define	Record(i)	(base + (width * (i)))
#define	Link(i)		(*((int *) (Record(i) + offset)))

msort (base, nel, width, offset, compar)
char *base;
int nel;
int width;
int offset;
int (*compar)();
{
	register int	i;
	register int	t;
	register int	s;
	register int	p;
	register int	q;
	char		*k1;
	char		*k2;

	if (nel < 2)
		return (0);

	/* Prepare two lists. */
	Link(0) = 1;
	Link(nel+1) = 2;
	for (i = nel - 2; i >= 1; i--)
		Link(i) = -(i+2);
	Link(nel-1) = 0;
	Link(nel) = 0;

	while (1)
	{
		/* Begin new pass */
		s = 0;
		t = nel + 1;
		p = Link(s);
		q = Link(t);
		if (q == 0)
			return (0);

		while (1)
		{
			/* Compare Kp: Kq */
			k1 = Record(p);
			k2 = Record(q);
			if ((*compar)(&k1, &k2) <= 0)
			{
				/* Advance p */
				i = abs(p);
				Link(s) = (Link(s) < 0) ? -i : i;
				s = p;
				p = Link(p);
				if (p > 0)
					continue;

				/* Complete the sublist */
				Link(s) = q;
				s = t;
				do
				{
					t = q;
					q = Link(q);
				} while (q > 0);
			}
			else
			{
				/* Advance q */
				i = abs(q);
				Link(s) = (Link(s) < 0) ? -i : i;
				s = q;
				q = Link(q);
				if (q > 0)
					continue;

				/* Complete the sublist */
				Link(s) = p;
				s = t;
				do
				{
					t = p;
					p = Link(p);
				} while (p > 0);
			}

			/* End of pass? */
			p = -p;
			q = -q;
			if (q == 0)
			{
				i = abs(p);
				Link(s) = (Link(s) < 0) ? -i : i;
				Link(t) = 0;
				break;
			}
		}
	}
}
