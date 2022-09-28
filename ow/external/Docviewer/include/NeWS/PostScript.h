#ifndef	_NEWS_POSTSCRIPT_H
#define _NEWS_POSTSCRIPT_H


#ident "@(#)PostScript.h	1.2 06/11/93 NEWS SMI"


/*
 * PostScript.h - Definitions for the Postscript interpreter.
 * Copyright (c) 1990-1991 by Sun Microsystems, Inc.
 */

#include <Core/core.h>
#include <cscript/cscript.h>
#include <nucleus/inputdist.h>
#include <nucleus/dictionary.h>
#include <nucleus/nucleus.h>
#include <nucleus/time.h>
#include <NeWS/processes.h>
#include <NeWS/parse.h>


#define ValidObjectBits (~3)
#define object_word0(p) *((int *)(p))
#define object_word1(p) *(((int *)(p))+1)
#define object_has_entity(p) ((unsigned) (p)->type >= (unsigned) ENTITY_TYPE)

/*
 * corpus_copy - Especially for use with event corpuses when they are of
 *	         interest to a given canvas.  The corpus is copied; if an
 *		 instance dict exists, it's copied, too.  NOTE that the
 *		 refcount on the "to" object stays the same.
 */
enum error_type dict_corpus_copy();
#define corpus_copy(type, to, from) 					  \
{ 									  \
    DICT *oldd, *newd;							  \
    int	refcnt = to->refcount;						  \
    int softcnt = to->softcount;					  \
    xnewscopy((char *)(to), (char *)(from), corpus_size(type)); 	  \
    if(oldd = instancedict_of(from)) {					  \
	newd = (DICT *) new_composite(NO_POOL,				  \
			(struct object *)NULL, dictionary_type,		  \
                        ((int)(oldd)->size - 1), (char *)NULL);           \
	dict_corpus_copy(newd, oldd);					  \
	set_instancedict(to, newd); 					  \
    } 									  \
    to->refcount = refcnt;						  \
    to->softcount = softcnt;						  \
}

#define UNPACKED_OBJECT_TBLSIZE		256
extern struct object unpacked_objects[];

struct object
    get_packedarray_interval(/* (struct object *)array, int i, int cnt */),
    get_packedarray_object(/*   (struct object *)array, int i */),
    make_executable_array(/* (char *)where, int num_objects, int length */),
    *unpack(/* struct body *b, int keep_old_array */);

#define unset_packed_array_flag(b)\
	((struct corpus *)(b))->corpus.array.packed_size = 0
#define set_packed_array_size(b, size)\
	((struct corpus *)(b))->corpus.array.packed_size = size

/* Test for packed array attached to this entity */
#define ispacked_array_corpus(b)\
    (((struct corpus *)(b))->corpus.array.packed_size != 0)

#ifndef	lint
/*
 * 	Set fields in an object.  Mostly applies to non-refcounted types.
 */
#define set_boolean_object(p, n) 					   \
  	((object_word0(p) = ((unsigned) boolean_type)<<TYPE_SHIFT),	   \
  	(p)->value.boolean = (n))
#define set_integer_object(p, n)					   \
  	((object_word0(p) = ((unsigned) integer_type)<<TYPE_SHIFT),	   \
  	(p)->value.integer = (n))
#define set_real_object(p, n)						   \
        ((object_word0(p) = ((unsigned) real_type)<<TYPE_SHIFT),	   \
        (p)->value.real = (n))
#ifndef LITTLEENDIAN
#define set_executable_name_from_address(p, addr) (                	   \
	(object_word0(p) =						   \
	    ((((unsigned)(name_type))<<TYPE_SHIFT) | 2)			   \
	         | (ADDR_MASK & (addr << 2))),				   \
	(object_word1(p) = 0))
#define set_name_from_address(p, addr) (                           	   \
	(object_word0(p) =						   \
	    (((unsigned)(name_type))<<TYPE_SHIFT)			   \
		 | (ADDR_MASK & (addr << 2))),				   \
	(object_word1(p) = 0))
