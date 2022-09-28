#ifndef _PROPS_H
#define _PROPS_H

#pragma ident	"@(#)props.h	1.20	93/03/08 SMI"

/**************************************************************************
* Typedefs
**************************************************************************/

typedef const char *const	strconst;


/**************************************************************************
* Defines
**************************************************************************/

#define ACCESS_RWIN_DB (1 << 0)
#define ACCESS_USER_DB (1 << 1)
#define ACCESS_DFLT_DB (1 << 2)

#define ACCESS_ALL_DB (ACCESS_RWIN_DB | ACCESS_USER_DB | ACCESS_DFLT_DB)


/**************************************************************************
* Structures
**************************************************************************/

typedef enum {
    dNumber, dString, dBoolean, dPrivate
} Deftype;

typedef struct _ItemInfo {
    const char *name;	  /* Resource specification */
    int         id;	  /* Optionally used by each category. Set by passing */
			  /* id as the 1st parameter to wspNewItemInfo()   */
    Deftype     type;	  /* dNumber, dString, or dBoolean                 */
    const char *domain;	  /* Used for dString. e.g. valid strings are "aye", */
			  /* "bee", "sea"; domain is "aye:bee:sea"            */
    int         currVal;  /* Current state of setting			      */
    int         backVal;  /* Setting as of last apply			      */
    Widget      caption;  /* Caption widget id for setting the change bars    */
} ItemInfo;

typedef struct _CategoryInfo {
    char    *name;
    Widget   (*createCategory)(Widget);
    void     (*createChangeBars)(Widget);
    void     (*deleteChangeBars)(Widget);
    void     (*backupSettings)(Widget, Boolean);
    void     (*restoreSettings)(Widget);
    void     (*readDbSettings)(Widget, int, Boolean);
    void     (*saveDbSettings)(Widget, int);
    Boolean  (*realizeSettings)(Widget);
    void     (*initializeServer)(int);
    Boolean  (*syncWithServer)(int, int);
    void     (*showCategory)(Widget);
    void     (*hideCategory)(Widget);
} CategoryInfo;


/**************************************************************************
* Externals
**************************************************************************/

extern Display      *display;
extern Screen       *screen;
extern Visual       *visual;
extern XtAppContext  appContext;

extern CategoryInfo *colorRegisterCategory(void);
extern CategoryInfo *fontsRegisterCategory(void);
extern CategoryInfo *keyboardRegisterCategory(void);
extern CategoryInfo *localeRegisterCategory(void);
extern CategoryInfo *menuRegisterCategory(void);
extern CategoryInfo *miscRegisterCategory(void);
extern CategoryInfo *mouseRegisterCategory(void);
extern CategoryInfo *wkspMenuRegisterCategory(void);


/**************************************************************************
* Public Functions
**************************************************************************/

/*
 * Supporting Functions
 */

extern void             wspChangeBusyState(Boolean);
extern void             wspChangeSettingCb(Widget, XtPointer, XtPointer);
extern void             wspClearFooter(void);
extern void		wspErrorFooter(const char*, ...);
extern void		wspErrorPopupConfirm(const char*, ...);
extern void		wspErrorPopupChoice(
				void(*)(Widget, XtPointer, XtPointer),
				const char*, ...);
extern void		wspErrorStandard(const char*, ...);
extern const char      *wspGetLocale(void);
extern const char      *wspGetOpenwinhome(void);
extern Boolean          wspGetResource(const char*, int, XrmValue*);
extern void             wspHelp(OlDefine, XtPointer, Cardinal, Cardinal,
                                OlDefine*, XtPointer*);
extern ItemInfo        *wspNewItemInfo(int, const char*, Deftype,
                                const char*, int, Widget);
extern void             wspPutResource(const char*, const char*, int);
extern Boolean          wspSettableWidgetClass(WidgetClass);
extern void             wspSetWidgetState(Widget, int);
extern Boolean          wspUpdateResourceManager(void);

/*
 * Generic Category Functions
 */

extern void      wspBackupSettings(Widget, Boolean);
extern void      wspCreateChangeBars(Widget);
extern void      wspDeleteChangeBars(Widget);
extern void      wspNoopHideCategory(Widget);
extern void      wspNoopInitializeServer(int);
extern Boolean   wspNoopRealizeSettings(Widget);
extern void      wspNoopShowCategory(Widget);
extern Boolean   wspNoopSyncWithServer(int, int);
extern void      wspReadDbSettings(Widget, int, Boolean);
extern void      wspRestoreSettings(Widget);
extern void      wspSaveDbSettings(Widget, int);
extern void      wspUpdateSettableWidgets(void (*)(Widget, int, Boolean),
                        Widget, int, Boolean);

/*
 * GetString wrapper around gettext()
 */

#ifdef OW_I18N_L3

extern	char		*gettext(const char *);
#define	GetString(s)	 gettext(s)

#else

#define	GetString(s)	 s

#endif

#endif /* _PROPS_H */
