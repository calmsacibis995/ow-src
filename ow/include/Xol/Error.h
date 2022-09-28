#ifndef	_XOL_ERROR_H
#define	_XOL_ERROR_H

#pragma	ident	"@(#)Error.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:Error.h 1.8	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*
 *************************************************************************
 *
 * Description:
 *		This file contains standard error message strings for
 *	use in the routines OlVaDisplayErrorMsg() and
 *	OlVaDisplayWarningMsg().
 *
 *	When adding strings, the following conventions should be used:
 *
 *		1. Error classes begin with OlC, e.g.,
 *			#define OleCOlToolkitWarning	"OlToolkitWarning"
 *
 *		2. Error names begin with OleN, e.g.,
 *			#define	OleNinvalidResource	"invalidResource"
 *
 *		3. Error types begin with OleT, e.g.,
 *			#define	OleTsetValues		"setValues"
 *
 *		4. Error message strings begin with OleM and is followed
 *		   by the name string, and underbar '_', and concatenated
 *		   with the error type.  For the above error name and type.
 *
 *			#define OleMinvalidResource_setValues \
 *			   "SetValues: widget \"%s\" (class \"%s\"): invalid\
 *			    resource \"%s\", setting to %s"
 *
 *	Using these conventions, an example use of OlVaDisplayWarningMsg() 
 *	for a bad resource in FooWidget's SetValues procedure would be:
 *
 *	OlVaDisplayWarningMsg(display, OleNinvalidResource, OleTsetValues,
 *		OleCOlToolkitWarning, OleMinvalidResource_setValues,
 *		XtName(w), XtClass(w)->core_class.class_name,
 *		XtNwidth, "23");
 *
 *******************************file*header*******************************
 */


#ifdef	__cplusplus
extern "C" {
#endif


/*************************************************************************
 * Define the error classes here:  Use prefix of 'OleC'
 *************************************************************************/

#define OleCOlToolkitError		"OlToolkitError"
#define OleCOlToolkitWarning		"OlToolkitWarning"


/*************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************/

#define OleNbadItemAddress		"badItemAddress"
#define OleNbadItemIndex		"badItemIndex"
#define OleNinternal			"internal"
#define OleNinvalidArgCount		"invalidArgCount"
#define OleNinvalidDimension		"invalidDimension"
#define OleNinvalidItemRecord		"invalidItemRecord"
#define	OleNinvalidResource		"invalidResource"
#define OleNinvalidParameters		"invalidParameters"
#define OleNinvalidProcedure		"invalidProcedure"
#define	OleNnullWidget			"nullWidget"
#define OleNtooManyDefaults		"tooManyDefaults"
#define OleNtooManySet			"tooManySet"


/*************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************/

#define OleTbadItemResource		"badItemResource"
#define OleTbadNodeReference		"badNodeReference"
#define OleTbadVersion			"badVersion"
#define OleTcorruptedList		"corruptedList"
#define	OleTolDoGravity			"olDoGravity"
#define OleTflatState			"flatState"
#define	OleTinitialize			"initialize"
#define OleTinheritanceProc		"inheritanceProc"
#define OleTnullList			"nullList"
#define	OleTsetValues			"setValues"
#define	OleTsetValuesAlmost		"setValuesAlmost"
#define OleTwidgetSize			"widgetSize"


/*************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************/
 
#define OleNbadItemAddress_flatState	"widget \"%s\" (class \"%s\"):\
 \"%s\" called with NULL expanded item address for item_index %d"

#define OleMbadItemIndex_flatState	"widget \"%s\" (class\
 \"%s\"): \"%s\" called with invalid index value of %u"

#define OleMinternal_badNodeReference	"Widget \"%s\" (class\
 \"%s\"): internal error when referencing node in \"%s\" list"

#define OleMinternal_badVersion		"Widget Class\
 \"%s\" version mismatch: class %d vs. OLIT Toolkit %d"

#define OleMinternal_corruptedList	"Widget \"%s\" (class\
 \"%s\"): widget's internal \"%s\" list is corrupted"

#define OleMinvalidArgCount_flatState	"Argument count = %u for NULL\
 Arg list in %s when operating on widget \"%s\" (class: \"%s\").  Sub-object\
 was %u"

#define OleMinvalidItemRecord_flatState	"Widget Class \"%s\": full item\
 record size of %u is smaller than superclass's record size of %u"

#define OleMinvalidParameters_olDoGravity	"Unknown gravity value\
 %d in _OlDoGravity"

#define OleMinvalidProcedure_inheritanceProc	"Widget Class \"%s\"\
 has unresolved inheritance for class field %s"

#define OleMinvalidProcedure_setValuesAlmost	"Widget Class \"%s\"\
 has NULL SetValuesAlmost procedure"

#define OleMinvalidResource_flatState	"widget \"%s\" (class\
 \"%s\"): resource \"%s\" has invalid value for sub-object %d, setting to %s"

#define OleMinvalidResource_initialize	"Initialize: widget \"%s\" (class\
 \"%s\"): resource \"%s\" has invalid value, setting to %s"

#define OleMinvalidResource_badItemResource	"Widget \"%s\" (class\
 \"%s\"): skipping resource \"%s\" since it's not a valid flat item resource"

#define OleMinvalidResource_nullList	"Widget \"%s\" (class \"%s\"):\
 list resource \"%s\" has NULL value for non-zero \"%s\" count"

#define OleMinvalidResource_setValues	"SetValues: widget \"%s\" (class\
 \"%s\"): resource \"%s\" has invalid value, setting to %s"

#define OleMtooManyDefaults_flatState	"widget \"%s\" (class\
 \"%s\"): unselecting item %u because item %u is already the default item"

#define OleMtooManySet_flatState	"widget \"%s\" (class\
 \"%s\"): unselecting item %u because item %u is already set"

#define OleMinvalidDimension_widgetSize	"widget \"%s\" (class\
 \"%s\"): widget's width or height is zero, setting bad dimension to 1"

#define OleMnullWidget_flatState	"NULL widget id passed to %s"


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ERROR_H */
