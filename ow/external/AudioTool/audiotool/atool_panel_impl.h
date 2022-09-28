/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_PANEL_IMPL_H
#define	_AUDIOTOOL_ATOOL_PANEL_IMPL_H

#ident	"@(#)atool_panel_impl.h	1.77	93/03/04 SMI"

#define AudPanel_List(p)		p->lp
#define AudPanel_Pts(p)			p->dp

#include <sys/param.h>
#include <netdb.h>
#include <xview/xview.h>	/* for XView types, etc. */

#ifndef PRE_493
#include "dstt.h"
#endif

#include "atool_types.h"
#include "atool_panel.h"
#include "loadsave/loadsave_panel.h"
#include "undolist_c.h"

/* default temp dir */
#define DEF_TEMP_DIR		"/tmp"

/* some default settings for config options */
#define DEF_AUTO_PLAY_SELECT		FALSE
#define DEF_AUTO_PLAY_LOAD		TRUE
#define DEF_HASHON			TRUE
#define DEF_GRAPH_SCALE			30.0

#define NORMAL_SPEED			1.0
#define FASTFWD_SPEED			3.0
#define FASTREV_SPEED			-3.0

/* call backs from sub panels, and sub canvas */
extern int	Open_proc(ptr_t, char*);
extern int	Save_proc(ptr_t, char*, int, Audio_hdr*);
extern int	Include_proc(ptr_t, char*);
extern int	File_filter_proc(ptr_t, char*, int);
extern		Set_insert_proc(ptr_t);
extern int	Select_proc(ptr_t);
extern int	Rubberband_select_proc(ptr_t);
extern int	Pointer_proc(ptr_t);
extern int	Cursor_proc(ptr_t);

/* call backs for config sub panel (some of this may disappear) */
extern void 	Config_autoplay_load_proc(ptr_t, int);
extern void 	Config_autoplay_sel_proc(ptr_t, int);
extern void 	Config_confirm_proc(ptr_t, int);
extern void 	Config_silence_proc(ptr_t, int);
extern void 	Config_threshold_proc(ptr_t, double, double, double, double);
extern void 	Config_tempdir_proc(ptr_t, char*);
extern void	Config_apply_proc(ptr_t);

extern int	Audpanel_Modify(ptr_t);
extern int	Audpanel_Unmodify(ptr_t);

/* state of audio tool */
typedef enum {
	Unloaded,		/* tool does not have a file loaded */
	Stopped,		/* file is loaded but not active */
	Playing,		/* file is being played */
	Recording,		/* file is being recorded */
	Loading,		/* file is being loaded */
	Saving,			/* file is being saved */
} Atool_state;

/* what type of ToolTalk interface we're using for this connection */
typedef enum {
	TT_None = 0,
	TT_V3compat,
	TT_MediaExchange,
	TT_Both,
} Atool_TtType;

struct atool_subpanel_data {
	ptr_t 		device;		/* audio device panel handle */
	ptr_t		segment;	/* segment display, UNUSED */
	ptr_t		file;		/* file panel handle */
	ptr_t		config;		/* tool props */
	ptr_t		format;		/* audio format settings panel */
#ifdef PRE_493
	ptr_t		playctl;	/* play gain panel handle */
	ptr_t		reclevel;	/* record gain panel handle */
#endif
};

struct atool_panel_data {
	/* window handles */
	ptr_t				panel;	/* handle of main panel */
	ptr_t				owner;	/* owner of main panel */
	struct atool_subpanel_data	subp;	/* handle to sub panels */

	/* button images */
	ptr_t		play_button_image;
	ptr_t		stop_play_button_image;
	ptr_t		record_button_image;
	ptr_t		stop_record_button_image;

	/* interesting menu handles */
	ptr_t		edit_menu;
	ptr_t		file_menu;
	ptr_t		format_menu;

	/* layout state info */
	int		frame_height;
	int		frame_width;
	int		lower_panel_width;
	int		lower_panel_y;
	int		right_panel_x;

	/* general info */
	char		cpath[MAXPATHLEN+1];	/* current path */
	char		cfile[MAXPATHLEN+1];	/* file name */
	char            *recpath;    		/* temp record file name */
	int		modified;		/* file was modified */

	/* for DnD/ToolTalk */
	char		link_app[MAXPATHLEN+1];	/* name of link app */
	int		compose;		/* in compose mode? */
#ifndef	PRE_493
	Tt_message	link_m;			/* tooltalk message */
	char		*link_msg;		/* message id */
	Data_t		link_data;		/* message data type */
#endif

	/* Audio handles */
	Audio_Object	shelf;			/* selection shelf data */
	Undolist	lp;			/* Audio edit list */
	AudioDetectPts	*dp;			/* silent ranges */
	Audio_hdr	save_hdr;		/* file header for saving */
	int		compression; 		/* compression for saving */
	Audio_hdr	rec_hdr;		/* file header for recording */
	Audio_Object	ctldev;			/* audio control device */

	/* selection parameters */
	double          insert;                 /* start of play or record */
	double		start;			/* offset to start in secs */
	double		end;			/* offset to end in secs */
	int		adjust_end;		/* adjusting end of seln */

	/* state */
	Atool_state	tstate;			/* current state */
	Atool_state	laststate; 		/* last state */
	int		EditLock;		/* edits are locked out */
	int		Async;			/* number of SIGIOs blocked */
	int		Save_sigmask;		/* saved signal mask */
	int		pointer_cleared;	/* pointer image cleared */
	int		empty;			/* stopped but no data */
	int		noplay;			/* play is disabled */
	int		tt_type;		/* type of tooltalk conn. */
	int		tt_started;		/* if started by tooltalk */
	double		Play_speed;		/* audio rate 1 = normal */

