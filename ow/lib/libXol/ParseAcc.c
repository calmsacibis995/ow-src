#pragma ident	"@(#)ParseAcc.c	1.3	97/03/26 lib/libXol SMI"	/* OLIT */
/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <Xol/DynamicP.h>
#include <Xol/ParseAccI.h>
#include <ctype.h>

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

static void avGetXtAcceleratorValue(Display *dpy, AcceleratorValue *avp,
    Boolean is_key, char *pos, BtnSym *btnsym, KeySym *keysym,
    Modifiers *modifiers);
static void avGetOLITAcceleratorValue(Display *dpy, AcceleratorValue *avp,
    Boolean is_key, char *pos, BtnSym *btnsym, KeySym *keysym,
    Modifiers *modifiers);
static void avGetXViewAcceleratorValue(Display *dpy, AcceleratorValue *avp,
    Boolean is_key, char *pos, BtnSym *btnsym, KeySym *keysym,
    Modifiers *modifiers);
static char *avAddKey(AcceleratorValue *avp, char *pos, KeySym *keysym);
static char *avAddBtn(AcceleratorValue *avp, char *pos, BtnSym *btnsym);
static void avAddModif(Display *dpy, AcceleratorValue *avp, AVModif modif,
    Modifiers *modifiers);

#define KWRD_KEYPRESS  "<Key>"


Boolean
_OlParseKeyOrBtnSyntax(Display *dpy, char *resourceString, Boolean is_key,
		       BtnSym *btnsym, KeySym *keysym, Modifiers *modifiers)
{
    AcceleratorValue av;

    memset( &av, 0, sizeof( av ) );
    avGetXtAcceleratorValue(dpy, &av, is_key, resourceString,
			    btnsym, keysym, modifiers);
    if (av.error || (is_key ? !av.keysym : !av.btnsym)) {
        memset( &av, 0, sizeof( av ) );
	/*
	 * It's possible that *modifiers got set in a partial parse above;
	 * this must be cleared, since the parse didn't complete (either
	 * error or didn't get a keysym).
	 */
	*modifiers = 0;
    	avGetOLITAcceleratorValue(dpy, &av, is_key, resourceString,
				  btnsym, keysym, modifiers);
    }
    if (av.error || (is_key ? !av.keysym : !av.btnsym)) {
        memset( &av, 0, sizeof( av ) );
	*modifiers = 0;
    	avGetXViewAcceleratorValue(dpy, &av, is_key, resourceString,
				   btnsym, keysym, modifiers);
    }
    if (is_key ? !av.keysym : !av.btnsym)
	av.error = 1;

    return (!av.error);
}

static void
avGetXtAcceleratorValue(Display *dpy, AcceleratorValue *avp, Boolean is_key,
			char *pos, BtnSym *btnsym, KeySym *keysym,
			Modifiers *modifiers) {
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
	avAddModif(dpy, avp, kp->modif, modifiers);
	avGetXtAcceleratorValue(dpy, avp, is_key, pos + strlen(kp->string),
				btnsym, keysym, modifiers);
	return;
    }

    /* look for '<' Key '>' <key-spec> and then nothing */
    if( !strncmp( KWRD_KEYPRESS, pos, strlen( KWRD_KEYPRESS ) ) ) {
	pos += strlen( KWRD_KEYPRESS );
	pos += strspn( pos, " \t" );
	pos = is_key ? avAddKey(avp, pos, keysym) : avAddBtn(avp, pos, btnsym);
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
avGetOLITAcceleratorValue(Display *dpy, AcceleratorValue *avp, Boolean is_key,
			  char *pos, BtnSym *btnsym, KeySym *keysym,
			  Modifiers *modifiers) {
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
	avAddModif(dpy, avp, kp->modif, modifiers);
	avGetOLITAcceleratorValue(dpy, avp, is_key, pos + strlen(kp->string),
				  btnsym, keysym, modifiers);
	return;
    }

    /* look for '<' key '>' and then nothing */
    if( *pos == '<' ) {
	pos = is_key ? avAddKey(avp, pos + 1, keysym) :
	    avAddBtn(avp, pos + 1, btnsym);
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
avGetXViewAcceleratorValue(Display *dpy, AcceleratorValue *avp, Boolean is_key,
			   char *pos,
			   BtnSym *btnsym, KeySym *keysym,
			   Modifiers *modifiers)
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
	avAddModif(dpy, avp, kp->modif, modifiers);
	avGetXViewAcceleratorValue(dpy, avp, is_key, pos + strlen(kp->string),
				   btnsym, keysym, modifiers);
	return;
    } 

    /* if no keysym name found yet, look for keysym */
    if( avp->keysym )
	avp->error = 1;
    else {
	pos = is_key ? avAddKey(avp, pos, keysym) : avAddBtn(avp, pos, btnsym);
	if(!avp->error)
	    avGetXViewAcceleratorValue(dpy, avp, is_key, pos,
				       btnsym, keysym, modifiers);
    }

    return;
}


