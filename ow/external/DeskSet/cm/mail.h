/*	static char sccsid[] = "@(#)mail.h 2.12 90/10/25 Copy 1988 Sun Micro; 
	mail.h
*/

/* used for storing mail panel items  */
#define MAIL_BINARY_NAME       "mailx"
#define MAIL_FIXED_PATH        "/usr/bin/mailx"

typedef struct {
	Textsw          comptextsw;
        Frame           compframe;
        Panel           comppanel;
        Panel_item      compmailto;
        Panel_item      compsubj;
        Panel_item      compcc;
        Panel_item      compbcc;
        Panel_item      include_button;
        Panel_item      deliver_button;
        Panel_item      header_button;
        Panel_item      clear_button;
}Mail;

/* get mail addresses to mail to */
extern void mail_list(/* Browser *, char * */);
