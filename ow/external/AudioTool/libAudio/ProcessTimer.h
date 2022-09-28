/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_PROCESSTIMER_H
#define	_MULTIMEDIA_PROCESSTIMER_H

#ident	"@(#)ProcessTimer.h	1.4	92/12/04 SMI"

#ifndef SUNOS41
// XXX - SVr4 defines CLK_TCK using an undefined _sysconf()
#define	_sysconf	sysconf
#endif /* SVr4 */

#include <unistd.h>
#include <sys/time.h>

#ifdef GETRUSAGE		// XXX - not supported in baseline SVr4?!
#ifdef SUNOS41
#include <sys/resource.h>
#define	timestruc	timeval		// getrusage time units

#else /* SVr4 */
#include <sys/rusage.h>
extern int	getrusage(int, struct rusage*);
extern int	gettimeofday(struct timeval*);
#endif /* SVr4 */

#endif /* !GETRUSAGE */


// Use the times(2) interface for process times
#include <sys/times.h>
#include <limits.h>

#ifdef SUNOS41
#include <unistd.h>
#define	CLK_TCK	sysconf(_SC_CLK_TCK)
#define	tv_nsec		tv_usec		// timeval "nanoseconds"
#define	NANOSEC		1000000		// fake "nanoseconds" for timeval
#define	MICROSEC	1000000		// real microseconds / second
#define	gettimeofday(X)	gettimeofday(X, NULL)

// XXX - the BSD times() interface does not return wall clock time
#define	times(X)	systimes(X)
#endif /* SVr4 */


// This is the class for a user/system process timer

class ProcessTimer {
protected:
	double		usertime;			// accumulated user time
	double		systemtime;			// accumulated sys time
	double		realtime;			// accumulated real time

	double		ustart;				// start user time
	double		sstart;				// start sys time
	double		rstart;				// start real time
	double		tick;				// cached ticks/sec
	int		enabled;			// enable flag
	unsigned	scale;				// divisor
	char		reportstring[81];		// temporary string

	ProcessTimer	operator=(ProcessTimer);	// Assignment is illegal

	void		accumulate();			// Accumulate times
	double		scaletime(double);		// Scale time value
	double		c2d(const clock_t&);		// Convert to double

#ifdef GETRUSAGE
	double		ts2d(const struct timestruc&);	// Convert to double
	double		tv2d(const struct timeval&);	// Convert to double
#endif /* !GETRUSAGE */
#ifdef SUNOS41
	clock_t		systimes(struct tms*);		// fake times()
#endif /* 4.x */

public:
	ProcessTimer();					// Constructor
	~ProcessTimer();				// Destructor

	void		start();			// Start timing
	void		stop();				// Stop timing
	void		reset();			// Clear accumulators
	void		set_scale(unsigned);		// Set # of passes

	unsigned	get_scale();			// Get # of passes
	double		get_totaltime();		// Get total time
	double		get_usertime();			// Get user time
	double		get_systemtime();		// Get system time
	double		get_realtime();			// Get elapsed time
	char*		report_total();			// Format total time
	char*		report_sysuser();		// Format sys/user time
	char*		report_real();			// Format elapsed time
	char*		timestring(double, char*);	// Format time
};

#endif /* !_MULTIMEDIA_PROCESSTIMER_H */
