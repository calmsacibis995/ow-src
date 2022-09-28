#ifndef lint
static 	char sccsid[] = "@(#)abbcache.c 3.7 92/06/26 Copyr 1992 Sun Micro";
#endif

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/*
 * Implementation of abbreviated appointment cache
 * This is a very simple-minded implementation
 */

#include <string.h>
#include <fcntl.h>

#include "appt.h"
#include "abbcache.h"

#ifdef DEBUG
static void dump_abbv_appt();
static void dump_abbv_appt_list();
void dump_range();
#endif

static Abb_Cache_List	abb_cache_list; /* List of abbreviated appointment 
					   caches.  One cache per calendar */

static int	cache_disabled = 0; /* Lets the client disable the cache */

/*
 * Turn the cache off.  Returns 1 if the cache was on, 0 if it was already off
 *
 * If the cache is off then abbcache_open() will always return NULL.
 */
int
abbcache_disable()

{
	int	rcode;

	rcode = !cache_disabled;
	cache_disabled = 1;
	return rcode;
}

/*
 * Turn cache back on.  Returns 1 if the cache was off, 0 if it was already on
 * By default the cache is on, so this routine needs to be used only if 
 * abbcache_disable() was previously called.
 */
int
abbcache_enable()

{
	int	rcode;

	rcode = cache_disabled;
	cache_disabled = 0;
	return rcode;
}

/*
 * Creates an abbreviated appointment cache for a specified calendar
 */
static Abb_Cache *
create_abbcache(calendar)

	char	*calendar;

{
	Abb_Cache	*p;

#ifdef DEBUG
	printf("*** create_abbcache()\n");
#endif
	if ((p = (Abb_Cache *)calloc(1, sizeof(Abb_Cache))) == NULL)
		return NULL;

	p->calendar = strdup(calendar);

	p->range.key1 = p->range.key2 = 0;

	return p;
}

static void
destroy_abbcache(cache)

	Abb_Cache	*cache;

{
#ifdef DEBUG
	printf("*** abbcache_destroy(%d)\n", cache);
#endif
	if (cache == NULL)
		return;

	if (cache->appt_list)
		destroy_abbrev_appt(cache->appt_list);

	if (cache->calendar)
		free(cache->calendar);

	free(cache);

	return;
}

/* 
 * Open an abbreviated appointment cache for a specified calendar.
 *
 * If an entry already exists in the cache list then it is returned.
 * If the entry does not already exist then NULL is returned unliess
 * O_CREAT is specified in which case a new cache is created.
 *
 * Upon succesfull completion abbcache_open() returns a handle to the
 * new cache.  If caching is disabled or if a new cache could not be
 * created then this routine returns NULL.
 */
Abb_Cache *
abbcache_open(calendar, oflag)

	char	*calendar;
	int	oflag;	/* O_CREAT to create entry if it doesn't exist */

{
	Abb_Cache	*p;

#ifdef DEBUG
	printf("*** abbcache_open(%s)\n", calendar);
#endif
	if (cache_disabled)
		return NULL;

	if (abb_cache_list.head == NULL) {
		/* List is empty. Create first entry */
		abb_cache_list.head =  create_abbcache(calendar);
		return abb_cache_list.head;
	}
		
	/* Search list of cache's to see if we have one for this calendar */
	p = abb_cache_list.head;
	while (1) {
		if (strcmp(p->calendar, calendar) == 0)
			return p;
		else if (p->next == NULL)
			break;
		else
			p = p->next;
	}

	/*
	 * Calendar's cache does not exist.  Create it and add to end
	 * of list.  We add it to the end since we expect the user's own
	 * calendar to be accessed first and most often so we want it to
	 * stay at the head.
	 */
	if (oflag & O_CREAT) {
		p->next = create_abbcache(calendar);
	}

	return p->next;
}

/*
 * Destroy the abbreviated appointment cache for a specified calendar and
 * remove it from the cache list
 */
void
abbcache_destroy(calendar)

	char	*calendar;

{
	Abb_Cache	*p, *trail_p;

	/* Search list for calendar */
	for (p = abb_cache_list.head, trail_p = NULL;
	     p != NULL;
	     trail_p = p, p = p->next) {

		if (strcmp(p->calendar, calendar) == NULL) {

			/* Found calnedar in cache list.  Remove node */
			if (trail_p == NULL) {
				/* First node in list */
				abb_cache_list.head = p->next;
			} else {
				trail_p->next = p->next;
			}

			/* Destory data structure */
			destroy_abbcache(p);
		}
	}

	return;
}

/*
 * Store a range of data in an abbreviated appointment cache.
 *
 * NOTE: The data is not copied! 
 */
void
abbcache_store_range(cache, range, r)

	Abb_Cache	*cache;
	Range		*range;
	Abb_Appt 	*r;

{
#ifdef DEBUG
	printf("*** abbcache_store_range(%d, %d, %d)\n", cache, range, r);
	dump_range(range);
#endif
	if (cache == NULL)
		return;

	/*
	 * For now we just cache one range of data.  Could enhance to
	 * cache noncontiguous chunks for multiple ranges.
	 */
	cache->range = *range;
	cache->appt_list = r;
	cache->last_key_fetched = 0;
	cache->last_appt_fetched = NULL;

	return;
}

/*
 * Flush all data out of the specified abbreviated appointment cache
 */
