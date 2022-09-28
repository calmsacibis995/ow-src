/*
 * Copyright (c) 1991, Sun Microsystems, Inc.
 * Copyright (c) 1991, Nihon Sun Microsystems K.K.
 */

#ident	"@(#)enconv.c	3.4 - 92/06/24"

#include <stdlib.h>
#include <dlfcn.h>
#include <sys/param.h>
#include "enconv.h"

enconv_t
enconv_open(const char *tocode, const char *fromcode)
{
	char libname[MAXPATHLEN];
	void *handle;
	enconv_t cd;
	void *(*openfp)();
	int f;

	sprintf(libname, _ICONV_PATH, fromcode, tocode);

	if ((cd = (enconv_t )malloc(sizeof(_enconv_info))) == NULL)
		return ((enconv_t)-1);
	
	handle = dlopen(libname, RTLD_LAZY);
	if (handle == NULL) {
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_handle = handle;

	openfp = (void *(*)())dlsym(handle, "_cv_open");
	if (openfp == NULL) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_close = (void (*)())dlsym(handle, "_cv_close");
	if (cd->ecv_close == NULL) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_enconv = (size_t (*)())dlsym(handle, "_cv_enconv");
	if (cd->ecv_enconv == NULL) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	if ((cd->ecv_state = (*openfp)()) == (struct _cv_state *)-1) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	return (cd);
}


void
enconv_close(enconv_t cd)
{
	(*cd->ecv_close)(cd->ecv_state);
	dlclose(cd->ecv_handle);
	free(cd);
}


size_t
enconv(enconv_t cd, char **inbuf, size_t *inbyteleft,
			char **outbuf, size_t *outbytesize)
{
	return ((*cd->ecv_enconv)(cd->ecv_state, inbuf, inbyteleft,
						outbuf, outbytesize));
}
