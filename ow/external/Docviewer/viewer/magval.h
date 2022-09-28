#ifndef	_DOCVIEWER_MAGVAL_H_
#define	_DOCVIEWER_MAGVAL_H_

#ident "@(#)magval.h	1.4 12/20/93 Copyright 1992 Sun Microsystems, Inc."


class	MAGVAL {
	private:

	float		val;
	STRING		valStr;
	OBJECT_STATE	objstate;

	void			Set(const float mag);

	public:

	MAGVAL::MAGVAL () :
		val	(0.0)
	{
	}

	float			Get() const
	{
		return(val);
	}

	STATUS			Init(const ViewerType vtype)
	{
		Reset(vtype);
		return(STATUS_OK);
	}

	void			Reset(const ViewerType vtype);
			
	const char *operator ~ () const
	{
		return(~valStr);
	}

	const MAGVAL	&operator = (const float mag)
	{
		Set(mag); return(*this);
	}

	float	operator = (const MAGVAL &mag) const
	{
		return(mag.Get());
	}

	operator float () const
	{
		return(Get());
	}

#ifdef	DEBUG
	friend ostream &operator << (ostream &ostr, const MAGVAL &magval)
	{
		return(ostr << ~magval.valStr);
	}
#endif	/* DEBUG */
};	


#endif	/* _DOCVIEWER_MAGVAL_H_ */
