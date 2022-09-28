#ifdef lint
#ifdef sccs
static  char sccsid[] = "@(#)tree.c 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";
#endif   
#endif

/*
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

/*	tree.c
	2-3-4 tree, a.k.a. red-black tree, implementation  */

#include <sys/param.h>
#include "util.h"
#include "tree.h"

extern int debug;

typedef struct {
	int size;		/* used in insert and size */
	int count;		/* used in checktree only */
	Rb_Status status;	/* used in checktree and insert */
	Data data;		/* used in lookups only */
	Node *i;		/* used in insert only */
	Key key;		/* used in insert only */
	Node *d;		/* used in delete only */
	Node *y;		/* dummy that is at both links of z */
	Node *z;		/* dummy used as child of leaf nodes */
	Rb_tree *tree;		/* back link to parent tree */
	Get_key get;
	Enumerate_proc enumerate;
	Compare_proc compare;
	} Private;

typedef void (*Action_proc)
	(/* Private *private; Node *y, *z, *root */);


static Node *
balance(gg, g, f, x)
	Node *gg, *g, *f, *x;
{
	Node *t;
	Color tc;
	if (gg == NULL || g == NULL) exit (-1);
	if (f == g->llink) {
	  	if (x == f->rlink) {
	    		f->rlink = x->llink;
	    		x->llink = f;
	    		t = f;
	    		f = x;
	    		x = t;
	    	}
	}
	else {
	  	if (x == f->llink) {
	    		f->llink = x->rlink;
	    		x->rlink = f;
	    		t = f;
	    		f = x;
	    		x = t;
	    	}
	}
	if (x == f->llink) {
	  	g->llink = f->rlink;
	  	f->rlink = g;
	}
	else {
	  	g->rlink = f->llink;
	  	f->llink = g;
	}
	if (g == gg->rlink) gg->rlink = f;
	else gg->llink = f;
	tc = g->color;
	g->color = f->color;
	f->color = tc;
	return(f);
}

static void 
doit(tree, proc)
	Rb_tree *tree; Action_proc proc;
{
	Private *private;
	Node *root;

	if (tree==NULL) return;
	private = (Private *) tree->private;
	root = tree->root;
	if (root == NULL || root->llink != NULL) {
		private->status = rb_badtable;
		return;
	}
	proc(private, private->y, private->z, root);
}

extern Rb_tree *
rb_create (get, compare)
	Get_key get; Compare_proc compare;
{
	Private *p;
	Node *root, *y, *z;
	Rb_tree *tree;

	p = (Private *) ckalloc (sizeof(Private));
	p->size = 0;
	p->count = 0;
	p->status = rb_ok;
	p->data = NULL;
	p->i = NULL;
	p->key = 0;
	p->d = NULL;
	p->y = (Node *) ckalloc (sizeof(Node));
	p->z = (Node *) ckalloc (sizeof(Node));
	p->get = get;
	p->enumerate = NULL;
	p->compare = compare;

	root = (Node *) ckalloc (sizeof(Node));
	y = p->y;
	z = p->z;
	tree = (Rb_tree *) ckalloc (sizeof(Rb_tree));
	tree->root = root;
	tree->private = (caddr_t) p;
	tree->client_data = NULL;
	p->tree = tree;   /* link back so callbacks can access */
	root->color = black;
	root->llink = NULL;
	root->rlink = z;
	y->color = red;
	y->llink = y->rlink = NULL;
	z->color = black;
	z->llink = z->rlink = y;

	return(tree);
}


extern void
rb_destroy(tree, destroy)
	Rb_tree *tree; Enumerate_proc destroy;
{
	Private *p = NULL;
	Data data = NULL;
	Node *node = NULL;
	Key key;

	/* NOTE:there is a client data field
		associated with the tree struct.
		It is up to the client to destroy
		these.				*/
	if (tree==NULL) return;
	p = (Private *) tree->private;
	data = rb_lookup_smallest(tree);

	/* enumerate tree, destroying data */
	while(data != NULL) {
		key = p->get(data);
		node = rb_delete(tree, key);
		if (destroy)
			destroy(data);
		free(node);
		data = rb_lookup_next_larger(tree, key);
	}

	/* destroy the private internal struct */
	free(p->y);
	free(p->z);
	free(p);

	/* destroy the root node */
	free(tree->root);

	/* destroy the tree */
	free(tree);
}

