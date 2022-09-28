#ifndef _PROPS_PRIVATE_H
#define _PROPS_PRIVATE_H

#pragma ident "@(#)propsP.h	1.8 93/05/11	SMI"

/**************************************************************************
* Defines
**************************************************************************/

/*
 * There is no X function to convert a database in memory to a string, so
 * the database in memory is written to a file using XrmPutFileDatabase().
 * This file is removed immediately after use.
 */
#define RWIN_DB_FILE    "/tmp/OWtemp"

/*
 * These strings are for getting or specifying the help text for a widget
 * which has help registered by no help text associated with it. This prevents
 * OLIT from crashing when props doesn't find the non-existant help text.
 */
#define PROPS_NOHELP_RES  "*noHelpString"
#define PROPS_NOHELP_STRING "There is no help available for this object."

/**************************************************************************
* Private Structures
**************************************************************************/

typedef enum {
    ModeSystemInitialize, ModeRun
} PropsMode;

typedef struct _PropsCatInfo {
    CategoryInfo *info;  	/* Info supplied by cat upon registration   */
    int           id;   	/* Assigned by foundation upon registration */
    Widget        widget;	/* Category top-level widget		    */
    struct _PropsCatInfo *next; /* Pointer for putting these in linked list */
} PropsCatInfo;
 
/**************************************************************************
* Macros For Accessing Category Functions
**************************************************************************/

#define CreateCategory(i,w)     ((*(i->info->createCategory))(w))
#define CreateChangeBars(i)     ((*(i->info->createChangeBars))(i->widget))
#define DeleteChangeBars(i)     ((*(i->info->deleteChangeBars))(i->widget))
#define BackupSettings(i,b)     ((*(i->info->backupSettings))(i->widget,b))
#define RestoreSettings(i)      ((*(i->info->restoreSettings))(i->widget))
#define ReadDBSettings(i,f,b)   ((*(i->info->readDbSettings)) \
                                        (i->widget,f,b))
#define SaveDBSettings(i,f)     ((*(i->info->saveDbSettings))(i->widget,f))
#define RealizeSettings(i)      ((*(i->info->realizeSettings))(i->widget))
#define InitializeServer(i,f)   ((*(i->info->initializeServer))(f))
#define SyncWithServer(i,fg,fp) ((*(i->info->syncWithServer))(fg,fp))
#define ShowCategory(i)         ((*(i->info->showCategory))(i->widget))
#define HideCategory(i)         ((*(i->info->hideCategory))(i->widget))

/**************************************************************************
* Private Functions
**************************************************************************/

/*
 * Generic Category Supporting Functions
 */

static void          propsBackupSetting(Widget, int, Boolean);
static void          propsCreateChangeBar(Widget, int, Boolean);
static void          propsDeleteChangeBar(Widget, int, Boolean);
static void          propsReadDbSetting(Widget, int, Boolean);
static void          propsRestoreSetting(Widget, int, Boolean);
static void          propsSaveDbSetting(Widget, int, Boolean);

/*
 * Initialization & Shutdown
 */

static PropsCatInfo *propsBuildCategoryList(void);
static void          propsCreateButtons(Widget, PropsCatInfo*);
static void          propsExit(Widget, XtPointer, XtPointer);
static void          propsSystemInitialize(PropsCatInfo*);

/*
 * Category List Handling Functions
 */

static PropsCatInfo *propsGetCategoryCurrentInfo(PropsCatInfo*);
static PropsCatInfo *propsGetCategoryInfo(PropsCatInfo*, int);
static int           propsGetNextId(void);
static void          propsPutCategoryInfo(PropsCatInfo**, CategoryInfo*);
static void          propsShowCategory(Widget, XtPointer, XtPointer);
 
/*
 * Database Handling Functions
 */

static void          propsApplyCb(Widget, XtPointer, XtPointer);
static void          propsDynamicUpdate(XtPointer);
static char         *propsGetRootWindowProperty(void);
static void          propsResetCb(Widget, XtPointer, XtPointer);
static void          propsStandCb(Widget, XtPointer, XtPointer);
static void          propsUpdateCategories(PropsCatInfo*);
static void          propsWriteDb(void);
 
/*
 * Supporting Functions
 */

static void	     propsCenterCategory(Widget);
static Widget	     propsSetCategoryMenu(Widget, int);

#endif /* _PROPS_PRIVATE_H */
