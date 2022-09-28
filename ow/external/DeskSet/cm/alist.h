/*static  char sccsid[] = "@(#)alist.h 3.2 92/07/30 Copyr 1991 Sun Microsystems, Inc.";
	alist.h
*/

/* used for storing alist panel items and appts */
struct Alist_item  {
        Panel_item pi, numpi;
        Abb_Appt *a;
        struct Alist_item *next;
};

typedef struct {
	Frame          	frame;
        Panel		panel;
        Scrollbar	sb;
	int 		num_items;
	char 		header[80];
	Panel_item      none_pi;
	Glance		glance;
	Boolean		changed;
	struct Alist_item       *head_alist, *last_alist;	
}Alist;

extern void a_create_alist();
extern Boolean alist_showing();
