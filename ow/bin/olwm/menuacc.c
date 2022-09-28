#ident	"@(#)menuacc.c	1.2	93/09/14 SMI"

/*
 *      (c) Copyright 1992 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */

#include <ctype.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>

/*
 * Note: This file contains a lot of code stolen directly from the 
 * XView file svr_parse.c.
 *
 * There is only 1 exported function:
 *
 *	int
 *	ParseKeyResource(dpy, defaults_db, keystr, keysym, modifiers)
 *	Display *dpy;
 *	XrmDatabase defaults_db;
 *	char            *keystr;
 *	KeySym          *keysym;
 *	int		*modifiers;
 */

/*
 * Globals
 */
extern unsigned int stringToModifier(Display *, char *);

/*
 * START of declarations for parsing engine
 */

typedef struct acceleratorValue {
    KeySym keysym;
    unsigned meta:1,
             shift:1,
             alt:1,
             ctrl:1,
             super:1,
             hyper:1,
             lock:1,
             modeswitch:1,
             mod1:1, mod2:1, mod3:1, mod4:1, mod5:1,
             error:1,
             none:1,
             some:1,
             reserved:16;
} AcceleratorValue;
 
typedef struct {
    enum { styleNone, styleXView, styleOLIT, styleXt } style;
    AcceleratorValue av;
    char *pos;
} AVState;
 
typedef enum {
    modifMeta, modifShift, modifAlt, modifCtrl, modifSuper, modifHyper,
    modifLock, modifModeswitch,
    modifMod1, modifMod2, modifMod3, modifMod4, modifMod5,
    modifNone
} AVModif;
 
typedef struct {
    char *string;
    AVModif modif;
} AVKeyword;

AVKeyword keywordTbl[] = {
{ "Meta", modifMeta },
{ "Shift", modifShift },
{ "Alt", modifAlt },
{ "Ctrl", modifCtrl },
{ "Super", modifSuper },
{ "Hyper", modifHyper },
{ "Lock", modifLock },
{ "ModeSwitch", modifModeswitch },
{ "Mod1", modifMod1 },
{ "Mod2", modifMod2 },
{ "Mod3", modifMod3 },
{ "Mod4", modifMod4 },
{ "Mod5", modifMod5 },
{ "None", modifNone }
};
 
AVKeyword shortKeywordTbl[] = {
{ "m", modifMeta },
{ "su", modifSuper },
{ "s", modifShift },
{ "a", modifAlt },
{ "c", modifCtrl },
{ "h", modifHyper },
{ "l", modifLock },
{ "1", modifMod1 },
{ "2", modifMod2 },
{ "3", modifMod3 },
{ "4", modifMod4 },
{ "5", modifMod5 },
{ "n", modifNone }
};
 
#define keywordTblEnd \
        (keywordTbl + sizeof(keywordTbl) / sizeof(AVKeyword))
 
#define shortKeywordTblEnd \
        (shortKeywordTbl + sizeof(shortKeywordTbl) / sizeof(AVKeyword))

/*
 * Functions to implement parsing engine
 */
static AcceleratorValue getAcceleratorValue(char *, XrmDatabase);
static void avGetXtAcceleratorValue(AcceleratorValue *, char *);
static void avGetOLITAcceleratorValue(AcceleratorValue *, char *);
static void avGetXViewAcceleratorValue(AcceleratorValue *, char *);
static char *avAddKey(AcceleratorValue *, char *);
static void avAddModif(AcceleratorValue *, AVModif);

#define KWRD_KEYPRESS  	"<Key>"
#ifndef TRUE
#define TRUE 		1
#define FALSE 		0
#endif

/*
 * END of declarations for parsing engine
 */

#define ADD_MODIFIERS(value, mod, err)	\
	if(value == 0)		  	\
	    err = 1;			\
	else				\
	    *mod |= value;

