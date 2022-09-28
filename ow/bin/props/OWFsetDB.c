#pragma ident	"@(#)OWFsetDB.c	1.4	93/06/11 SMI"	/* Olit */

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

#include "OWFsetDBI.h"

#ifdef DBMALLOC
	#include "dbmalloc.h"
#endif


#define EXT_START	0
#define EXT_LHS		1
#define EXT_TYPE	2
#define EXT_PART_RHS	3
#define EXT_DONE	4
#define EXT_ERROR	5

static void
_OWFree(void * p)
{
    free(p);
}

static void *
_OWMalloc(int	size)
{
    void *	p = malloc(size);

    if (p == (void *) NULL) {
	fprintf(stderr, "Cannot allocate memory\n");
	exit(1);
    }
    return(p);
}

static void *
_OWRealloc(void *	p,
	   int		size)
{
    p = realloc(p, size);
    if (p == (void *) NULL) {
	fprintf(stderr, "Cannot reallocate memory\n");
	exit(1);
    }
    return(p);
}

static char *
_OWNewString(char *	str)
{
    char *	p = (char *) NULL;
    
    if (str != (char *) NULL) {
	int len = strlen(str);

	p = (char *) _OWMalloc(len + 1);
	strcpy(p, str);
    }
    return(p);
}

static char *
skip_field(char *	source)
{
    /* Guaranteed to be at the beginning of a field */
    char *	trav = source;

    while (*trav != '\0' && *trav != '-' && *trav != ':')
	    trav++;
    if (*trav == '-')
	trav++;
    else if (*trav != ':')
	trav = (char *) NULL;
    /* Guaranteed to be at the beginning of a field */
    return(trav);
}

static int
extract_lfd_fields(char *	fset_spec,
		   char **	fset_fields,
		   int		be_strict)
{
    char *	temp = _OWNewString(fset_spec);
    char *	trav;
    int		i = 0;

    fset_fields[i] = temp;
    for (trav = temp; *trav != '\0'; trav++) {
	if (*trav == '-') {
	    *trav = '\0';
	    fset_fields[++i] = trav+1;
	}
    }
    if (be_strict == 1) {
        if (i != XLFD_COUNT-1)
	    return(1);
	else
	    return(0);
    } else {
	i++;
	for (; i < XLFD_COUNT; i++)
	    fset_fields[i] = (char *) NULL;
    }
}

static char *
strip_line(char *	line)
{
    char *	trav = line;
    
    if (trav == (char *) NULL)
	return(trav);

    for (; *trav != '\0'; trav++)
	if (!isspace(*trav))
	    break;
    
    if (*trav == COMMENT_CHAR)
	return((char *) NULL);
    else
	return(trav);
}

static int
extract_rhs(char *	cline,
	    char *	extract)
{
    char * trav	= cline;
    char * end;
    int    ret_val = EXT_DONE;

    for (; *trav != '\0' && *trav != '\n'; trav++) {
	if (*trav == ',')
	    break;
    }
    end = trav;
    trav++;			/* skip the comma */
    for (; *trav != '\0' && *trav != '\n'; trav++) {
	if (*trav == '\\')
	    break;
    }
    if (*trav == '\\' && *(trav+1) == '\n')
	ret_val = EXT_PART_RHS;
    *end = '\0';
    strcpy(extract, cline);
    return(ret_val);
}

static int
extract_type_rhs(char *		cline,
		 char *		extr_type,
		 char *		extr_rhs)
{
    char * trav = cline;
    char * end;
    int	   ret_val = EXT_TYPE;

    for (; *trav != '\0' && *trav != '\n'; trav++)
	if (*trav == ',')
	    break;
    if (*trav != ',')
	return(EXT_ERROR);
    end = trav;
    trav++;			/* skip the , */
    for (; *trav != '\0' && *trav != '\n'; trav++)
	if (!isspace(*trav))
	    break;
    if (*trav != '\0' && *trav != '\n') {
	if (*trav == '\\' && *(trav+1) == '\n')
	    ret_val = EXT_TYPE;
	else
	    ret_val = extract_rhs(trav, extr_rhs);
    } else
	ret_val = EXT_ERROR;
    *end = '\0';
    strcpy(extr_type, cline);
    return(ret_val);
}

