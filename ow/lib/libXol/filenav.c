#pragma ident	"@(#)filenav.c	1.10    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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


/************************************************************************
 *
 * File chooser file navigation module
 *		
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces
 *
 ************************************************************************/

#include <ctype.h>		/* isalnum() */
#include <dirent.h>
#include <libintl.h>		/* dgettext() */
#include <limits.h>		/* NGROUPS_MAX, MAX_PATH */
#include <errno.h>		/* errno */
#include <pwd.h>		/* getpwnam(), struct passwd* */
#include <stdio.h>		/* NULL */
#include <stdlib.h>		/* getenv(), exit(), EXIT_SUCCESS */
#include <string.h>		/* strlen(), strchr() */
#include <widec.h>		/* wschr() */

#if	!defined(NGROUPS_MAX) || !defined(MAX_PATH)
	#include <sys/param.h>	/* NGROUPS_UMAX, MAXPATHLEN */
#endif	/* NGROUPS_MAX || MAX_PATH */

#include <sys/stat.h>		/* stat() */
#include <sys/types.h>		/* boolean_t, uid_t, gid_t, cnt_t */
#include <unistd.h>		/* geteuid(), getegid(), getgroups() */

#include <X11/Intrinsic.h>	/* String, XtNumber(), XtCallbackProc */

#include <Xol/FileCh.h>		/* OlStat, OlFNavNode */
#include <Xol/FileChP.h>	/* _OlUserInfo */
#include <Xol/OpenLook.h>	/* OlGlyphRec */
#include <Xol/OpenLookP.h>	/* OlMsgsDomain */
#include <Xol/Datum.h>
#include <Xol/SBTree.h>
#include <Xol/diags.h>		/* _OlReport() */

#include <Xol/filenav.h>	/* Interface of this implementation */


/***********************************************************************
 *
 *	Module private manifest constants and macros definitions
 *
 ************************************************************************/

#ifdef	NGROUPS_MAX
	#define	ATMOST_NGROUPS	(NGROUPS_MAX)
#else
	#define	ATMOST_NGROUPS	(NGROUPS_UMAX)
#endif	/* NGROUPS_MAX */

#ifdef	MAX_PATH
	#define	BIGGEST_PATH	(MAX_PATH + 1)
#else
	#define	BIGGEST_PATH	(MAXPATHLEN)
#endif	/* MAX_PATH */

#define	WHITE_SPACE_CHARS	("\n \t")


/************************************************************************
 *
 *      Forward Declaration of module private functions
 *
 ************************************************************************/

static boolean_t	can_search_folder(const char *const path, 
	const uid_t euid, const gid_t egid, const gid_t** groupsp, 
	int* num_groupsp, OlStat* stat_bufpp);

static boolean_t	can_visit_folder(const char *const path, 
	const uid_t euid, const gid_t egid, const gid_t** groupsp, 
	int* num_groupsp, OlStat* stat_bufpp);

static boolean_t	can_read(OlStat stat_bufp, const uid_t euid, 
	const gid_t egid, const gid_t** groupsp, int* num_groupsp);

static boolean_t	can_write(OlStat stat_bufp, const uid_t euid, 
	const gid_t egid, const gid_t** groupsp, int* num_groupsp);

static boolean_t	belongs_to_gid(const int key_gid, const gid_t egid, 
	const gid_t** grouplistp, int* gidsetsizep);

static boolean_t	init_grouplist(const gid_t** grouplistp, 
	int* gidsetsizep);

static boolean_t	copy_path_to_buffer(String to, const char*const from);
static boolean_t	strip_leading_space(String path);
static boolean_t	strip_trailing_space(String path);
static boolean_t	strip_trailing_slashes(String path);
static boolean_t	expand_tilde_n_substitue_shell_variables(String path);
static boolean_t	expand_dot(String path);

#ifdef	DEBUG
static void		visit_folder_test(void);

static void		effective_access_test(const int argc, 
	const char* const argv[]);
#endif	/* DEBUG */


