/*      @(#)listbox.h 1.8 IEI SMI
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


typedef	int	(*PFI)();

extern void list_clear_rgn (/* Panel_item lp; short low, high */);
extern void list_select_all (/* Panel_item lp;  */);
extern void list_deselect_all (/* Panel_item lp;  */);
extern void list_entry_select_string (/* Panel_item lp;  char *name; */);
extern void list_entry_select_n (/* Panel_item lp;  int n; */);
extern int list_dup (/* Panel_item lp;  char *text; */);
extern int list_add_entry (/* Panel_item lp; char *text; Pixrect *picture;
				int client_data; int position; int duplicate; */);
extern void list_delete_entry_n (/* Panel_item lp; int n */);
extern int list_delete_entry (/* Panel_item lp; char *string */);
extern void list_flush (/* Panel_item lp; */);
extern void list_cd_flush (/* Panel_item lp; */);
extern char *list_get_entry (/* Panel_item lp; int n */);
extern int list_item_selected (/* Panel_item lp; PFI selection_report_rtn */);
extern void list_delete_selected (/* Panel_item lp; int low, high; */);
extern int list_item_selected (/* Panel_item lp; int n; */);
extern int list_num_selected (/* Panel_item lp; */);
extern int list_client_data (/* Panel_item  lp; int n; */);
extern int list_is_selected (/* Panel_item  lp; int n; */);
extern int list_in_list (/* Panel_item  lp; char *name; int *row; */);
extern void list_copy_list (/* Panel_item  src; Panel_item dest; */);