static int
extract_all(char *		cline,
	 char *		extr_lhs,
	 char *		extr_type,
	 char *		extr_rhs)
{
    char * trav = cline;
    char * end;
    int    ret_val = EXT_LHS;

    for (; *trav != '\0' && *trav != '\n'; trav++)
	if (*trav == ':')
	    break;
    if (*trav != ':')
	return(EXT_ERROR);
    end = trav;
    trav++;			/* skip the : */
    for (; *trav != '\0' && *trav != '\n'; trav++)
	if (!isspace(*trav))
	    break;
    if (*trav != '\0' && *trav != '\n') {
	if (*trav == '\\' && *(trav+1) == '\n')
	    ret_val = EXT_LHS;
	else
	    ret_val = extract_type_rhs(trav, extr_type, extr_rhs);
    } else
	ret_val = EXT_ERROR;
    *end = '\0';
    strcpy(extr_lhs, cline);
    return(ret_val);
    
}

extern OWFsetDB
CreateOWFsetDB(char *	path_name)
{
    FILE *	fp;
    char	file_line[OWFS_MAXFSETLEN];
    char *	clean_line;
    OWFsetDB	fsdb = (OWFsetDB) NULL;
    int		num_alloced;
    int		cur_entry;
    char	lhs[OWFS_MAXFSETLEN];
    char	rhs[OWFS_MAXFSETLEN];
    char	rhs_suffix[OWFS_MAXFSETLEN];
    char	desc_type[OWFS_MAXFSETLEN];
    int		state;

    if ((fp = fopen(path_name, "r")) == (FILE *) NULL)
	return((OWFsetDB) NULL);

    fsdb = (OWFsetDB) _OWMalloc(sizeof(struct _OWFsetDBRec));
    num_alloced = INITIAL_ENTRY_COUNT;
    fsdb->entries = (FsetRec *) _OWMalloc(sizeof(FsetRec) * num_alloced);
    fsdb->num_entries = cur_entry = 0;
    
    state = EXT_START;
    while (fgets(file_line, OWFS_MAXFSETLEN, fp) != (char *) NULL) {
	if ((clean_line = strip_line(file_line)) != (char *) NULL) {
	    switch (state) {
	    case EXT_START:		/* starting */
		state = extract_all(clean_line, lhs, desc_type, rhs);
		break;
	    case EXT_LHS:		/* done till lhs, including */
		state = extract_type_rhs(clean_line, desc_type, rhs);
		break;
	    case EXT_TYPE:		/* done till type, including */
		state = extract_rhs(clean_line, rhs);
		break;
	    case EXT_PART_RHS:		/* done partly through rhs */
		state = extract_rhs(clean_line, rhs_suffix);
		strcat(rhs, ",");
		strcat(rhs, rhs_suffix);
		break;
	    default:
		break;
	    }
	    if (state == EXT_DONE) {		/* read completely */
		/* update database */
		if (! strcmp(desc_type, DEFINITION_STR)) {
		    /* insert new entry */
		    if (cur_entry == num_alloced) {
			num_alloced += num_alloced;
			fsdb->entries = (FsetRec *)
			    _OWRealloc((void *) fsdb->entries,
				       sizeof(FsetRec) * num_alloced);
		    }
		    if (extract_lfd_fields(lhs,
			       fsdb->entries[cur_entry].owfset_fields, 1) != 0)
			continue;
		    
		    fsdb->entries[cur_entry].fset_defn = _OWNewString(rhs);
		    fsdb->entries[cur_entry].fset_name = _OWNewString(lhs);
		    fsdb->entries[cur_entry].aliases = (char **) NULL;
		    fsdb->entries[cur_entry].num_aliases = 0;
		    fsdb->num_entries = ++cur_entry;
		    
		} else if (! strcmp(desc_type, ALIAS_STR)) {
		    register int i;
		    
		    /* add alias at appropriate place */
		    for (i = 0; i < fsdb->num_entries; i++) {
			if (! strcmp(rhs, fsdb->entries[i].fset_name))
			    break;
			if (fsdb->entries[i].num_aliases > 0) {
			    register int j;
			    
			    for (j = 0; j < fsdb->entries[i].num_aliases;
				 j++) {
				if (! strcmp(rhs, fsdb->entries[i].aliases[j]))
				    break;
			    }
			    if (j != fsdb->entries[i].num_aliases)
				break;
			}
		    }
		    if (i != fsdb->num_entries) {
			if (fsdb->entries[i].num_aliases == 0)
			    fsdb->entries[i].aliases = (char **)
				_OWMalloc(sizeof(char *));
			else {
			    fsdb->entries[i].aliases = (char **)
				_OWRealloc((void *) fsdb->entries[i].aliases,
					   sizeof(char *) *
					   (fsdb->entries[i].num_aliases + 1));
			}
			fsdb->entries[i].aliases[fsdb->entries[i].num_aliases]
			    = _OWNewString(lhs);
			fsdb->entries[i].num_aliases++;
		    } else {
			/* inapplicable alias */
		    }
		}
		state = EXT_START;
	    } else if (state == EXT_ERROR) {
	    	/* there was an error */
		state = EXT_START;
	    }
	}
    }
    fclose(fp);
    if (cur_entry == 0) {
	/* Empty database */
	_OWFree((void *) fsdb->entries);
	_OWFree((void *) fsdb);
	fsdb = (OWFsetDB) NULL;
    } else if (cur_entry < num_alloced) {
	fsdb->entries = (FsetRec *)
	    _OWRealloc((void *) fsdb->entries, sizeof(FsetRec) * cur_entry);
    }
    return(fsdb);
}

