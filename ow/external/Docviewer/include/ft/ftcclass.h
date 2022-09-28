/*static char *sccsid="@(#) ftcclass.h (1.3)  14:21:02 07/08/97";

   ftcclass.h -- define character classes for user interface

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  | wtr 5.1A 91/05/14 | Get rid of nested comments
  | bf  5.0D 90/10/01 | WANTSTAT
  | bf  5.0D 90/09/28 | static CONST ftcclass
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

#define FTCRG	1	/* regular character */
#define FTCTB	2	/* tab */
#define FTCSP	3	/* space */
#define FTCEP	4	/* end of paragraph */
#define FTCNL	5	/* newline */
#define FTCFF	6	/* form feed */
#define FTCEF	7	/* end of file */
#define FTCSI	8	/* escape sequence introducer (CSI, ESC) */
#define FTCHY	9	/* hyphen */
#define FTCUS	10	/* underscore */
#define FTCBS	11	/* backspace */
#define FTCAC	12	/* accent character */
#define FTCN0	13	/* null character (always ignored) */

#define FTCCLASS(c)	((c == EOF) ? FTCEF : ftcclass[c])

#ifdef	WANTTAB
#ifdef	WANTSTAT
static
#endif
CONST char	ftcclass[256] = {
/*		00	01	02	03	04	05	06	07 */
/*		08	09	0a	0b	0c	0d	0e	0f */
/* -------------------------------------------------------------------------- */
/* 00 */	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
		FTCBS,	FTCTB,	FTCNL,	FTCN0,	FTCFF,	FTCN0,	FTCN0,	FTCN0,
/* 10 */	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
		FTCN0,	FTCN0,	FTCN0,	FTCSI,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
/* 20 */	FTCSP,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* 30 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* 40 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* 50 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCUS,
/* 60 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* 70 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCN0,
/* 80 */	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCEP,	FTCN0,	FTCN0,
		FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
/* 90 */	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
		FTCN0,	FTCN0,	FTCN0,	FTCSI,	FTCN0,	FTCN0,	FTCN0,	FTCN0,
/* a0 */	FTCN0,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* b0 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* c0 */	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,
		FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,	FTCAC,
/* d0 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* e0 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
/* f0 */	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,
		FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCRG,	FTCN0
};

#else
extern char	ftcclass[];
#endif /* WANTTAB */

