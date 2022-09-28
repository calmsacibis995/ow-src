#ifndef _NEWS_DEBUG_H
#define _NEWS_DEBUG_H


#ident "@(#)debug.h	1.2 06/11/93 NEWS SMI"


/* Copyright (c) 1990-1991 by Sun Microsystems, Inc. */


struct proc_bkpt_data {
    struct object process;	/* not counted */
    struct object proc;		/* not counted */
    int start;
    int length;
    struct object obj;
};


struct error_bkpt_data {
    struct object process;	/* not counted */
    struct object error;	/* not counted */
    struct object obj;
};


extern struct proc_bkpt_data *proc_bkpts;
extern int proc_bkpts_allocated;
extern int proc_bkpts_used;

extern struct error_bkpt_data *error_bkpts;
extern int error_bkpts_allocated;
extern int error_bkpts_used;


/* nucleus level */

extern enum error_type set_breakpoint(/* process, array, start, length, obj */);
extern enum error_type clear_breakpoint(/* process, array, start, length */);
extern enum error_type get_breakpoints(/* reply */);
extern enum error_type check_breakpoint(/* process, array, reply */);
extern void copy_breakpoints(/* from_ee, to_ee */);
extern void match_breakpoint(/* ee, array, start, obj */);
extern int has_a_breakpoint(/* ee, array */);
extern void disable_breakpoints(/* ee */);
extern void enable_breakpoints(/* ee */);
extern void clear_process_breakpoint(/* process */);
extern void clear_array_breakpoint(/* array */);

/* NeWS level */

extern int debug_cont(/* ee */);


#endif /* _NEWS_DEBUG_H */
