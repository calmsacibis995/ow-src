/*static  char sccsid[] = "@(#)cm_i18n.h 3.2 93/02/12 Copyr 1991 Sun Microsystems, Inc."; */
/* cm_i18n.h */


/* CM font file description:
 *
 * Each line cannot exceed MAX_LINE_LEN. 
 * File format is as follows: 
 * <Locale name> <fontset1_fontname1> <fontset2_fontname1>
 *               <fontset1_fontname2> <fontset2_fontname2>
 * Line starts at left most margin.
 * Categories separated by space(s).
 * Comment lines start with an '%'
 */


#define MAX_LINE_LEN    128
#define MAX_FONT_LEN    40
#define COMMENT_SYMBOL  "%"
#define CHAR_SIZE       sizeof(char)
#define isEUC(c)    ((c) & 0x80 ? TRUE : FALSE)

/* cm_get_fonts()'s return values */
#define OPEN_FAIL         -1
#define NO_LOCALE_ENTRY   -2 
#define EXTRACT_FAIL      -3
#define OKAY               1
#define NO_I18N_HEADER     2

extern char *fontset1[];
extern char *fontset2[];
extern int use_default_fonts;
extern int use_octal;

extern int cm_get_fonts(/* char * */); 
extern void ps_i18n_header(/* FILE *, Frame */);
extern char *euc_to_octal(/* char * */);
extern char *cm_get_i18n_date(/* Frame, char * */);
extern void convert_date();
extern char *cm_printf();

int is_comment();
int match_locale();
int extract_fontsets();
