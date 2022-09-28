/*
**   ----------------------------------------------------------------- 
**          Copyright (C) 1990  Sun Microsystems, Inc
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


#pragma ident "@(#)getbuildtype.c	1.27 92/07/06	SMI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "debugger.h"
#include "define.h"

static char *expand(char *thislib);
extern char *expandmacrosin(char *thislib);
static char *basename(char * pathname);
static char *libnameof(char * thislib);

#define EMPTY_STRING	    ""

#define DEFAULT_TYPE        "optimized"
#define DEFAULT_DATABASE    "~/.xnews_config.db"
#define DEFAULT_PREFIX      ""
#define DEFAULT_OBJECT      ""
#define DEFAULT_OBJDIR      "."
#define DEFAULT_SOURCEDIR   "../.."
#define DEFAULT_DESTDIR     ""

#define EXT_NONE	    ""
#define EXT_OPT		    ""
#define	EXT_DEBUG	    "_d"
#define EXT_GPROF	    "_p"
#define EXT_TOOLOPT	    "_t"
#define	EXT_TOOLDEBUG	    "_td"
#define EXT_TOOLGPROF	    "_tp"
#define EXT_NOBUILD	    EXT_OPT

#define MODE_GETBUILD	'G'
#define MODE_PROCLIBS	'P'
#define MODE_LIBDEPEND	'D'
#define MODE_LINTLIBS	'L'

#define LIBBASENAMELEN  255		/* Max len of a library filename */

static char	mode;
static char    *default_type;  /* If this name isn't found in the DB, use this*/
static char    *objdir;        /* The name of the object subdir (DIR.target)  */
static char    *srcroot;       /* $SOURCEDIR or sourcedir                     */
static char    *destroot;      /* $DESTDIR or destdir                         */
static DB      *db;            /* Database handle                             */

