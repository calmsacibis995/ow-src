/*static  char sccsid[] = "@(#)yearglance.h 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";*/
/*	static char sccsid[] = "@(#)yearglance.h 3.1 92/04/03 Copy 1989 Sun Micro;  
	yearglance.h 
*/
/* if positioning of months in year view changes this must change too!
This specifies row and col for easy selection in year view */
static int month_row_col[12][2] = {
        0, 0,
        0, 1,
        0, 2,
        1, 0,
        1, 1,
        1, 2,
        2, 0,
        2, 1,
        2, 2,
        3, 0,
        3, 1,
        3, 2,
        };

#define ROW 0
#define COL 1
