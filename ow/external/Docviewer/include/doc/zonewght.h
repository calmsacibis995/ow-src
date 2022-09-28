#ifndef	_ZONEWGHT_H
#define	_ZONEWGHT_H

#ident "@(#)zonewght.h	1.6 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/list.h>


// The zone weight object holds query weighting information.
//
class	ZONEWGHT {

    private:

	int	zone;
	int	weight;
	STRING	name;


    public:

	ZONEWGHT(int zn, int wt, const STRING &nm) :
			zone(zn),
			weight(wt),
			name(nm)	{ }
	~ZONEWGHT()			{ }

	int		Zone() const	{ return(zone); }
	int		Weight() const	{ return(weight); }
	const STRING	&Name() const	{ return(name); }
};

STATUS	ReadWeightsFile(const STRING &weightpath,
			LIST<ZONEWGHT*> &weights,
			ERRSTK &);

#endif	_ZONEWGHT_H
