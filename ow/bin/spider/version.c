/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)version.c	2.1	90/04/25
 *
 */

/*
 * Using the version number to reflect a patch level as well.  For patch to
 * succeed, it must have a space before and after the string to match.
 * In the case below, add the line Prereq: "1.0.1" to the next patch to be
 * sure patches are installed in the correct order.
 */
char	*version = "1.1" ;
char	*build_date = DATE;