#else
#define set_executable_name_from_address(p, addr) (                	   \
	(object_word0(p) =						   \
	    ((((unsigned)(name_type))<<TYPE_SHIFT) | 2)			   \
	         | (ADDR_MASK & (addr >> 6))),				   \
	(object_word1(p) = 0))
#define set_name_from_address(p, addr) (                           	   \
	(object_word0(p) =						   \
	    (((unsigned)(name_type))<<TYPE_SHIFT)			   \
		 | (ADDR_MASK & (addr >> 6))),				   \
	(object_word1(p) = 0))
#endif	/* ifndef LITTLEENDIAN */
#define set_name_object(p, name)					   \
	set_typed_entity_object(p, name_type, (ENTITY *) name)
#define set_typed_object(p, type) 					   \
  	(object_word0(p) = ((unsigned) (type))<<TYPE_SHIFT)
#define set_entire_typed_object(p, type) 				   \
  	(object_word0(p) = ((unsigned)(type))<<TYPE_SHIFT, object_word1(p) = 0)
#define clear_object(p) 						   \
	(object_word0(p) = 0, object_word1(p) = 0)
/*
 * 	Set fields in an object, including a pointer to the ENTITY
 * 	that represents the associated type-specific information.
 */
#define set_typed_entity_object(p, type, e) ( 				   \
	(object_word0(p) = (((unsigned)(type))<<TYPE_SHIFT) | (int)(e)),   \
	(object_word1(p) = 0))
#define set_executable_typed_entity_object(p, type, e) ( 		   \
	(object_word0(p) = ((((unsigned)(type))<<TYPE_SHIFT) | 2) | (int)(e)),\
	(object_word1(p) = 0))
/*
 * 	Set fields in an object struct by copying the specified ref.  Used 
 *	when an object struct is already defined, an entity is created with 
 *	the object information, and now the object must reference the entity.  
 *	Copy the information from the ref to word0 of the object.
 */
#define set_object_from_ref(p, ref) (					   \
	(object_word0(p) = refword(&(ref))), (object_word1(p) = 0))
/*
 * 	Test whether two objects are bit-for-bit equal.
 */
#define equal_object(p1, p2) 						   \
   	((object_word1(p1)==object_word1(p2)) &&                           \
 	(((is_encoded_type(p1) ? (object_word0(p1)&0xf3ffffff)             \
 					: object_word0(p1))^               \
 	   (is_encoded_type(p2) ? (object_word0(p2)&0xf3ffffff)            \
 					: object_word0(p2)))               \
 	 &ValidObjectBits) == 0)
#else	/* lint */

extern unsigned set_boolean_object(/* struct object *, unsigned */);
extern unsigned set_typed_object(/* struct object *, type */);
extern unsigned set_entire_typed_object(/* struct object *, type */);
extern void clear_object(/* struct object * */);
extern unsigned set_typed_entity_object(/* struct object *, type, ENTITY * */);
extern unsigned set_executable_typed_bodied_object(/* struct object *, type, ENTITY * */);
extern unsigned set_object_from_ref(/* struct object *, REF */);
extern unsigned equal_object(/* struct object *,struct object * */);

#endif	/* lint */


#define MAX_STRING_LENGTH	65535
#define MAX_ARRAY_LENGTH	65535


#define object_incref(objp) incref(getref(objp))
#define object_decref(objp) decref(getref(objp))

/* 
 * struct corpus - used for objects that exist only at the nucleus level;
 *
 * 	NOTE that union section names must match the type name (minus '_type').
 *	This is necessary for macros (like corpus_size()) that use the type 
 *	name to work.
 */
struct corpus {
    ENTITY_FIELDS				/* header and ref info */

