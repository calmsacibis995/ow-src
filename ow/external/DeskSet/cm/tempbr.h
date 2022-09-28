/*static  char sccsid[] = "@(#)tempbr.h 1.1 92/09/15 Copyr 1991 Sun Microsystems, Inc.";
	tempbr.h
*/

typedef struct {
	Frame          	frame;
	Panel_item      name;
} Tempbr;

extern caddr_t make_tempbr();
