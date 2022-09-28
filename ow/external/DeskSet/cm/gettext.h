/*static  char sccsid[] = "@(#)gettext.h 3.2 93/01/25 Copyr 1991 Sun Microsystems, Inc.";
		gettext.h */

#ifdef XGETTEXT
#define MSGFILE_ERROR     "SUNW_DESKSET_CM_ERR"
#define MSGFILE_LABEL     "SUNW_DESKSET_CM_LABEL"
#define MSGFILE_MESSAGE   "SUNW_DESKSET_CM_MSG"
#else
extern char *MSGFILE_ERROR;
extern char *MSGFILE_LABEL;
extern char *MSGFILE_MESSAGE;
#endif

#define EGET(s)    (char *)dgettext(MSGFILE_ERROR, s)
#define LGET(s)    (char *)dgettext(MSGFILE_LABEL, s)
#define MGET(s)    (char *)dgettext(MSGFILE_MESSAGE, s)

