#ifndef lint
        static char sccsid[] = "@(#)uisschema.c	1.2\t06/11/93 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * uisschema.c - NetISAM Programmer Toolkit Utility Program
 *
 * Usage:
 *  uisstat  schemafile
 *
 * Description:
 *	Parse schema file 
 *
 * Change history:
 * 9/15/88 V. Matena - Emit one .h file and one .c for the entire schema
 */

#include <stdio.h>
#include <ctype.h>
#include "isam_impl.h"

extern FILE *fopen();
FILE 		*fph;
FILE 		*fpc;

main (argc,argv)
    int	  	argc;
    char  	**argv;
{
    int		emitfiles ();
    char	*filename;
    FILE	*fp;
    char	rootname [80];		     /* argv[1] with .q stripped */
    char	buf [80];
    int		slen;

    if (argc != 2) 
	usage ();

    filename = argv[1];

    if ((fp = fopen (filename,"r")) == (FILE *) 0) {
	perror (filename);
	exit (1);
    }

    /* Strip .q if any */
    (void)strncpy (rootname, filename, sizeof (rootname) - 3);
    rootname [sizeof (rootname) - 3] = '\0';
    slen = strlen (rootname);
    if (strcmp (rootname + slen - 2, ".q") == 0)
	rootname [slen - 2] = '\0';	     /* strip .q */

    /* Open .h file */
    sprintf (buf,"%s.h", rootname);
    if ((fph = fopen (buf,"w")) == (FILE *) 0) {
	perror (buf);
	exit (1);
    }

    /* Open .c file */
    sprintf (buf,"%s.c", rootname);
    if ((fpc = fopen (buf,"w")) == (FILE *) 0) {
	perror (buf);
	exit (1);
    }

    /* Include .h file in *.c file */
    (void)fprintf (fpc,"#include \"isam.h\"\n\n");
    (void)fprintf (fpc,"#include \"%s.h\"\n\n",rootname);

    isschema (fp,emitfiles);

    (void)fclose (fph);
    (void)fclose (fpc);
    exit(0);
}

static usage ()
{
    (void) fprintf (stderr, "Usage: uisschema schemafile\n");
    exit (1);
}

emitfiles (ptb)
    struct istableinfo *ptb;
{
    emithfile (ptb);
    emitcfile (ptb);
}


emithfile (ptb)
    struct istableinfo *ptb;
{
    char		capname [50];	     /* Table name in capitols */
    char		*tabname = ptb->tbi_tablename;
    register int	i;

    /* Make string in capitals. */
    (void)strcpy (capname, tabname);
    strtocaps (capname);

    /* Emit comment. */
    (void)fprintf (fph,"/*------------------ %s -------------------------*/\n",
		   tabname);

    /* Emit record length. */
    (void)fprintf (fph,"#define %s_RECLEN %d\n",capname,ptb->tbi_reclength);

    /* Emit user buffer structure. */
    (void)fprintf (fph,"\nstruct %s {\n",tabname);
    
    for (i = 0; i < ptb->tbi_nfields; i++) {
	emit_fld_emit (fph,ptb->tbi_fields + i);
    }
    (void)fprintf (fph,"};\n");

    /* Emit field descriptors. */
    (void)fprintf(fph,"\nextern struct isfldmap %s_f [];\n",tabname);

    /* Emit key descriptors. */
    (void)fprintf(fph,"extern struct keydesc %s_k [];\n",tabname);

    /* Make pointers by name to key descriptors. */
    for (i = 0; i < ptb->tbi_nkeys; i++) {
	
	/* don't declare dummy primary key. */
	if (i == 0 && ptb->tbi_keys[0].k_nparts == 0)
	    continue;

	(void)fprintf (fph,"#define %s (&%s_k [%d])\n",
		       ptb->tbi_keynames[i],tabname,i);
    }
    
    /* Declare table of keynames. */
    (void)fprintf(fph,"extern char *%s_kn [];\n",tabname);

    /* Declare table descriptor. */
    (void)fprintf(fph,"extern struct istableinfo %s_t;\n", tabname);

    (void)fputc ('\n',fph);
    (void)fputc ('\n',fph);
}

emitcfile (ptb)
    struct istableinfo *ptb;
{
    char		capname [50];	     /* Table name in capitols */
    char		*tabname = ptb->tbi_tablename;
    register int	i;

    /* Make string in capitals. */
    (void)strcpy (capname, tabname);
    strtocaps (capname);

    /* Emit comment. */
    (void)fprintf (fpc,"/*------------------ %s -------------------------*/\n",
		   tabname);

