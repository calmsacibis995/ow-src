/* @(#)hash.h	3.1 - 92/04/03 */


/* hash.h -- a simple hash management scheme */

/* N.B.: case is ignored in the key names */


struct __hash_method {
	void *(*hm_alloc)();
	void (*hm_free)( /* void *hash_table */ );
	void (*hm_add)( /* void *table, char *name, void *value, int size */ );
	void (*hm_delete)( /* void *table, char *name */ );
	void *(*hm_test)( /* void *table, char *name */ );
	int (*hm_enumerate)( /* void *table, int (*func)(), caddr_t param */ );
};


extern struct __hash_method hash_method;