/************************************************************************
 *
 *      Implementation of this module's public functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlFileNavFisitFolder -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavVisitFolder(
	const char* const	path,			/* IN */
	
	const uid_t		euid,			/* IN */
	const gid_t		egid,			/* IN */
	const gid_t**		groupsp,		/* IN/OUT */
	int*			ngroupsp,		/* IN/OUT */
	
	const OlDefine		operation,		/* IN */
	const boolean_t		hide_dot_files,		/* IN */
	const boolean_t		show_inactive,		/* IN */	
	const boolean_t		show_glyphs,		/* IN */	
	const String		filter_string,		/* IN */	
	const XtCallbackProc	filter_proc,		/* IN */
	const OlComparisonFunc	comparison_func,	/* IN */
		
	_OlSBTree*		new_treep,		/* OUT */
	OlStat*			stat_bufpp		/* IN/OUT */
)
{
	Cardinal		count;			/* unused */
	DIR*			dirp;			/* directory pointer */
	struct dirent*		entryp;
	char			*ep_buf;		/* entry path buffer */
	OlGlyphRec		eg_buf;			/* entry glyph buffer */
	OlStatRec		es_buf;			/* entry stat buffer */
	OlFNavNodeRec		en_buf;			/* entry node buffer */
	_OlDatumRec		ed_buf;			/* entry datum buffer */

	if (!can_visit_folder(path, euid, egid, groupsp, ngroupsp, stat_bufpp))
		return B_FALSE;
	
	if ((DIR*)NULL == (dirp = opendir(path))) {
		_OlReport("opendir()", dgettext(OlMsgsDomain, 
			"_OlFileNavVisitFolder():  NULL directory pointer for "
			"%s.\n"), path);

		return B_FALSE;
	}

	if (!(ep_buf = malloc(MAXNAMLEN))) 
		return B_FALSE;

	_OlSBTreeConstruct(new_treep, (_OlDatumComparisonFunc)comparison_func);
	
	ed_buf.type = _OL_DATUM_TYPE_FNAVNODE;
	ed_buf.content = (XtPointer)&en_buf;

	eg_buf.type = OL_GLYPH_TYPE_XIMAGE;
	eg_buf.rep = (OlGlyphRep)NULL;

	en_buf.glyph = &eg_buf;
	en_buf.sbufp = &es_buf;

	for (entryp = readdir(dirp), count = 0; 
			(struct dirent*)NULL != entryp; 
			entryp = readdir(dirp), count++) {

		en_buf.name = entryp->d_name;

		if (_OlFileNavIsDotFolder(en_buf.name) ||	
				_OlFileNavIsDotDotFolder(en_buf.name) || 
				(hide_dot_files && 
					_OlFileNavIsDotFile(en_buf.name)))
			continue;

		(void) snprintf(ep_buf, MAXNAMLEN, "%s/%s", path, en_buf.name);

		if (BAD_SYSCALL == stat(ep_buf, en_buf.sbufp))
			if (BAD_SYSCALL == lstat(ep_buf, en_buf.sbufp))
				_OlReport("stat()", dgettext(OlMsgsDomain, 
					"_OlFileNavVisitFolder():  failed on "
					"%s.\n"), ep_buf);

		if (S_ISDIR(en_buf.sbufp->st_mode)) {
			en_buf.is_folder = B_TRUE;

			en_buf.operational = can_visit_folder(ep_buf, euid, 
				egid, groupsp, ngroupsp, &en_buf.sbufp);
		} else {
			/*
			 * special files have not been excluded
			 */
			en_buf.is_folder = B_FALSE;

			switch (operation) {

			case OL_OPEN:
			/*FALLTHROUGH*/
			case OL_INCLUDE:
				en_buf.operational = can_read(en_buf.sbufp, 
					euid, egid, groupsp, ngroupsp);
				break;

			case OL_SAVE:
			/*FALLTHROUGH*/
			case OL_SAVE_AS:
			/*FALLTHROUGH*/
			case OL_DO_COMMAND:
				en_buf.operational = can_write(en_buf.sbufp, 
					euid, egid, groupsp, ngroupsp);
				break;
			}
		}

		/*! Filter callback and glyphs !*/
		en_buf.filtered = B_TRUE;

		en_buf.active = (en_buf.is_folder && en_buf.operational) || 
			(!en_buf.is_folder && en_buf.operational && 
				!en_buf.filtered);

		if (en_buf.active || show_inactive)
			_OlSBTreeInsert(new_treep, (const _OlDatum)&ed_buf);
	}

	closedir(dirp);
	free(ep_buf);
	return B_TRUE;
} /* end of _OlFileNavVisitFolder() */


