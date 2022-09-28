/*
 * Copyright (c) 1991, Sun Microsystems, Inc.
 * Copyright (c) 1991, Nihon Sun Microsystems K.K.
 */

#pragma ident	"@(#)enconv.c 1.2	92/03/15	SMI"

#include <stdlib.h>
#include <dlfcn.h>
#include <sys/param.h>
#include "enconv.h"

#pragma weak enconv_open = _enconv_open

enconv_t
_enconv_open(const char *tocode, const char *fromcode)
{
	char libname[MAXPATHLEN];
	void *handle;
	enconv_t cd;
	void *(*openfp)();
	int f;

	sprintf(libname, _ICONV_PATH, fromcode, tocode);

	if ((cd = (enconv_t )malloc(sizeof(_enconv_info))) == NULL)
		return ((enconv_t)-1);
	
	if ((f = open(libname, 0)) >= 0)
		close(f);

	handle = dlopen(libname, RTLD_LAZY);
	if (dlerror() != NULL) {
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_handle = handle;

	openfp = (void *(*)())dlsym(handle, "_cv_open");
	if (dlerror() != NULL) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_close = (void (*)())dlsym(handle, "_cv_close");
	if (dlerror() != NULL) {
		dlclose(handle);
		free(cd);
		return ((enconv_t)-1);
	}

	cd->ecv_enconv = (size_t (*)())dlsym(handle, "_cv_enconv");
	if (dlerror() != NULL) {
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


#pragma weak enconv_close = _enconv_close
void
_enconv_close(enconv_t cd)
{
	(*cd->ecv_close)(cd->ecv_state);
	dlclose(cd->ecv_handle);
	free(cd);
}


#pragma weak enconv = _enconv
size_t
_enconv(enconv_t cd, char **inbuf, size_t *inbyteleft,
			char **outbuf, size_t *outbytesize)
{
	return ((*cd->ecv_enconv)(cd->ecv_state, inbuf, inbyteleft,
						outbuf, outbytesize));
}
