#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)es_mem.c 20.26 93/08/31";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Entity stream implementation for one block of virtual memory.
 */

#include <sys/types.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview_private/primal.h>
#include <xview_private/es.h>
#ifdef SVR4 
#include <stdlib.h> 
#endif /* SVR4 */

typedef struct es_mem_text {
    Es_status       status;
    void           *value;
    u_int           length;
    u_int           position;
    u_int           max_length;
    u_int           initial_max_length;
    Xv_opaque       client_data;
}               Es_mem_text;
typedef Es_mem_text *Es_mem_data;
#define	ABS_TO_REP(esh)	(Es_mem_data)esh->data

Pkg_private Es_handle es_mem_create();
static Es_status es_mem_commit();
static Es_handle es_mem_destroy();
static Es_index es_mem_get_length();
static Es_index es_mem_get_position();
static Es_index es_mem_set_position();
static Es_index es_mem_read();
static Es_index es_mem_replace();
static int      es_mem_set();

static struct es_ops es_mem_ops = {
    es_mem_commit,
    es_mem_destroy,
    es_mem_get,
    es_mem_get_length,
    es_mem_get_position,
    es_mem_set_position,
    es_mem_read,
    es_mem_replace,
    es_mem_set
};

Pkg_private     Es_handle
es_mem_create(max, init)
    u_int           max;
    void           *init;
{
    Es_handle       esh = NEW(Es_object);
    Es_mem_data     private = NEW(Es_mem_text);

    if (esh == ES_NULL) {
	return (ES_NULL);
    }
    if (private == 0) {
	free((char *) esh);
	return (ES_NULL);
    }
    private->initial_max_length = max;
    if (max == ES_INFINITY) {
	max = 10000;
    }
    if (!multibyte)
        private->value = xv_malloc(max + 1);
    else
        private->value = xv_malloc((max + 1)*sizeof(wchar_t));

    if (private->value == NULL) {
	free((char *) private);
	free((char *) esh);
	return (ES_NULL);
    }

    if (!multibyte) {
        (void) strncpy(private->value, init, (int) max);
        ((char *)private->value)[max] = '\0';
        private->length = strlen(private->value);
    } else {
        (void) wsncpy(private->value, init, (int) max);
        ((wchar_t *)private->value)[max] = '\0';
        private->length = wslen(private->value);
    }

    private->position = private->length;
    private->max_length = max - 1;

    esh->ops = &es_mem_ops;
    esh->data = (caddr_t) private;
    return (esh);
}

/* ARGSUSED */
static          Es_status
es_mem_commit(esh)
    Es_handle       esh;
{
    return (ES_SUCCESS);
}

static          Es_handle
es_mem_destroy(esh)
    Es_handle       esh;
{
    register Es_mem_data private = ABS_TO_REP(esh);

    free((char *) esh);
    free(private->value);
    free((char *) private);
    return (ES_NULL);
}

/* ARGSUSED */
static          caddr_t
#ifdef ANSI_FUNC_PROTO
es_mem_get(Es_handle esh, Es_attribute attribute, ...)
#else
es_mem_get(esh, attribute, va_alist)
    Es_handle       esh;
    Es_attribute    attribute;
va_dcl
#endif
{
    register Es_mem_data private = ABS_TO_REP(esh);
#ifndef lint
    va_list         args;
#endif

    switch (attribute) {
      case ES_CLIENT_DATA:
	return ((caddr_t) (private->client_data));
      case ES_NAME:
	return (0);
      case ES_STATUS:
	return ((caddr_t) (private->status));
      case ES_SIZE_OF_ENTITY:
        if (!multibyte)
	    return ((caddr_t) sizeof(char));
        else 
	    return ((caddr_t) sizeof(wchar_t));
      case ES_TYPE:
	return ((caddr_t) ES_TYPE_MEMORY);
      default:
	return (0);
    }
}

static int
es_mem_set(esh, attrs)
    Es_handle       esh;
    Attr_avlist     attrs;
{
    register Es_mem_data private = ABS_TO_REP(esh);
    Es_status       status_dummy = ES_SUCCESS;
    register Es_status *status = &status_dummy;

    for (; *attrs && (*status == ES_SUCCESS); attrs = attr_next(attrs)) {
	switch ((Es_attribute) * attrs) {
	  case ES_CLIENT_DATA:
	    private->client_data = attrs[1];
	    break;
	  case ES_STATUS:
	    private->status = (Es_status) attrs[1];
	    break;
	  case ES_STATUS_PTR:
	    status = (Es_status *) attrs[1];
	    *status = status_dummy;
	    break;
	  default:
	    *status = ES_INVALID_ATTRIBUTE;
	    break;
	}
    }
    return ((*status == ES_SUCCESS));
}

