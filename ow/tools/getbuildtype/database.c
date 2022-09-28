#pragma ident "@(#)database.c	1.19 92/07/17	SMI"

/*
**   ----------------------------------------------------------------- 
**          Copyright (C) 1986,1990  Sun Microsystems, Inc
**                      All rights reserved. 
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**   Use, duplication, or disclosure by the Government is subject 
**   to restrictions as set forth in subparagraph (c)(1)(ii) of 
**   the Rights in Technical Data and Computer Software clause at 
**   DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**   FAR Supplement. 
**   ----------------------------------------------------------------- 
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "database.h"
#include "debugger.h"

extern char *expandmacrosin(char *thislib);

static void
commatospace( char *string )
{
        char *cp;

        while ( (cp = strchr(string, ',')) )
                *cp = ' ';
}


DB *
open_db( char *database_name, const char *mode )
{
	FILE		*file;
	DB		*local_db_handle;
	static char	*fullname,
			 fnargs[FNARGSLEN];

	(void)sprintf(fnargs,"open_db(%s,%s)", database_name, mode);
	Denter(fnargs);

	fullname = expandmacrosin( database_name );

	if ( (file = fopen( fullname, mode )) == NULL) {
		Dprintf(TRACE_WARNING,"Could not open \"%s\"", fullname);
		RETURN(NULL);
	} else {
		local_db_handle           = (DB *)malloc(sizeof( struct DB ));
		local_db_handle->file	  = file;
		local_db_handle->filename = strdup(database_name);
		local_db_handle->fullname = strdup(fullname);
		RETURN(local_db_handle);
	}
}


int
close_db( DB *db )
{
	static char fnargs[FNARGSLEN];

	if (db) {
		(void)sprintf(fnargs,"close_db(%s)", db->filename);
		Denter(fnargs);
		if (db->filename) {
			free(db->filename);
			db->filename = NULL;
		}
		if (db->fullname) {
			free(db->fullname);
			db->fullname = NULL;
		}
		free(db);
	} else {
		Denter("close_db(NULL)");
	}
	RETURN(0);
}


static char    line[BUFSIZ+1];
static char    loc [BUFSIZ+1];
static char    bt  [BUFSIZ+1];
static char    lt  [BUFSIZ+1];

DB_OBJ *
getobjectbyname( DB *db, char *object_name )
{
	DB_OBJ		*object;
	static char	 fnargs[FNARGSLEN];
	int		 linenumber;
 
        if (!db) {
		(void)sprintf(fnargs,
			      "getobjectbyname(db(NULL), object_name=%s)",
			      object_name);
		Denter(fnargs);
                RETURN(NULL);
	}
	(void)sprintf(fnargs,
		      "getobjectbyname(db(%s), object_name=%s)",
		      db->filename, object_name);
	Denter(fnargs);

	rewind(db->file);
	linenumber = 0;
	*loc = '\0';
	*bt  = '\0';
	*lt  = '\0';
        while (fgets(line, BUFSIZ, db->file) != NULL) {
		linenumber++;
                if (strchr(line, '\n') == NULL) {
                        while (strchr(line, '\n') == NULL) {
                                if (fgets(line, BUFSIZ, db->file) == NULL) {
                                        break;
                                }
                        }
                        continue;
                }
		if ((*line == '\n') || (*line == '#'))
			continue;
		if (strncmp(line, "include", strlen("include")) == 0) {
			DB	*newdb;
			char	*cp,
				*error,
				 filename[127];
			int	 pos = 0;

			pos = strlen("include");
			cp  = &line[pos];
			while (cp && ((*cp == ' ') || (*cp == '\t'))) {
				cp++;
				pos++;
			}
			if (*cp != '<') {
				error = "No opening '<'";
				goto badinclude;
			}
			cp++;
			pos++;
			(void)strcpy(filename, cp);
			if ( (cp = strchr(filename, '>')) == NULL) {
				error = "No matching '>'";
				goto badinclude;
			}
			pos++;
			*cp = '\0';
			/* we have a valid filename - now try it... */
			if ( (newdb = open_db(filename, "r")) == NULL) {
				error = "File not found";
				goto badinclude;
			}
			object = getobjectbyname( newdb, object_name );
			close_db(newdb);
			if (object) {
				RETURN(object);
			} else {
				continue;
			}
badinclude:
			cp = strchr(line,'\n');
			if ( cp ) {
				*cp = '\0';
			}
			if (TRACE_WARNING & debug_state) {
			    (void)fprintf(stderr,"\n");
			    (void)fprintf(stderr,
				"Error while processing include directive\n");
			    (void)fprintf(stderr,
			    	"File: %s, line %d looks like this:\n",
			 	db->filename, linenumber);
			    (void)fprintf(stderr,"\"%s\"\n ", line);
			    for(;pos; pos--) {
			 	(void)fprintf(stderr," ");
			    }
			    (void)fprintf(stderr," ^ Error: %s\n", error);
			}
			RETURN(NULL);
		}
		if (   (strncmp(object_name, line, strlen(object_name)) == 0)
		    && (   (line[strlen(object_name)] == ' ')
		        || (line[strlen(object_name)] == '\t') ) ) {
			/*
			** At this point we have the line we need in the form:
			** name    location    buildtype    linktype
			*/

			(void)sscanf(line,"%*s %s %s %s", loc, bt, lt);

			object            = (DB_OBJ *)
					    malloc(sizeof(struct DB_OBJ));
			object->name      = strdup(object_name);
			object->location  = strdup(loc);
			object->buildtype = strdup(bt);
			object->linktype  = strdup(lt);

			commatospace(object->buildtype);
			commatospace(object->linktype);

			Dprintf(TRACE_DB, "Found: %s %s %s %s\n",
				object->name,
				object->location,
				object->buildtype,
				object->linktype);
			RETURN(object);
		}
        }
        RETURN(NULL);
}


int
freeobject( DB_OBJ *obj )
{
        static char fnargs[FNARGSLEN];

        if (!obj) {
                Denter("freeobject(NULL)");
                RETURN(1);
        }
        (void)sprintf(fnargs,"freeobject(%s)", obj->name);
        Denter(fnargs);
 
        if (obj->name)          free(obj->name);
        if (obj->location)      free(obj->location);
        if (obj->buildtype)     free(obj->buildtype);
        if (obj->linktype)      free(obj->linktype);
        free(obj);
        RETURN(0);
}