    /* Emit field descriptors. */
    (void)fprintf(fpc,"\nstruct isfldmap %s_f [] = {\n",tabname);
    for (i = 0; i < ptb->tbi_nfields; i++) {
	emit_fmap_emit (fpc,ptb->tbi_fields + i, tabname);
	if (i < ptb->tbi_nfields - 1)	
	    (void)fputc (',',fpc);
	(void)fputc ('\n',fpc);
    }
    (void)fprintf(fpc,"};\n");

    /* Emit key descriptors. */
    (void)fprintf(fpc,"\nstruct keydesc %s_k [] = {\n",tabname);
    for (i = 0; i < ptb->tbi_nkeys; i++) {
	emit_fkey_emit (fpc,ptb->tbi_keys + i);
	if (i < ptb->tbi_nkeys - 1)	
	    (void)fputc (',',fpc);
	(void)fputc ('\n',fpc);
    }
    /* handle special case if there no keys */
    if (ptb->tbi_nkeys == 0) {
	(void)fputc('0', fpc);
    }
    (void)fprintf(fpc,"};\n");

    /* Declare table of keynames. */
    (void)fprintf(fpc,"\nchar *%s_kn [] = {",tabname);
    for (i = 0; i < ptb->tbi_nkeys; i++) {
	(void)fprintf (fpc,"\"%s\"",ptb->tbi_keynames[i]);
	if (i < ptb->tbi_nkeys - 1)	
	    (void)fputc (',',fpc);
    }
    /* handle special case if there no keys */
    if (ptb->tbi_nkeys == 0) {
	(void)fputc('0', fpc);
    }
    (void)fprintf(fpc,"};\n");

    /* Declare table descriptor. */
    (void)fprintf(fpc,"\nstruct istableinfo %s_t = { \"%s\", %d, %d, %s_f, %d, %s_k, %s_kn };\n",
		  tabname, tabname, ptb->tbi_reclength,ptb->tbi_nfields,tabname,
		  ptb->tbi_nkeys, tabname, tabname);
    
    (void)fputc ('\n',fpc);
    (void)fputc ('\n',fpc);
}

strtocaps (s)
    register char	*s;
{
    while (*s) {
	if (islower (*s))
	    *s = toupper(*s);
	s++;
    }
}


static emit_fld_emit (fp, p)
    FILE		*fp;
    struct isfldmap 	*p;
{   
    char		*prtype();

    (void)fprintf(fp,"    %-8s  %s",prtype(p->flm_type),p->flm_fldname);

    if (p->flm_type == CHARTYPE) {
	(void)fprintf(fp,"[%d]",p->flm_length+1);
    }

    if (p->flm_type == BINTYPE) {
	(void)fprintf(fp,"[%d]",p->flm_length);
    }

    (void)fprintf (fp,";\n");
}

static emit_fmap_emit (fp, p, tabname)
    FILE		*fp;
    struct isfldmap 	*p;
    char		*tabname;
{   
    char		*prtype();
    char		fldnamebuf [30];

    (void)sprintf (fldnamebuf,"\"%s\"",p->flm_fldname);
    (void)fprintf(fp, "    { %-14s,%3d, (int) %s((struct %s *)0)->%s,%3d,%3d }",
		  fldnamebuf,p->flm_recoff,
		  (p->flm_type==CHARTYPE ||p->flm_type==BINTYPE)?"":"&",
		  tabname,p->flm_fldname,p->flm_type,p->flm_length);
}

static emit_fkey_emit (fp, p)
    FILE		*fp;
    struct keydesc 	*p;
{   
    char		*prtype();
    struct keypart	*pkp;
    int			i;

    (void)fprintf(fp, "    { %2d,%2d", p->k_flags,p->k_nparts);

/* need to put {... } to init the structure  for ANSI C */
    
    if (p->k_nparts > 0)
    {
    (void)fprintf(fp, ", {" );
    for (i = 0; i < p->k_nparts; i++) {
	pkp = p->k_part + i;
        if (i == 0)
	   (void)fprintf(fp, "{%d,%d,%d}", pkp->kp_start, pkp->kp_leng, 
		      pkp->kp_type);
 	else 
	   (void)fprintf(fp, ",{%d,%d,%d}", pkp->kp_start, pkp->kp_leng, 
		      pkp->kp_type);
	
    }
    (void)fprintf(fp, "}" );
    }

    (void)fprintf (fp,"}");
}

static char *prtype (typ)
    int		typ;
{
    switch (typ) {
    case LONGTYPE:
	return ("long");
    case SHORTTYPE:
	return ("short");
    case CHARTYPE:
	return ("char");
    case BINTYPE:
	return ("unsigned char");
    case FLOATTYPE:
	return ("float");
    case DOUBLETYPE:
	return ("double");
    default:
	return ("UNKNOWN-TYPE");
    }
}