int
ParseKeyResource(Display *dpy, XrmDatabase defaults_rdb, 
		 char *keystr, KeySym *keysym, 
		 int *modifiers)
{
    AcceleratorValue    av;
    char		*tmp_str = NULL;
    int			err = 0, mod_tmp;

    if(!dpy || !keystr || !keysym || !modifiers)
        return 1;

    /* 
     * Make a duplicate of keystr -- actions here may modify it
     */
    tmp_str = strdup(keystr);

    /*
     * Parse the string using the core algorithm
     */
    av = getAcceleratorValue(tmp_str, defaults_rdb);
    if(av.error) {
	if(tmp_str)
	    (void)free(tmp_str);
	return 1;
    }

    *keysym = av.keysym;

    /*
     * Some olwm-specific mannerisims in the way in which it deals with
     * shifting. If the keysym is upper case, add a shift-mask. If it
     * is lower case, convert it to upper case.
     */
    if (XK_A <= *keysym && *keysym <= XK_Z)
	av.shift = 1;
    if(XK_a <= *keysym && *keysym <= XK_z)
	*keysym -= (XK_a - XK_A);

    /*
     * Set modifier mask
     */
    *modifiers = 0;
    if (av.meta)  {
	mod_tmp = stringToModifier(dpy, "Meta");
	ADD_MODIFIERS(mod_tmp, modifiers, err);
    }
    if(av.shift) {
	mod_tmp = stringToModifier(dpy, "Shift");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.alt) {
	mod_tmp = stringToModifier(dpy, "Alt");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.ctrl) {
	mod_tmp = stringToModifier(dpy, "Ctrl");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.super) {
	mod_tmp = stringToModifier(dpy, "Super");
	ADD_MODIFIERS(mod_tmp, modifiers, err);
    }
    if(av.hyper) {
	mod_tmp = stringToModifier(dpy, "Hyper");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.lock) {
	mod_tmp = stringToModifier(dpy, "Lock");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.mod1) {
	mod_tmp = stringToModifier(dpy, "mod1");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.mod2) {
	mod_tmp = stringToModifier(dpy, "mod2");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.mod3) {
	mod_tmp = stringToModifier(dpy, "mod3");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.mod4) {
        mod_tmp = stringToModifier(dpy, "mod4");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }
    if(av.mod5) {
	mod_tmp = stringToModifier(dpy, "mod5");
	ADD_MODIFIERS(mod_tmp, modifiers, err); 
    }

    (void)free(tmp_str);
    if(err)
	return 1;
    else
        return 0;
}

static AcceleratorValue
getAcceleratorValue(char *resourceString, XrmDatabase db)
{
    AcceleratorValue av;

    /* if its starts with 'coreset', look for coreset resource */
    if(db && !strncasecmp(resourceString,"coreset",sizeof("coreset")-1)) {
        char funcname[100], resname[100];
        XrmValue value;
        char *strtype;

        *funcname = '\0';
        sscanf( resourceString, "%*s%s", funcname );
        sprintf( resname, "OpenWindows.MenuAccelerator.%s", funcname );
        if( False == XrmGetResource( db, resname, "*", &strtype, &value ) )
            av.error = 1;
        else
            av = getAcceleratorValue( value.addr, db );
        return av;
    }

    /* try the three syntaxes, until one parses resourceString */
    memset(&av, 0, sizeof(av));
    avGetXtAcceleratorValue( &av, resourceString );
    if( av.error || !av.keysym ) {
        memset(&av, 0, sizeof(av));
        avGetOLITAcceleratorValue( &av, resourceString );
    }
    if( av.error || !av.keysym ) {
        memset(&av, 0, sizeof(av)); 
        avGetXViewAcceleratorValue( &av, resourceString );
    }
    if( !av.keysym )
        av.error = 1;

    return av;
}

static void
avGetXtAcceleratorValue(AcceleratorValue *avp, char *pos)
{
    AVKeyword *kp;

    /* skip blanks */
    pos += strspn( pos, " \t" );
    if( !*pos )
        return;

    /* look for one of the regular or abbrv. keywords */
    for( kp = keywordTbl ; kp < keywordTblEnd ; kp++ )
        if( !strncmp( kp->string, pos, strlen( kp->string ) ) )
            break;
    if( kp == keywordTblEnd )
        for( kp = shortKeywordTbl ; kp < shortKeywordTblEnd ; kp++ )
            if( !strncmp( kp->string, pos, strlen( kp->string ) ) )
                break;
    if( kp != shortKeywordTblEnd ) {
        /* disallow modifs after keysym is known */
        if( avp->keysym ) {
            avp->error = 1;
            return;
        }
        avAddModif( avp, kp->modif );
        avGetXtAcceleratorValue( avp, pos + strlen( kp->string ) );
        return;
    }

    /* look for '<' Key '>' <key-spec> and then nothing */
    if( !strncmp( KWRD_KEYPRESS, pos, strlen( KWRD_KEYPRESS ) ) ) {
        pos += strlen( KWRD_KEYPRESS );
        pos += strspn( pos, " \t" );
        pos = avAddKey( avp, pos );
        pos += strspn( pos, " \t" );
        if( *pos )
                avp->error = 1;
        return;
    }

    /* an error occured */
    avp->error = 1;
    return;
}