    union {
        struct {                                /* == array_type: growable */
            unsigned short size;                /* Number of objects in
						 * array */
            unsigned short packed_size;         /* Number of bytes allocated
                                                 * for packed representation
						 * (0 == unpacked array) */
	    IntBackPtr *interest_backptr;	/* the interests that contain
						 * this array */
            struct object *objects;		/* the objects */
        } array;

        struct {                                /* == string_type: growable */
            unsigned short size;                /* Number of characters in
                                                 * string */
            unsigned char chars[1];             /* the string */
        } string;

    	struct {                                /* == file_type */
            PSFILE      *inbuf;                 /* Input side of this IO
                                                 * channel */
            PSFILE      *outbuf;                /* Output side of this IO
                                                 * channel */
            struct object fname;                /* Name of the file */
	    struct object xstr;			/* The string being exec'd */
            struct object *tok;                 /* Compressed tokens declared
                                                 * for file*/
            short       ntok;                   /* Number of such tokens */
        } file;

        struct {                                /* == event_type */
            struct object name;
            struct object action;
            struct object client_data;
            struct object runnable_match;
            struct object canvas;
            REF process; 			/*  process interested in
                                                 * this event, or intended
                                                 * recipient */
            REF inter;                		/* interest matched by this
                                                 * event */
            REF kstate;                         /* array of down keys */
            REF next;                           /* next event in queue or
						 * next interest on the
						 * processes interest list */
	    MatchMask nameMask;			/* name match bits */
	    MatchMask actionMask;		/* action match bits */
	    short loc_x;                  	/* locator position */
	    short loc_y;
	    short delta_x;
	    short delta_y;
            unsigned  serial;                   /* event #, or interest's last
                                                 * match */
            TimeStamp time;                     /* Event's timestamp */
            fract priority;                     /* applies only to interests */
            unsigned is_queued:1;               /* event is in the eventq */
            unsigned is_interest:1;             /* event is in an interest list */
            unsigned is_exclusive:1;            /* interest gobbles matching
                                                 * events */
            unsigned pre_child:1;               /* interest matches ahead of
                                                 * interests on child canvases
                                                 */
            unsigned name_match:2;              /* was /AnyValue specified in
                                                 * an interest name or action */
            unsigned action_match:2;            /* and how */
	    unsigned cancelmotionhint:1;	/* PointerMotionHint - X11 */
	    unsigned damage_interest:1;		/* interested in damage? */
	    unsigned motion_event:1;
	    unsigned locationnotset:1;
	    unsigned keyboard_event:1;
	    unsigned mouse_event:1;
	    unsigned sync:1;
	    unsigned validMatchMask:1;
	    unsigned synthetic:1;		/* Synthetic event */
	    unsigned systeminterest:1;		/* E.g. X11 focus interest */
	    unsigned grab_event:1;		/* is this event caused by a grab 
						 * used for Enter/Exit and 
						 * FocusIn/FocusOut events 
						 */
	    unsigned pad:5;
	    unsigned char deviceid;		/* Input Device ID */
        } event;

	struct {                                /* == monitor_type */
            struct execution_environment *holder;
            int reentry_depth;                /* # times current holder has
                                               * entered the same monitor */
            struct pq_header monitor_waitq;
        } monitor;

        struct environment *environ;        	/* == environ_type */
        char *opaque;				/* == opaque_type */
	SHAPE shape;				/* == shape_type */
	struct {
	    Pool		*vm_pool;
	    struct object	vm_name;
	} vm;
    } corpus;
};

extern struct object type_table[]; 	/* type to name mapping table */

extern char delim[];			/* A table indicating which characters 
					 * are delimiters */
extern char translate_digit[];		/* A table which maps characters to
				   	 * their value as a digit */

#define corpus_size(t) (((int)&((struct corpus *)0)->corpus) \
			       + sizeof ((struct corpus *)0)->corpus.t)
#define array_end(a)	((struct object *)(((char *)a) + corpus_size(array)))
/*
 * get instance dict from an entity pointer
 */
