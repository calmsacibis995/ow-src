/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)ProcessTimer.cc	1.5	93/02/04 SMI"

#include <stdlib.h>
#include <stdio.h>
#include "AudioError.h"
#include "ProcessTimer.h"
#include "libaudio.h"


// class ProcessTimer methods

// Constructor
ProcessTimer::
ProcessTimer():
	scale(0), enabled(0), usertime(0.), systemtime(0.), realtime(0.)
{
	tick = (double) CLK_TCK;
}

// Destructor
ProcessTimer::
~ProcessTimer()
{
}

// Convert a clock_t to double
double ProcessTimer::
c2d(
	const clock_t&	t)
{
	return ((double) t / tick);
}

#ifdef GETRUSAGE
// Convert a timeval to double
double ProcessTimer::
ts2d(
	const struct timestruc&	t)
{
	return ((double)t.tv_sec + ((double)t.tv_nsec / (double)NANOSEC));
}

// Convert a timeval to double
double ProcessTimer::
tv2d(
	const struct timeval&	t)
{
	return ((double)t.tv_sec + ((double)t.tv_usec / (double)MICROSEC));
}
#endif /* GETRUSAGE */

#ifdef SUNOS41
// The BSD interface doesn't return wall clock time
clock_t ProcessTimer::
systimes(
	struct tms	*ts)
{
	struct timeval	rt;

	(void) times(ts);
	(void) gettimeofday(&rt);
	return ((clock_t)((0.5 + (double)rt.tv_sec +
	    ((double)MICROSEC * (double)rt.tv_usec)) / tick));
}
#endif /* 4.x */


// Scale a time appropriately
double ProcessTimer::
scaletime(
	double		t)
{
	if (scale > 1)
		return (t / (double) scale);
	return (t);
}

// Format time into a resonable string
char* ProcessTimer::
timestring(
	double		t,
	char*		str)
{
	return (audio_secs_to_str(t, str, 2));
}


// Set scaling factor
void ProcessTimer::
set_scale(
	unsigned	newscale)
{
	scale = newscale;
}

// Get scaling factor
unsigned ProcessTimer::
get_scale()
{
	return (scale);
}

// Get scaled user time
double ProcessTimer::
get_usertime()
{
	// Accumulate current values, if enabled
	accumulate();
	return (scaletime(usertime));
}

// Get scaled system time
double ProcessTimer::
get_systemtime()
{
	// Accumulate current values, if enabled
	accumulate();
	return (scaletime(systemtime));
}

// Get scaled total time
double ProcessTimer::
get_totaltime()
{
	// Accumulate current values, if enabled
	accumulate();
	return (scaletime(usertime + systemtime));
}

// Get scaled elapsed time
double ProcessTimer::
get_realtime()
{
	// Accumulate current values, if enabled
	accumulate();
	return (scaletime(realtime));
}

// Convert total time to a string
char* ProcessTimer::
report_total()
{
	char	t[AUDIO_MAX_TIMEVAL];

	// Accumulate current values, if enabled
	accumulate();

	if (scale < 1) {
		sprintf(reportstring, "%s",
		    timestring(systemtime + usertime, t));
	} else {
		sprintf(reportstring,
		    _MGET_("%d %s: %s (avg time)\n"),
		    scale, (scale > 1 ? _MGET_("passes") : _MGET_("pass")),
		    timestring(scaletime(systemtime + usertime), t));
	}
	return (reportstring);
}

// Convert system & user to a string
char* ProcessTimer::
report_sysuser()
{
	char	s[AUDIO_MAX_TIMEVAL];
	char	u[AUDIO_MAX_TIMEVAL];

	// Accumulate current values, if enabled
	accumulate();

	if (scale < 1) {
		sprintf(reportstring, _MGET_("%s system / %s user"),
		    timestring(systemtime, s),
		    timestring(usertime, u));
	} else {
		sprintf(reportstring,
		    _MGET_("%d %s: %s system / %s user (avg time)\n"),
		    scale, (scale > 1 ? _MGET_("passes") : _MGET_("pass")),
		    timestring(scaletime(systemtime), s),
		    timestring(scaletime(usertime), u));
	}
	return (reportstring);
}

// Convert elapsed time to a string
char* ProcessTimer::
report_real()
{
	char	r[AUDIO_MAX_TIMEVAL];

	// Accumulate current values, if enabled
	accumulate();

	if (scale < 1) {
		sprintf(reportstring, "%s", timestring(realtime, r));
	} else {
		sprintf(reportstring,
		    _MGET_("%d %s: %s (avg elapsed time)\n"),
		    scale, (scale > 1 ? _MGET_("passes") : _MGET_("pass")),
		    timestring(scaletime(realtime), r));
	}
	return (reportstring);
}


// Accumulate times
void ProcessTimer::
accumulate()
{
	double		x;
#ifdef GETRUSAGE
	struct rusage	ru;
	struct timeval	tv;

	if (enabled) {
		(void) getrusage(RUSAGE_SELF, &ru);
		x = ts2d(ru.ru_utime);
		usertime += x - ustart;
		ustart = x;
		x = ts2d(ru.ru_stime);
		systemtime += x - sstart;
		sstart = x;
		(void) gettimeofday(&tv);
		x = tv2d(tv);
		realtime += x - rstart;
		rstart = x;
	}
#else /* !GETRUSAGE */
	struct tms	ts;

	if (enabled) {
		x = c2d(times(&ts));
		realtime += x - rstart;
		rstart = x;
		x = c2d(ts.tms_utime);
		usertime += x - ustart;
		ustart = x;
		x = c2d(ts.tms_stime);
		systemtime += x - sstart;
		sstart = x;
	}
#endif /* !GETRUSAGE */
}

// Start timer
void ProcessTimer::
start()
{
	if (enabled)
		return;

#ifdef GETRUSAGE
	{
	struct rusage	ru;
	struct timeval	tv;

	(void) getrusage(RUSAGE_SELF, &ru);
	ustart = ts2d(ru.ru_utime);
	sstart = ts2d(ru.ru_stime);
	(void) gettimeofday(&tv);
	rstart = tv2d(tv);
	}
#else /* !GETRUSAGE */

	{
	struct tms	ts;

	rstart = c2d(times(&ts));
	ustart = c2d(ts.tms_utime);
	sstart = c2d(ts.tms_stime);
	}
#endif /* !GETRUSAGE */

	enabled = 1;
}

// Stop timer
void ProcessTimer::
stop()
{
	accumulate();
	enabled = 0;
}

// Reset timer values
void ProcessTimer::
reset()
{
	// Reset start times if enabled, then reset accumulated times
	accumulate();
	usertime = 0.;
	systemtime = 0.;
	realtime = 0.;
}