static void
avGetOLITAcceleratorValue(AcceleratorValue *avp, char *pos)
{
    AVKeyword *kp;
         
    /* skip blanks */
    pos += strspn( pos, " \t" );
    if( !*pos )
        return;
 
    /* look for one of the regular or abbrv. keywords */
    for( kp = keywordTbl ; kp < keywordTblEnd ; kp++ )
        if( !strncmp( kp->string, pos, strlen( kp->string ) ) )
                break;
    if( kp == keywordTblEnd )
        for( kp = shortKeywordTbl ; kp < shortKeywordTblEnd ; kp++ )
            if( !strncmp( kp->string, pos, strlen( kp->string ) ) )
                break;
    if( kp != shortKeywordTblEnd ) {
        /* disallow modifs after keysym is known */
        if( avp->keysym ) {
            avp->error = 1;
            return;
        }
        avAddModif( avp, kp->modif );
        avGetOLITAcceleratorValue( avp, pos + strlen( kp->string ) );
        return;
    }
 
    /* look for '<' key '>' and then nothing */
    if( *pos == '<' ) {
        pos = avAddKey( avp, pos + 1 );
        if( avp->error ) return;
        pos += strspn( pos, " \t" );
        if( *pos != '>' )
            avp->error = 1;
        else {
            pos += 1 + strspn( pos+1, " \t" );
            if( *pos )
                avp->error = 1;
        }
        return;
    }    
 
    /* an error occured */
    avp->error = 1;
    return;
}

static void
avGetXViewAcceleratorValue(AcceleratorValue *avp, char *pos)
{
    AVKeyword *kp;
 
    /* skip blanks */
    pos += strspn( pos, " \t" );
    if( !*pos )
        return;
 
    /* if mods or keysyms already found, look for '+' */
    if( avp->keysym || avp->some || avp->none )
        if( *pos != '+' ) {
            avp->error = 1;
            return;
        } else
            pos += 1 + strspn( pos+1, " \t" );
              
    /* look for one of the regular keywords */
    for( kp = keywordTbl ; kp < keywordTblEnd ; kp++ )
        if( !strncmp( kp->string, pos, strlen( kp->string ) ) )
                break;
 
    if( kp != keywordTblEnd ) {
        avAddModif( avp, kp->modif );
        avGetXViewAcceleratorValue( avp, pos + strlen( kp->string ) );
        return;
    }
 
    /* if no keysym name found yet, look for keysym */
    if( avp->keysym )
        avp->error = 1;
    else {
        pos = avAddKey( avp, pos );
        if( !avp->error )
            avGetXViewAcceleratorValue( avp, pos );
    }
 
    return;
}

static void
avAddModif(AcceleratorValue *avp, AVModif modif)
{
    if( modif == modifNone )
        avp->none = 1;
    else {
        avp->some = 1;
        switch( modif ) {
        case modifMeta: avp->meta = 1;  break;
        case modifShift:avp->shift = 1; break;
        case modifAlt:  avp->alt = 1;   break;
        case modifCtrl: avp->ctrl = 1;  break;
        case modifSuper:avp->super = 1; break;
        case modifHyper:avp->hyper = 1; break;
        case modifLock: avp->lock = 1;  break;
        case modifModeswitch:   avp->modeswitch = 1;    break;
        case modifMod1: avp->mod1 = 1;  break;
        case modifMod2: avp->mod2 = 1;  break;
        case modifMod3: avp->mod3 = 1;  break;
        case modifMod4: avp->mod4 = 1;  break;
        case modifMod5: avp->mod5 = 1;  break;
        }
    }
 
    if( avp->none && avp->some )
        avp->error = 1;
}

static char *
avAddKey(AcceleratorValue *avp, char *pos)
{
    char *sp, *dp, strbuf[100];
 
    /* if keysym already set, that's an error */
    if( avp->keysym ) {
        avp->error = 1;
        return NULL;
    }
 
    /* look for 'raw' space or punctuation */
    if( ispunct( *pos ) || isspace( *pos )) {
        avp->keysym = *pos;
        pos++;
    } else {                            /* look for valid keysym name */
        for( sp = pos, dp = strbuf ;
             dp < strbuf + sizeof( strbuf ) && (isalnum( *sp ) || *sp == '_') ;
             *dp++ = *sp++ );
        *dp = '\0';
        if( avp->keysym = XStringToKeysym( strbuf ) )
            pos = sp;
        else
            avp->error = 1;             /* nothing parses as a key */
    }
 
    return pos;
}
