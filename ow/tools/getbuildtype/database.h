/*
**	Build info database
**	@(#)database.h	1.8 91/10/16
*/

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



#ifndef _database_h_
#define _database_h_

typedef struct DB {		/* Database handle */
	FILE	*file;
	char	*filename;	
	char	*fullname;
} DB;

typedef struct DB_OBJ {	/* Pointer to structure returned by getobjectbyname() */
	char *name;
	char *location;
	char *buildtype;
	char *linktype;
} DB_OBJ;

DB *
open_db( char *database_name, const char *mode );

int
close_db( DB *db );

DB_OBJ *
getobjectbyname( DB *db, char *object_name );

int
freeobject( DB_OBJ *obj );

#endif

