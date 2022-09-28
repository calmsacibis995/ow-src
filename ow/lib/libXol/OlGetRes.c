#pragma ident	"@(#)OlGetRes.c	302.4	97/03/26 lib/libXol SMI"	/* olcommon:src/OlGetRes.c 1.12	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *************************************************************************
 *
 * Date:	November 1988
 *
 * Description:
 *
 *	This file defines the "OlGetResolution()" routine.
 * 
 *	This routine generates a single letter that identifies the
 *	resolution of a screen. It refers to the .Xdefaults file to
 *	get a user specified mapping of resolution to character.
 *	If no entry is given in .Xdefaults, this routine uses a default,
 *	hard-coded mapping.
 *
 *	A modified BNF description of the syntax for specifing mapping
 *	follows.
 *
 *		mapping := "+" map-list
 *		mapping := map-list
 *
 *		map-list := map
 *		map-list := map-list "," map
 *
 *		map := resolution "=" character
 *
 *		resolution := integer "x" integer
 *		resolution := integer "x" integer "c"
 *
 *	An example is
 *
 *		"60x60=a,60x72=b"
 *
 *	A leading "+" means add the mapping to the built-in default;
 *	without the leading "+" the mapping replaces the default.
 *	A resolution is by default specified in dots per inch, with
 *	the horizontal value first. Appending a "c" to a resolution
 *	means dots per centimeter.
 *
 *	On lookup, the "OlGetResolution()" routine will interpolate
 *	to find the closest match. It will never fail to return a
 *	match, although (1) the match may be poor for unexpected screens,
 *	and (2) bad values of the "screen" parameter may cause problems,
 *	including a core dump.
 *
 *	The character returned is saved internally, so that a subsequent
 *	call to "OlGetResolution()" with the same screen argument can
 *	return quickly. To clear the saved character so that a complete
 *	lookup is performed again, call "OlGetResolution()" with a
 *	zero argument; nothing interesting is returned, but the NEXT
 *	call with a valid screen argument will work.
 *
 ******************************file*header********************************
 */


#include <Xol/OpenLookP.h>
#include <Xol/OlI18nP.h>
#include <Xol/Util.h>

#include <X11/Intrinsic.h>

#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define OL_RESOLUTION_MAP		"ResolutionMap"
#define OL_DEFAULT_RESOLUTION_MAP	"\
76x55=a,\
76x63=b,\
76x76=c,\
95x95=d,\
66x48=e,\
66x55=f,\
66x66=g,\
83x83=h"

typedef struct _ResolutionItem {
	struct _ResolutionItem	*next;
	int			horz,
				vert;
	char			character;
}			ResolutionItem;

static ResolutionItem	*parse_resolution_map(Screen *screen, char *str);
static ResolutionItem	*search_map(ResolutionItem *list, ResolutionItem item);

/**
 ** OlGetResolution()
 **/

char
OlGetResolution (Screen *screen)
{
	char			*s_resolution_map;

	ResolutionItem		resolution,
				*p_item,
				*map;

	static Screen		*last_screen	= 0;

	static char		last_character	= 0;
	Boolean			need_to_free = False;


	/*
	 * See if we've seen this screen argument before.
	 * If so, we know what to return without doing any more
	 * work. A zero argument clears what we've saved to force
	 * a lookup next time.
	 */
	if (screen == last_screen)
		return (last_character);
	last_screen = screen;

	if (!screen)
		return (0);


	s_resolution_map = XGetDefault(
		DisplayOfScreen(screen),
		XrmQuarkToString(_OlApplicationName),
		OL_RESOLUTION_MAP
	);
	if (!s_resolution_map || !*s_resolution_map) {
		s_resolution_map = Strdup(OL_DEFAULT_RESOLUTION_MAP);
		need_to_free = True;
	}

	if (!(map = parse_resolution_map(screen, s_resolution_map))) {
		/*
		 * Either the user gave an empty map, or one full of
		 * errors, or our default map is bad. Try parsing just
		 * the default map to see which problem we have.
		 */
		if (!(map = parse_resolution_map(screen, OL_DEFAULT_RESOLUTION_MAP)))
		        OlError (dgettext(OlMsgsDomain,
				"Internal error: no screen resolution map!\n"));
		else
			OlWarning (dgettext(OlMsgsDomain,
			   "Screen resolution map is bad--using default.\n"));
	}

	if (need_to_free)
		XtFree(s_resolution_map);


#define INCHES(X)	((X) / (double)25.4)
	resolution.horz = WidthOfScreen(screen)
			/ INCHES(WidthMMOfScreen(screen));
	resolution.vert = HeightOfScreen(screen)
			/ INCHES(HeightMMOfScreen(screen));


	if (!(p_item = search_map(map, resolution)))
		OlError (dgettext(OlMsgsDomain,
			"Internal error: Resolution map search failed!\n"));


	return (last_character = p_item->character);
}

