/*	@(#)abbcache.h 3.2 IEI SMI	*/

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

#ifndef abbcache_h
#define	abbcache_h

/*
 * Interface for abbreviated appointment cache
 */
#define	abbcache_close(c)	/* Currently a no-op */

typedef enum cache_status {
	cacheHit,		/* Data found in cache */
	cacheMiss,		/* No data found in cache */
	cacheBad,		/* Cache is corrupted */
	cacheFailed,		/* Operation failed */
} Cache_Status;


/*
 * A cache of abbreviated appointments for a particular calendar
 */
typedef struct abb_cache {
	char		*calendar;		/* Calendar this cache is for*/
	Range		range;			/* Range cache covers */
	Abb_Appt	*appt_list;		/* Cache of appointments */
	long		last_key_fetched;	/* Key used by last fetch */
	Abb_Appt	*last_appt_fetched;	/* First appointment that
						 * corresponds to
					         * last_fetched_key */
	struct abb_cache *next;			/* Next calendar's cache */
} Abb_Cache;

/*
 * A collection of Abb_Caches.  We make it a struct since we'll need 
 * a spot for locks when we go MT.
 */
typedef struct abb_cache_list {
	Abb_Cache	*head;
} Abb_Cache_List;

extern int		abbcache_disable();
extern int		abbcache_enable();
extern Abb_Cache	*abbcache_open();
extern Cache_Status	abbcache_fetch_range();
extern void		abbcache_store_range();
extern void		abbcache_flush();
extern void		abbcache_flush_all();
extern void		abbcache_cflush();
extern void		abbcache_destroy();

#endif