/* ARGSUSED */
static void
size_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	/* dummy proc for monitor */
}

extern int
rb_size(tree)
	Rb_tree *tree;
{
	Private *p;
	if (tree==NULL) return(0);
	p = (Private *) tree->private;
	if (tree != NULL) {
		doit(tree, size_callback);
		return(p->size);
	}
	else return(0);
}

/* ARGSUSED */
static void
insert_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Node *x=NULL, *gg=NULL, *g=NULL, *f=NULL;
	Comparison c = greater;

	f = root;
	x = f->rlink;
	for (;;) {
		if (x->llink->color == red && x->rlink->color == red) {
			if (x == z) {
				if (c == equal) {
					private->status = rb_duplicate;
					root->rlink->color = black;
					return;
				}
	      			x = private->i;
	      			x->llink = z;
	      			x->rlink = z;
	      			if (c == less) f->llink = x; else f->rlink = x;
	      			c = equal;
	      			private->size++;
	      		}
	    		x->llink->color = black;
	    		x->rlink->color = black;
	    		x->color = red;
	    		if (f->color == red) {
	      			g = balance (gg, g, f, x);
	      			x = g;
	      		}
		}
	  	if (c == equal) break;
	  	gg = g; g = f; f = x;
	  	c = private->compare (private->key, x->data);
	  	if (c==equal) {
			private->status=rb_duplicate;
			root->rlink->color=black;
			return;
		}
	  	x = (c == less) ? x->llink : x-> rlink;
	} 
	root->rlink->color = black;
}

extern Rb_Status
rb_insert_node(tree, node, key)
	Rb_tree *tree; Node *node; Key key;
{
	Private *private;

	if (tree==NULL) return(rb_notable);
	private = (Private *) tree->private;
	private->status = rb_ok;
	private->i = node;
	private->key = key;
	doit (tree, insert_callback);
	return (private->status);
}

extern Rb_Status
rb_insert(tree, data, key)
	Rb_tree *tree; Data data; Key key;
{
	Node *node;

	if (tree==NULL) return(rb_notable);
	node = (Node *) ckalloc (sizeof(Node));
	node->data = data;
	return(rb_insert_node(tree, node, key));
}

static void
delete_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Node *f, *result, *parent;
	Node *x, *g, *b;
	Comparison c;

	f = root;
	x = f->rlink;
	result = NULL;

	if (x == z) return;
	y->color = black;
	if (x->llink->color == black && x->rlink->color == black)
		x->color = red;
	c = private->compare(private->key, x->data);
	if (c == equal) {
		result = x;
		parent = f;
	}
	for (;;) {
		g = f;
	  	f = x;
	  	if (c == less) {
	    		b = x->rlink;
	    		x = x->llink;
	    	}
	  	else {
	    		b = x->llink;
	    		x = x->rlink;
	    	}
	  	if (x != z) {
	    		c = private->compare(private->key, x->data);
	    		if (c == equal) {
	      			result = x; 
	      			parent = f;
	      		}
	    	}
		if (x->color == red || x->llink->color == red ||
	     		x->rlink->color == red) continue;
		if (b->color == red) {
	  		if (b == f->llink) {
	    			f->llink = b->rlink;
	    			b->rlink = f;
	    		}
	  		else {
	    			f->rlink = b->llink;
	    			b->llink = f;
	    		}
	  		f->color = red;
	  		b->color = black;
	  		if (f == g->llink) g->llink = b;
	  		else g->rlink = b;
	  		x = b;
	  		c = private->compare(private->key, x->data);
	  		continue;
		}
		if (x == z) break;
		x->color = red;
		if (b->llink->color == red) {
	  		b->llink->color = black;
	  		x = balance (g, f, b, b->llink);
	  		c = private->compare(private->key, x->data);
	  		continue;
	  	}
		if (b->rlink->color == red) {
	  		b->rlink->color = black;
	  		x = balance(g, f, b, b->rlink);
	  		c = private->compare(private->key, x->data);
	  		continue;
		}
		f->color = black;
		b->color = red;
	} /* end for-loop */ 
	root->rlink->color = black;
	z->color = black;
	y->color = red;
	if (result != NULL) {
		if (g->llink == f) g->llink = z;
	  	else g->rlink = z;
	  	if (f != result) {
	    		if (parent->llink == result) parent->llink = f;
	    		else parent->rlink = f;
	    		f->llink = result->llink;
	    		f->rlink = result->rlink;
	    		f->color = result->color;
	    	}
	  	private->size--;
	} 
	private->d = result;
}

