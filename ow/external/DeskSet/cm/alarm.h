/*static  char sccsid[] = "@(#)alarm.h 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";*/

/*	alarm.h
	Routines which actually implement the reminder notification */

extern Notify_value ring_it (/* Notify_client client, int arg  */);

extern Notify_value flash_it(/* Notify_client client, int arg  */);
 
extern Notify_value mail_it (/* Notify_client client, int arg  */);

extern Notify_value open_it (/* Notify_client client, int arg  */);
 
extern Notify_value reminder_driver (/* Notify_client client, int arg */);

