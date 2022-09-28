#define POINT_IN_RECT(event, rect) ((event_x(event) > rect->r_left) && \
				   (event_x(event) < (rect->r_left + rect->r_width)) && \
				   (event_y(event) > rect->r_top) && \
				   (event_y(event) < (rect->r_top + rect->r_height)))

#define Min(a,b) (a > b ? b : a)
#define Max(a,b) (a > b ? a : b)

/* Temp buffer size for managing INCR transfers thru 
   the selection service */

#define MGET(s)   (char *)gettext(s)
#define MSGFILE   "SUNW_DESKSET_TEXTEDIT"

#define XFER_BUFSIZ	5000

#define TARGET_RESPONSE 	0x1
#define HOST_RESPONSE 		0x2
#define TRANSPORT_RESPONSE 	0x4
#define FILE_NAME_RESPONSE 	0x8
#define DATA_LABEL_RESPONSE 	0x10
#define COUNT_RESPONSE	 	0x20
#define LOAD_RESPONSE 		0x40
#define BYTES_REQUESTED		0x80


#define RESPONSE_LEVEL_1        0x3f
#define RESPONSE_LEVEL_2	0x7f


typedef struct {
		char	*data;
		int	alloc_size;
		int	bytes_used;
		} CHAR_BUF;

/* context structure for describing the current 
   drag/drop action */

typedef struct {
		Atom		*target_list;
		int		num_targets;
		Atom		*transport_list;
		int		num_transports;
		char		*data_label;
		int		num_objects;
		char		*source_host;
		char		*source_name;
		char		*source_filename;
		Atom		chosen_target;
		Atom		chosen_transport;
		int		transfer_state;
		int		processing_stream;
		int		state_mask;
		int		stop_hit;
		int		dnd_stopped;
		CHAR_BUF	*transfer_data;
		} DND_CONTEXT;


extern Atom		current_type;
extern char		Hostname[MAXHOSTNAMELEN];
extern DND_CONTEXT	*dnd_context;
extern Atom		current_link_atom;

extern Atom text_atom;
extern Atom incr_atom;
extern Atom targets_atom;
extern Atom length_atom;
extern Atom host_atom;
extern Atom file_name_atom;
extern Atom atm_file_name_atom;
extern Atom delete_atom;
extern Atom dragdrop_done_atom;
extern Atom data_label_atom;
extern Atom load_atom;
extern Atom alternate_transport_atom;
extern Atom available_types_atom;
extern Atom enumeration_count_atom;
extern Atom insert_selection_atom;
extern Atom null_atom;
extern char xfer_data_buf[XFER_BUFSIZ];


extern Xv_drop_site    		Drop_site;
extern Selection_requestor	Sel;
extern Dnd			Dnd_object;
extern Xv_Server		My_server;

extern int		debug_on;
extern int		read_only;
extern int		exiting;
extern int		discarded;
extern int		edited;
extern int		textedit_key_data;
extern Frame		base_frame;
extern Textsw		textsw;
extern Panel		panel;
extern char		current_directory[];
extern char		current_filename[];
extern Panel_item	source_panel_item;
extern Server_image	source_drag_ptr_image;
extern Server_image	source_drop_ptr_image;

extern int		TexteditSelectionConvert();
extern void		text_insert();
