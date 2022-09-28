/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)silent_params.c	1.3	92/11/14 SMI"


#include <stdio.h>
#include "silent_params.h"
#include "atool_debug.h"

void
sdf_init(
	int	ds,
	int	de)
{
	dstart = ds;
	dend = de;
}


/*
 * Given an x value and two points, interpolate y
 */
double
sdf_interpolate(
	int		x,	/* domain value */
	double		x1,	/* first interpolation point */
	double		y1,
	double		x2,	/* second interpolation point */
	double		y2)
{
	double		m;	/* slope */
	double		b;	/* intercept */

	m = (y2 - y1) / (x2 - x1);
	b = y1 - (m * x1);

	return ((m * x) + b);
}


sdf_params(
	int		x)	/* domain value */
{
	DBGOUT((1, "threshold = \t%.2f\n", threshold_scale(x)));
	DBGOUT((1, "min_silence = \t%.2f\n", min_silence(x)));
	DBGOUT((1, "min_sound = \t%.2f\n", min_sound(x)));
	DBGOUT((1, "noise_ratio = \t%.2f\n", noise_ratio(x)));
}

/*
 *	
 */
double
threshold_scale(
	int		x)		/* slider value */
{
	if ((x >= X(0)) && (x <= X(1/8)))
		return(sdf_interpolate(x, X(0), 4.0, X(1/8), 2.0));
	if ((x > X(1/8)) && (x <= X(1/4)))
		return(2.0);
	if ((x > X(1/4)) && (x <= X(1/2)))
		return(sdf_interpolate(x, X(1/4), 2.0, X(1/2), 1.0));
	if ((x > X(1/2)) && (x <= X(3/4)))
		return(sdf_interpolate(x, X(1/2), 1.0, X(3/4), .5));
	if ((x > X(3/4)) && (x <= X(7/8)))
		return(.5);
	if ((x > X(7/8)) && (x <= X(1)))
		return(sdf_interpolate(x, X(7/8), .5, X(1), 0.0));
	
	return(SDF_RANGE_ERROR);
}

double
min_silence(
	int		x)		/* slider value */
{
	if ((x >= X(0)) && (x <= X(1/4)))
		return(sdf_interpolate(x, X(0), 0.0, X(1/4), .2));
	if ((x > X(1/4)) && (x <= X(3/4)))
		return(.2);
	if ((x > X(3/4)) && (x <= X(1)))
		return(sdf_interpolate(x, X(3/4), .2, X(1), .5));
	
	return(SDF_RANGE_ERROR);
}

double
min_sound(
	int		x)		/* slider value */
{
	if ((x >= X(0)) && (x <= X(1/4)))
		return(sdf_interpolate(x, X(0), 1.0, X(1/4), .3));
	if ((x > X(1/4)) && (x <= X(3/4)))
		return(.3);
	if ((x > X(3/4)) && (x <= X(1)))
		return(sdf_interpolate(x, X(3/4), .3, X(1), 0.0));
	
	return(SDF_RANGE_ERROR);
}

double
noise_ratio(
	int		x)		/* slider value */
{
	if ((x >= X(0)) && (x <= X(1/2)))
		return(sdf_interpolate(x, X(0), 1.0, X(1/2), .15));
	if ((x > X(1/2)) && (x <= X(1)))
		return(sdf_interpolate(x, X(1/2), .15, X(1), 0.0));
	
	return(SDF_RANGE_ERROR);
}

