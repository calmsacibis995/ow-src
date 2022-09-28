#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)es_util.c 20.22 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Utilities for use with entity streams.
 */

#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview_private/primal.h>
#include <xview_private/es.h>

Pkg_private     Es_index
es_bounds_of_gap(esh, around, last_plus_one, flags)
    register Es_handle esh;
    Es_index        around;
    Es_index       *last_plus_one;
    int             flags;
/*
 * If there is no gap, then around == return value == *last_plus_one. Else,
 * if not looking for the start of the gap (flags & 0x1 == 0), around <
 * return value == *last_plus_one. Else, if looking for the start of the gap,
 * return value <= around <= *last_plus_one (return value will be
 * ES_CANNOT_SET if gap extends from 0 on).
 */
{
    wchar_t     buf[ES_SCRATCH_SIZE+4];
                /* the buffer is allocated to accomodate wide char case
                   as well as single byte case */

    int         count_read;
    register Es_index new_pos, low, high, probe;

    low = es_set_position(esh, around);
    new_pos = es_read(esh, 1, buf, &count_read);
    if (count_read == 0) {
	low = new_pos;
    }
    if (last_plus_one)
	*last_plus_one = low;
    if ((low != around) && (flags & 0x1) &&
	!READ_AT_EOF(around, new_pos, count_read)) {
	/*
	 * Try binary search to locate the start of the gap. Invariants for
	 * loop: 0 <= low <= start of gap <= high <= around low < probe =
	 * (high + low) / 2 < high Essentially, choose a probe point and try
	 * to read. 3 cases: 1) new_pos < around: null, partial or full read
	 * => low = new_pos; if low >=high, gap starts at probe+count_read 2)
	 * new_pos > around: null read => high = probe 3) new_pos > around:
	 * partial read => gap starts at probe+count_read "new_pos == around
	 * || (new_pos > around: full read)" impossible as there is gap
	 */
	low = 0;
	high = around;
	while (low + 1 < high) {
	    probe = es_set_position(esh, (high + low) / 2);
	    new_pos = es_read(esh, ES_SCRATCH_SIZE, buf, &count_read);
	    if (new_pos < around) {
		if (new_pos >= high) {
		    low = probe + count_read;
		    break;
		} else {
		    low = new_pos;
		}
	    } else if (count_read == 0) {
		high = probe;
	    } else {
		low = probe + count_read;
		break;
	    }
	}
	if (low == 0) {
	    probe = es_set_position(esh, 0);
	    new_pos = es_read(esh, ES_SCRATCH_SIZE, buf, &count_read);
	    if (count_read == 0)
		low = ES_CANNOT_SET;
	}
    }
    return (low);
}

/*
 * Caller must make sure that esbuf->last_plus_one is the current position in
 * the entity stream.
 */

Pkg_private int
es_advance_buf(esbuf)
    Es_buf_handle   esbuf;
{
    int             read = 0;
    register Es_index next = esbuf->last_plus_one;
    while (read == 0) {
	esbuf->first = next;
	next = es_read(esbuf->esh, esbuf->sizeof_buf, esbuf->buf, &read);
	esbuf->last_plus_one = esbuf->first + read;
	if READ_AT_EOF
	    (esbuf->first, next, read) {
	    return (1);
	    }
    }
    return (0);
}

Pkg_private     Es_index
es_backup_buf(esbuf)
    Es_buf_handle   esbuf;
{
    /*
     * Move buffer back in stream (i.e., change so that esbuf->first is
     * smaller). Backing the buffer up is complicated by possible gap before
     * the current beginning of the buffer. This routine moves over such a
     * gap.
     */
    register Es_index prev;
    register Es_index esi = esbuf->first - 1;

Retry:
    switch (es_make_buf_include_index(esbuf, esi, esbuf->sizeof_buf - 1)) {
      case 0:
	break;
      case 2:
	prev = esi;
	esi = es_bounds_of_gap(esbuf->esh, prev, NULL, 0x1);
	if (esi != ES_CANNOT_SET && esi < prev) {
	    goto Retry;
	}
	/*
	 * else gap extends back to 0 OR unexpected error => fall through.
	 */
      case 1:
      default:			/* Conservative in face of new cases */
	esi = ES_CANNOT_SET;
	break;
    }
    return (esi);
}

/*
 * esbuf->first and ->last_plus_one are only adjusted when entities are
 * actually read.  This makes it easier for callers to figure out how to
 * correct after failure to re-position a buffer. Returns: 0	iff it
 * managed to align the buffer as requested, 1	if read at end of stream
 * while trying to align, 2	if desired entity is in a "hole" in the
 * stream.
 */
Pkg_private int
es_make_buf_include_index(esbuf, index, desired_prior_count)
    register Es_buf_handle esbuf;
    Es_index        index;
    int             desired_prior_count;
{
    register Es_index last_plus_one, next;
    int             read;

    last_plus_one = (desired_prior_count > index) ? 0
	: index - desired_prior_count;
    es_set_position(esbuf->esh, last_plus_one);
    FOREVER {
	next = es_read(esbuf->esh, esbuf->sizeof_buf, esbuf->buf, &read);
	if READ_AT_EOF
	    (last_plus_one, next, read)
		return (1);
	esbuf->first = last_plus_one;
	esbuf->last_plus_one = last_plus_one + read;
	if (next > index) {
	    if (esbuf->last_plus_one < index) {
		return (2);
	    } else
		return (0);
	}
	last_plus_one = next;
    }
}

Pkg_private     Es_status
es_copy(from, to, newline_must_terminate)
    register Es_handle from, to;
    int             newline_must_terminate;
{
    void            *buf;
    int             read, write;
    Es_index        new_pos, pos, to_pos;
    int             test;

    if (!multibyte)
        buf = xv_malloc(ES_READ_BUF_LEN+4);
    else
        buf = xv_malloc((ES_READ_BUF_LEN+4) * sizeof(wchar_t));

    /*
     * Find start of "from" (it need not actually be 0, e.g. secondary pieces
     * that compose a selection)
     */
    pos = es_set_position(from, 0);
    write = 0;

    /*
     * Cannot just "es_replace(to, ES_INFINITY, ...)" in case not copying at
     * the end of "to", so get (and maintain) current position in "to".
     */

    to_pos = es_get_position(to);
    FOREVER {
	new_pos = es_read(from, ES_READ_BUF_LEN, buf, &read);
	if (read > 0) {
	    to_pos = es_replace(to, to_pos, read, buf, &write);
	    if (write < read)
		return (ES_SHORT_WRITE);
	} else if (pos == new_pos)
	    break;
	pos = new_pos;
    }

    if (!multibyte) {
        if (newline_must_terminate && (
		 (write <= 0) || 
                 (write > (ES_READ_BUF_LEN+4)) ||
		 (((char *)buf)[write - 1] != '\n'))) {
	    ((char *)buf)[0] = '\n';
	    (void) es_replace(to, ES_INFINITY, 1, buf, &write);
	    if (write < 1)
	        return (ES_SHORT_WRITE);
        }
    } else {
        if (newline_must_terminate && (
	       (write <= 0) || 
               (write > (ES_READ_BUF_LEN+4)) ||
	       (((wchar_t *)buf)[write - 1] != '\n'))) {
	    ((wchar_t *)buf)[0] = '\n';
	    (void) es_replace(to, ES_INFINITY, 1, buf, &write);
	    if (write < 1)
	        return (ES_SHORT_WRITE);
        }
    }
    
    free(buf);
    return(es_commit(to));
}
