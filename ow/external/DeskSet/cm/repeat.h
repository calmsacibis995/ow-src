/*static  char sccsid[] = "@(#)repeat.h 1.1 92/09/15 Copyr 1991 Sun Microsystems, Inc.";
	goto.h
*/

typedef struct {
	Frame          	frame;
	Panel_item    	repeatunit;
	Panel_item    	repeatunitmessage;
	Panel_item    	repeat_menu;
	int    		repeatunitval;
}Repeat;

extern caddr_t make_repeat();
extern void show_repeat();
