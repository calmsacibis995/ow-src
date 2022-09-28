/* @(#)select.h	3.2 - 92/12/03 */

/* select.h -- some random stuff needed to implement
 * the selection service
 */

/* this structure keeps track of a selection */
struct mt_select {
	char *ms_buffer;
	char *ms_ptr;
	int ms_size;
	int ms_haveselection;
};


/* this structure is used to follow a read of a selection across
 * multiple reads
 */
struct mt_context {
	int mc_inuse;
	char *mc_ptr;
	char *mc_end;
	void *mc_headerdata;
};

enum mt_ranks { MR_PRIMARY, MR_SECONDARY, MR_SHELF, MR_SIZE };

extern struct mt_select mt_sel_buffers[MR_SIZE];