/************************************************************************
 *
 *      _OlFileNavCanVisitFolder -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavCanVisitFolder(
	const char*const	path,	/* IN */
	_OlUserInfo		u_info	/* IN */
)
{
	OlStat			stat_bufp = NULL;
		
	return can_visit_folder(path, u_info->euid, u_info->egid, 
		&u_info->groups, &u_info->num_groups, &stat_bufp);
} /* end of _OlFileNavVisitFolder() */


/************************************************************************
 *
 *      _OlFileNavCanReadDocument -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavCanReadDocument(
	const char*const	path,	/* IN */
	_OlUserInfo		u_info	/* IN */
)
{
	boolean_t		ok = B_FALSE;
	OlStatRec		stat_buf;
		
	if (BAD_SYSCALL == stat(path, &stat_buf))
		_OlReport("stat()", dgettext(OlMsgsDomain, 
			"_OlFileNavCanReadDocument():  failed on %s.\n"), path);
	else if (!S_ISDIR(stat_buf.st_mode))
		ok = can_read(&stat_buf, u_info->euid, u_info->egid, 
			&u_info->groups, &u_info->num_groups);

	return ok;
} /* end of _OlFileNavCanReadDocument() */


/************************************************************************
 *
 *      _OlFileNavCanWriteDocument -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavCanWriteDocument(
	const char*const	path,	/* IN */
	_OlUserInfo		u_info	/* IN */
)
{
	boolean_t		ok = B_FALSE;
	OlStatRec		stat_buf;
		
	if (BAD_SYSCALL == stat(path, &stat_buf))
		_OlReport("stat()", dgettext(OlMsgsDomain, 
			"_OlFileNavCanWriteDocument():  failed on %s.\n"), path);
	else if (!S_ISDIR(stat_buf.st_mode))
		ok = can_write(&stat_buf, u_info->euid, u_info->egid, 
			&u_info->groups, &u_info->num_groups);

	return ok;
} /* end of _OlFileNavCanWriteDocument() */


/************************************************************************
 *
 *      _OlFileNavIsDotFolder -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavIsDotFolder(
	const char*const	path	/* IN */
)
{

	/*!I18N!*/
	return (0 == strcmp(path, "."));
} /* end of _OlFileNavIsDotFolder() */


/************************************************************************
 *
 *      _OlFileNavIsDotDotFolder -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavIsDotDotFolder(
	const char*const	path	/* IN */
)
{

	/*!I18N!*/
	return (0 == strcmp(path, ".."));
} /* end of _OlFileNavIsDotDotFolder() */


/************************************************************************
 *
 *      _OlFileNavIsPath -- is the OlStr string a path specification? 
 *
 ************************************************************************/

boolean_t 
_OlFileNavIsPath(
	const OlStr		olstr,		/* IN */
	const OlStrRep		text_format	/* IN */
)
{
	Boolean			found;

	switch (text_format) {
	case OL_SB_STR_REP:
	/*FALLTHROUGH*/
	case OL_MB_STR_REP:
		found = (NULL != strchr((const char*)olstr, '/'));
		break;
	case OL_WC_STR_REP:
		found = (NULL != wschr((wchar_t*)olstr, L'/'));
		break;
	}
	
	return found;		
} /* end of _OlFileNavIsPath() */


/************************************************************************
 *
 *      _OlFileNavIsAbsolutePath -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavIsAbsolutePath(
	const char*const	path	/* IN */
)
{

	/*!I18N!*/
	return (NULL != path && '/' == *path);
} /* end of _OlFileNavIsAbsolutePath() */


/************************************************************************
 *
 *      _OlFileNavIsDotFile -- 
 *
 ************************************************************************/

boolean_t 
_OlFileNavIsDotFile(
	const char*const	path	/* IN */
)
{

	/*!I18N!*/
	return ('.' == *path);
} /* end of _OlFileNavIsDotFile() */