#define instancedict_of(entityp) \
	((DICT *)(refword(entityp) & ADDR_MASK))
#define set_instancedict(entityp, dictp) \
	(refword(entityp) = refword(entityp) & ~ADDR_MASK | (int)(dictp))

#define corpus_of(ref) ((struct corpus *)(entity_addr(ref)))


struct corpus *new_composite(/* *pool, *object, type, elements, initbytes */);

#define new_kwd_name(h,n,l)		syntax
#define lookup_kwd_name(n,l)		syntax

void convert_string_to_name();
void convert_string_to_existing_name();

extern struct object parse_file (/* f */);
extern struct object parse_string (/* str */);
extern struct object open_file (/* fn, len, dir */);
extern struct execution_environment *create_process();
extern struct object *write_matrix(/*matrix object*/);
extern struct object *read_matrix(/*matrix object*/);
extern enum error_type coerce_to_real(/* *objstart, *objend */);
extern enum error_type coerce_to_reals(/* *objstart, *objend */);

#ifndef incref
extern void incref(), decref();
#endif

extern enum error_type define_object_in_dictionary();
extern enum error_type array_copy();
extern enum error_type get_processgroup_as_object();
extern enum error_type set_dstack_from_object();
extern enum error_type set_ostack_from_object();
extern enum error_type get_EventQueue_as_object();

#ifndef	lint

#define object_type_pair(o1,o2) ((((int) (o1).type)<<6)+(int)(o2).type)
#define tp(t1,t2) (((int)(t1) << 6) + (int)(t2))

#define numeric_type_pair(o1,o2) (typebyte(o1)|(typebyte(o2)>>2))
#define is_numeric_pair(o1,o2)		\
	((typebyte(o1)|typebyte(o2)) <= (NUMERIC_TYPE_MASK << BYTE_TYPE_SHIFT))
#define ntp(t1,t2) (((int)(t1) << 2) + (int)(t2))

#else	/* lint */

extern unsigned object_type_pair(/* struct object *, struct object * */);
#define tp(t1,t2) ((((int) CPPCONCAT(t1,_type))<<6)+(int)CPPCONCAT(t2,_type))

extern unsigned numeric_type_pair(/* struct object *, struct object * */);
extern unsigned is_numeric_pair(/* struct object *, struct object * */);

#define ntp(t1,t2) ((((int) CPPCONCAT(t1,_type))<<2) | (int)CPPCONCAT(t2,_type))

#endif	/* lint */

/* REMIND: Temporary hack to use X-style initialization */
/* #define pr_open(x)	private_pr_open(x) */
#define TemporaryInitHack




/* REMIND BJB: almost everything from here down should be moved to nucleus.h */

/*
 * Now for information about the execution of the interpreter and processes.
 */

enum error_type {
    no_error_code,
    accept_error_code,
    dictfull_error_code,
    dictstackoverflow_error_code,
    dictstackunderflow_error_code,
    execstackoverflow_error_code,
    interrupt_error_code,
    invalidaccess_error_code,
    invalidexit_error_code,
    invalidfileaccess_error_code,
    invalidfont_error_code,
    invalidpackage_error_code,
    invalidrestore_error_code,
    ioerror_error_code,
    limitcheck_error_code,
    nocurrentpoint_error_code,
    rangecheck_error_code,
    stackoverflow_error_code,
    stackunderflow_error_code,
    syntaxerror_error_code,
    timeout_error_code,
    typecheck_error_code,
    undefined_error_code,
    undefinedfilename_error_code,
    undefinedresult_error_code,
    unimplemented_error_code,
    unmatchedmark_error_code,
    unregistered_error_code,
    VMerror_error_code,
    killprocess_error_code
};

extern char *error_names[];	/* Printable versions of the error codes */

typedef int (*psproc)();

/* 
 * struct estack_ent  -  The execution stack contains all information needed
 *			    to run the interpreter at each level of the call 
 *			    stack: it has return information & indications of 
 * 			    the type of operation in progress.
 */