static          Es_index
es_mem_get_length(esh)
    Es_handle       esh;
{
    register Es_mem_data private = ABS_TO_REP(esh);
    return (private->length);
}

static          Es_index
es_mem_get_position(esh)
    Es_handle       esh;
{
    register Es_mem_data private = ABS_TO_REP(esh);
    return (private->position);
}

static          Es_index
es_mem_set_position(esh, pos)
    Es_handle       esh;
    Es_index        pos;
{
    register Es_mem_data private = ABS_TO_REP(esh);

    if (pos > private->length) {
	pos = private->length;
    }
    return (private->position = pos);
}

static          Es_index
es_mem_read(esh, len, bufp, resultp)
    Es_handle       esh;
    u_int           len, *resultp;
    register void  *bufp;
{
    register Es_mem_data private = ABS_TO_REP(esh);

    if (private->length - private->position < len) {
	len = private->length - private->position;
    }
    if (!multibyte)
        XV_BCOPY((char *)private->value + private->position, bufp, (int) len);
    else
        XV_BCOPY((wchar_t *)private->value + private->position, bufp,
                                               (int)len*sizeof(wchar_t));
    *resultp = len;
    return (private->position += len);
}

static          Es_index
es_mem_replace(esh, end, new_len, new, resultp)
    Es_handle       esh;
    int             end, new_len, *resultp;
    void           *new;
{
    int             old_len, delta;
    void           *start, *keep, *restore;
    register Es_mem_data private = ABS_TO_REP(esh);

    *resultp = 0;
    if (new == 0 && new_len != 0) {
	private->status = ES_INVALID_ARGUMENTS;
	return ES_CANNOT_SET;
    }

    if (end > private->length) {
	end = private->length;
    } else 
        if (end < private->position) {
	    int  tmp = end;
	    end = private->position;
	    private->position = tmp;
        }

    old_len = end - private->position;
    delta = new_len - old_len;

    if (delta > 0 && private->length + delta > private->max_length) {
        void *new_value = NULL;
        if (multibyte) {
	    if (private->initial_max_length == ES_INFINITY)
	        new_value = (wchar_t *)realloc(private->value,
		              ((private->max_length + delta + 10000 + 1) 
                                                       * sizeof(wchar_t)));
        } else {
	    if (private->initial_max_length == ES_INFINITY)
	        new_value = (char *)realloc(private->value,
		              ((private->max_length + delta + 10000 + 1)));
        }
	if (new_value) {
            private->value = new_value;
            private->max_length += delta + 10000;
	} else {
	    private->status = ES_SHORT_WRITE;
	    return ES_CANNOT_SET;
        }
    }

    if (multibyte) {
        start = (wchar_t *)private->value + private->position;
        keep = (wchar_t *)private->value + end;
        restore = (wchar_t *)start + new_len;
        if (delta != 0) {
	    XV_BCOPY(keep, restore, 
                     ((int)private->length - end + 1) * sizeof(wchar_t));
        }
        if (new_len > 0) {
	    XV_BCOPY(new, start, new_len * sizeof(wchar_t));
        }
        private->position = end + delta;
        private->length += delta;
        ((wchar_t *)private->value)[private->length] = (wchar_t)'\0';
        *resultp = new_len;
        return (wchar_t *)restore - (wchar_t *)private->value;
    } else {
        start = (char *)private->value + private->position;
        keep = (char *)private->value + end;
        restore = (char *)start + new_len;
        if (delta != 0) {
	    XV_BCOPY(keep, restore, (int)private->length - end + 1);
        }
        if (new_len > 0) {
	    XV_BCOPY(new, start, new_len);
        }
        private->position = end + delta;
        private->length += delta;
        ((char *)private->value)[private->length] = '\0';
        *resultp = new_len;
        return (char *)restore - (char *)private->value;
    }
}

#ifdef DEBUG
Pkg_private void
es_mem_dump(fd, pdh)
    FILE           *fd;
    Es_mem_data     pdh;
{
    extern wchar_t _xv_null_string_wc[];

    (void) fprintf(fd, "\n\t\t\t\t\t\tmax length:  %ld", pdh->max_length);
    (void) fprintf(fd, "\n\t\t\t\t\t\tcurrent length:  %ld", pdh->length);
    (void) fprintf(fd, "\n\t\t\t\t\t\tposition:  %ld", pdh->position);
    (void) fprintf(fd, "\n\t\t\t\t\t\tvalue (%lx):  ",pdh->value);
    if (!multibyte)    
        (void) fprintf(fd, "\"%s\"",(pdh->value ? pdh->value : NULL));
    else
        (void) fprintf(fd, "\"%ws\"",(pdh->value ? pdh->value : NULL_STRING));

}

#endif /* DEBUG */
