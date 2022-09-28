#ifndef _PS_LINK_H_
#define _PS_LINK_H_

#ident "@(#)ps_link.h	1.8 06/11/93 Copyright 1989 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/dvlink.h>

// Link Attributes
#define		SINGLE_CLICK	(LinkAttr) 0x00000001
#define		DOUBLE_CLICK	(LinkAttr) 0x00000000

typedef u_int	LinkAttr;


class PSLINK : public DVLINK {
	private:

	LinkAttr	attrs;
	BBox		bbox;

	STATUS		SetAttrs(const STRING &str, ERRSTK &err);
	STATUS		SetBBox(const STRING &str, ERRSTK &err);

	u_int		IsSet(const LinkAttr attr) const
	{
		return(attrs & attr);
	}

	public:

	PSLINK() : attrs(DOUBLE_CLICK)
	{
		memset((void *) &bbox, 0, sizeof(bbox));
	}

	LinkAttr	GetAttrs() const
	{
		return(attrs);
	}

	const BBox	&GetBBox() const
	{
		return(bbox);
	}

	BOOL	IncludesPoint(const int x, const int y) const
	{
		const BOOL ret	= (((x >= bbox.ll_x && y >= bbox.ll_y) &&
				    (x < bbox.ur_x && y < bbox.ur_y)) ?
				   BOOL_TRUE : BOOL_FALSE);

		return(ret);
	}

	STATUS		Init(const STRING &str, ERRSTK  &err);

	BOOL		IsSingleClick() const
	{
		return((IsSet(SINGLE_CLICK)) ? BOOL_TRUE : BOOL_FALSE);
	}

#ifdef	DEBUG
	friend ostream	&operator <<(ostream &ostr, const PSLINK &link);
#endif	/* DEBUG */
};

#endif /* _PS_LINK_H_ */
