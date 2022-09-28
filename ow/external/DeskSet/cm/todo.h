/*static  char sccsid[] = "@(#)todo.h 3.3 94/09/01 Copyr 1991 Sun Microsystems, Inc.";
	todo.h
*/

/* used for storing todo panel items and appts */
struct Todo_item  {
        Panel_item pi, box_pi, numpi;
        Uid uid;
        struct Todo_item *next;
};

typedef struct {
	Frame          	frame;
        Panel		panel;
        Scrollbar	sb;
	int 		curr_page, num_pages, num_items;
	char 		header[128];
	Panel_item	none_pi;
	Glance 		glance;
	Boolean		changed;
	struct Todo_item       *head_todo, *last_todo;	
}Todo;

#define MAX_TODO 36
#define MAX_TODO_LP 29

extern void t_create_todolist();
extern Boolean todo_showing();