	/* configurable parameters */
	int		Autoplay_load;		/* play loaded files */
	int		Autoplay_select;	/* play after a selection */
	int		Detect_silence;		/* silence detection? */
	int		Confirm_clear;		/* conirm before clear? */

	/* state during asynchronous load/save */
	char		tmppath[MAXPATHLEN+1];	/* conversion tmpfile */
	char		savepath[MAXPATHLEN+1];	/* full path to save as ... */
	char		prevpath[MAXPATHLEN+1];	/* to restore name on failure */
	Audio_hdr	prevhdr;		/* to restore audio format */
	int		prevmod;		/* to restore modified flag */
	int		loadplay;		/* play when load is done? */
	FilePanel_Type	async_op;		/* what are we doing? */
	ptr_t		outfile; 		/* output audio obj (save) */
	int		convpid; 		/* pid of audioconvert proc */
	char*		busymsg;		/* what are we doing? */
	int		saveback;		/* saveback to linked app? */
	int		donepending;		/* unmap after complete? */

	/* play/record state */
	double		frompos;		/* where to write from/to */
	double		topos;
	long		timer_delay;		/* for play/rec/save timer */

	/* device names */
	char		*devname;		/* ctldev derived from this */

	/* cached I18N translations */
	char		*Cursor_string;		/* Cursor: */
	char		*Pointer_string;	/* Pointer: */
	char		*Time_string;		/* %10.10s */
	char		*Play_string;		/* Play */
	char		*Rec_string;		/* Rec */
	char		*Stop_string;		/* Stop */
	char		*Length_string;		/* Length: %6s              */
	char		*Lengthsel_string;	/* Length: %6s [%6s] */
	char		*Space_string;	/* Space remaining: %2d seconds */
};


/* Define private interfaces */
extern ptr_t	AudPanel_INIT(ptr_t owner, ptr_t ap, int *argcp, char **argv);
extern void	AudPanel_SETLABEL(ptr_t sp, char *str);
extern void	AudPanel_SHOW(ptr_t sp);
extern void	AudPanel_UNSHOW(ptr_t sp);

extern void	AudPanel_SETFILEMENU(ptr_t sp, int can_save, int can_saveas,
				     int can_load);
extern void	AudPanel_SETEDITMENU(ptr_t sp, int hassel, int can_paste, 
				     int can_undo, int can_redo);

extern void	AudPanel_CREATE_DONE_BUTTON(ptr_t sp, char *file_type);
extern void	AudPanel_DESTROY_DONE_BUTTON(ptr_t sp);
extern void	AudPanel_RESET_DONE_BUTTON(ptr_t sp);
extern void	AudPanel_INACTIVE_DONE_BUTTON(ptr_t sp);

extern int	AudPanel_ISICON(ptr_t sp);
extern void	AudPanel_CURSORLABEL(struct atool_panel_data*, char*);
extern void	AudPanel_CURSORTIME(struct atool_panel_data*, char*);
extern void	AudPanel_POINTERLABEL(struct atool_panel_data*, char*);
extern void	AudPanel_POINTERTIME(struct atool_panel_data*, char*);

extern void	RevButton_held(void*);
extern void	FwdButton_held(void*);
extern void	RevButton_letgo(void*);
extern void	FwdButton_letgo(void*);

/* more of the same */
extern ptr_t	AudPanel_FilePanel_hndl(ptr_t atd);
extern ptr_t	AudPanel_FormatPanel_hndl(ptr_t atd);
extern ptr_t	AudPanel_DataInfoPanel_hndl(ptr_t atd);
extern ptr_t	AudPanel_PropsPanel_hndl(ptr_t atd);
extern ptr_t	AudPanel_ConfigPanel_hndl(ptr_t atd);
extern int	AudPanel_Cleanup(ptr_t atd, int cflag);
extern void	AudPanel_Changestate(ptr_t atd);
extern int	AudPanel_Canplay(ptr_t, Audio_hdr*);
extern void	AudPanel_Sizemsg(ptr_t, double, double);
extern void	AudPanel_Showtypeinfo(ptr_t);
extern void	AudPanel_Namestripe(ptr_t);
extern void	AudPanel_Setlink(ptr_t, char*, int);
extern void	AudPanel_Breaklink(ptr_t);
extern int	Print_detect(ptr_t);

/* editing functions */
extern void AudPanel_Undo(ptr_t atd);
extern void AudPanel_Undoall(ptr_t atd);
extern void AudPanel_Redo(ptr_t atd);
extern void AudPanel_Redoall(ptr_t atd);
extern void AudPanel_Copy(ptr_t atd);
extern void AudPanel_Deletesilentends(ptr_t atd);
extern void AudPanel_Deleteallsilence(ptr_t atd);
extern void AudPanel_Deleteunselected(ptr_t atd);
extern void AudPanel_Finishsaveback(ptr_t atd, int cflag);
extern void AudPanel_Cut(ptr_t atd);
extern void AudPanel_Paste(ptr_t atd);
extern void AudPanel_Again(ptr_t atd);
extern void AudPanel_Clearselect(ptr_t atd);
extern void AudPanel_Getdevinfo(ptr_t dp);
extern struct atool_panel_data *AudPanel_KEYDATA(ptr_t obj);

extern int	update_pointer_time(ptr_t, double);
extern int	update_cursor_time(ptr_t, double);
extern int	enter_audio_canvas(ptr_t);
extern int	exit_audio_canvas(ptr_t);
extern int	audio_play(ptr_t atd, double pos, double speed);
extern char*	audiotool_release();
extern void	sigio_block(struct atool_panel_data *ap);
extern void	sigio_unblock(struct atool_panel_data *ap);

#endif /* !_AUDIOTOOL_ATOOL_PANEL_IMPL_H */
