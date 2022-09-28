/*
 * "@(#)dstt.h 1.6 96/02/20"
 * Copyright (c) 1993 Sun Microsystems, Inc.
 */

#ifndef __DSTT_H__
#define __DSTT_H__
#include <xview/xview.h>
#include <desktop/tt_c.h>

typedef	char	*media_t;

typedef	enum
{
	FAIL,
	REJECT,
	HOLD,
	OLD,
	OK
} status_t;

typedef	enum
{
	false = 0,
	true = 1
} dstt_bool_t;

typedef	enum
{
	buffer,
	view
} bufftype_t;

typedef	enum
{
	path,
	contents,
	x_selection
} Data_t;

typedef	enum
{
	dstt_status_not_valid,
	dstt_status_req_rec,
	dstt_status_file_not_avail,
	dstt_status_invalid_message,
	dstt_status_user_request_cancel,
	dstt_status_quit_rec,
	dstt_status_discontinue,
	dstt_status_data_wrong_size,
	dstt_status_data_wrong_format,
	dstt_status_data_not_avail,
	dstt_status_unmodified,
	dstt_status_protocol_error,
	dstt_status_service_not_found,
	dstt_status_process_died
} dstt_status_t;

#define	STARTED			"Started"
#define	STOPPED			"Stopped"
#define	CREATED			"Created"
#define	DELETED			"Deleted"
#define	MODIFIED		"Modified"
#define	SAVED		"Saved"
#define	STATUS			"Status"
	
#define	GET_STATUS		"Get_Status"
#define	SET_ENVIRONMENT		"Set_Environment"
#define	GET_ENVIRONMENT		"Get_Environment"
#define	SET_GEOMETRY		"Set_Geometry"
#define	GET_GEOMETRY		"Get_Geometry"
#define	SET_ICONIFIED		"Set_Iconified"
#define	GET_ICONIFIED		"Get_Iconified"
#define	SET_MAPPED		"Set_Mapped"
#define	GET_MAPPED		"Get_Mapped"
#define	SET_XINFO		"Set_XInfo"
#define	GET_XINFO		"Get_XInfo"
#define	SET_LOCALE		"Set_Locale"
#define	GET_LOCALE		"Get_Locale"
#define	RAISE			"Raise"
#define	LOWER			"Lower"
#define	DO_COMMAND		"Do_Command"
#define	QUIT			"Quit" 		
#define	SIGNAL			"Signal" 		
#define SET_SITUATION		"Set_Situation"
#define GET_SITUATION           "Get_Situation" 

#define	DISPLAY			"Display"
#define	EDIT			"Edit"
#define	TRANSLATE		"Translate"
#define	ABSTRACT		"Abstract"
#define	DEPOSIT			"Deposit"
#define	PRINT			"Print"
#define	BUFF_OPEN		"Open"

#define	PROP_EDIT		"Prop_Edit"

#define	OPEN			"Open"
#define	PASTE			"Paste"
#define	CLOSE			"Close"

#define	CLOSE_BUFFER		"Close_Buffer"
#define	PASTE_BUFFER		"Paste_Buffer"
#define	CUT_BUFFER		"Cut_Buffer"
#define	COPY_BUFFER		"Copy_Buffer"
#define	VIEW_BUFFER		"View_Buffer"
#define	CLOSE_VIEW		"Close_View"
#define	SEEK_BUFFER		"Seek_Buffer"
#define	ANNOTATE_BUFFER		"Annotate_Buffer"
#define	SET_ANNOTATION		"Set_Annotation"
#define	CLEAR_ANNOTATION	"Clear_Annotation"