/************************************************************************
 *
 *  _OlFileNavParentFolder -- 
 *
 *	Return the logical parent folder name (by stripping the last
 *	component of the path).  Works similarly to /usr/bin/dirname
 *
 *	IN		OUT
 *	/		/
 *	//		/
 *	//foo//		/
 *	//foo//bar//	/foo
 *	(error)		NULL
 *
 ************************************************************************/

String
_OlFileNavParentFolder(
	const char*const		path	/* IN */
)
{
	static char		path_buf[BIGGEST_PATH];

	String			ptr;
	boolean_t		found = B_FALSE;

	GetToken();

	if (
		_OlFileNavIsAbsolutePath(path) &&
		copy_path_to_buffer(path_buf, path)
	) {
		/*! I18N !*/
	
		/*
		 * Trim single or multiple trailing slash characters (/p/ /p//)
		 */
		ptr = path_buf + strlen(path_buf) - 1; /* point to last char */
		while (path_buf != ptr && '/' == *ptr)
			--ptr;
	
		/* 
		 * "/"
		 */
		if (path_buf == ptr && '/' == *ptr)
			found = B_TRUE;
		else while (!found && path_buf != ptr--)
			if ('/'  == *ptr)
				found = B_TRUE;
	}
	
	if (found) {
		while (path_buf != ptr && '/' == *ptr)
			ptr--;
		*(ptr + 1) = '\0';
		ReleaseToken();
		return path_buf;
	} else{
		ReleaseToken();
		return NULL;
	}
} /* end of _OlFileNavParentFolder() */


/************************************************************************
 *
 *  _OlFileNavExpandedPathName -- 
 *
 ************************************************************************/

String
_OlFileNavExpandedPathName(
	const char*const path_exp		/* IN */
)
{
	static char		str_buf[BIGGEST_PATH];

	String			expanded_path = str_buf;

	GetToken();

	if (
		NULL != path_exp && 
		copy_path_to_buffer(expanded_path, path_exp) &&
		strip_leading_space(expanded_path) && 
		strip_trailing_space(expanded_path) &&
		strip_trailing_slashes(expanded_path) &&
		expand_tilde_n_substitue_shell_variables(expanded_path) &&
		expand_dot(expanded_path)

		/*! 
			still need to cannonize
				..
				<current_folder>/./foo, 
				<current_folder>/../foo
		!*/
	) {
		ReleaseToken();
		return expanded_path;
	} else {
		ReleaseToken();
		return NULL;
	}
} /* end of _OlFileNavExpandedPathName() */


/************************************************************************
 *
 *      Implementation of this module's private functions
 *
 ************************************************************************/
 
 
/************************************************************************
 *
 *      can_search_folder
 *      can_visit_folder
 *      can_read
 *	can_write --
 *
 *	If the stat buffer is available it may be provided through the 
 *	pointer stat_bufpp to avoid the expense of the stat(2) system call.
 *	Otherwise, the new stat buffer may be retrieved through this same
 *	pointer, while care is taken by the caller to copy that information
 *	from static data storage.
 *
 *      can_search_folder
 *      can_visit_folder
 *	can_read
 *      can_write
 *	belongs_to_gid --
 *
 *	If the membership list is available it may be provided through
 *	the pointers gidsetsizep and grouplistp to avoid the expense of
 *	the getgroups(2) system call.  Otherwise, the new membership list
 *	may be retrieved through these same pointers, while care is taken
 *	by the caller to copy that information from static data storage.
 *
 ************************************************************************/

/************************************************************************
 *
 *      can_search_folder -- can the directory be searched by the current 
 *		user, using her effective user and group ids?
 *
 ************************************************************************/

static struct stat	stat_buf;

