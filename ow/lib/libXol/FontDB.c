#pragma       ident   "@(#)FontDB.c 1.3     97/03/26 SMI"        /* OLIT */

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

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

#include <Xol/FontDBI.h>

static String
skip_fields(String	source,
	    int		num_skip)
{
    /* Guaranteed to be at the beginning of a field */
    String	trav = source;

    for (; num_skip > 0; num_skip--) {
	while (*trav != '\0' && *trav != '-')
	    trav++;
	if (*trav == '-')
	    trav++;
	else
	    trav = (String) NULL;
    }
    /* Guaranteed to be at the beginning of a field */
    return(trav);
}

extern Boolean
_OlExtractFontFields(String	font_name,
		     ArgList	args,
		     Cardinal	num_args)
{
    /* Expects the args to be in sorted order of FontFields */
    register int	k;
    register int	i;
    String		trav = font_name;
    String		next;
    int			cur;
    Boolean		ret_val = TRUE;
    
    cur = 0;
    for (k = 0; k < num_args; k++) {
	register int	num_skip;
	int		len;
	String		temp;
	
	num_skip = (FontFieldID) args[k].name - cur;
	cur      = (FontFieldID) args[k].name;
	if (num_skip > 0)
	    trav = skip_fields(trav, num_skip);
	if (trav == (String) NULL)
	    break;
	if (cur < FC_XLFD_LAST) {
	    next = skip_fields(trav, 1);
	    if (next == (String) NULL)
		break;
	    len  = next - trav;
	    cur++;
	} else {
	    len  = strlen(trav) + 1;
	    next = trav;
	}
	
	temp          = (char *) XtMalloc(len);
	strncpy(temp, trav, len);
	temp[len-1]     = '\0';
	args[k].value = (XtArgVal) temp;
	trav          = next;
    }
    if (trav == (String) NULL || next == (String) NULL) {
	for (i = 0; i < k; i++)
	    XtFree((XtPointer) args[i].value);
	ret_val = FALSE;
    }
    return(ret_val);
}

extern FontDB
_OlCreateFontDatabase(
	String *	font_list,
	int		font_count,	    
	FontFieldID *	fields_interested,
	Cardinal	num_fields,
	String *	charset_registries,
	String *	charset_encodings,
        Cardinal	num_registries_and_encodings)
{
    register int	fm_cnt;
    register int	i;
    register int	j;
    register int	k;
    Boolean		done;

    Arg			arg[FC_XLFD_LAST+1];
    register int	num_args;
    
    struct _FontDBRec *	fdb;
    XrmQuark *		Qchset_reg;
    XrmQuark *		Qchset_enc;
    
    if (font_list == (String *) NULL			||
	font_count <= 0					||
	fields_interested == (FontFieldID *) NULL	||
	num_fields <= 0					||
	num_registries_and_encodings < 0)
	return((FontDB) NULL);
    
    fdb = (struct _FontDBRec *) XtMalloc(sizeof(struct _FontDBRec));

    /* Overestimate the number of entries; at the end we'll do a realloc */
    fdb->font_entries = (struct _FontRec *) XtMalloc(sizeof(struct _FontRec) *
						     font_count);
    fdb->num_fonts    = font_count;

    if (num_registries_and_encodings > 0) {
	Qchset_reg = (XrmQuark *) XtMalloc(num_registries_and_encodings
					   * sizeof(XrmQuark));
	Qchset_enc = (XrmQuark *) XtMalloc(num_registries_and_encodings
					   * sizeof(XrmQuark));
    }

    for (k=0; k < num_registries_and_encodings; k++) {
	Qchset_reg[k] = XrmStringToQuark(charset_registries[k]);
	Qchset_enc[k] = XrmStringToQuark(charset_encodings[k]);
    }

    /*
     * Do a shell-sort on the fields; no need for
     * nlogn algos because size of array is too small
     *
     */
    done = FALSE;
    while (!done) {
	done = TRUE;
	for (i = 0; i < num_fields-1; i++) {
	    FontFieldID	temp;
	    
	    if (fields_interested[i] > fields_interested[i+1]) {
		temp = fields_interested[i];
		fields_interested[i] = fields_interested[i+1];
		fields_interested[i+1] = temp;
		done = FALSE;
	    }
	}
    }
    
    for (num_args=0; num_args < num_fields; num_args++)
	XtSetArg(arg[num_args], (String) fields_interested[num_args], NULL);
    XtSetArg(arg[num_args], (String) FC_CHARSET_REGISTRY, NULL); num_args++;
    XtSetArg(arg[num_args], (String) FC_CHARSET_ENCODING, NULL); num_args++;

    fdb->field_ids  = (FontFieldID *) XtMalloc(sizeof(FontFieldID) * num_args);
    fdb->num_fields = num_args;
    for (j = 0; j < num_args; j++)
	fdb->field_ids[j] = (FontFieldID) arg[j].name;
    
    for (fm_cnt = i = 0; i < font_count; i++) {

	if (!_OlExtractFontFields(font_list[i], arg, num_args))
	    continue;

	for (k=0; k < num_registries_and_encodings; k++)
	    if (XrmStringToQuark((String) arg[num_args-2].value) ==
							Qchset_reg[k] &&
		XrmStringToQuark((String) arg[num_args-1].value) ==
							Qchset_enc[k])
		break;
	
	if (num_registries_and_encodings == 0 ||
	    k < num_registries_and_encodings) { /* A match */
	    struct _FontRec * cur = &(fdb->font_entries[fm_cnt]);

	    cur->font_fields = (FontField *) XtMalloc(sizeof(FontField) *
						      num_args);
	    for (j = 0; j < num_args; j++) {
		cur->font_fields[j].name     = (String) arg[j].value;
		cur->font_fields[j].quark    = XrmStringToQuark(
						(String) arg[j].value);
	    }
	    fm_cnt++;
	} else
	    for (j=0; j < num_args; j++)
		XtFree((XtPointer) arg[j].value);
    }

    if (num_registries_and_encodings > 0) {
	XtFree((XtPointer) Qchset_reg);
	XtFree((XtPointer) Qchset_enc);
    }

    if (fm_cnt == 0) {
	XtFree((XtPointer) fdb->font_entries);
	XtFree((XtPointer) fdb->field_ids);
	XtFree((XtPointer) fdb);
	fdb = (FontDB) NULL;
    } else if (fm_cnt < font_count) {
	fdb->font_entries = (struct _FontRec *) XtRealloc(
				  (XtPointer) fdb->font_entries,
				  sizeof(struct _FontRec) * fm_cnt);
	fdb->num_fonts    = fm_cnt;
    }

    return(fdb);
}

