#ident "@(#)itimer.cc	1.13 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/itimer.h>
#include <xview/xview.h>
#include <doc/console.h>

extern CONSOLE	console;

/*
 * Setup interval timer.
 * Generates timer event after specified number of seconds,
 * and calls TimerEvent to handle the event.
 */
void
ITIMER::TimeOut(int secs, int usecs, TimerCallBack funcPtr, caddr_t funcArg)
{
	struct itimerval	itimer;

	DbgFunc("ITIMER::TimeOut: " << secs << "." << usecs << endl);


	// Set timeout value.
	itimer.it_value.tv_sec  = secs;
	itimer.it_value.tv_usec = usecs;

	/*
	 * Set interval value to zero so that timer
	 * will automatically expire after its first invocation.
	 */
	itimer.it_interval.tv_sec  = 0;
	itimer.it_interval.tv_usec = 0;

	cbFunc	= funcPtr;
	cbArg	= funcArg;

	// notify_set_itimer_func returns a pointer to the previous
	// callback - ignore this

	(void) notify_set_itimer_func((Notify_client) this,
				      (Notify_func) &ITIMER::TimeOutEvent,
				      ITIMER_REAL, &itimer, NULL);

	return;
}

int
ITIMER::TimeOutEvent(Notify_client client_data, int /* which */)
{
	ITIMER	*ptrToSelf = (ITIMER *) client_data;

	DbgFunc("ITIMER::TimeOutEvent: entered" << endl);

	// Call event callback
	if (ptrToSelf && ptrToSelf->cbFunc) {
		DbgLow("ITIMER::TimeOutEvent: "
		       << "calling ptrToSelf->cbFunc(ptrToSelf->cbArg)"
		       << endl);
		(void) ptrToSelf->cbFunc(ptrToSelf->cbArg);
	}
	else {
		DbgHigh("ITIMER::TimeOutEvent: ");
		if (!ptrToSelf) {
			DbgHigh("ptrToSelf is NULL! ");
		}
		else {
			DbgHigh("ptrToSelf->CbFunc is NULL! ");
		}
		DbgHigh ("Cannot make callback" << endl);
	}

	return((int)NOTIFY_DONE);
}


void
ITIMER::Cancel( void )
{

	(void) notify_set_itimer_func((Notify_client) this,
				      (Notify_func) NOTIFY_FUNC_NULL,
				      ITIMER_REAL, NULL, NULL);

}
