#ifndef SOUNDTOOL_H
#define SOUNDTOOL_H

extern int main (int argc, char **argv, char **envp);
extern int main_play_proc (Panel_item item, Event *event);
extern int main_record_proc (Panel_item item, Event *event);
extern int main_pause_proc (Panel_item item, Event *event);
extern int main_describe_proc (Panel_item item, Event *event);
extern double scale_gain (int g);
extern int unscale_gain (double g);
extern int main_play_volume_proc (Panel_item item, int value, Event *event);
extern int main_record_volume_proc (Panel_item item, int value, Event *event);
extern int control_output_proc (Panel_item item, int value, Event *event);
extern int control_loop_proc (Panel_item item, int value, Event *event);
extern int main_update_panel (int init);
extern int file_load_proc (Panel_item item, Event *event);
extern int file_store_proc (Panel_item item, Event *event);
extern int file_append_proc (Panel_item item, Event *event);
extern Panel_setting file_name_proc (Panel_item item, Event *event);
extern int file_wait_cursor (void);
extern int file_restore_cursor (void);
extern int file_error_handler (char *error_str);
extern int file_menu_proc (Menu menu, Menu_item menu_item);
extern int file_select (struct direct *entry);
extern int alloc_buffer (unsigned int size);
extern int soundfile_path (char *str);
extern Notify_value describe_destroy_proc (Frame *frame);
extern int null_panel_event (Panel_item item, Event *event);
extern int waveform_zoom_proc (Panel_item item, int value, Event *event);
extern int scope_clear (void);
extern int scope_display (int off);
extern void waveform_cursor_proc (Canvas canvas, Event *event, caddr_t arg);
extern int set_item_val (Panel_item item, int val);
extern int audio_close (void);

extern int main (int argc, char **argv, char **envp);
extern int main_play_proc (Panel_item item, Event *event);
extern int main_record_proc (Panel_item item, Event *event);
extern int main_pause_proc (Panel_item item, Event *event);
extern int main_describe_proc (Panel_item item, Event *event);
extern double scale_gain (int g);
extern int unscale_gain (double g);
extern int main_play_volume_proc (Panel_item item, int value, Event *event);
extern int main_record_volume_proc (Panel_item item, int value, Event *event);
extern int control_loop_proc (Panel_item item, int value, Event *event);
extern int main_update_panel (int init);
extern int file_load_proc (Panel_item item, Event *event);
extern int file_store_proc (Panel_item item, Event *event);
extern int file_append_proc (Panel_item item, Event *event);
extern Panel_setting file_name_proc (Panel_item item, Event *event);
extern int file_wait_cursor (void);
extern int file_restore_cursor (void);
extern int file_error_handler (char *error_str);
extern int file_menu_proc (Menu menu, Menu_item menu_item);
extern int file_select (struct direct *entry);
extern int alloc_buffer (unsigned int size);
extern int soundfile_path (char *str);
extern Notify_value describe_destroy_proc (Frame *frame);
extern int null_panel_event (Panel_item item, Event *event);
extern int waveform_zoom_proc (Panel_item item, int value, Event *event);
extern int scope_repaint_proc (Canvas canvas,
                               Pixwin *pw,
                               Rectlist *repaint_area);
extern int scope_clear (void);
extern int scope_display (int off);
extern void waveform_cursor_proc (Canvas canvas, Event *event, caddr_t arg);
extern void waveform_repaint_proc (Canvas canvas,
                                   Pixwin *pw,
                                   Rectlist *repaint_area);
extern Notify_value sigpoll_async_handler (Notify_client client,
                                           int sig,
                                           Notify_signal_mode when);
extern Notify_value sigpoll_sync_handler (Notify_client client,
                                          Notify_event event,
                                          Notify_arg arg,
                                          Notify_event_type when);
extern Notify_value timer_handler (Notify_client client, int which);
extern int set_item_val (Panel_item item, int val);
extern int audio_close (void);

extern int main (int argc, char **argv, char **envp);
extern int main_play_proc (Panel_item item, Event *event);
extern int main_record_proc (Panel_item item, Event *event);
extern int main_pause_proc (Panel_item item, Event *event);
extern int main_describe_proc (Panel_item item, Event *event);
extern double scale_gain (int g);
extern int unscale_gain (double g);
extern int main_play_volume_proc (Panel_item item, int value, Event *event);
extern int main_record_volume_proc (Panel_item item, int value, Event *event);
extern int control_output_proc (Panel_item item, int value, Event *event);
extern int control_loop_proc (Panel_item item, int value, Event *event);
extern int main_update_panel (int init);
extern int file_load_proc (Panel_item item, Event *event);
extern int file_store_proc (Panel_item item, Event *event);
extern int file_append_proc (Panel_item item, Event *event);
extern Panel_setting file_name_proc (Panel_item item, Event *event);
extern int file_wait_cursor (void);
extern int file_restore_cursor (void);
extern int file_error_handler (char *error_str);
extern int file_menu_proc (Menu menu, Menu_item menu_item);
extern int file_select (struct direct *entry);
extern int alloc_buffer (unsigned int size);
extern int soundfile_path (char *str);
extern Notify_value describe_destroy_proc (Frame *frame);
extern int null_panel_event (Panel_item item, Event *event);
extern int waveform_zoom_proc (Panel_item item, int value, Event *event);
extern int scope_repaint_proc (Canvas canvas,
                               Pixwin *pw,
                               Rectlist *repaint_area);
extern int scope_clear (void);
extern int scope_display (int off);
extern void waveform_cursor_proc (Canvas canvas, Event *event, caddr_t arg);
extern void waveform_repaint_proc (Canvas canvas,
                                   Pixwin *pw,
                                   Rectlist *repaint_area);
extern Notify_value sigpoll_async_handler (Notify_client client,
                                           int sig,
                                           Notify_signal_mode when);
extern Notify_value sigpoll_sync_handler (Notify_client client,
                                          Notify_event event,
                                          Notify_arg arg,
                                          Notify_event_type when);
extern Notify_value timer_handler (Notify_client client, int which);
extern int set_item_val (Panel_item item, int val);
extern int audio_close (void);

#endif
