/*static  char sccsid[] = "@(#)list.h 3.2 92/12/03 Copyr 1991 Sun Microsystems, Inc.";
 *  list.h	
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

typedef struct lnode {
	struct lnode *llink;
	struct lnode *rlink;
	Data  data;
	long lasttick;
	} Lnode;

typedef struct {
	Lnode *root;
	caddr_t private;	/* for internal tool state */
	} Hc_list;

extern Hc_list* hc_create (/* Get_key get; Compare_proc compare */);

extern void hc_destroy (/* Hc_list *l */); 

extern int hc_size (/* Hc_list *l */);

extern Rb_Status hc_insert (/* Hc_list *l; Data data; Key key */);

extern Lnode *hc_delete_node (/* Hc_list *l; Lnode *node */);

extern Lnode * hc_delete (/* Hc_list *l; Key key */);

extern Data hc_lookup (/* Hc_list *l; Key key */);

extern Data hc_lookup_next_larger (/* Hc_list *l; Key key */);

extern Data hc_lookup_next_smaller (/* Hc_list *l; Key key */);

extern Data hc_lookup_smallest (/* Hc_list *l */);

extern Data hc_lookup_largest (/* Hc_list *l */);

extern void hc_enumerate_up (/* Hc_list *l; Enumerate_proc doit */);

extern void hc_enumerate_down (/* Hc_list *l; Enumerate_proc doit */);

extern Rb_Status hc_check_list (/* Hc_list *l */);

#define	hc_lookup_next(p_node)		(p_node)->rlink

#define	hc_lookup_previous(p_node)	(p_node)->llink
