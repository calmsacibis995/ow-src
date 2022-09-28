/*static char *sccsid="@(#) ftfq1def.h (1.3)  14:21:05 07/08/97";

   ftfq1def.h -- Q1 filter

  | Who Rel  Date     | Comment
  |-------------------|---------------------------------
  |-------------------|---------------------------------

  | Fulcrum Ful/Text Copyright (c) Fulcrum Technologies Inc. 1984 - 1990.
  | All rights strictly reserved.
  | This is an unpublished work; reproduction, disclosure, or re-creation
  | of embodied computer programs or algorithms is prohibited.    */

/* Flags used in determining which columns the calling program needs     */
/* the "PARM" flags are the flags which the user will pass ORed 	*/
/* into the colflags parameter when cs_read is called.		    */

#define PARM_ISO	1
#define PARM_TYP	2
#define PARM_OCASE	2|4
#define PARM_CLSEQ	8
#define PARM_DECTB	16
#define PARM_SPELL	32

#define UNKNOWN		-2
#define FROMCHAR	1
#define ISO		2

struct GSET {
	int numelems;		/* How many elements are in this gset.     */
				/* If numelems > CUTOFF then fully alloc'd */
	char  *  g_fromchar;
	short *  g_type;	/* includes opposite case gset number */
	char  *  g_dectab;	/* == NULL if char is not dectab delimiter */
	char  *  g_opcase;
	short *  g_colseq;
	short *  g_spell;	/* speller code */
	char  *  g_iso1;	/* ISO contains two bytes  */
	char  *  g_iso2;
};

#define GOULD 1

#ifdef PC
#define CUTOFF 64
#endif
#ifdef FTHMSDOS
#define CUTOFF 64
#endif
#ifndef CUTOFF
#define CUTOFF 0
#endif
/* Where CUTOFF is the number of characters we need for each Gset before */
/* fully allocating the table.  This allows for memory vs. speed tuning. */

/* Flags used in determining which columns the calling program needs     */
#define FL_ISO		1
#define FL_TYP		2 
#define FL_OCASE	4
#define FL_CLSEQ	8
#define FL_DECTB	16
#define FL_SPELL	32

#define MAXGSETS	4
#define MAXGSIZ		128
#define MAXLIN		80
#define MAXOCT   	017000
#define SIZTBUF		100
#define CHSIZ		sizeof(char)
#define WRDLEN		sizeof(short int) / 2
#define CMTCHAR		'#'
#define TAB		'\t'
#define PBREAK		'\f'
#define DTBCHR		't'

#ifndef FTHMSDOS
#define TERM_DFL 	"/usr/qlib/files"
#else
#define TERM_DFL 	"files"
#endif /*FTHMSDOS*/

struct GSET coll_gs[MAXGSETS];		/* Gsets. */

/*
 * The data file is delimited with the following special characters
 * within the text portion of the document. All text is ASCII with the
 * normal meaning for tab, newline, and form feed. A newline preceeded
 * with a Cwordw indicates the soft type of wordwrap break, a form feed
 * preceeded with a Csoftpg indicates the soft type of page break.
 */

#define Cstatl 0201
#define Csreturn 0202	/* soft return , leaves indent on */
#define Cfrmt 0203	/* start format,strings  Cend */
#define Chedr 0204	/* start of header page end with \f */
#define Cfoot 0205	/* start of footer page end with \f */
#define Cwork 0206	/* start of work page end with \f */
#define Cignore 0207	/* ignore this character on rewrap */
#define Cundon 0210	/* underline on */
#define Cundof 0211	/* underline off */
#define Cdblon 0212	/* double underline on */
#define Cdblof 0213	/* double underline off */
#define Cbldon 0214	/* bold on */
#define Cbldof 0215	/* bold off */
#define Covron 0216	/* overstrike on */
#define Covrof 0217	/* overstrike off */
#define Cat1on 0220	/* attr 1 on */
#define Cat1of 0221	/* attr 1 off */
#define Cat2on 0222	/* attr 02 on */
#define Cat2of 0223	/* attr 02 off */
#define Csuper 0224	/* superscript */
#define Csubsr 0225	/* subscript */
#define Cindent 0226	/* indent */
#define Cwordw 0227	/* word wrap */
#define Ccenter 0230	/* center */
#define Cdectab 0231	/* decimal tab */
#define Cmrgon 0232	/* merge on */
#define Cmrgof 0233	/* merge off */
#define Cnote 0234	/* note */
#define Chyphen 0235	/* discretionary hyphen */
#define Cfxsp 0236	/* fixed space */
#define Cgnhyp 0237	/* generated hyphen */
#define Cgnind 0240	/* generated indent */
#define Csoftpg 0241	/* soft page break */
#define Cmulti 0242	/* multi char sequence */
#define Cmulnd 0243	/* multi char sequence end */
#define Cgformat 0244	/* global format callout */
#define Cend 0245	/* char used to end any ambiguous numeric string */
#define Cspell 0246	/* word following is misspelled */
#define Cspend 0247	/* end of misspelled word */
#define Ccentab 0250	/* center about a tab stop */
#define Ctabrig 0251	/* right justified tab */
#define Clasted 0252	/* last edit page # Cend */
#define Cdottab 0253	/* dot leader tab */
#define Cdotdec 0254	/* dot leader dectab */
#define Cdotcen 0255	/* dot leader center tab */
#define Cdotrig 0256	/* dot leader right tab */
#define Crevdir 0257	/* toggle fonts(0 - 1) switch direction */
#define Cstatr 0260	/* right half of magic # */
#define Cxformat 0261	/* graphic representation callout */
#define Cscolumn 0262	/* soft column break */
#define Chcolumn 0263	/* hard column break */
#define Ceformat 0264	/* end of graphic callout */
#define Cfootnote	0265	/* footnote callout */
#define Cfootend	0266	/* end of footnote callout */
#define Cautonum	0267	/* automatic # specifier */
#define Cautoend	0270	/* automatic # specifier end */
#define Cvoice		0271	/* start of voice file specifier */
#define Cvoicend	0272	/* end of voice file specifier */
#define Ctable		0273	/* start of table specification */
#define Ctablend	0274	/* end of table */
#define Ceqn		0275	/* start of math equation */
#define Ceqend		0276	/* end of math equation */
#define Creserved	0277	/* private use for qprint do-no-use */
/*
 * 0300 - 0367 are for font switches
 */
#define Cfont0		0300	/* font 0 */
#define Cfontmax	0367	/* largest font number */
#define CG0		0370	/* switch to G0 char set */
#define CG1		0371	/* switch to G1 char set */
#define CG2		0372	/* switch to G2 char set */
#define CG3		0373	/* switch to G3 char set */
#define CG4		0374	/* following four are reserved */
#define CG5		0375
#define CG6		0376
#define CG7		0377

#define Cmagic ((Cstatl <<8) + Cstatr)
#define Cmagical ((Cstatr <<8) + Cstatl)
