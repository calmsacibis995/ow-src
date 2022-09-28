/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_SILENT_PARAMS_H
#define	_AUDIOTOOL_SILENT_PARAMS_H

#ident	"@(#)silent_params.h	1.2	92/12/14 SMI"


#define		X(p)		dstart + ((double)p * (dend - dstart))
#define		SDF_RANGE_ERROR	-1.0

int		dstart;		/* start of silent detection function domain */
int		dend;		/* end of the silent detection function domain */

/* sdf = silent detection function */

void		sdf_init(int, int);
double		threshold_scale(int);
double		min_silence(int);
double		min_sound(int);
double		noise_ratio(int);

#endif /* !_AUDIOTOOL_SILENT_PARAMS_H */
