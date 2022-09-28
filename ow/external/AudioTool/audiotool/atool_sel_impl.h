/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_SEL_IMPL_H
#define	_AUDIOTOOL_ATOOL_SEL_IMPL_H

#ident	"@(#)atool_sel_impl.h	1.26	93/03/03 SMI"

#ifndef OWV2
#include <xview/dragdrop.h>

#include <stdio.h>
#include <netdb.h>
#include <sys/param.h>

#include "atool_types.h"
#include "atool_debug.h"
#include "atool_i18n.h"
#include "atool_sel.h"

/* long ulgy macro to free "to" if set and copy "from", which is
 * not null terminated
 */
#define COPY_STRING(to, from, len) if (to) {free(to);to=NULL;} \
    if (to=(char*)calloc(1,len+1)) { \
	memcpy(to, (char*)from, len); to[len] = NULL; \
    }

/* copy an atom list */
#define COPY_ATOM_LIST(to, from, len) if (to) {free(to);to=NULL;} \
    if (to=(Atom*)calloc(1,len*sizeof(Atom))) { \
	memcpy((char*)to, (char*)from, len*sizeof(Atom)); \
    }

/* determine if the point in the event struct is in the given rect */
#define POINT_IN_RECT(event, rect) ((event_x(event) > rect->r_left) && \
                    (event_x(event) < (rect->r_left + rect->r_width)) && \
                    (event_y(event) > rect->r_top) && \
                    (event_y(event) < (rect->r_top + rect->r_height)))

/* get audio seln data ptr from xview object */
#define GET_ASDP(asdp, obj, fcn) \
	asdp = (AudioSelData*) xv_get(obj, XV_KEY_DATA, AUD_SEL_DATA_KEY); \
	if (asdp == NULL) { \
	    DBGOUT((1, "%s: can't get audio seln data.\n", fcn)); \
	    return; \
	}

/* used to reference items in our atom table (abbreviated) */
typedef enum {
	A_NONE = 0,		/* no atom */
	A_SUN_SEL_END,		A_SUN_SEL_ERROR,A_LENGTH,
	A_DELETE,		A_TARGETS,	A_MULTIPLE,
	A_TIMESTAMP,		A_FILE_NAME,	A_NAME,
	A_HOST_NAME,		A_INCR,		A_INSERT_SELN,
	A_TEXT,			A_STRING,	A_INTEGER,
	A_SUN_DND_BEGIN,	A_SUN_DND_DONE,	A_SUN_DND_ERROR,
	A_SUN_AVAIL_TYPES,	A_SUN_ATMS,	A_SUN_ATM_FNAME,
	A_SUN_LOAD,		A_SUN_DATA_LABEL, A_SUN_FILE_HNAME,
	A_SUN_LENGTH_TYPE,	A_SUN_ENUM_COUNT, A_SUN_ENUM_ITEM,
	A_SUN_TYPE_AUDIO,	A_SUN_ATM_TCP_SOCKET, A_SUN_COMPOSE_TYPE,
	A_AUDIO_PRIMARY_SEL,	A_CLIPBOARD,	A_ATOM_PAIR,
}  AtomIndex;

typedef struct _atom_list_entry {
	AtomIndex	index;
	char *		name;
	Atom		atom;
	int		(*cvt_proc)(); /* sel cvt proc for this atom */
	void		(*repl_proc)(); /* sel reply proc for this atom */
} AtomListEntry;

/* for TCP x-fer */
#define AUDIO_PORT		2112		/* port # to start with... */
#define BINDCOUNT		20 		/* max trys to bind TCP port */

#define CANVAS_SITE		415		/* drop site id (arbitrary) */
#define PANEL_SITE		416

#define DEF_INCR_DATA_SIZE	(64*1024)	/* def size of INCR xfer */

#define AUDIO_SELN_TMPL		"_SUN_AUDIO_SELN_%d"
#define DEF_SELN_RANK_NAME	"DEFAULT_SELN_RANK"
#define DEF_DATA_LABEL		"audio-file.au"

#define SELN_RESET_ITEM (-1)	/* enum item for flagging a reset */

/* context for doing a LOAD/INSERT_SELECTION */

struct sel_load_context {
	Atom		load_atom; 	/* load atom (rank) for insert */
	Atom		insert_atom;	/* insert atom (property of insert) */
	SelXferType	xfertype; 	/* how we obtained the load */
	int		compose;	/* compose load? */
	Atom		type;		/* type of data to send back */
	int		done;		/* break link after insert? */
};