extern void
_OlDestroyFontDatabase(FontDB	fdb)
{
    register int	i;
    register int	j;
    
    if (fdb == (FontDB) NULL)
	return;

    for (i = 0; i < fdb->num_fonts; i++) {
	for (j = 0; j < fdb->num_fields; j++)
	    XtFree((XtPointer) fdb->font_entries[i].font_fields[j].name);
	XtFree((XtPointer) fdb->font_entries[i].font_fields);
    }
    XtFree((XtPointer) fdb->font_entries);
    XtFree((XtPointer) fdb->field_ids);
    XtFree((XtPointer) fdb);
}

extern FontField **
_OlQueryFontDatabase(
	FontDB		fdb,
	FontFieldID *	req_fields,
	Cardinal	num_req_fields,		   
	ArgList		constraints,
	Cardinal	num_constraints,
	Cardinal *	num_values)
{
    Boolean		done;
    register int	i;
    register int	j;
    register int	k;
    register int	l;
    register int	num_sel;
    register int	req_pos;
    register int	ret_size;
    int *		field_pos      = (int *) NULL;
    int *		req_field_pos  = (int *) NULL;
    FontField **	ret_list;

    if (num_constraints < 0		||
	num_req_fields <= 0		||
	req_fields == (FontFieldID *) NULL)
	return((FontField **) NULL);

    if (num_constraints) {
	field_pos  = (int *) XtMalloc(num_constraints * sizeof(int));

	/* Quarkify the constraint values */
	for (i = 0; i < num_constraints; i++)
	    constraints[i].value = (XtArgVal) XrmStringToQuark(
					       (String) constraints[i].value);

	/* Shell-sort the constraints */
	done = FALSE;
	while (!done) {
	    done = TRUE;
	    for (i = 0; i < num_constraints-1; i++) {
		Arg	temp;
	    
		if ((FontFieldID) constraints[i].name >
		    (FontFieldID) constraints[i+1].name) {
		    temp             = constraints[i];
		    constraints[i]   = constraints[i+1];
		    constraints[i+1] = temp;
		    done = FALSE;
		}
	    }
	}

	for (j = i = 0; i < fdb->num_fields; i++)
	    if (fdb->field_ids[i] == (FontFieldID) constraints[j].name) {
		field_pos[j++] = i;
		if (j == num_constraints)
		    break;
	    }
	if (j != num_constraints) {
	    /*
	     * fdb isn't interested in some
	     * field including req_field
	     */
	    XtFree((XtPointer) field_pos);
	    return ((FontField **) NULL);
	}
    }

    req_field_pos  = (int *) XtMalloc(num_req_fields * sizeof(int));
    for (i = 0; i < num_req_fields; i++) {
	for (req_pos = 0; req_pos < fdb->num_fields; req_pos++)
	    if (fdb->field_ids[req_pos] == req_fields[i])
		break;
	if (req_pos == fdb->num_fields) {
	    /*
	     * fdb isn't interested in some
	     * req_field
	     */
	    if (field_pos != (int *) NULL)
		XtFree((XtPointer) field_pos);
	    XtFree((XtPointer) req_field_pos);
	    return ((FontField **) NULL);
	}
	req_field_pos[i] = req_pos;
    }

    ret_list = (FontField **) XtMalloc(num_req_fields * sizeof(FontField *));
    ret_size = DEF_RET_SIZE;
    for (i = 0; i < num_req_fields; i++)
	ret_list[i] = (FontField *) XtMalloc(ret_size * sizeof(FontField));
    
    for (num_sel = i = 0; i < fdb->num_fonts; i++) {
	struct _FontRec * cur = &(fdb->font_entries[i]);

	if (num_constraints) {
	    for (j = 0; j < num_constraints; j++)
		if ((XrmQuark) constraints[j].value !=
		    cur->font_fields[field_pos[j]].quark)
		    break;
	} else
	    j = 0;
	    
	if (j == num_constraints) { /* all matched */
	    /* Ensure that this value isn't there in the list yet */
	    for (k = 0; k < num_sel; k++) {
		for (l = 0; l < num_req_fields; l++) {
		    FontField * cur_list = ret_list[l];
		    
		    if (cur_list[k].quark !=
			cur->font_fields[req_field_pos[l]].quark)
			break;
		}
		if (l == num_req_fields)
		    break;
	    }
	    /* Insert font_field[req_pos] into selected list, if so */
	    if (k == num_sel) {
		if (num_sel == ret_size) {
		    ret_size += ret_size;
		    for (l = 0; l < num_req_fields; l++)
			ret_list[l] =
			    (FontField *) XtRealloc((XtPointer) ret_list[l],
					    ret_size * sizeof(FontField));
		}
		for (l = 0; l < num_req_fields; l++) {
		    FontField * cur_list = ret_list[l];
		    
		    cur_list[num_sel] = cur->font_fields[req_field_pos[l]];
		}
		num_sel++;
	    }
	}
    }
    if (field_pos != (int *) NULL)
	XtFree((XtPointer) field_pos);
    XtFree((XtPointer) req_field_pos);
    
    if (num_sel == 0) {
	XtFree((XtPointer) ret_list);
	for (l = 0; l < num_req_fields; l++)
	    XtFree((XtPointer) ret_list[i]);
	ret_list = (FontField **) NULL;
    } else if (num_sel < ret_size)
	for (l = 0; l < num_req_fields; l++)
	    ret_list[l] = (FontField *) XtRealloc((XtPointer) ret_list[l],
					   num_sel * sizeof(FontField));

    *num_values = num_sel;
    return(ret_list);
}