struct estack_ent {
    psproc p;
    int cnt;
    struct object ob1;
    struct object ob2;
    struct object ob3;
    struct object ob4;
};


/* 
 * struct dstack_ent - An entry in a dictionary stack 
 */
struct dstack_ent {
    REF dict;		/* The actual dictionary or a magic object */
};

/*
 * default process stack sizes and limits
 */

#define NEW_STACK_SIZE		10
#define NEW_ESTACK_SIZE		5
#define NEW_DSTACK_SIZE		5

#define MAX_STACK_SIZE		1500
#define MAX_ESTACK_SIZE		250
#define MAX_DSTACK_SIZE		250

/*
 * struct execution_environment -
 * 	The environment in which PostScript executes.  It's the context for a
 * 	single thread of control.  Supposedly PostScript() should be able to
 *	switch between execution environments to effect timeslicing [eg. for 
 * 	when errors occur].
 */
struct execution_environment {

    ENTITY_FIELDS		/* entity header */

    struct object process_name; /* string object for name */
    struct object
        terminal_value;         /* process termination value */
    REF psstdout;		/* process stdout - default for output */
    REF psstderr;		/* process stderr - default for diagnostics */
    REF privatevm;		/* VM process lives in */
    REF currentvm;		/* current VM for allocation */
    REF errordict;		/* This process's private errordict */
    REF dollar_error;		/* This process's private $error dict */
    REF override_dict;          /* Dictionary of operators overriden */
    REF gontext;		/* pointer to cscript graphics context */
    REF interests;		/* The events that are interesting to this
				 * process */
    REF eq_head;		/* head of process event queue (see eq_tail) */
    struct corpus *eq_tail;	/* tail of process event queue */
    struct pool_desc
        *private_pool;          /* private VM for process */
    struct pq_element allq;	/* LL of all processes that have ever
				 * been created */
    struct pq_element nextq;    /* list of processes in same state as us */
    struct execution_environment
	*pgnext, *pgprev;	/* A doubly linked list of processes in a
				 * process group */
    struct pq_header
        process_waitq;		/* queue header of processes waiting on us */
    struct saveset *savelog;	/* A log of all the changes to variables
				 * that have been made under the influence
				 * of a "save" command.  It is undo-able by
				 * a later "restore" */
    char *error_detail;		/* More detailed description of what exactly
				 * happened */
    struct parse_state *ps;	/* parser state variables */
    struct object
        *ostack_base,		/* base of the operand stack */
        *ostack_ptr,		/* current top of operand stack */
        *ostack_limit;		/* maximum extent of the operand stack */
    struct estack_ent
        *estack_base,		/* base of the execution stack */
        *estack_ptr,		/* current top of execution stack */
        *estack_limit;		/* maximum extent of the execution stack */
    struct dstack_ent
        *dstack_base,		/* base of the dictionary stack */
	*dstack_userdict,	/* pointer to userdict */
        *dstack_ptr,		/* current top of dictionary stack */
        *dstack_limit;		/* maximum extent of the dictionary stack */
    short dstack_lock;		/* dictionary stack entry that is locked */
    short dstack_send;		/* beginning of area effected by send */
    short supersend_end;       	/* Where the previous supersend chain ended */
    short priority;		/* the process priority */
    short process_state;	/* The current process state */
    short detail_desired;	/* The amount of error reporting detail
				 * desired in the $error directory */
    short autoload;		/* depth of recursive autoloads in progress */
    unsigned timeout;		/* seconds to timeout, 0 = donttimeout */
    unsigned short random_seed[3]; /* seed for nrand48 */
    unsigned short xcid;	/* An X client's unique ID */
    enum error_type error_code;	/* the error that is currently being processed */
    unsigned int
        autobind:1,             /* true=>automatically bind operators */
        bindoverride:1,         /* true=>unbind operators */
        compat_input_dist:1,    /* true=>input distribution compatibility */
        debug_disable:1,        /* true=>disable debugging */
        events_awaken_me:1,     /* true=>awaken process when event arrives */
        is_debugger:1,		/* true=>process can access debugger primitives */
        packedarrays:1,         /* true=>pack executable arrays */
        timedout:1;		/* true=>process has exceeded time limit */
};