void
abbcache_flush(cache)

	Abb_Cache	*cache;

{
#ifdef DEBUG
	printf("*** abbcache_flush(%d)\n", cache);
#endif
	if (cache == NULL)
		return;

	if (cache->appt_list) {
		destroy_abbrev_appt(cache->appt_list);
		cache->appt_list = NULL;
	}

	cache->range.key1 = cache->range.key2 = 0;
}

/*
 * Same as abbcache_flush but lets the caller specify a calendar name instead
 * of an Abb_Cache *
 */
void
abbcache_cflush(calendar)

	char	*calendar;
{
	Abb_Cache	*abb_cache;

#ifdef DEBUG
	printf("*** abbcache_cflush(%s)\n", calendar);
#endif

	if ((abb_cache = abbcache_open(calendar, 0)) != NULL) {
		abbcache_flush(abb_cache);
		abbcache_close(abb_cache);
	}

	return;
}

/*
 * Flush all data out of all abbreviated appointment caches
 */
void
abbcache_flush_all()

{
	Abb_Cache	*p;
#ifdef DEBUG
	printf("*** abbcache_flush_all()\n");
#endif

	for (p = abb_cache_list.head; p != NULL; p = p->next) {
		abbcache_flush(p);
	}

	return;
}

/*
 * Fetch data for a given range out of the speicifed abbreviated appointment
 * cache.
 */
Cache_Status
abbcache_fetch_range(cache, range, r)

	Abb_Cache	*cache;
	Range		*range;
	Abb_Appt	**r;

{
	Abb_Appt	*start_p, *last_p, *tmp_p;

#ifdef DEBUG
	printf("*** abbcache_fetch_range(%d, %d, %d)\n", cache, range, r);
	dump_range(range);
#endif

	if (cache == NULL)
		return cacheFailed;

#ifdef DEBUG
	printf("\nContents of cache\n");
	dump_range(&(cache->range));
	dump_abbv_appt_list(cache->appt_list);
	printf("\nLast key fetched: ");
	print_tick(cache->last_key_fetched);
#endif

	/* 
	 * See if we have a hit
	 */
	if (range->key1 < cache->range.key1 || range->key2 > cache->range.key2)
		return cacheMiss;

	if (cache->appt_list == NULL) {
		/* Have a hit, but there are no appts to return */
		*r = NULL;
		return cacheHit;
	}

	/*
	 * We have a hit.  See if we recently made a similar fetch.  This
	 * is an optimization to reduce searching on a repaint.
	 */
	if (cache->last_appt_fetched != NULL && 
	    range->key1 >= cache->last_key_fetched) {
		start_p = cache->last_appt_fetched;
	} else {
		start_p = cache->appt_list;
	}

#ifdef DEBUG
	printf("\nStarting search at ");
	dump_abbv_appt(start_p);
#endif

	/*
	 * Find the appt which matches first key
	 */
	for (; start_p != NULL && range->key1 > start_p->appt_id.tick;
	     start_p = start_p->next) {
		;
	}

	/* 
	 * There were appointments at the start of the cache, but none
	 * in the range requested.
	 */
	if (start_p == NULL) {
		*r = NULL;
		return cacheHit;
	}

	/*
	 * Find appt which matches last key
	 */
	for (last_p = start_p;
	     last_p->next != NULL && range->key2 > last_p->next->appt_id.tick;
	     last_p = last_p->next) {
		;
	}

	/*
	 * Check for possibility that there are no appointments in the
	 * specified range.  We check for >= instead of just > since
	 * by cmsd semantics appointments falling on key2 are not included
	 * in the range.
	 */
	if (last_p->appt_id.tick >= range->key2) {
		*r = NULL;
	} else {
		cache->last_key_fetched = range->key1;
		cache->last_appt_fetched = start_p;
		/*
	 	 * Copy appointment list.  We temporarily NULL terminate list
		 * to fake-out the copy routine
		 */
		tmp_p = last_p->next;
		last_p->next = NULL;
		*r = copy_abbrev_appt(start_p);
		last_p->next = tmp_p;
	} 

#ifdef DEBUG
	printf("\nContents of return list\n");
	dump_range(range);
	dump_abbv_appt_list(*r);
	putchar('\n');
#endif

	return cacheHit;
}

#ifdef DEBUG

static void
dump_abbv_appt(p)

	Abb_Appt	*p;

{
	char	buf[80], *c;

	if (p != NULL) {
		c = strchr(p->what, '\n');
		strncpy(buf, p->what,  c - p->what);
		buf[c - p->what] = '\0';
		printf("%d\t%20s\n", p->appt_id.tick, buf);
	}

	return;
}

static void
dump_abbv_appt_list(p)

	Abb_Appt	*p;

{
	if (p != NULL) {
		dump_abbv_appt(p);
		dump_abbv_appt_list(p->next);
	}

	return;
}

static void
dump_range(r)
        Range *r;
{
        char *k1, *k2, *p;
 
        k1 = strdup(ctime(&(r->key1)));
        k2 = strdup(ctime(&(r->key2)));

	/* Nuke the @$!@# trailing newline */
	*(strrchr(k1, '\n')) = '\0';
	*(strrchr(k2, '\n')) = '\0';

        (void) printf ("\t(range = [%d -- %d]\n\t[%s -- %s])\n", r->key1,
			r->key2, k1, k2);

	free(k1);
	free(k2);
	return;
}

#endif