main( int argc, char **argv) {
	int      ch;            /* for processing command line args           */
	int      errflg = 0;    /* error indication for cmd line processing   */
	int      ai;		/* Integer form of the argument (for -x)      */
	char    *prog;          /* The name of this program                   */
	char    *database_name; /* Use this as the name of the database       */
	char    *prefix;	/* The prefix attached to the buildtype       */
	char    *object_name;   /* The name to look up in the database        */
	char    *sourcedir;     /* The name of the top of the source tree     */
	char    *destdir;       /* The name of the top of the install tree    */
	DB_OBJ  *object;        /*Pointer to structure returned by lookup_db()*/
	
#ifndef SVR4
	int getopt(int, char *const *, const char *);
	
	extern char	*optarg;
	extern int	 optind,
			 opterr,
			 optopt;
#endif /* SVR4 */

	default_type    = DEFAULT_TYPE;
	database_name   = DEFAULT_DATABASE;
	prefix          = DEFAULT_PREFIX;
	object_name     = DEFAULT_OBJECT;
	objdir          = DEFAULT_OBJDIR;
	sourcedir       = DEFAULT_SOURCEDIR;
	destdir         = DEFAULT_DESTDIR;
	debug_state     = 0;

	opterr          = 0;    /* no messages from getopt()                  */

	prog = basename(argv[0]);
	mode = MODE_GETBUILD;

	while ((ch = getopt(argc, argv, "gplLx:D:P:S:O:d:f:n:")) != -1) {
		switch (ch) {
		case 'g': mode          = MODE_GETBUILD;	    break;
		case 'p': mode          = MODE_PROCLIBS;	    break;
		case 'l': mode          = MODE_LIBDEPEND;	    break;
		case 'L': mode          = MODE_LINTLIBS;	    break;
		case 'x': ai = atol(optarg);
			  if (ai) debug_state |= ai;
			  else    debug_state  = 0;		    break;
		case 'D': destdir       = optarg;		    break;
		case 'P': prefix        = optarg;		    break;
		case 'S': sourcedir     = optarg;		    break;
		case 'O': objdir        = optarg;		    break;
		case 'd': default_type  = optarg;		    break;
		case 'f': database_name = optarg;		    break;
		case 'n': object_name   = optarg;		    break;
		case '?': errflg++;             		    break;
		}
	}

	if (errflg) {
		(void)fprintf(stderr, "usage: %s [-gplLx] [-D destdir] [-S sourcedir] [-O objdir] [-f db] [-d default] -n name\n", prog);
		(void)fprintf(stderr, "       -g -> get build type\n");
		(void)fprintf(stderr, "       -p -> process library names\n");
		(void)fprintf(stderr, "       -l -> generate library dependencies\n");
		(void)fprintf(stderr, "       -L -> generate lintlib dependencies\n");
		(void)fprintf(stderr, "       -x -> set debugging level\n");
		(void)fprintf(stderr, "       -P -> prefix added to buildtype - defaults to \"\"\n");
		(void)fprintf(stderr, "       -D -> value for ${CONFIG.Destdir} macro in db file\n");
		(void)fprintf(stderr, "       -S -> value for ${SOURCEDIR} macro in db file\n");
		(void)fprintf(stderr, "       -O -> value for ${DIR.targetarch} macro in db file\n");
		(void)fprintf(stderr, "       If not specified, -f is \"%s\" and -d is \"%s\"\n",
		    DEFAULT_DATABASE, DEFAULT_TYPE);
		Dleave("Usage error");
		exit (1);
	}

	if ( (srcroot = getenv("SOURCEDIR")) == NULL) {
		srcroot = sourcedir;
	}
	if ( (destroot = getenv("CONFIG.Destdir")) == NULL) {
		destroot = destdir;
	}

	newlevel();	/* init the symbol table */
	define( "SOURCEDIR",      srcroot );	/* top of source tree */
	define( "CONFIG.Destdir", destroot);	/* top of destdir tree */
	define( "DIR.targetarch", objdir  );	/* Dir. that holds OBJ files */

	if ( (db = open_db(database_name, "r")) == NULL ) {
		Dprintf(TRACE_WARNING,"Could not open db: %s\n", database_name);
		if ( (db = open_db(DEFAULT_DATABASE, "r")) == NULL) {
			Dprintf(TRACE_WARNING,"Could not open db: %s\n",
				DEFAULT_DATABASE);
		} else {
			define("DatabaseName", DEFAULT_DATABASE);
		}
	} else {
		define("DatabaseName", database_name);
	}

	if (mode == MODE_PROCLIBS) {			/* processlibs */
		Denter("Process Libraries");
		define("LIBPREFIX",	"lib");
		define("LIBSUFFIX",	".a");
		define("optimized",	EXT_OPT);
		define("opt",		EXT_OPT);
		define("debug",		EXT_DEBUG);
		define("gprof",		EXT_GPROF);
		define("tool",		EXT_TOOLOPT);
		define("tooloptimized",	EXT_TOOLOPT);
		define("tooldebug",	EXT_TOOLDEBUG);
		define("toolgprof",	EXT_TOOLGPROF);
		for (; optind < argc; optind++) {
			(void)fprintf(stdout, "%s\t", expand(argv[optind]));
		}
		Dleave("");
	} else if (mode == MODE_LINTLIBS) {		/* processlibs */
		Denter("Process Lint Libraries");
		define("LIBPREFIX",	"llib-l");
		define("LIBSUFFIX",	".ln");
		define("optimized",	EXT_NONE); /* lintlibs are the same */
		define("opt",		EXT_NONE); /* lintlibs are the same */
		define("debug",		EXT_NONE);
		define("gprof",		EXT_NONE);
		define("tool",		EXT_NONE);
		define("tooloptimized",	EXT_NONE);
		define("tooldebug",	EXT_NONE);
		define("toolgprof",	EXT_NONE);
		for (; optind < argc; optind++) {
			char *cp;
			char *expanded_name = expand(argv[optind]);
			cp = &expanded_name[strlen(expanded_name)-2];
			/*
			** We don't want libname.a files while linting
			*/
			if ( *cp != '.' && *(cp+1) != 'a' ) {
			    /*
			    ** If it is in the form -llibname
			    ** or it is the pathname of an existing llib-*
			    **  lint library, use it
			    */
			    if (*expanded_name == '-' ||
				(debug_state & TRACE_MACROS) ||
				access( expanded_name, R_OK|F_OK) == 0) {
				(void)fprintf(stdout, "%s\t", expanded_name);
			    }	    /* endif -llib or good path/libname.ln */
			}	/* endif  no *.a files */
		}	/* end for(all arguments) */
		Dleave("");
	} else if (mode == MODE_LIBDEPEND) {            /* libdepend */
		Denter("Process Dependencies on Libraries");
		define("LIBPREFIX",	"lib");
		define("LIBSUFFIX",	".a");
		define("optimized",	EXT_OPT);
		define("opt",		EXT_OPT);
		define("debug",		EXT_DEBUG);
		define("gprof",		EXT_GPROF);
		define("tool",		EXT_TOOLOPT);
		define("tooloptimized",	EXT_TOOLOPT);
		define("tooldebug",	EXT_TOOLDEBUG);
		define("toolgprof",	EXT_TOOLGPROF);
		for (; optind < argc; optind++) {
			char *lib = expand(argv[optind]);
			if (*lib != '-') {
				(void)fprintf(stdout, "%s ", lib);
			}
		}
		Dleave("");
	} else if (mode == MODE_GETBUILD) {             /* getbuildtype */
		char	 dt[80],
			*comma,
			*part;

		Denter("Query build type information");
		(void)strcpy(dt, prefix);
		(void)strcat(dt, default_type);

		if ( object_name == NULL) {
			(void)fprintf(stdout, "%s\n", dt);
			Dprintf(TRACE_WARNING,"No object specified\n");
			exit(0);
		}
		if (!db) {
			(void)fprintf(stdout, " %s\n", dt);
		} else if ( (object = getobjectbyname(db,object_name))==NULL) {
			(void)fprintf(stdout, " %s\n", dt);
		} else if ( *(object->buildtype) == '\0' ||
		            *(object->buildtype) == '-'  ||
		            strcmp(object->buildtype, "nobuild") == 0) {
			(void)fprintf(stdout, "%s\n", prefix);
		} else if ( (comma=strchr(object->buildtype, ',')) != NULL ) {
			part = object->buildtype;
			do {
				*comma = '\0';
				if (strcmp(part,"opt")== 0) {
					part="optimized";
				}
				(void)fprintf(stdout, "%s%s ", prefix, part);
				*comma = ',';
				part   = comma+1;
			} while (comma = strchr(part, ','));
			(void)fprintf(stdout, "%s%s\n", prefix, part);
		} else {
			part = object->buildtype;
			if (strcmp(part,"opt")== 0) {
				part="optimized";
			}
			(void)fprintf(stdout,"%s%s\n",prefix,part);
		}
		Dleave("");
	} else {
		Dprintf(TRACE_WARNING,"Unknown mode\n");
	}


	if (close_db(db) == -1) {
		Dprintf(TRACE_WARNING,"close_db(\"%s\")=-1\n", database_name);
	}
	endlevel();
	RETURN(0);
}

