#ifndef	_DOC_COMMON_H
#define	_DOC_COMMON_H

#ident "@(#)common.h	1.28 11/15/96 Copyright 1989 Sun Microsystems, Inc."


#include <stdio.h>
#include <stream.h>
#include <osfcn.h>
#include <libc.h>
#include <sys/types.h>

// Turn assertions off depending on DEBUG
#ifndef DEBUG
#define NDEBUG
#endif /* DEBUG */
#include <assert.h>


#define MAX_LINE_LEN	(256)

typedef	enum {
	BOOL_FALSE = 0,
	BOOL_TRUE = 1
} BOOL;


// What kind of viewer is it?
typedef enum {
	NULLVIEWER = 0,
	DOCVIEWER,
	HELPVIEWER
} ViewerType;

// Is this a DOCVIEWER client or a HELPVIEWER client?
typedef ViewerType ClientType;


// Procedure return status.
//
typedef	enum {
	STATUS_OK = 0,
	STATUS_FAILED = -1
} STATUS;

typedef	caddr_t	WINHANDLE;

typedef struct PSBoundingBox {
	int ll_x, ll_y;
	int ur_x, ur_y;
} BBox;


// Support for C++ callbacks.
// This is a real hack that assumes the underlying implementation
// of C++ methods always passes 'this' (pointer to the current object)
// as the first parameter to a method.
//
// XXX obsolete XXX do not use XXX
//
typedef void (*CALLBACK)(caddr_t ...);

typedef struct {
	CALLBACK	proc;	// callback procedure
	caddr_t		arg;	// first param to callback (typically 'this')
} CBDATA;

typedef void (*EVENT_HANDLER)(int event, caddr_t event_obj, caddr_t clnt_data);


// Debugging support.
//
#ifdef	DEBUG

extern int	debug;		// Set this to "1" to turn on debugging output
extern int	profile;	// Set this to "1" to turn on profiling output

// Conditionally print arbitrary 'stream' expression
// for debugging purposes.  Uses C++ 'stream' package.
// Verbosity of debugging output depends on value of debug:
//	0: no debugging output
//	1: only high level events
//	2: high and medium level events
//	3: #2 plus function entry and exit events
//	4: #3 plus lots of low level stuff
//	5: More debugging output than you could ever hope for
//
#define	DbgHigh(x)	(debug > 0  ?  ((cerr << x),1)  :  0)
#define	DbgMed(x)	(debug > 1  ?  ((cerr << x),1)  :  0)
#define	DbgFunc(x)	(debug > 2  ?  ((cerr << x),1)  :  0)
#define	DbgLow(x)	(debug > 3  ?  ((cerr << x),1)  :  0)
#define	DbgNit(x)	(debug > 4  ?  ((cerr << x),1)  :  0)

// Obselete.  Don't use it anymore.
//
#define	DbgPrint(x)	DbgNit(x)

#else

#define	DbgHigh(x)
#define	DbgMed(x)
#define	DbgFunc(x)
#define	DbgLow(x)
#define	DbgNit(x)
#define	DbgPrint(x)

#endif	DEBUG


#ifdef	LOG

#include <doc/logger.h>

// Event logging support.
// Applications that use this are responsible for providing 'logger' object.
// 'logger' object should be created and 'Init()'ed
// before using any of the following macros.
//
extern LOGGER	*logger;	// application must provide (and init) this

#define EVENTLOG(ev)		(logger ? logger->LogIt(ev),1           : 0)
#define EVENTLOG1(ev,fmt,a)	(logger ? logger->LogIt(ev,fmt,a),1     : 0)
#define EVENTLOG2(ev,fmt,a,b)	(logger ? logger->LogIt(ev,fmt,a,b),1   : 0)
#define EVENTLOG3(ev,fmt,a,b,c)	(logger ? logger->LogIt(ev,fmt,a,b,c),1 : 0)

#else	LOG

#define EVENTLOG(ev)
#define EVENTLOG1(ev,fmt,a)
#define EVENTLOG2(ev,fmt,a,b)
#define EVENTLOG3(ev,fmt,a,b,c)

#endif	LOG


// Stuff added for SVR4
//
#ifdef SVR4
extern	"C"	{
	void get_myaddress(struct sockaddr_in *);
	int getrlimit(int, struct rlimit*);
};
#endif


// Forward declarations for common classes
//
class	CONSOLE;


// Ubiquitous header files.
//
#include <doc/objstate.h>
#include <doc/string.h>
#include <doc/errstk.h>

#endif	_DOC_COMMON_H