extern Node *
rb_delete(tree, key)
	Rb_tree *tree; Key key;
{
	Private *p;
	if (tree==NULL) return((Node *)NULL);
	p = (Private *) tree->private;
	p->key = key;
	p->d = NULL;	/* in case the key is not found */
	doit (tree, delete_callback);
	return(p->d);
}

/* ARGSUSED */
static void
lookup_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Comparison c;
	Node *eq = root->rlink;
	for (;;) {
		if (eq == z) return;
		c = private->compare(private->key, eq->data);
	  	switch(c) {
	    	case equal:
	      		goto bye;
	    	case less:
	      		eq = eq->llink;
	      		break;
	    	case greater:
	      		eq = eq->rlink;
	      		break;
	    	default:
	      		break;
	    	}
	} 
	bye: private->data = eq->data;
}

extern Data
rb_lookup(tree, key)
	Rb_tree *tree; Key key;
{
	Private *private;
	if (tree==NULL) return((caddr_t)NULL);
	private = (Private *)tree->private;
	private->key = key;
	private->data = NULL; /* might have been previously used */
	doit (tree, lookup_callback);
	return (private->data);
}

/* ARGSUSED */
static void
lookup_smallest_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Node *smallest = root->rlink;
	if (smallest == z) return;
	while (smallest->llink != z) {
		  smallest = smallest->llink;
	}
	private->data = smallest->data;
}

extern Data
rb_lookup_smallest(tree)
	Rb_tree *tree;
{
	Private *private;
	if (tree==NULL) return((caddr_t)NULL);
	private = (Private *)tree->private;
	private->data = NULL; /* might have been previously used */
	doit (tree, lookup_smallest_callback);
	return (private->data);
}

/* ARGSUSED */
static void
next_larger_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Node *larger = NULL;
	Node *x = root->rlink;
	while (x != z) {
		if (private->compare (private->key, x->data) == less) {
	    		larger = x;
	    		x = x->llink;
		}
		else x= x->rlink;
	}
	if (larger != NULL) private->data = larger->data;
}

extern Data
rb_lookup_next_larger(tree, key)
	Rb_tree *tree; Key key;
{
	Private *private;
	if (tree==NULL) return((caddr_t)NULL);
	private = (Private *) tree->private;
	private->key = key;
	private->data = NULL; /* might have been previously used */
	doit (tree, next_larger_callback);
	return(private->data);
}

/* ARGSUSED */
static void
lookup_largest_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	Node *largest = root->rlink;
	if (largest == z) return;
	while (largest->rlink != z) {
		largest = largest->rlink;
	}
	private->data = largest->data;
}

extern Data
rb_lookup_largest(tree)
	Rb_tree *tree;
{
	Private *private;

	if (tree==NULL) return((caddr_t)NULL);
	private = (Private *) tree->private;
	private->data = NULL; /* might have been previously used */
	doit (tree, lookup_largest_callback);
	return (private->data);
}

/* ARGSUSED */
static void
next_smaller_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{

	Node *smaller = NULL;
	Node *x = root->rlink;
	while (x != z) {
		if (private->compare(private->key, x->data) == greater) {
			smaller = x;
			x = x->rlink;
		}
		else x = x->llink;
	}
	if (smaller != NULL) private->data = smaller->data;
}

