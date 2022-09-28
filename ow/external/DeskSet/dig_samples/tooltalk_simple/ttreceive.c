/*
 * ttreceive - show receiving tooltalk message based on pattern.
 * 
 * This simple example program is the counterpart to ttsend.  It registers
 * a pattern which describes the message it is interested in, and then 
 * waits for them. 
 */

#include <xview/xview.h>
#include <xview/panel.h>
#include <desktop/tt_c.h>

#include "ttdig.h"

Frame base_frame;
Panel_item controls;
Panel_item gauge;

char *my_procid;

void	receive_tt_message();
void	create_ui_components();

void
main(argc, argv)
int argc;
char **argv;
{
  int ttfd;
  Tt_pattern pat;

  /* Initialize XView. */
  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 0);
  create_ui_components();

  /* Initialize ToolTalk and obtain file descriptor for incoming messages. */
  my_procid = tt_open();
  ttfd = tt_fd();

  /* Tell XView to call my receive procedure when there are messages. */
  notify_set_input_func(base_frame, 
			(Notify_func)receive_tt_message, ttfd);
  /*		       
   * Create and register the pattern we are interested in.  We are
   * registering as an observer; all observers will receive a message
   * destined for them (try a few ttreceives).  If we had registered
   * as a TT_HANDLE, we would be the only one to get the message. 
   */
  pat = tt_pattern_create();
  tt_pattern_category_set(pat, TT_OBSERVE);
  tt_pattern_scope_add(pat, TT_SESSION);
  tt_pattern_op_add(pat, RECEIVE_PATTERN);
  tt_pattern_register(pat);

  /* Join the default session to get messages. */
  tt_session_join(tt_default_session());
  xv_main_loop(base_frame);

  /* Clean up ToolTalk on exit. */
  tt_close();
  exit(0);
}

/*
 * receive_tt_message is the procedure that gets called by the XView
 * notifier when my tooltalk file descripter becomes active with a message.
 */
void
receive_tt_message()
{
  Tt_message msg_in;
  int mark;
  int val_in;

  /* 
   * Pull in my message handle.  If it is null, we became active even
   * though there wasn't a real message for us.
   */
  msg_in = tt_message_receive();
  if (msg_in == NULL) return;

  /*
   * Get a storage mark so we can free storage that tt obtains for
   * our message contents.
   */
  mark = tt_mark();

  /* If the message pattern matches our interest, fetch the value.  */
  if (0==strcmp(RECEIVE_PATTERN, tt_message_op(msg_in))) {
    tt_message_arg_ival(msg_in, 0, &val_in);
    xv_set(gauge, PANEL_VALUE, val_in, NULL);
  }

  tt_message_destroy(msg_in);
  tt_release(mark);
  return;
}
	    
/*
 * create_ui_components is the procedure called to set up the panel.
 */
void
create_ui_components()
{
  base_frame = xv_create(NULL, FRAME,
			 XV_LABEL, "TT Receiver Example",
			 FRAME_SHOW_RESIZE_CORNER, FALSE,
			 NULL);
  controls = xv_create(base_frame, PANEL,
		       WIN_BORDER, FALSE,
		       NULL);
  gauge = xv_create(controls, PANEL_GAUGE,
		    PANEL_LABEL_STRING, "Received:",
		    PANEL_MIN_VALUE, RECEIVE_MIN,
		    PANEL_MAX_VALUE, RECEIVE_MAX,
		    PANEL_SHOW_RANGE, FALSE,
		    NULL);
  window_fit(controls);
  window_fit(base_frame);
}