static boolean_t
can_search_folder(
	const char* const	path,		/* IN */
	const uid_t		euid,		/* IN */
	const gid_t		egid,		/* IN */
	const gid_t**		groupsp,	/* IN/OUT */
	int*			num_groupsp,	/* IN/OUT */
	OlStat*			stat_bufpp	/* IN/OUT */
)
{
	OlStat			stat_bufp;
	boolean_t		ok = B_FALSE;
	
	if (NULL == *stat_bufpp)
		*stat_bufpp = &stat_buf;
	
		if (BAD_SYSCALL == stat(path, &stat_buf))
			_OlReport("stat()", dgettext(OlMsgsDomain, 
				"can_search_folder():  failed on %s.\n"),
				path);

	stat_bufp = *stat_bufpp;
	if (S_ISDIR(stat_bufp->st_mode)) {
		if (stat_bufp->st_uid == euid) {
			if (stat_bufp->st_mode & S_IXUSR)
				ok = B_TRUE;
		} else if (belongs_to_gid(stat_bufp->st_gid, egid, groupsp,
				num_groupsp)) {
			if (stat_bufp->st_mode & S_IXGRP)
				ok = B_TRUE;
		} else if (stat_bufp->st_mode & S_IXOTH)
			ok = B_TRUE;
	} else
		_OlReport(NULL, dgettext(OlMsgsDomain, 
			"can_search_folder(): %s is not a folder.\n"), path);
	
	return ok;
} /* end of can_search_folder() */


/************************************************************************
 *
 *      can_visit_folder -- can the directory contents be seen by the 
 *		current user, using her effective user and group ids?
 *
 ************************************************************************/

static boolean_t
can_visit_folder(const char* const path, const uid_t euid, const gid_t egid,
	const gid_t** groupsp, int* num_groupsp, OlStat* stat_bufpp)
{
	OlStat			stat_bufp;
	boolean_t		ok = B_FALSE;
	
	if (NULL == *stat_bufpp)
		*stat_bufpp = &stat_buf;
	
		if (BAD_SYSCALL == stat(path, &stat_buf)) {
			/*! Notice !*/
			_OlReport("stat()", dgettext(OlMsgsDomain, 
				"can_visit_folder():  failed on %s.\n"),
				path);
			return B_FALSE;
		}

	stat_bufp = *stat_bufpp;
	if (S_ISDIR(stat_bufp->st_mode)) {
		if (stat_bufp->st_uid == euid) {
			if (stat_bufp->st_mode & (S_IRUSR | S_IXUSR))
				ok = B_TRUE;
		} else if (belongs_to_gid(stat_bufp->st_gid, egid, groupsp,
				num_groupsp)) {
			if (stat_bufp->st_mode & (S_IXGRP | S_IXGRP))
				ok = B_TRUE;
		} else if (stat_bufp->st_mode & (S_IXOTH | S_IXOTH))
			ok = B_TRUE;
	} else
		_OlReport(NULL, dgettext(OlMsgsDomain, 
			"can_visit_folder(): %s is not a folder.\n"), path);
	
	return ok;
} /* end of can_visit_folder() */


/************************************************************************
 *
 *      can_read -- can the file be read by the current user,
 *		using her effective user and group ids?
 *
 ************************************************************************/

static boolean_t
can_read(OlStat stat_bufp, const uid_t euid, const gid_t egid,
	const gid_t** groupsp, int* num_groupsp)
{
	boolean_t		ok = B_FALSE;
	
	if (stat_bufp->st_uid == euid) {
		if (stat_bufp->st_mode & S_IRUSR)
			ok = B_TRUE;
		} else if (belongs_to_gid(stat_bufp->st_gid, egid, groupsp,
				num_groupsp)) {
		if (stat_bufp->st_mode & S_IRGRP)
			ok = B_TRUE;
	} else if (stat_bufp->st_mode & S_IROTH)
		ok = B_TRUE;
	
	return ok;
} /* end of can_read() */

/************************************************************************
 *
 *      can_write -- can the file be written to by the current 
 *		user, using her effective user and group ids?
 *
 ************************************************************************/

static boolean_t
can_write(OlStat stat_bufp, const uid_t euid, const gid_t egid,
	const gid_t** groupsp, int* num_groupsp)
{
	boolean_t		ok = B_FALSE;
	
	if (stat_bufp->st_uid == euid) {
		if (stat_bufp->st_mode & S_IWUSR)
			ok = B_TRUE;
		} else if (belongs_to_gid(stat_bufp->st_gid, egid, groupsp,
				num_groupsp)) {
		if (stat_bufp->st_mode & S_IWGRP)
			ok = B_TRUE;
	} else if (stat_bufp->st_mode & S_IWOTH)
		ok = B_TRUE;
	
	return ok;
} /* end of can_write() */


