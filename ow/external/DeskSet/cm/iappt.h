/*static  char sccsid[] = "@(#)iappt.h 3.2 92/10/30 Copyr 1991 Sun Microsystems, Inc.";
 *
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#define	GET_R_ACCESS(p_info)		((Access_Entry *) (p_info->r_access))
#define	SET_R_ACCESS(p_info,v)		(p_info)->r_access = (caddr_t) v
#define	GET_W_ACCESS(p_info)		((Access_Entry *) (p_info->w_access))
#define	SET_W_ACCESS(p_info,v)		(p_info)->w_access = (caddr_t) v
#define	GET_D_ACCESS(p_info)		((Access_Entry *) (p_info->d_access))
#define	SET_D_ACCESS(p_info,v)		(p_info)->d_access = (caddr_t) v
#define	GET_X_ACCESS(p_info)		((Access_Entry *) (p_info->x_access))
#define	SET_X_ACCESS(p_info,v)		(p_info)->x_access = (caddr_t) v
#define	APPT_TREE(info)			((Rb_tree *) ((info)->rb_tree))
#define	REPT_LIST(info)			((Hc_list *) ((info)->hc_list))
#define	RMND_QUEUE(info)		((Rm_que *)  ((info)->rm_queue))
#define	KEY(p_appt)			((Appt *) (p_appt))->appt_id.key
#define	CM_TICK(p_appt)			((Appt *) (p_appt))->appt_id.tick

typedef	struct data_info {
	Boolean modified;		/* if true, do garbage collection */
	caddr_t	rb_tree;		/* for single appointments */
	caddr_t	hc_list;		/* for repeating appointments */
	caddr_t	rm_queue;		/* active reminder queue */
	caddr_t	r_access;		/* read access */
	caddr_t	w_access;		/* write access */
	caddr_t d_access;		/* delete access */
	caddr_t x_access;		/* exec access */
} Info;

extern void appt_free_excpt();
extern Except *copy_excpt();

#define is_appointment(p_appt)  	((p_appt)->period.period == single)
#define is_repeater(p_appt)     	((p_appt)->period.period != single)