/* 
 *	The kinds of match an interest-event's name or action may require 
 */ 
#define DirectMatch     0
#define SimpleAnyMatch  1
#define DictAnyMatch    2 

/*
 * 	These are the various common process priorities in the server
 */

#define         user_priority           0
#define         system_priority         100

/* 
 *	The various that a process can be in.
 */

#define fileio_event_wait(fd)	((fd)+1)
#define event_write_flag 0x4000 	/* Indicates waiting on a write */

#define file_read_wait(fd)	((fd)+1)
					/* waiting for input on fd */
#define file_write_wait(fd)	((fd)+1+event_write_flag)
					/* waiting for output on fd */
#define runnable_process	0	/* process can run */
#define suspended_process	-1	/* ps suspendprocess, breakpoint */
#define zombie_process	 	-2	/* dead, outstanding references */
#define dead_process	 	-3	/* dead, no references */
#define event_wait		-4	/* ps awaitevent */
#define eventdist_wait		-5	/* x11 awaiting event distribution */
#define process_wait		-6	/* ps waitprocess */
#define monitor_wait		-7	/* ps monitor */
#define priority_wait		-8	/* waiting for server priority change */
#define time_wait		-9	/* waiting for time to elapse */

/*
 * 	All processes that have ever been created.
 */
extern struct pq_header all_processes;
/*
 * 	The currently executing process 
 */
extern struct execution_environment *current_process;
/*
 *	The current pool (VM space) to allocate objects from.
 */
extern Pool			    *current_pool;
/*
 *	The VM object for the current pool.
 */
extern REF			     current_vm;
/*
 *	The VM space to allocate objects not relating to a process.
 */
extern Pool			    *server_pool;
/*
 *	The VM object for the server pool.
 */
extern REF			     server_vm;
/*
 * 	The set of globally defined compressed input tokens.
 */
extern struct object syscommon_vec[];
/*
 * Special stuff for new interpreter loop.
 */

#ifdef TCOPT
extern int JUMP(/* ee, p */);
#define JUMP_NEW		{ return JUMP(ee, estack_p); }
#define JUMP_OLD		{ adj_estack(-1); return JUMP(ee, estack_p); }
#else
#define JUMP_NEW		{ adj_estack(1); return 0; }
#define JUMP_OLD		{ return 0; }
#endif

#define STOP			{ return 1; }
#define FAIL			{ declare_error(unimplemented); }

#define ostack_ob(n)		ee->ostack_ptr[n]

#define estack_p		ee->estack_ptr->p
#define estack_cnt		ee->estack_ptr->cnt
#define estack_ob1		ee->estack_ptr->ob1
#define estack_ob2		ee->estack_ptr->ob2
#define estack_ob3		ee->estack_ptr->ob3
#define estack_ob4		ee->estack_ptr->ob4

#define declare_error(error) {			     \
    handle_error(ee, CPPCONCAT(error,_error_code));  \
    JUMP_NEW;					     \
}

#define adj_ostack(delta) \
    ee->ostack_ptr += (delta)

#define adj_estack(delta) \
    ee->estack_ptr += (delta)

#define adj_dstack(delta) \
    ee->dstack_ptr += (delta)

#define check_ostackunderflow(count) {			\
    if (ee->ostack_ptr-(count) < ee->ostack_base) {	\
        handle_error(ee, stackunderflow_error_code);	\
        JUMP_NEW;					\
    }							\
}

extern int handle_ostackoverflow(/* ee, count */);
extern int handle_estackoverflow(/* ee, count */);
extern int handle_dstackoverflow(/* ee, count */);

