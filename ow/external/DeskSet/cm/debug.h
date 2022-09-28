/*	static char sccsid[] = "@(#)debug.h 3.1 92/04/03 Copyr 1988 Sun Micro";
	debug.h
*/

/*	level 0 debugging: error/diagnostic messages only*/
extern int debug;

/*	level 1 debugging: complete data structure trace */
extern int debug1;

extern int expert;

extern int spill_tree (/* Table *t */);

extern void print_tick (/* Tick *t */);
