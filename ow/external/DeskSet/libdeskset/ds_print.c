#ifndef lint
static 	char sccsid[] = "@(#)ds_print.c Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/********************************************************************
*
*       Function:	ds_def_printer
*
*       Description:  	get the default printer name for SVR4 	
*
*       Parameters:     none     
*		
*       Returns:   	char * (printer name)        
*		
*
*********************************************************************/
/* Parse output of "lpstat -d"
 * The default printer name is the string after the colon.
 */
#include <stdio.h>

extern char*
ds_def_printer()
{
        FILE *fp;
        char message[257];
	char *tmp=NULL;
        char *printer_name=NULL;

#ifdef SVR4
	tmp = (char*)getenv("LPDEST");
	if (tmp != NULL && *tmp != NULL) {
		printer_name = (char*)malloc(strlen(tmp)+1);
		strcpy(printer_name, tmp);
	}
	else {
        	fp = popen("lpstat -d", "r");
        	fread(message, 256, 1, fp);
        	tmp = (char *)strtok(message, ":");
        	tmp = (char *)strtok((char *)NULL, "\n");
		if (tmp != NULL && *tmp != NULL) {
			printer_name = (char*)malloc(strlen(tmp)+1);
			strcpy(printer_name, tmp);
		}
		else {
			printer_name = (char*)malloc(3);
			strcpy(printer_name, "lp");
		}
	}
#else
	tmp = (char*)getenv("PRINTER");
	if (tmp != NULL && *tmp != NULL) {
		printer_name = (char*)malloc(strlen(tmp)+1);
		strcpy(printer_name, tmp);
	}
	else {
		printer_name = (char*)malloc(3);
		strcpy(printer_name, "lw");
	}
#endif
	return printer_name;
}

