/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audiotool.c	1.10	93/02/17 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <sys/types.h>

#include <xview/xview.h>

#include "atool_panel_impl.h"
#include "atool_panel.h"
#include "atool_i18n.h"

char	*I18N_Message_File = I18N_DOMAIN;

main(argc, argv)
	int		argc;
	char		**argv;
{
	caddr_t		pdata;	/* panel data */
	char		*Prog;	/* name of program */
	char		bind_home[MAXPATHLEN+1];	/* locale dir */

	/* Get the program name */
	Prog = strrchr(argv[0], '/');
	if (Prog == NULL)
		Prog = argv[0];
	else
		Prog++;

	/* I18N init */
	ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
	bindtextdomain(I18N_Message_File, bind_home);
	textdomain(I18N_Message_File);

	/* Initialize the panel */
	if ((pdata = AudPanel_Init(NULL, &argc, argv)) == NULL) {
		fprintf(stderr, 
		    MGET("%s: could not create audio panel\n"), Prog);
		exit(1);
	}

	/* Turn control over to XView */
	xv_main_loop((Xv_opaque)AudPanel_Gethandle(pdata));
	fflush(stderr);
	exit(0);
}
