/*static  char sccsid[] = "@(#)find.h 3.3 93/01/29 Copyr 1991 Sun Microsystems, Inc.";
	find.h
*/

typedef struct {
	Frame          	frame;
        Panel_item	months;
        Panel_item	ff_button;
        Panel_item	apptstr;
	int 		no_months;
	char		*apptval;
}Find;

extern caddr_t make_find();