static void
avAddModif(Display *dpy, AcceleratorValue *avp, AVModif modif,
	   Modifiers *modifiers) {
    if( modif == modifNone )
	avp->none = 1;
    else {
   	avp->some = 1; 
    	switch( modif ) {
	case modifShift:
	    *modifiers |= ShiftMask;
	    break;
	case modifCtrl:
	    *modifiers |= ControlMask;
	    break;
	case modifLock:
	    *modifiers |= LockMask;
	    break;
	case modifMod1:
	    *modifiers |= Mod1Mask;
	    break;
	case modifMod2:
	    *modifiers |= Mod2Mask;
	    break;
	case modifMod3:
	    *modifiers |= Mod3Mask;
	    break;
	case modifMod4:
	    *modifiers |= Mod4Mask;
	    break;
	case modifMod5:
	    *modifiers |= Mod5Mask;
	    break;
	case modifMeta:
	    *modifiers |= _OlGetModifierBinding(dpy, Meta);
	    break;
	case modifAlt:
	    *modifiers |= _OlGetModifierBinding(dpy, Alt);
	    break;
	case modifSuper:
	    *modifiers |= _OlGetModifierBinding(dpy, Super);
	    break;
	case modifHyper:
	    *modifiers |= _OlGetModifierBinding(dpy, Hyper);
	    break;
	case modifModeswitch:
	    *modifiers |= _OlGetModifierBinding(dpy, ModeSwitch);
	    break;
    	}
    }

    if( avp->none && avp->some )
	avp->error = 1;
}


static char *
    avAddKey (AcceleratorValue *avp, char *pos, KeySym *keysym)
{
    char *sp, *dp, strbuf[100];
    
    /* if keysym already set, that's an error */
    if( avp->keysym ) {
	avp->error = 1;
	return (char *)NULL;
    }
    
    /* look for 'raw' space or punctuation */
    if( ispunct( *pos ) || isspace( *pos )) { 
	*keysym = avp->keysym = *pos;
	pos++;
    } else {				/* look for valid keysym name */
	for (sp = pos, dp = strbuf;
            dp < strbuf + sizeof( strbuf ) && (isalnum( *sp ) || *sp == '_');
	    *dp++ = *sp++);
	*dp = '\0';
	if (*keysym = avp->keysym = XStringToKeysym(strbuf))
	    pos = sp;
        else
	    avp->error = 1;		/* nothing parses as a key */
    }
    
    return pos;
}


static char *
avAddBtn (AcceleratorValue *avp, char *pos, BtnSym *btnsym)
{
    char *sp, *dp, strbuf[100];

    /* if keysym already set, that's an error */
    if( avp->btnsym ) {
	avp->error = 1;
	return (char*)NULL;
    }

	for( sp = pos, dp = strbuf ; 
	     dp < strbuf + sizeof( strbuf ) && isalnum( *sp ) ; 
	     *dp++ = *sp++ );
	*dp = '\0';

	if (*btnsym = avp->btnsym = _OlStringToButton(strbuf))
	    pos = sp;
        else
	    avp->error = 1;		/* nothing parses as a key */

    return pos;
}
