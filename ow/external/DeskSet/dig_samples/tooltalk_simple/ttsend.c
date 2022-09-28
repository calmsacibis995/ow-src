/*
 * ttsend - Demonstrate sending a message with a particular pattern.
 * 
 * This simple program is the counterpart to ttreceive.  It sends
 * a message with a particular pattern that all receivers that are
 * listening will receive.
 *
 */

#include <xview/xview.h>
#include <xview/panel.h>
#include <desktop/tt_c.h>

#include "ttdig.h"

Frame base_frame;
Panel_item controls;
Panel_item slider;

char *my_procid;

void	broadcast_value();
void	create_ui_components();

void
main(argc, argv)
int argc;
char **argv;
{

  /* Initialize XView and Tooltalk; enter XView main loop.  */
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);
  create_ui_components();
  my_procid = tt_open();
  xv_main_loop(base_frame);

  /* Clean up ToolTalk on exit.  */
  tt_close();
  exit(0);
}


/*
 * broadcast_value is the procedure that gets called when you
 * release the slider.  It gets the current slider
 * value and broadcasts it with ToolTalk.
 */
void
broadcast_value(item, value, event)
Panel_item item;
int value;
Event *event;
{
  Tt_message msg_out;

  /* Create and send ToolTalk msg.  */
  msg_out = tt_pnotice_create(TT_SESSION, RECEIVE_PATTERN);
  tt_message_arg_add(msg_out, TT_IN, "integer", NULL);
  tt_message_arg_ival_set(msg_out, 0, value);
  tt_message_send(msg_out);

  /* Destroy the handle since we don't expect a reply.  */
  tt_message_destroy(msg_out);
}

	    
/*
 * create_ui_components is the procedure called to set up the panel.
 */
void
create_ui_components()
{
  base_frame = xv_create(NULL, FRAME,
			 XV_LABEL, "TT Send Example",
			 FRAME_SHOW_RESIZE_CORNER, FALSE,
			 NULL);
  controls = xv_create(base_frame, PANEL,
		       WIN_BORDER, FALSE,
		       NULL);
  slider = xv_create(controls, PANEL_SLIDER,
		     PANEL_LABEL_STRING, "Send:",
		     PANEL_SLIDER_END_BOXES, FALSE,
		     PANEL_SHOW_RANGE, FALSE,
		     PANEL_SHOW_VALUE, FALSE,
		     PANEL_MIN_VALUE, RECEIVE_MIN,
		     PANEL_MAX_VALUE, RECEIVE_MAX,
		     PANEL_TICKS, 0,
		     PANEL_NOTIFY_PROC, broadcast_value,
		     NULL);
  window_fit(controls);
  window_fit(base_frame);
}