extern void
DestroyOWFsetDB(OWFsetDB	fsdb)
{
    register int	i;

    for (i = 0; i < fsdb->num_entries; i++) {
	if (fsdb->entries[i].num_aliases > 0)
	    _OWFree((void *) fsdb->entries[i].aliases);
	if (fsdb->entries[i].fset_name != (char *) NULL)
	    _OWFree((void *) fsdb->entries[i].fset_name);
	if (fsdb->entries[i].fset_defn != (char *) NULL)
	    _OWFree((void *) fsdb->entries[i].fset_defn);
	if (fsdb->entries[i].owfset_fields[0] != (char *) NULL)
	    _OWFree((void *) fsdb->entries[i].owfset_fields[0]);
    }
}

extern char *
GetFsetForOWFset(
		 OWFsetDB	fsdb,
		 char *		fset_name)
{
    register int	i;
    
    if (fset_name == (char *) NULL || *fset_name == '\0')
	return((char *) NULL);
    for (i = 0; i < fsdb->num_entries; i++)
	if (! strcmp(fset_name, fsdb->entries[i].fset_name))
	    break;
    if (i != fsdb->num_entries)
	return(fsdb->entries[i].fset_defn);
    else
	return((char *) NULL);
}

extern char *
GetOWFsetForAlias(
		  OWFsetDB	fsdb,
		  char *	fset_alias)
{
    register int	i;
    register int	j;
    
    if (fset_alias == (char *) NULL || *fset_alias == '\0')
	return((char *) NULL);
    for (i = 0; i < fsdb->num_entries; i++) {
	for (j = 0; j < fsdb->entries[i].num_aliases; j++)
	    if (! strcmp(fset_alias, fsdb->entries[i].aliases[j]))
		break;
	if (j != fsdb->entries[i].num_aliases)
	    break;
    }
    if (i != fsdb->num_entries)
	return(fsdb->entries[i].fset_name);
    else
	return((char *) NULL);
}

extern char *
GetOWFsetForSpec(
		OWFsetDB	fsdb,
		char *		fset_spec)
{
    char *	lfd_fields[XLFD_COUNT];
    int		i;
    int		j;
    int		icnt;
    int		inter_idx[XLFD_COUNT];
    int		fully_specified = 1;

    if (fset_spec == (char *) NULL || fsdb == (OWFsetDB) NULL)
	return((char *) NULL);

    extract_lfd_fields(fset_spec, lfd_fields, 0);

    for (icnt = i = 0; i < XLFD_COUNT; i++) {
	if (lfd_fields[i] == (char *) NULL)
	    break;
	if (! strcmp(lfd_fields[i], "*")) {
	    fully_specified = 0;
	    continue;
	}
	inter_idx[icnt++] = i;
    }

    for (i = 0; i < fsdb->num_entries; i++) {
	for (j = 0; j < icnt; j++) {
	    if (strcmp(fsdb->entries[i].owfset_fields[inter_idx[j]],
			 lfd_fields[inter_idx[j]]))
		break;
	}
	if (j == icnt)
	    break;
    }
    if (i != fsdb->num_entries)
	return(fsdb->entries[i].fset_name);
    else if (fully_specified == 1) {	/* fully specified, maybe an alias */
	return(GetOWFsetForAlias(fsdb, fset_spec));
    } else
	return((char *) NULL);
}

extern char **
ListOWFsets(
	    OWFsetDB	fsdb,
	    int *	ret_num_fsets)
{
    register int	i;
    char **		ret_val = (char **) NULL;

    if (fsdb == (OWFsetDB) NULL)
	return(ret_val);
    
    ret_val = _OWMalloc(sizeof(char *) * fsdb->num_entries);

    for (i = 0; i < fsdb->num_entries; i++)
	ret_val[i] = fsdb->entries[i].fset_name;

    *ret_num_fsets = i;
    return(ret_val);
}

extern void
FreeOWFsetList(char ** list)
{
    if (list != (char **) NULL)
	_OWFree((void *) list);
}