/************************************************************************
 *
 *      belongs_to_gid -- find out if the user belongs to any of the 
 *		supplementary groups
 *
 ************************************************************************/

static boolean_t
belongs_to_gid(
	const int	key_gid,	/* IN */
	const gid_t	egid,		/* IN */
	const gid_t**	grouplistp,	/* IN/OUT */
	int*		gidsetsizep	/* IN/OUT */
)
{			
	int		i;
	boolean_t	found = B_FALSE;

	#define	ELEMENT(ptr, type, index) \
		(*((type*)((char*)(ptr) + (index) * sizeof (type))))

	if (egid == key_gid)
		found = B_TRUE;
	else {
		if (NULL == *grouplistp) {
			static gid_t	gl_buf[ATMOST_NGROUPS];
			
			*grouplistp = gl_buf;
			(void) init_grouplist(grouplistp, gidsetsizep);
		}

		for (i = 0, found = B_FALSE; i < *gidsetsizep && !found; i++)
			if (ELEMENT(*grouplistp, gid_t, i) == key_gid)
				found = B_TRUE;
	}
	
	return found;
	#undef	ELEMENT
} /* end of belongs_to_gid() */


/************************************************************************
 *
 *      init_grouplist -- Allocate and fill the group membership list
 *		for a user
 *
 ************************************************************************/

static boolean_t
init_grouplist(const gid_t** grouplistp, int* gidsetsizep)
{
	boolean_t		ok = B_FALSE;
	int			egidsetsize;
	
	/*
	 * Find the size of the array needed to store the supplementary 
	 * set of group id's.
	 */
	if (BAD_SYSCALL == (*gidsetsizep = getgroups(0, NULL)))
		_OlReport("getgroups()", dgettext(OlMsgsDomain, 
		"init_grouplist():  could not process user group "
		"information.\n"));
	else {
		if (*gidsetsizep == (egidsetsize = getgroups(*gidsetsizep,
				(gid_t*)*grouplistp)))
			ok = B_TRUE;
		else
			_OlMessage("gidsetsize mismatch:  put = %d, "
				"got = %d.\n", *gidsetsizep, egidsetsize);
	}
	
	return ok;
} /* end of init_grouplist() */


/************************************************************************
 *
 *  copy_path_to_buffer --
 *
 *	Assumes the target to be large enough.
 *
 ************************************************************************/

static boolean_t
copy_path_to_buffer(
	String			to,		/* OUT */
	const char*const	from		/* IN */
)
{
	Boolean		ok = B_FALSE;

	if (
		NULL != from && '\0' != *from && 
		(size_t)BIGGEST_PATH >= (strlen(from) + 1) &&
		NULL != strncpy(to, from, BIGGEST_PATH)
	)
		ok = B_TRUE;

	return ok;

} /* end of copy_path_to_buffer() */


/************************************************************************
 *
 *  expand_tilde_n_substitue_shell_variables -- 
 *
 ************************************************************************/