#ifdef DEBUG
extern int debug_handle_ostackoverflow();
extern int debug_handle_estackoverflow();
extern int debug_handle_dstackoverflow();

#define check_ostackoverflow(count) \
    if (debug_handle_ostackoverflow(ee, count, __FILE__,__LINE__)) { JUMP_NEW; }

#define check_estackoverflow(count) \
    if (debug_handle_estackoverflow(ee, count, __FILE__,__LINE__)) { JUMP_NEW; }

#define check_dstackoverflow(count) \
    if (debug_handle_dstackoverflow(ee, count,__FILE__,__LINE__)) { JUMP_NEW; }

#else

#define check_ostackoverflow(count) {			\
    if (ee->ostack_ptr+(count) > ee->ostack_limit) {	\
        if (handle_ostackoverflow(ee, count)) {		\
            JUMP_NEW;					\
        }						\
    }							\
}

#define check_estackoverflow(count) {			\
    if (ee->estack_ptr+(count) > ee->estack_limit) {	\
        if (handle_estackoverflow(ee, count)) {		\
            JUMP_NEW;					\
        }						\
    }							\
}

#define check_dstackoverflow(count) {			\
    if (ee->dstack_ptr+(count) > ee->dstack_limit) {	\
        if (handle_dstackoverflow(ee, count)) {		\
            JUMP_NEW;					\
        }						\
    }							\
}

#endif /* DEBUG */

#define check_dstackunderflow(depth,pop) {				\
    if (ee->dstack_ptr-(depth) < ee->dstack_base ||			\
            ee->dstack_ptr-(pop) < ee->dstack_base+ee->dstack_lock) {	\
        handle_error(ee, dictstackunderflow_error_code);		\
        JUMP_NEW;							\
    }									\
}

extern void handle_error(/* ee, error_code */);

extern void trim_dstack(/* ee, newtop */);
extern void trim_estack(/* ee, newtop */);
extern void trim_ostack(/* ee, newtop */);
extern void cleanoff_dstack(/* ee */);
extern void cleanoff_estack(/* ee */);
extern void cleanoff_ostack(/* ee */);
extern void get_dstack_as_object(/* ee, objp */);
extern void get_estack_as_object(/* ee, objp */);
extern void get_ostack_as_object(/* ee, objp */);

extern void parse_cleanup(/* ee */);
extern void free_process(/* ee */);
extern void determine_fate(/* ee */);
extern void x_kill_one_process(/* ee */);
extern int  kill_one_process(/* ee */);
extern int  kill_process_group(/* ee */);
extern void WaitForInput(/* now, nowait */);
extern void add_event_to_queue(/* event, collapse_motion */);
extern void enqueue_runnable(/* ee, head */);
extern void notify_process_waiters(/* ne */);
extern void notify_monitor_waiters(/* monitor */);
extern void notify_io_waiters(/* fd, for_read */);


#define is_dict(refp) \
    ((refp)->type == dictionary_type || \
    magicdict_table[(unsigned)(refp)->type] != 0)


/* masks used to decode access-encoded types */

#define type_mask       0x3c    /* 111100 */
#define access_mask     0x03    /* 000011 */

/* Macros for checking and manipulating the access_encoded types */
#define is_encoded_type(refp) \
        ((refp)->type >= first_encoded_type && \
         (refp)->type <= last_encoded_type)
 
#define get_access(refp) \
        ((enum access) (is_dict(refp) ? GET_DICT_ACCESS(refp) : GET_ENC_ACCESS(refp)))
 
#define set_access(refp, acc_mode) \
        (is_dict(refp) ? SET_DICT_ACCESS(refp, acc_mode) : \
                         SET_ENC_ACCESS(refp, acc_mode))
 
/* p can be either a ref ptr or an entity/corpus ptr */
#define get_type(p) ((enum type) (is_encoded_type(p) ? (unsigned) ((p)->type & type_mask) : (unsigned) (p)->type))