/**
 ** parse_resolution_map()
 **/

static char *
strip (register char *str)
{
	while (*str && isspace(*str))
		str++;
	return (str);
}

static void
res_complain (char *reason, char *str, int offset)
{
	char			*msg;
	static char		*prefix; 

	prefix = dgettext(OlMsgsDomain,"Resolution map error");


	msg = XtMalloc(
		  strlen(prefix)
		+ 2	/* --   */
		+ strlen(reason)
		+ 1	/* sp   */
		+ 1	/* sp   */
		+ 1	/* [    */
		+ strlen(str)
		+ 1	/* ]    */
		+ 1	/* nl   */
		+ 1	/* null */
	);

	sprintf (
		msg,
		"%s--%s %.*s [%s]\n",
		prefix,
		reason,
		offset,
		str,
		str + offset
	);
	OlWarning (msg);
	XtFree (msg);
}

static ResolutionItem *
parse_resolution_map (Screen *screen, char *str)
{
	char			*str_copy,
				*pair,
				*p;

	ResolutionItem		*ret		= 0;


	if (!str)
		OlError (dgettext(OlMsgsDomain,
			"Internal error: No resolution map to parse!\n"));

	/*
	 * Need a copy of the string, since "strtok()" is destructive.
	 * (Need the copy only for reporting errors.)
	 */
	str_copy = Strdup(p = str = strip(str));

	if (*p == '+') {
		char			*def = OL_DEFAULT_RESOLUTION_MAP;


		p++;

		/*
		 * Get the list of default resolution maps.
		 * To avoid endless loops, make sure there are
		 * no leading "+" characters!
		 */
		while (*def == '+')
			def++;
		ret = parse_resolution_map(screen, def);
	}

	/*
	 * Step through each resolution ``pair'' (HxV).
	 */
	while ((pair = strtok(p, ","))) {
		char			*x,
					*rest;

		ResolutionItem		*p_item;


		p = 0;	/* for subsequent calls to "strtok()" */

		if (!(x = strchr(pair, 'x'))) {
			res_complain (
				"missing \"x\"?",
				str_copy,
				pair - str
			);
			continue;
		}
		*x++ = 0;

		p_item = XtNew(ResolutionItem);

		p_item->horz = strtol(pair, &rest, 10);
		if (*rest)
			res_complain (
				"bad number?",
				str_copy,
				rest - str
			);

		p_item->vert = strtol(x, &rest, 10);
		if (*rest == 'c') {
			p_item->horz *= 2.54;
			p_item->vert *= 2.54;
			rest++;
		}
		if (*rest != '=') {
			res_complain (
				"missing \"=\"?",
				str_copy,
				rest - str
			);
			continue;
		}

		if (!(p_item->character = *++rest)) {
			res_complain (
				"missing character?",
				str_copy,
				rest - str
			);
			continue;
		}
		if (rest[1])
			res_complain (
				"missing \",\"?",
				str_copy,
				rest + 1 - str
			);

		/*
		 * Link the resolution map into a list.
		 * The list is assumed to be small and order-independent.
		 * Thus we take it easy and link the new item at the
		 * head--this produces a backwards list.
		 */
		if (!ret)
			(ret = p_item)->next = 0;
		else {
			p_item->next = ret->next;
			ret->next = p_item;
		}
	}

	XtFree(str_copy);

	return (ret);
}

/*
 * search_map() - SEARCH RESOLUTION MAP FOR CLOSEST RESOLUTION
 */

static long
dist (register ResolutionItem *a, register ResolutionItem *b)
{
	register long		dhorz	= (a->horz - b->horz),
				dvert	= (a->vert - b->vert);


	return (dhorz * dhorz + dvert * dvert);
}

static ResolutionItem *
search_map (ResolutionItem *list, ResolutionItem item)
{
	long			best_distance,
				distance;

	ResolutionItem		*best_item;


	for (
		best_distance = dist((best_item = list), &item),
			list = list->next;
		list;
		list = list->next
	) {
		distance = dist(list, &item);
		if (distance < best_distance) {
			best_item = list;
			best_distance = distance;
		}
	}

	return (best_item);
}