/* audio selection data (all info needed for selecion x-fer) */

typedef struct _audio_sel_data {
	AtomListEntry	*atoms;		/* the atom list table */

	ptr_t		ap;		/* audio panel data */

	Xv_server	server;		/* handle to server */
	Display		*dpy;		/* handle to display */

	Xv_opaque	canvas,
			dropsite;	/* XView obj's we need to deal with */

	Server_image	drag_image,	/* icon image handles for dropsite */
			busy_image;

	SelXferType	xfertype;	/* type of xfer we're doing */

	Selection_owner audio_primary,	/* primary audio selection */
			clipboard,	/* the clipboard */
			insert_sel;	/* the load selection */

	Selection_requestor sel_req;	/* selection requestor object */

	Dnd		dnd_item;	/* drag 'n drop item */

	Xv_drop_site	canvas_site,
			panel_site;	/* drop sites (bullseye!) */

	/* XXX - should pull this stuff out ... */
	int		req_fd,
			holder_fd;	/* socket handles for TCP xfer */
	unsigned long	req_addr,
			holder_addr;	/* IP addr to send/recv to/from */
	unsigned short	req_port,
			holder_port; 	/* port number for TCP xfer */

	Atom		*supported_types; /* types we will source */
	Atom		*supported_atms; /* ATM's we can use */
	Atom		*supported_targets; /* targets we can cvt */
	Atom		*holder_types;	/* types sel holder will source */
	Atom		*holder_atms;	/* atms sel holder can use */
	Atom		*holder_targets; /* atms sel holder can cvt */

	Atom		load_atom; /* atom to use for sending back a load (insert) */
	Atom		insert_atom; /* selection to use for INSERT */

	int		num_sup_types;	/* # of all those above  */
	int		num_sup_atms;
	int		num_sup_targets;

	int		num_holder_types; /* # of all those above  */
	int		num_holder_atms;
	int		num_holder_targets;

	unsigned int	num_items;	/* number of items holder's got */
	int		cur_item;	/* current enum item */
	int		last_item;	/* last enum item we req'd */

	Atom		atm;		/* atm being used for this xfer */
	Atom		type;		/* type being used for this xfer */

	Audio_hdr	*audio_filehdr;	/* file header of audio being xfer'd */
	Audio_Object	audio_data;	/* actual audio data being xfer'd */
	unsigned long	sel_length;	/* byte length of selection */
	unsigned long	sel_position;	/* byte offset within selection */
	int		incr_size;	/* size of incr xfer */
	unsigned char	*xfer_buf;	/* buffer for sel xfer */

	Audio_Object	*audio_buffer_list; /* list of x'ferd buffers */
	int		num_bufs;	/* # of buffers to insert */

	char		*hostname; 	/* for file x-port */
	char		*filename;
	char		*data_label;
	char		*app_name;	/* name of DnD client */
	char		*compose_type;	/* ce type for compose ... */

	char		this_host[MAXHOSTNAMELEN+1];	/* our host name */

	struct sel_load_context *load_context;	/* for load/insert seln */

	struct {
	  unsigned int	is_forwarded : 1;	/* forwarded DnD from icon? */
	  unsigned int	is_insert : 1;		/* doing INSERT_SELECTION */
	  unsigned int	is_incr : 1;		/* doing INCR xfer  */
	  unsigned int  is_busy : 1; 		/* busy doing DnD? */
	  unsigned int	is_local : 1;		/* try to DnD to self? */
	  unsigned int	sel_started : 1;	/* selection request started */
	  unsigned int	xfer_started : 1;	/* data transfer started */
	  unsigned int	req_cancelled : 1;	/* flag if req cancelled */
	  unsigned int	xfer_success : 1;	/* set if xfer was a success */
        } flags;
} AudioSelData;

extern Attr_attribute AUD_SEL_DATA_KEY; /* global key for audio seln data */

/* for doing atom lookup's, etc. */

extern Atom atom_get_atom(AtomListEntry*, AtomIndex);
extern char *atom_get_name(AtomListEntry*, AtomIndex);
extern is_atom_in_list(Atom*, Atom);
extern AtomListEntry *atom_lookup_atom(AtomListEntry*, Atom);
extern AtomListEntry *atom_lookup_name(AtomListEntry*, char*);
extern init_atoms(Xv_server, AtomListEntry *);

