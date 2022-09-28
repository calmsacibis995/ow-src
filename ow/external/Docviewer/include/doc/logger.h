#ifndef	_LOGGER_H_
#define	_LOGGER_H_

#ident "@(#)logger.h	1.8 06/11/93 Copyright 1990 Sun Microsystems, Inc."

#include <sys/time.h>
#include <sys/types.h>


class LOGGER {

    private:

	FILE	       *fp;
	pid_t		pid;
	STRING		logfile;
	struct timeval *startTime;
	struct rusage  *RUsageTime;

	// Private functions
	STATUS	TimeStamp();
	STATUS  RUsageStamp();

    public:

	LOGGER(char *file)
	{
		logfile = file; startTime = NULL; pid = 0;
	}
       ~LOGGER();

	STATUS	Init();
	void	LogIt(char *event, char *fmt=NULL, ...);
};
#endif	/* _LOGGER_H_ */
