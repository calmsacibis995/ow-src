#ifndef	_ITIMER_H
#define	_ITIMER_H

#ident "@(#)itimer.h	1.9 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <xview/notify.h>

typedef void (*TimerCallBack)(caddr_t ...);

class	ITIMER {

    private:

	TimerCallBack	cbFunc;
	caddr_t		cbArg;

	// Private callback
	static	int	TimeOutEvent(Notify_client client_data, int which);

    public:
	ITIMER()
	{
		cbFunc	= NULL;
		cbArg	= NULL;
	}

	void	TimeOut(int		secs,
			int		usecs,
			TimerCallBack	funcPtr,
			caddr_t		arg);

	void	TimeOut(int		secs,
			TimerCallBack	funcPtr,
			caddr_t		arg)
	{
		TimeOut(secs, 0, funcPtr, arg);
	}

	void	Cancel( void );

};

#endif	_ITIMER_H