static boolean_t
expand_tilde_n_substitue_shell_variables(
	String 			path	/* IN/OUT */
)
{
	String			beg_word;
	String			next_slash;
	String			start;
	String			end;
	String			temp;
	char			copied_str[BIGGEST_PATH];
	String			expans;
	String			final_str;
	int			t;
	int			cur_len;

	strncpy(copied_str, path, BIGGEST_PATH);
	start = temp = copied_str;
	cur_len = strlen(temp);
	end = temp + cur_len;

	for (beg_word = temp; beg_word < end;) {
		/* for each slash component */

		/* find next slash */
		for (next_slash = beg_word;
			next_slash < end && *next_slash != '/'; next_slash++);

		switch (*beg_word) {
		case '/':	/* double slash means root */
			if (beg_word != start) {
				start = beg_word;
			}
			beg_word = next_slash + 1;
			break;

		case '~':	/* a home directory name */
			if (next_slash <= beg_word + 1) {
				/* current user's directory */
				expans = getenv("HOME");
			} else {
				/* another user's directory */
				struct passwd*		pwd;

				t = *next_slash;
				*next_slash = '\0';
				pwd = getpwnam(beg_word + 1);
				*next_slash = t;
				if (pwd == NULL)
					goto DEFAULT;
				expans = pwd->pw_dir;
			}
			goto EXPAND;

		case '$':	/* an environment variable */
			t = *next_slash;
			*next_slash = '\0';
			expans = getenv(beg_word + 1);
			*next_slash = t;

	EXPAND:
			if (expans == NULL)
				goto DEFAULT;

			t = strlen(expans);

			/* Strip trailing slash in expansion */
			if (expans[t - 1] == '/')
				t--;

			{
				int	siz_change = t + (next_slash - beg_word);

				cur_len += siz_change;
				if (cur_len > BIGGEST_PATH - 1)
					return NULL;
			}

			(void) memmove(beg_word + t, next_slash, 
				end + 1 - next_slash);
			end = beg_word + t + (end - next_slash);
			(void) memcpy(beg_word, expans, t);
			break;

		default:	/* all unrecognized words */
	DEFAULT:
			beg_word = next_slash + 1;
			break;
		}
	}

	/* Copy out the final output string from start */
	temp = path;
	/* tack on current directory if necessary: */
	if (*start != '/') {
		(void) getcwd(temp, BIGGEST_PATH - cur_len);
		temp += strlen(temp);
		if (*(temp - 1) == '/')
			temp--;
	}
	(void) strncpy(path, start, BIGGEST_PATH);
	
	return ('\0' != *path);
} /* end of expand_tilde_n_substitue_shell_variables() */


/************************************************************************
 *
 *  expand_dot --
 *
 * 		.	=>	current folder
 *		..	=>	parent folder
 *							(absolute paths)
 *
 ************************************************************************/

static boolean_t
expand_dot(
	String		path		/* IN/OUT */
)
{
	boolean_t	ok = B_FALSE;

	/*! should be with respect to the current folder !*/

	if (0 == strcmp(".", path))
		if (NULL != (path = getcwd(path, BIGGEST_PATH)))
			ok = B_TRUE;
		else
			_OlReport("getcwd()", dgettext(OlMsgsDomain, 
				"expand_dot():  could not determine the "
				"current working directory.\n"));
	else
		ok = B_TRUE;

	return ok;
} /* end of expand_dot() */


/************************************************************************
 *
 *  strip_leading_space --
 *
 ************************************************************************/

static boolean_t
strip_leading_space(
	String		path		/* IN/OUT */
)
{
	String			ptr = path;

	while ('\0' != *ptr && NULL != strchr(WHITE_SPACE_CHARS, *ptr))
		ptr++;
	memmove(path, ptr, strlen(ptr) + 1);
	
	return ('\0' != *ptr);
} /* end of strip_leading_space() */


/************************************************************************
 *
 *  strip_trailing_space --
 *
 ************************************************************************/

static boolean_t
strip_trailing_space(
	String		path		/* IN/OUT */
)
{
	String	src;
	cnt_t	len;

	src = path + (len = strlen(path)) - 1;

	while (0 != len-- && NULL != strchr(WHITE_SPACE_CHARS, *src))
		*src-- = '\0';

	return ('\0' != *path);
} /* end of strip_trailing_space() */


/************************************************************************
 *
 *  strip_trailing_slashes -- Trim single or multiple trailing slash characters
 *
 *		/foo/ /foo//	=>	foo
 *
 ************************************************************************/

static boolean_t
strip_trailing_slashes(
	String		path		/* IN/OUT */
)
{
	String			ptr = path + strlen(path) - 1; 
					/* point to last char */

	while (path != ptr && '/' == *ptr)
		*ptr-- = '\0';

	return ('\0' != *path);
} /* end of strip_trailing_slashes() */


#ifdef	DEBUG
/************************************************************************
 *
 *  self-test
 *
 ************************************************************************/


/************************************************************************
 *
 *      effective_access_test -- 
 *
 ************************************************************************/