extern void
_OlFreeFontFieldLists(FontFieldList * field_lists,
		      Cardinal	      num_fields)
{
    if (num_fields <= 0				||
	field_lists == (FontFieldList *) NULL) {
	register int	i;
    
	for (i = 0; i < num_fields; i++)
	    XtFree((XtPointer) field_lists[i]);
	XtFree((XtPointer) field_lists);
    }
}

extern FontField **
_OlIntersectFieldLists(
	Cardinal	num_lists,
	FontField ***	lists,
	Cardinal *	list_sizes,
	Cardinal	num_fields,		   
	Cardinal *	num_values)
{
    register int	f;
    register int	l;
    register int	m;
    register int	n;
    FontField **	ret_list = (FontField **) NULL;
    Cardinal		min_list_size;
    Cardinal		ret_count;

    if (num_lists <= 0			||
	lists == (FontField ***) NULL	||
	num_fields <= 0)
	return((FontField **) NULL);

    min_list_size = list_sizes[0];
    for (l = 0; l < num_lists; l++) {
	if (list_sizes[l] <= 0)
	    break;
	if (list_sizes[l] < min_list_size)
	    min_list_size = list_sizes[l];
    }

    if (l < num_lists)		/* Some list has non-positive length */
	return((FontField **) NULL);

    ret_list = (FontField **) XtMalloc(sizeof(FontField *) * num_fields);

    for (f = 0; f < num_fields; f++)
	ret_list[f] = (FontField *) XtMalloc(sizeof(FontField) *
						 min_list_size);

    ret_count = 0;
    for (l = 0; l < list_sizes[0]; l++) {
	for (m = 1; m < num_lists; m++) {
	    for (n = 0; n < list_sizes[m]; n++) {
		for (f = 0; f < num_fields; f++)
		    if (((lists[0])[f])[l].quark != ((lists[m])[f])[n].quark)
			break;
		if (f == num_fields)	/* There is a match in this list */
		    break;
	    }
	    if (n == list_sizes[m])	/* There was no match in this list */
		break;
	}
	if (m == num_lists) {	/* There was a match in every list */
	    for (f = 0; f < num_fields; f++)
		(ret_list[f])[ret_count] = ((lists[0])[f])[l];
	    ret_count++;
	}
    }
    *num_values = ret_count;
    if (ret_count == 0) {
	_OlFreeFontFieldLists(ret_list, num_fields);
	return((FontField **) NULL);
    } else if (ret_count < min_list_size)
	for (f = 0; f < num_fields; f++)
	    ret_list[f] = (FontField *) XtRealloc((XtPointer) ret_list[f],
					sizeof(FontField) * ret_count);
    return(ret_list);
}

