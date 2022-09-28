/* @(#)debug.h	3.1 - 92/04/03 */


/* debug.h -- a common way to do debugging output */

#ifndef debug_h_
#define debug_h_

/* This file defines a common debug output method.
**
** Basically, you embed lines that look like the following examples
** in your code:
**
**	DP(( "main_routine: ax is %d", ax ));
**
** The key points: you need double parentheses around the insides of the DP.
** If DEBUG is not defined, nothing will be printed.
**
** If you also want run time control over you output, define the variable
** DEBUG_FLAG before you include this file.  for example:
**
** #define DEBUG_FLAG debug_video
** #include "debug.h"
**
** will only print things if debug_video is true.  You should also
** update debugcomm.h to allow people to set the flag from a menu.
**
*/


#ifdef DEBUG

#ifndef DEBUG_FLAG
#define DEBUG_FLAG	(1)
#endif !DEBUG_FLAG

#define DP(a)	{if( DEBUG_FLAG ) printf a ; }
#define MP(a)
#else DEBUG
#define DP(a)
#define MP(a)
#endif DEBUG


#endif debug_h_
