/*      @(#)file_list.h 1.8 93/04/07 SMI      */

/*
 *	(c) Copyright 1992, 1993 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE file
 *	for terms of the license.
 */

#ifndef xview_file_list_DEFINED
#define xview_file_list_DEFINED

#include <xview/panel.h>
#include <sys/stat.h>


extern Xv_pkg		file_list_pkg;
#define FILE_LIST	&file_list_pkg

typedef Xv_opaque File_list;


#define FILE_LIST_ATTR(type, ordinal)	ATTR(ATTR_PKG_FILE_LIST, type, ordinal)

typedef enum {
    FILE_LIST_DIRECTORY 	= FILE_LIST_ATTR(ATTR_STRING,		1),
    FILE_LIST_FILTER_STRING	= FILE_LIST_ATTR(ATTR_STRING,		2),
    FILE_LIST_MATCH_GLYPH 	= FILE_LIST_ATTR(ATTR_OPAQUE,		3),
    FILE_LIST_MATCH_GLYPH_MASK 	= FILE_LIST_ATTR(ATTR_OPAQUE,		20),
    FILE_LIST_FILTER_FUNC	= FILE_LIST_ATTR(ATTR_FUNCTION_PTR,	4),
    FILE_LIST_FILTER_MASK	= FILE_LIST_ATTR(ATTR_SHORT,		5),
    FILE_LIST_SHOW_DOT_FILES	= FILE_LIST_ATTR(ATTR_BOOLEAN,		6),
    FILE_LIST_AUTO_UPDATE	= FILE_LIST_ATTR(ATTR_BOOLEAN,		7),
    FILE_LIST_UPDATE		= FILE_LIST_ATTR(ATTR_NO_VALUE,		8),
    FILE_LIST_SHOW_DIR		= FILE_LIST_ATTR(ATTR_BOOLEAN,		9),
    FILE_LIST_USE_FRAME		= FILE_LIST_ATTR(ATTR_BOOLEAN,		10),
    FILE_LIST_DOTDOT_STRING	= FILE_LIST_ATTR(ATTR_STRING,		11),
    FILE_LIST_ABBREV_VIEW	= FILE_LIST_ATTR(ATTR_BOOLEAN,		12),
    FILE_LIST_CHANGE_DIR_FUNC	= FILE_LIST_ATTR(ATTR_FUNCTION_PTR,	13),
    FILE_LIST_COMPARE_FUNC	= FILE_LIST_ATTR(ATTR_FUNCTION_PTR,	14),
    FILE_LIST_ROW_TYPE		= FILE_LIST_ATTR(ATTR_INT,		15)

#ifdef OW_I18N
    /*
     * Wide Char Interface
     */
    /* carry over comma from above */,
    FILE_LIST_DIRECTORY_WCS 		= FILE_LIST_ATTR(ATTR_WSTRING,		16),
    FILE_LIST_FILTER_STRING_WCS		= FILE_LIST_ATTR(ATTR_WSTRING,		17),
    FILE_LIST_DOTDOT_STRING_WCS		= FILE_LIST_ATTR(ATTR_WSTRING,		18),
    FILE_LIST_WCHAR_NOTIFY		= FILE_LIST_ATTR(ATTR_BOOLEAN,		19)
#endif /* OW_I18N */    
} File_list_attr;




typedef enum {
    /* valid return values for FILE_LIST_FILTER_FUNC */
    FILE_LIST_IGNORE,
    FILE_LIST_ACCEPT,

    /* Ops to FILE_LIST_CHANGE_DIR_FUNC */
    FILE_LIST_BEFORE_CD,
    FILE_LIST_AFTER_CD,

    /*
     * values for FILE_LIST_FILTER_FUNC, matched field.
     * specifies if entry matched the FILE_LIST_FILTER_STRING
     */
    FILE_LIST_NOT_MATCHED,
    FILE_LIST_MATCHED
} File_list_op;




typedef enum {
    FILE_LIST_DOTDOT_TYPE,
    FILE_LIST_DIR_TYPE,
    FILE_LIST_FILE_TYPE
} File_list_row_type;




#define FILE_LIST_NULL_FILTER_FUNC	(File_list_op (*)())NULL

typedef enum {
    FL_NONE_MASK		= 0,
    FL_MATCHED_FILES_MASK	= (1<<0),
    FL_NOT_MATCHED_FILES_MASK	= (1<<1),
    FL_MATCHED_DIRS_MASK	= (1<<2),
    FL_NOT_MATCHED_DIRS_MASK	= (1<<3),
    FL_DOTDOT_MASK		= (1<<4),
    FL_ALL_MASK			= 0xffff
} File_list_filter_mask;




/*
 * Row structure for filter and compare funcs.
 */
typedef struct {
    File_list			file_list;
    Panel_list_row_values	vals;
    struct stat			stats;
    File_list_op		matched;
    char *			xfrm;		/* returned by strxfrm() */
    Xv_opaque			reserved; 	/* reserved for future use */
} File_list_row;

#ifdef OW_I18N
typedef struct {
    File_list			file_list;
    Panel_list_row_values_wcs	vals;
    struct stat			stats;
    File_list_op		matched;
    char *			xfrm;		/* returned by strxfrm() */
    Xv_opaque			reserved; 	/* reserved for future use */
} File_list_row_wcs;
#endif


/*
 * Built-in comparison functions
 */
#define FILE_LIST_DEFAULT_COMPARE_FUNC	file_list_no_case_ascend_compare
#define FILE_LIST_NULL_COMPARE_FUNC	(int (*)())NULL
EXTERN_FUNCTION(int file_list_no_case_ascend_compare, (File_list_row *row1, File_list_row *row2) );
EXTERN_FUNCTION(int file_list_no_case_descend_compare, (File_list_row *row1, File_list_row *row2) );
EXTERN_FUNCTION(int file_list_case_ascend_compare, (File_list_row *row1, File_list_row *row2) );
EXTERN_FUNCTION(int file_list_case_descend_compare, (File_list_row *row1, File_list_row *row2) );



/*
 * File_list object handle.
 */
typedef struct {
    Xv_panel_list	parent_data;
    Xv_opaque		private_data;
} File_list_public;

#endif	/* ~xview_file_list_DEFINED */
