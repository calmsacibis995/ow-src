/* @(#)ck_strings.h	3.2 - 92/04/07 */

/* ck_strings.h -- define "no-error" string package */

#ifndef ck_strings_h_
#define ck_strings_h_

char *ck_strdup( /* char *buf */ );
void *ck_malloc( /* int len */ );
void ck_free( /* void *buf */ );
void *ck_zmalloc( /* int len */ );
void *ck_zfree( /* void *buf */ );
void *ck_mmap( /* char *path, int *size */ );
void *ck_wmmap( /* char *path, int *size */ );
void *ck_unmap( /* void *buf */ );

#endif ck_strings_h_
