#ifndef	perfmeter_HEADER
#define	perfmeter_HEADER

/*
 * perfmeter_ui.h - User interface object and function declarations.
 * This file was generated by `gxv' from `perfmeter.G'.
 * DO NOT EDIT BY HAND.
 */

#ifndef SVR4
EXTERN_FUNCTION( char *bindtextdomain, (const char *, const char *) );
EXTERN_FUNCTION( char *dgettext, (const char *, const char *) );
#else
#include <locale.h>
#endif

extern Attr_attribute	INSTANCE;

extern Xv_opaque	perfmeter_pm_menu_create(caddr_t, Xv_opaque);

typedef struct {
	Xv_opaque	frame;
	Xv_opaque	canvas;
} perfmeter_frame_objects;

extern perfmeter_frame_objects	*perfmeter_frame_objects_initialize(perfmeter_frame_objects *, Xv_opaque);

extern Xv_opaque	perfmeter_frame_frame_create(perfmeter_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_frame_canvas_create(perfmeter_frame_objects *, Xv_opaque);

typedef struct {
	Xv_opaque	pm_props_frame;
	Xv_opaque	pm_props_panel;
	Xv_opaque	track_type;
	Xv_opaque	dir_type;
	Xv_opaque	disp_type;
	Xv_opaque	machine;
	Xv_opaque	sampletime;
	Xv_opaque	hour_hand;
	Xv_opaque	minute_hand;
	Xv_opaque	apply_but;
	Xv_opaque	pgroup;
	Xv_opaque	graph_type;
	Xv_opaque	machine_name;
	Xv_opaque	stime_secs;
	Xv_opaque	do_log;
	Xv_opaque	htime_secs;
	Xv_opaque	mtime_secs;
	Xv_opaque	log_name;
	Xv_opaque	def_but;
	Xv_opaque	reset_but;
} perfmeter_pm_props_frame_objects;

extern perfmeter_pm_props_frame_objects	*perfmeter_pm_props_frame_objects_initialize(perfmeter_pm_props_frame_objects *, Xv_opaque);

extern Xv_opaque	perfmeter_pm_props_frame_pm_props_frame_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_pm_props_panel_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_track_type_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_dir_type_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_disp_type_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_machine_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_sampletime_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_hour_hand_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_minute_hand_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_apply_but_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_pgroup_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_graph_type_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_machine_name_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_stime_secs_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_do_log_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_htime_secs_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_mtime_secs_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_log_name_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_def_but_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
extern Xv_opaque	perfmeter_pm_props_frame_reset_but_create(perfmeter_pm_props_frame_objects *, Xv_opaque);
#endif
