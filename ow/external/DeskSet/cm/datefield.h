/*static  char sccsid[] = "@(#)datefield.h 3.5 93/01/06 Copyr 1991 Sun Microsystems, Inc.";
 *      datefield.h
 */
extern void create_datefield();
extern void set_date_on_panel();
extern char* get_date_str();

typedef enum {
        Order_MDY = 0,
        Order_DMY = 1,
        Order_YMD = 2
} Ordering_Type;

typedef enum {
        Separator_Blank = 0,
        Separator_Slash = 1,
        Separator_Dot = 2,
        Separator_Dash = 3
} Separator_Type;
