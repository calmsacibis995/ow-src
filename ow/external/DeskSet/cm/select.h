/* static char sccsid[] = "@(#)select.h 3.6 92/12/18 Copyr 1988 Sun Micro";
 *
 * select.h
 *
 */

typedef enum {
daySelect, weekSelect, monthSelect, hourSelect, weekdaySelect, weekhotboxSelect 
} Selection_unit;

typedef struct {
	int row;
	int col;
	int nunits;
	int active;
	int boxw;
	int boxh;
} Selection;

extern void calendar_select();
extern void paint_selection();

extern void calendar_deselect (/* Calendar *c; */);

extern void activate_selection (/* Selection *s */);

extern void deactivate_selection (/* Selection *s */);

extern int selection_active (/* Selection *s */);	/* query */

extern void browser_select();	
extern void browser_deselect();	
extern void monthbox_deselect();	
extern void monthbox_select();	
extern void weekchart_select();	