#define is_executable(refp) (get_access(refp) <= executeonly_access)
 
#define is_readable(refp) (get_access(refp) <= readonly_access)
 
#define is_writeable(refp) (get_access(refp) == full_access)
 
#define is_array_type(typ) (((enum type)((unsigned int) typ & type_mask)) == array_type)
#define is_packedarray_type(typ) (((enum type)((unsigned int) typ & type_mask)) == packedarray_type)
#define is_file_type(typ) (((enum type)((unsigned int) typ & type_mask)) == file_type)
#define is_string_type(typ) (((enum type) ((unsigned int ) typ & type_mask)) == string_type)
 
/* The following, capitalized macros are pretty low level and should not be */
/* needed elsewhere except by the preceding macros in this file.            */
 
#define GET_DICT_ACCESS(refp) ((enum access)(corpus_of(*(refp)))->access)
 
#define GET_ENC_ACCESS(refp) ((enum access)(((unsigned) (refp)->type) & access_mask))
 
#define SET_DICT_ACCESS(refp, acc_mode) \
        ((void)((corpus_of(*(refp)))->access = acc_mode))
 
#define SET_ENC_ACCESS(refp, acc_mode) \
        ((void) ((refp)->type = ((enum type) ((refp)->type & type_mask)) | (enum type) acc_mode))

/*
 * Special primitive definitions.
 */

extern int array_exec( /* ee */ );
extern int array_debug_exec( /* ee */ );

extern int packedarray_exec( /* ee */ );
extern int packedarray_debug_exec( /* ee */ );

extern int buildimage_cont( /* ee */ );
extern int end_exec( /* ee */ );
extern int file_exec( /* ee */ );
extern int file_exec_done( /* ee */ );
extern int flush_stdfile( /* ee */ );
extern int for_integer_cont( /* ee */ );
extern int for_real_cont( /* ee */ );
extern int forall_array_cont( /* ee */ );
extern int forall_dict_cont( /* ee */ );
extern int forall_magicdict_cont( /* ee */ );
extern int forall_packedarray_cont( /* ee */ );
extern int forall_string_cont( /* ee */ );
extern int handle_error_cont( /* ee */ );
extern int inputstream_exec( /* ee */ );
extern int loop_cont( /* ee */ );
extern int monitor_cont( /* ee */ );
extern int name_exec( /* ee */ );
extern int object_exec( /* ee */ );
extern int psparse_token( /* ee */ );
extern int pathforallvec_cont( /* ee */ );
extern int repeat_cont( /* ee */ );
extern int send_cont( /* ee */ );
extern int setscreen_cont( /* ee */ );
extern int stopped_cont( /* ee */ );
extern int string_exec( /* ee */ );
extern int superstopped_cont( /* ee */ );
extern int token_done( /* ee */ );

extern void define_operator_in_dict(/* dict,name,function */);
extern void define_magic_in_dict(/* dict,name,function,index */);

#define define_operator(name,function) \
    define_operator_in_dict(system_dictionary,name,function)

#define define_magic(name,dict,function,index) \
    define_magic_in_dict(dict,name,function,index)

#define define_debug_operator(name,function) \
    define_operator_in_dict(debugdict,name,function)

#define define_package_operator(name, function, package) \
    define_operator_in_dict(package, name, function)

#define object_to_double(p, d) {		\
    switch ((p)->type) {			\
    case integer_type:				\
        (d) = (p)->value.integer;		\
        break;					\
    case real_type:				\
        (d) = (p)->value.real;			\
        break;					\
    default:					\
        declare_error(typecheck);		\
    }						\
}

struct lookup_return {
    struct dstack_ent *dsent;
    struct object *val;
};

extern struct lookup_return *find_obj_in_dictstack();
extern struct object *find_obj_in_dict();

#endif /* _NEWS_POSTSCRIPT_H */
