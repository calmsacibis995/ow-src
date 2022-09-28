/* static char sccsid[] = "@(#)tree.h 3.1 92/04/03 Copyr 1988 Sun Micro"; */

/*	tree.h
	2-3-4 tree, a.k.a. red-black tree	*/

typedef enum {red=0, black=1} Color;
typedef enum {
	rb_ok=0, rb_duplicate=1, rb_badtable=2,
	rb_notable=3, rb_failed=4, rb_other=5} Rb_Status;
typedef enum {less, equal, greater} Comparison;
typedef caddr_t Key;
typedef caddr_t Data;

typedef struct node {
	struct node *llink;
	struct node *rlink;
	Color color;
	Data  data;
} Node;

typedef struct {
	Node *root;
	caddr_t private;	/* for internal tool state */
	caddr_t client_data;	/* available to clients    */
} Rb_tree;

typedef Key (*Get_key) (/* Data data */);

typedef Comparison (*Compare_proc)(/* Key key; Data data */);

typedef Boolean (*Enumerate_proc) (/* Data data */);

extern Rb_tree* rb_create (/* Get_key get; Compare_proc compare */);

extern void rb_destroy (/* Rb_tree *t; Enumerate_proc destroy */); 

extern int rb_size (/* Rb_tree *t */);

extern Rb_Status rb_insert (/* Rb_tree *t; Data data; Key key */);

extern Rb_Status rb_insert_node (/* Rb_tree *t; Node *node; Key key */);

extern Node * rb_delete (/* Rb_tree *t; Key key */);

extern Data rb_lookup (/* Rb_tree *t; Key key */);

extern Data rb_lookup_next_larger (/* Rb_tree *t; Key key */);

extern Data rb_lookup_next_smaller (/* Rb_tree *t; Key key */);

extern Data rb_lookup_smallest (/* Rb_tree *t */);

extern Data rb_lookup_largest (/* Rb_tree *t */);

extern void rb_enumerate_up (/* Rb_tree *t; Enumerate_proc doit */);

extern void rb_enumerate_down (/* Rb_tree *t; Enumerate_proc doit */);

extern Rb_Status rb_check_tree (/* Rb_tree *t */);