extern void audio_sel_done_proc(Selection_owner,Xv_opaque, Atom);
extern void audio_sel_lose_proc(Selection_owner);
extern int audio_sel_convert_proc(Selection_owner, Atom*, Xv_opaque*,
	long int*, int *);
extern void audio_sel_reply_proc(Selection_requestor, Atom, Atom, Xv_opaque,
	long unsigned int, int);
extern void sel_clear_data(AudioSelData*);

extern void sel_free_buffer(AudioSelData*);
extern void sel_free_buffer_list(AudioSelData*);
extern void sel_cancel_req(AudioSelData*, char*);
extern void sel_cancel_xfer(AudioSelData*);
extern void sel_xfer_finished(AudioSelData*);
extern void sel_request_finished(AudioSelData*);
extern void init_selection_request(AudioSelData*, SelXferType, char*, int);
extern void sel_process_next_request(AudioSelData*);
extern int sel_load_file(AudioSelData*);
extern int get_audio_selection_data(AudioSelData*, int, int, Audio_Object);

extern Notify_value panel_dnd_event_proc(Xv_opaque, Xv_opaque, Event*);

extern void Audio_SetDropsiteFull(Xv_opaque, int);
extern void reset_drop_target(Xv_opaque);
extern void set_dropsite_busy(AudioSelData*, int);
extern int is_dropsite_busy(AudioSelData*);
extern Atom conjure_insert(AudioSelData*);
extern Atom conjure_transient_selection(AudioSelData *);

extern int audio_sel_write_data(AudioSelData*, Atom*, Xv_opaque*, 
	long int*, int*);

extern void audio_sel_read_data(AudioSelData*, Atom, Xv_opaque,
	long unsigned int, int);

extern void do_insert_buffer_list(AudioSelData*);
extern void finish_insert_data(AudioSelData*);
extern void start_request_for_data(AudioSelData*);

extern void cancel_insert_seln(AudioSelData*, char*);
extern void discard_load(Xv_opaque);
extern void sel_create_load_context(AudioSelData *asdp);
extern void sel_destroy_load_context(AudioSelData *asdp);

/* convert proc's */

extern int sel_cvt_atms(AudioSelData*, Selection_owner, Atom*, Xv_opaque*,
			long int*, int*);
extern int sel_cvt_delete(AudioSelData*, Selection_owner, Atom*, Xv_opaque*,
			long int*, int*);
extern int sel_cvt_dnd_begin(AudioSelData*, Selection_owner, Atom*, 
			Xv_opaque*, long int*, int*);
extern int sel_cvt_dnd_done(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_dnd_error(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_sel_end(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_sel_error(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_enum_ct(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_enum_item(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_insert(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_label(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_filename(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_hostname(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_load(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_name(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_socket(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_sun_length(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_targets(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_type_audio(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);
extern int sel_cvt_types(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);

/* reply proc's */

extern void sel_repl_multiple(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_atms(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_delete(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_dnd_begin(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_dnd_done(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_dnd_error(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_sel_end(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_sel_error(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_enum_ct(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_enum_item(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_label(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_filename(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_hostname(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_load(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_compose_type(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_insert(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_name(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_socket(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_sun_length(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_targets(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_type_audio(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);
extern void sel_repl_types(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);

#ifdef USE_ATM_TCP		/* unsupported ... */
extern Notify_value audio_tcp_reader();

extern void sel_repl_atm_tcp_socket(AudioSelData*, Selection_requestor, Atom,
			Atom, Xv_opaque, long unsigned int, int);

extern int sel_cvt_atm_tcp_socket(AudioSelData*, Selection_owner, Atom*,
			Xv_opaque*, long int*, int*);

extern int audio_atm_tcp_socket_init(AudioSelData *asdp);
extern int audio_tcp_send_start(long unsigned int addr,
                                short unsigned int port);
extern int audio_tcp_req_start(AudioSelData *asdp);
extern void audio_tcp_req_done(AudioSelData *asdp);
extern Notify_value audio_tcp_accept(AudioSelData *asdp, register int sockfd);
extern Notify_value audio_tcp_read(AudioSelData *asdp, register int sockfd);
#endif /* !USE_ATM_TCP  */

#endif /* OWV2 */

#endif /* !_AUDIOTOOL_ATOOL_SEL_IMPL_H */