static char *
expand( char * thislib )
{
	char *lib;
	static char fnargs[FNARGSLEN];

	(void)sprintf(fnargs, "expand( thislib=\"%s\" )", thislib);
	Denter(fnargs);
	newlevel();	/* gsave :-) */
	lib = libnameof( thislib );
	lib = expandmacrosin( lib );
	/* if ( access(lib,...) == -1) lib = thislib */
	endlevel();	/* grestore :-) */
	sRETURN(lib);
}

/*
**	Returns the (partially) interpreted info from the database
**	about what the expanded/modified library name should be
**
**	This name still needs to be Macro expanded
**
**	If so indicated in the DB, for each library
**	convert from the form -llibname
**	to either  ../../lib/name/Obj/liblibname.a
**	where
**	   ../..  is the argument to -S,
**	   name   is found in the build DB, and
**	   liblibname.a is derived from the DB
**	or "some arbitrary name", depending on the
**	contents of the DB entry
*/
static char *
libnameof( char * thislib ) {
	char lib_basename[LIBBASENAMELEN];
	static char buffer[FILENAME_MAX];
	static char fnargs[FNARGSLEN];
	DB_OBJ  *object;       /* Pointer to structure returned by lookup_db()*/

	(void)sprintf(fnargs,"libnameof( thislib=\"%s\" )", thislib);
	Denter(fnargs);
	if (!db) {
		sRETURN(thislib);
	} else {											 /* -lname */
		if ( *thislib == '-' && *(thislib+1) == 'l'){
			(void)strcpy(lib_basename, thislib+2);
		} else {										 /* libfoo.a */
			(void)strcpy(lib_basename, thislib+3);
			lib_basename[strlen(lib_basename)-2] = '\0';
		}

														 /* not found */
		if ( (object = getobjectbyname(db, lib_basename)) == NULL) {
			sRETURN(thislib);
		} else {
			char *newlibname;
			char *directory;

			/* libname location buildtype linktype */

			/* we can have the following scenarios:
			**    foo bar optimized /path/to/baz.a
			**    foo bar optimized optimized
			**    foo bar optimized
			**    foo bar nobuild
			**    foo bar nobuild variants
			**    foo bar nobuild optimized
			**    foo bar
			**    foo bar variants -lbaz
			**    foo bar variants -
			**    foo bar variants /path/to/baz.a
			**    foo bar variants optimized
			**    foo bar variants
			*/

			if (   strlen(object->linktype)             == 0
			    || strcmp(object->linktype, "variants") == 0)
			{
				if (strcmp(object->buildtype, "nobuild") == 0) {
					directory  = "${FULL_OW_BUILD}";
					newlibname = default_type;
				} else {
					directory  = "${SOURCEDIR}";
					newlibname = default_type;
				}
			} else {
				directory  = "${SOURCEDIR}";
				newlibname = object->linktype;
			}
			if (   strcmp(newlibname, "variants") == 0 
			    || strcmp(newlibname, "nobuild")  == 0)
			{
				newlibname = default_type;
			}

			if (strncmp(default_type, "tool", 4) == 0) {/*override*/
				newlibname = default_type;
			}


			define("LIBBASE",  lib_basename);
			define("PATHNAME", directory);
			define("LOCATION", object->location);
			define("LIBNAME",  "${LIBPREFIX}${LIBBASE}${LIBBUILDSUFFIX}${LIBSUFFIX}");
			define("LIBRARY",  "${PATHNAME}/${LOCATION}/lib${LIBBASE}/${DIR.targetarch}/${LIBNAME}");
			define("LIBBUILDSUFFIX",  "");


	/* -lfoo */	if (*newlibname == '-' && *(newlibname+1) == 'l') {
				(void)strcpy(buffer, newlibname);
	/* - */		} else if (strcmp(newlibname, "-") == 0) {
				(void)strcpy(buffer, thislib);
	/* optimized */	} else if((strcmp(newlibname, "optimized") == 0) ||
	/* opt */	          (strcmp(newlibname, "opt")       == 0) ||
	/* debug */	          (strcmp(newlibname, "debug")     == 0) ||
	/* gprof */	          (strcmp(newlibname, "gprof")     == 0) ||
	/* tool */	          (strcmp(newlibname, "tool")      == 0) ||
	/* tooloptimized */       (strcmp(newlibname, "tooloptimized") == 0) ||
	/* tooldebug */	          (strcmp(newlibname, "tooldebug") == 0) ||
	/* toolgprof */	          (strcmp(newlibname, "toolgprof") == 0)) {
				(void)sprintf(buffer, "${%s}", newlibname);
				define("LIBBUILDSUFFIX",  buffer);
				(void)strcpy(buffer, "${LIBRARY}");
			} else {
				(void)strcpy(buffer, newlibname);
			}
			freeobject( object );
			Dprintf(TRACE_DB, "DB lookup returns \"%s\"\n", buffer);
			sRETURN(buffer);
		}
	}	/* Database was found */
}


