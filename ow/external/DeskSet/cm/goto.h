/*static  char sccsid[] = "@(#)goto.h 1.1 92/09/15 Copyr 1991 Sun Microsystems, Inc.";
	goto.h
*/

typedef struct {
	Frame          	frame;
	Panel_item	datetext;
}Goto;

extern caddr_t make_goto();
