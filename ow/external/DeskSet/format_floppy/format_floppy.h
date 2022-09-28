/* Do not put gettext around the following strings */
#define MSGFILE    "SUNW_DESKSET_DS_FORMAT"
#define FORMAT     "format"
#define UNFORMAT   "unformatted"
#define UNLABEL    "unlabeled"

typedef enum {Format, Unformatted, Unlabeled} Popup_Type;

extern char *raw_device;
extern char *mnt_point;
extern char *popup;
extern char *label;
extern int x_position;
extern int y_position;

extern void init_program();
extern void format_floppy();

extern Popup_Type popup_type;
