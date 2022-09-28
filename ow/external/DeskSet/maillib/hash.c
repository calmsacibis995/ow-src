/* @(#)hash.c	3.2 - 93/05/20 */

/* hash.c -- a simple hash management package */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include "hash.h"
#include "ck_strings.h"

static void *hm_alloc();
static void hm_free();
static void hm_add();
static void hm_delete();
static void *hm_test();
static int hm_enumerate();

struct __hash_method hash_method = {
	hm_alloc,
	hm_free,
	hm_add,
	hm_delete,
	hm_test,
	hm_enumerate,
};



#define HASHSIZE	59

struct hash {
	struct hash *h_next;
	unsigned char *h_key;
	void *h_value;
};

char *nullfield = "";

int
hash_index(key)
char *key;
{
	register unsigned h;
	register u_char *s;
	register unsigned c;

	s = (u_char *) key;
	h = 0;
	while (*s) {
		c = *s++;
		if (isupper(c)) {
			c = tolower(c);
		}
		h = (h << 2) + c;
	}

	return (h % HASHSIZE);
}

void
free_hash(h)
struct hash *h;
{
	ck_free(h->h_key);
	if (h->h_value != nullfield) {
		ck_free(h->h_value);
	}
	ck_free(h);
}


static void *
hm_alloc()
{
	struct hash **table;

	table = ck_malloc(sizeof (struct hash) * HASHSIZE);
	memset(table, '\0', sizeof (struct hash) * HASHSIZE);
	return (table);
}


static void
hm_free(table)
struct hash **table;
{
	int i;
	struct hash *h;
	struct hash *next;

	if (!table)
		return;

	for (i = 0; i < HASHSIZE; i++) {
		for (h = table[i]; h; h = next) {
			/* remember this before freeing */
			next = h->h_next;

			free_hash(h);
		}
		table[i] = NULL;
	}
}

static void
hm_add(table, key, value, size)
struct hash **table;
char *key;
void *value;
int size;
{
	int index;
	register struct hash *h;

	if (!table)
		return;

	index = hash_index(key);
	h = ck_malloc(sizeof (struct hash));
	h->h_next = table[index];
	table[index] = h;
	h->h_key = (u_char *) ck_strdup(key);
	if (size && value != NULL) {
		h->h_value = ck_malloc(size);
		memcpy(h->h_value, value, size);
	} else {
		h->h_value = nullfield;
	}
}


static void
hm_delete(table, key)
struct hash **table;
char *key;
{
	register int index;
	register struct hash *h;
	register struct hash *old;

	if (!table)
		return;

	index = hash_index(key);
	old = NULL;
	h = table[index];
	while (h) {
		if (strcasecmp(h->h_key, key) == 0) {
			/* found the match */
			if (old == NULL)
				table[index] = h->h_next;
			else
				old->h_next = h->h_next;

			free_hash(h);
			break;
		}

		old = h;
		h = h->h_next;
	}
}


static void *
hm_test(table, key)
struct hash **table;
char *key;
{
	register struct hash *h;

	if (!table)
		return (NULL);

	h = table[hash_index(key)];

	while (h) {
		if (strcasecmp(h->h_key, key) == 0) {
			/* found a match */
			return (h->h_value);
		}

		h = h->h_next;
	}

	return (NULL);
}


static int
hm_enumerate(table, func, param)
struct hash **table;
int (*func)();
{
	register int	i;
	register int	e;
	register struct hash *h;

	if (!table)
		return (0);

	i = HASHSIZE;
	while (--i >= 0) {
		h = *table++;
		while (h) {
			if (e = (*func) (h->h_key, h->h_value, param))
				return (e);
			h = h->h_next;
		}
	}
	return (0);
}