char *
expandmacrosin( char *thislib ) {
	/*
	**	Maintain 2 buffers:
	**		source		The Macro-full source text
	**		working     Macro-less expanded text
	*/

	static char fnargs [FNARGSLEN];
	static char source [FILENAME_MAX];
	static char working[FILENAME_MAX];
	char	 *sp,
		 *macro,
		 *token,
		 *closebracket,
		 *value;
	char tempchar;


	(void)sprintf(fnargs,"expandmacrosin(\"%s\")", thislib);
	Denter(fnargs);

	(void)strcpy(source, thislib);
	while ( strchr(source, '$') ) { /* while there are macros */
		working[0] = '\0';
		sp         = source;

		/*
		**  /foo/${MACRO}/other/stuff
		**  ^
		**  |
		**  +---- *sp
		*/

		while ( (macro=strchr(sp, '$')) != NULL) { /* found a Macro */
			if (macro != sp) {	/* if not at current position */
				/*
				**  /foo/${MACRO}/other/stuff
				**  ^    ^
				**  |    |
				**  |    +-- *macro
				**  +---- *sp
				*/
				/* copy non-macro text to this point */
				*macro = '\0';
				(void)strcat(working, sp);
				Dprintf(TRACE_MACROS,"adding prefix \"%s\"\n",
					sp);
				*macro = '$';
				sp = macro;
			}
			/*
			**	/foo/${MACRO}/other/stuff
			**       ^
			**       |\
			**       | +-- *macro
			**       |
			**       +---- *sp
			*/

			if (*(macro+1) != '{') {
				if (*macro+1 == '\0')
					goto BadMacro;
				token        = macro+1;
				closebracket = macro+2;
				sp           = macro+2;
			} else {
				if (   (closebracket = strchr(macro+1, '}'))
				    == NULL)
				{
					goto BadMacro;
				}
				token = macro+2;
				sp    = closebracket +1;
		    }
			tempchar = *closebracket;
			*closebracket = '\0';
				Dprintf(TRACE_MACROS,"valueof(%s) = ", token);
				/* not defined here */
				if ( (value = valueof(token)) == NULL ) {
				    /* try the environment */
				    value = getenv(token);
				}
				if (value) {
				    Dprintf(TRACE_MACROS, "\"%s\"\n", value);
				} else {
				    Dprintf(TRACE_MACROS, "<undefined>\n");
				}
			*closebracket = tempchar;

			if (value) {
				(void)strcat(working, value);
			}
		}
		Dprintf(TRACE_MACROS,"adding suffix \"%s\"\n", sp);
		(void)strcat(working, sp);
		Dprintf(TRACE_MACROS,"expanded libname ==>\"%s\"\n", working);
		(void)strcpy(source, working);
	}
	sRETURN(source);

BadMacro:
	Dprintf(TRACE_WARNING, "\tError: Badly formed Macro: \"%s\"\n", macro);
	RETURN(NULL);
}


static char *
basename( char *pathname ) {
	char *lastslash;

	if (!pathname) {
		return "";
	}

	if (lastslash = strrchr(pathname, '/')) {
		return lastslash+1;
	} else {
		return pathname;
	}
}