#ifdef __cplusplus
extern "C" {
#endif

/* handlers */
extern	Tt_status dstt_check_startup(void (*)(char**,char **,char**),
		int *, char ***);
extern	int dstt_xview_start_notifier();

extern	int dstt_xview_desktop_callback(Xv_opaque, ...);
extern	int dstt_xview_desktop_register(Xv_opaque, ...);

extern	int dstt_notice_callback(char *, ...);
extern	int dstt_notice_register(char *, ...);

extern	int dstt_prop_callback(char *, ...);
extern	int dstt_prop_register(char *, ...);

extern	int dstt_editor_callback(char *, ...);
extern	int dstt_editor_register(char *, ...);

extern	char	*dstt_messageid();

/* notices */
extern int	dstt_created(char *type, char *path,.../* filenames */);
extern int	dstt_deleted(char *, char *,...);
extern int	dstt_opened(char *, char *);
extern int	dstt_closed(char *, char *);
extern int	dstt_saved(char *, char *);
extern int	dstt_modified(char *, char *);
extern int	dstt_reverted(char *, char *);
extern int	dstt_moved(char *, char *, char *);
extern int	dstt_started(void);
extern int	dstt_stopped(void);
extern int	dstt_iconified(char *, char *);
extern int	dstt_deiconified(char *, char *);
extern int	dstt_mapped(char *, char *);
extern int	dstt_unmapped(char *, char *);
extern int	dstt_raised(char *, char *);
extern int	dstt_lowered(char *, char *);
extern int	dstt_status(char *, char *, char *, char *);

/* request calbacks */
typedef	int	Get_Status_CB(Tt_message, void *, int,
		char *, char *, char *, char *, char *);
typedef	int	Environment_CB(Tt_message, void *, int,
		char *, char *);
typedef int	Geometry_CB(Tt_message, void *, int,
		int, int, int, int);
typedef int	Iconified_CB(Tt_message, void *, int,
		int, char *, char *);
typedef int	Locale_CB(Tt_message, void *, int,
		char **, char **);
typedef int	Mapped_CB(Tt_message, void *, int,
		int, char *, char *);
typedef int	XInfo_CB(Tt_message, void *, int,
		char *, char *, int, char **, char **, char *);
typedef int	Raise_Lower_CB(Tt_message, void *, int,
		char *, char *);
typedef int	Do_Command_CB(Tt_message, void *, int,
		char *, char *, char *);
typedef int	Quit_CB(Tt_message, void *, int,
		int, int, char *);
typedef int	Signal_CB(Tt_message, void *, int,
		int);
typedef int	Situation_CB(Tt_message, void *, int,
		char *);

typedef int	Prop_Edit_CB(Tt_message, void *, int,
		char *, void *, int,
		void *, int,
		void *, int,
		char *, char *);

typedef int	Display_CB(Tt_message m, void *key, int status, char *media,
			Data_t type, void *data, int size,
			char *msgID, char *title);
typedef int	Display_Handle(Tt_message m, char *media,
			Data_t type, void *data, int size,
			char *msgID, char *title);

typedef int	Edit_CB(Tt_message m, void *key, int status, char *media,
			Data_t type, void *data, int size,
			char *msgID, char *title);
typedef int	Edit_Handle(Tt_message m, char *media,
			Data_t type, void *data, int size,
			char *msgID, char *title);

typedef int	Deposit_CB(Tt_message, void *, int, char *,
		Data_t, void *, int, char *, char *);
typedef int	Print_CB(Tt_message, void *, int, char *,
		Data_t, void *, int, int, int, char *, char *);


/* requsts */
extern	int	dstt_get_status(Get_Status_CB *, void *, char *,
		char *);
extern	int	dstt_set_environment(Environment_CB *, void *, char *,
		char **, char **);
extern	int	dstt_get_environment(Environment_CB *, void *, char *,
		char **);
extern	int	dstt_set_geometry(Geometry_CB *, void *, char *,
		int, int, int, int, char *, char *);
extern	int	dstt_get_geometry(Geometry_CB *, void *, char *,
		char *,char *);
extern	int	dstt_set_iconified(Iconified_CB *, void *, char *,
		int , char *, char *);
extern	int	dstt_get_iconified(Iconified_CB *, void *, char *,
		char *, char *);
extern	int	dstt_set_locale(Locale_CB *, void *, char *,
		char *, char *);
extern	int	dstt_get_locale(Locale_CB *, void *, char *,
		char **);
extern	int	dstt_set_mapped(Mapped_CB *, void *, char *,
		int , char *, char *);
extern	int	dstt_get_mapped(Mapped_CB *, void *, char *,
		char *, char *);
extern	int	dstt_set_xinfo(XInfo_CB *, void *, char *,
		char *, char *, int, char **, char **, char *);
extern	int	dstt_get_xinfo(XInfo_CB *, void *, char *,
		char **, char *);
extern	int	dstt_raise(Raise_Lower_CB *, void *, char *,
		char *, char *);
extern	int	dstt_lower(Raise_Lower_CB *, void *, char *,
		char *, char *);
extern	int	dstt_do_command(Do_Command_CB *, void *, char *,
		char *, char *);
extern	int	dstt_quit(Quit_CB *, void *, char *,
		int, int, char *);
extern	int	dstt_signal(Signal_CB *, void *, char *,
		int);
extern	int	dstt_set_situation(Situation_CB *, void *, char *,
		char *);
extern	int	dstt_get_situation(Situation_CB *, void *, char *);

extern	int	dstt_prop_edit(Prop_Edit_CB *, void *,
		char *,	void *, int,
		void *, int,
		void *, int,
		char *, char *);

extern	int	dstt_display(Display_CB *, void *, char *,
		Data_t, void *, int, char *, char *);
extern	int	dstt_edit(Edit_CB *, void *, char *,
		Data_t, void *, int, char *, char *);
extern	int	dstt_print(Print_CB *, void *, char *,
		Data_t, void *, int, int, int, char *, char *);

/* OPEN */
typedef int	Open_CB(
	Tt_message	m,
	void		*key,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	bufftype_t	bufftype,
	char		*ID,
	dstt_bool_t	readonly,
	dstt_bool_t	mapped,
	int		shareLevel,
	char		*locator);

extern	int	dstt_open(
	Open_CB		*cb,
	void		*key,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	bufftype_t	bufftype,
	char		*ID,
	dstt_bool_t	readonly,
	int		shareLevel,
	dstt_bool_t	mapped,
	char		*locator);
typedef int	Open_Handle(
	Tt_message	m,
	void		*key,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	bufftype_t	bufftype,
	char		**ID,
	dstt_bool_t	readonly,
	dstt_bool_t	mapped,
	int		shareLevel,
	char		*locator);
extern void	dstt_handle_open(
	Open_Handle	*cb,
	void		*key,
	char		*media,
	Data_t		type,
	bufftype_t	bufftype,
	dstt_bool_t	readonly);

/* PASTE */
typedef int	Paste_CB(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	int		offset,
	char		*locator);

extern	int	dstt_paste(
	Paste_CB	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	int		offset,
	char		*locator);
typedef int	Paste_Handle(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	int		offset,
	char		*locator);
extern void	dstt_handle_paste(
	Paste_Handle	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	int		offset);

/* CLOSE */
typedef int	Close_CB(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	int		inquisitive,
	int		force);

extern	int	dstt_close(
	Close_CB	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	int		inquisitive,
	int		force);

typedef int	Close_Handle(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	int		inquisitive,
	int		force);

extern void	dstt_handle_close(
	Close_Handle	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID);


#ifdef __cplusplus
};
#endif

#endif __DSTT_H__