static void
effective_access_test(const int argc, const char *const argv[])
{
	const uid_t		my_euid = geteuid();
	const gid_t		my_egid = getegid();
	const gid_t*		groups;
	int			ngroups;
	String			path;
	int			i;
	OlStat			sbufp;
	boolean_t		s, v, r, w;
	const char *const	test_data[] = { "/usr", "/etc/shadow",  
						"/etc/passwd" };

	#define	ARGS(a_path, a_search, a_visit, a_read, a_write) \
		"You can%s search folder, can%s visit folder, \n" \
		"\tcan%s read, and can%s write %s.\n", \
		(a_search) ? "" : " not", (a_visit) ? "" : " not", \
		(a_read) ? "" : " not", (a_write) ? "" : " not", (path)

	errno = 0;
	
	for (i = 0; path = (String)test_data[i], i < XtNumber(test_data); ++i) {
		groups = NULL;
		ngroups = 0;
		sbufp = NULL;
		
		s = can_search_folder(path, my_euid, my_egid, &groups, &ngroups,
			 &sbufp);
		v = can_visit_folder(path, my_euid, my_egid, &groups, &ngroups,
			 &sbufp);
		r = can_read(sbufp, my_euid, my_egid, &groups, &ngroups);
		w = can_write(sbufp, my_euid, my_egid, &groups, &ngroups);

		_OlMessage(ARGS(path, s, v, r, w));
	}

	if (argc > 1)	/* invoked from the command line */
		for (i = 0; path = (String)argv[i], i < argc; ++i) {
			groups = NULL;
			ngroups = 0;
			sbufp = NULL;
			
			s = can_search_folder(path, my_euid, my_egid, &groups, 
				&ngroups, &sbufp);
			v = can_visit_folder(path, my_euid, my_egid, &groups, 
				&ngroups, &sbufp);
			r = can_read(sbufp, my_euid, my_egid, &groups, 
				&ngroups);
			w = can_write(sbufp, my_euid, my_egid, &groups, 
				&ngroups);
	
			_OlMessage(ARGS(path, s, v, r, w));
		}

	#undef	ARGS
} /* end of effective_access_test() */


/************************************************************************
 *
 *      visit_folder_test -- 
 *
 ************************************************************************/

static void
visit_folder_test(void)
{
	uid_t			my_euid = geteuid();
	gid_t			my_egid = getegid();
	const gid_t*		groups = NULL;
	int			ngroups = 0;
	OlStat			sbufp;
	_OlSBTree		tree;

	_OlFileNavVisitFolder(".", my_euid, my_egid, &groups, &ngroups, OL_OPEN, 
		TRUE, TRUE, TRUE, NULL, NULL, NULL, &tree, &sbufp);

	_OlSBTreePrint(tree);
	_OlSBTreeDestruct(&tree);
} /* end of visit_folder_test() */


/************************************************************************
 *
 *  output --
 *
 ************************************************************************/

static void
output(
	const char*		path		/* IN */
)
{
	const char*const	null = "NULL";
	String			result = _OlFileNavExpandedPathName(path);

	(void) printf("\"%s\"\t\t=>\t\"%s\"\n", NULL == path ? null : path, 
		NULL == result ? null : result);
} /* end of output() */


/************************************************************************
 *
 *  expanded_path_test
 *
 ************************************************************************/

static void
expanded_path_test(void)
{
	const char*		str;
	int			i;

	const char*const	test_data[] = { 
					".", 
					"~",  
					"~/", 
					"./", 
					" 	 ", 
					"~root", "~root/",
					" 	/usr	 ", 
					"/usr/",
					"..", "../", 
					"$HOME", 
					"$HOME$OPENWINHOME", "$HOME.c",
					"xxx$HOME", "${HOME}", 
					"${HOME}!", "\\$HOME"
				};

	for (i = 0; str = test_data[i], i < XtNumber(test_data); ++i)
		output((String)str);
} /* end of expanded_path_test() */


/************************************************************************
 *
 *  _OlFileNavTest --
 *
 ************************************************************************/

void
_OlFileNavTest(const int argc, const char*const argv[])
{

	errno = 0;

	effective_access_test(argc, argv);
	visit_folder_test();
	expanded_path_test();

	exit(EXIT_SUCCESS);
} /* end of _OlFileNavTest() */

#endif	/* DEBUG */


/* end of filenav.c */
