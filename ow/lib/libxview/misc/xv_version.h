#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)xv_version.h 1.69 94/05/26";
#endif
#endif

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *      pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *      file for terms of the license.
 */

#ifndef xview_xv_version_DEFINED
#define xview_xv_version_DEFINED

 /*
  *  XView version number:
  *
  *  thousands digit signifies major release number
  *  hundreds digit signifies minor release number
  *  tens digit signifies dotdot release number
  *  ones digit signifies patch or specials release number
  *
  *  XView Base Version number:
  *
  *  Always remains at the value as if no patches existed.
  */

  /* For compatibility with old version variables */

#define xv_version_number xview_version
#define xv_version_string xv_version

  /* REMIND: this will need to be bumped with each release of XView */

#define XV_VERSION_NUMBER 3500
#define XV_BASE_VERSION_NUMBER 3500
#define XV_VERSION_STRING "XView Version 3.6 FCS"







extern unsigned short xview_version;
extern char *xv_version;

#endif /* ~xview_xv_version_DEFINED */