extern Data
rb_lookup_next_smaller(tree, key)
	Rb_tree *tree; Key key;
{
	Private *private;
	if (tree==NULL) return((caddr_t)NULL);
	private = (Private *) tree->private;
	private->key = key;
	private->data = NULL; /* might have been previously used */
	doit (tree, next_smaller_callback);
	return(private->data);
}

typedef enum {up, down} Direction;

static Boolean
visit_subtree(node, p, z, dir)
	Node *node; Private *p; Node *z; Direction dir;
{
	Node *link;
	link = (dir == up) ? node->llink : node->rlink;
	if (link != z && visit_subtree(link, p, z, dir)) return(true);
	if (p->enumerate(node->data)) return(true);
	link = (dir == up) ? node->rlink : node->llink;
	if (link != z) return(visit_subtree(link, p, z, dir));
	else return(false);
}

/* ARGSUSED */
static void
enumerate_up_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	if (root == NULL || root->rlink == z) return;
	(void) visit_subtree(root->rlink, private, z, up);
}

extern void
rb_enumerate_up(tree, proc)
	Rb_tree *tree; Enumerate_proc proc;
{
	Private *private;
	if (tree==NULL) return;
	private = (Private *) tree->private;
	private->enumerate = proc;
	doit (tree, enumerate_up_callback);
}

/* ARGSUSED */
static void
enumerate_down_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	if (root == NULL || root->rlink == z) return;
	(void) visit_subtree(root->rlink, private, z, down);
}

extern void
rb_enumerate_down(tree, proc)
	Rb_tree *tree; Enumerate_proc proc;
{
	Private *private;
	if (tree==NULL) return;
	private = (Private *) tree->private;
	private->enumerate = proc;
	doit (tree, enumerate_down_callback);

}

/* --------------------DEBUGGING-------------------------*/

static int 
assert(p)
	int p;
{
	return(p);
}

typedef struct {Key max; int i; Boolean bool;} Rec;

static void
check1(x, max, z, rec, private)
	Node *x; Key max; Node *z; Rec *rec; Private *private;
{
	int dl, dr;
	Boolean redchild;
	Rec localrec; Rec *localp = &localrec;
	if (x == z) {
		rec->max = max;
	  	rec->i = 0;
	  	rec->bool = false;
	  	return;
	}
	check1(x->llink, max, z, localp, private);
	if (private->status == rb_badtable) return;
	max = localp->max;
	dl = localp->i;
	redchild = localp->bool;
	if (!assert (!(redchild && (x->color == red)))) {
		private->status = rb_badtable;
	  	return;
	} 
	if (!assert (private->compare(max, x->data) ==
		     (private->count == 0 ? equal : less))) {
		private->status = rb_badtable;
	  	return;
	}
	private->count++;
	check1(x->rlink, private->get(x->data), z, localp, private);
	if (private->status == rb_badtable) return;
	max = localp->max;
	dr = localp->i;
	redchild = localp->bool;
	if (!assert (!(redchild && (x->color == red)))) {
		private->status = rb_badtable;
	  	return;
	}
	if (!assert (dl == dr)) {
	  	private->status = rb_badtable;
	  	return;
	}
	rec->max = max;
	rec->i = dl + ((x->color == black) ? 1 : 0);
	rec->bool = ((x->color == red) ? true : false);
}

static void
check_tree_callback(private, y, z, root)
	Private *private; Node *y, *z, *root;
{
	if (!assert (z->llink == y)) {
		private->status = rb_badtable;
		return;
	}
	if (!assert (z->rlink == y)) {
		private->status = rb_badtable;
		return;
	}
	if (root->rlink != z) {
		Rec localrec;
		Rec *localp = &localrec;
		Node *smallest = root->rlink;
		while (smallest->llink != z) {
			smallest = smallest->llink;
		}
		check1(root->rlink, private->get(smallest->data), z, localp, private);
		if (private->status == rb_badtable) return;
	}
}

extern Rb_Status
rb_check_tree(tree)
	Rb_tree *tree;
{
	Private *p;
	if (tree==NULL) return(rb_notable);
	p = (Private *) tree->private;
	p->status = rb_ok;
	p->count = 0;
	doit (tree, check_tree_callback);
	return(p->status);
}
