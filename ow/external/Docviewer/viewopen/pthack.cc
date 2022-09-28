#ident "@(#)pthack.cc	1.3 06/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/list.h>
//#include <doc/listx.h>
//#include <doc/abclient.h>
//#include <doc/abgroup.h>
//#include <doc/abinfo.h>
//#include <doc/abname.h>
//#include <doc/attrlist.h>
//#include <doc/book.h>
//#include <doc/bookmark.h>
//#include <doc/bookname.h>
//#include <doc/cardcat.h>
//#include <doc/cardcats.h>
//#include <doc/ps_link.h>
//#include <doc/scopekey.h>
//#include <doc/searchdoc.h>
//#include <doc/zonewght.h>

static int	SortCompare(const void *, const void *) { return(0); }

void
ParameterizedTypesHack()
{
	int	n;

	// XXX I don't know why this is necessary, but without it,
	// we occasionally get undefined symbols:
	//
	//	Undefined			first referenced
	//	 symbol  			    in file
	//   ABCLIENT::~ABCLIENT(void)    ./ptrepository/LISTX__pt__11_P8ABCLIENT.o
	//
//	CARDCATS *cardcats = new CARDCATS;
//	ABGROUP	*abgroup = new ABGROUP(*cardcats);
//	delete abgroup;

//	LIST<ABCLIENT*>	abclient_list;
//	abclient_list.Add(NULL);
//	abclient_list.Insert(NULL, 0);
//	abclient_list[0] = NULL;
//	abclient_list.Delete(0);
//	n = abclient_list.Count();
//
//	LISTX<ABCLIENT*>	abclient_listx;
//	abclient_listx.Add(NULL);
//	abclient_listx.Insert(NULL, 0);
//	abclient_listx[0] = NULL;
//	abclient_listx.Delete(0);
//	n = abclient_listx.Count();

//	LIST<ABINFO*>	abinfo_list;
//	abinfo_list.Add(NULL);
//	abinfo_list.Insert(NULL, 0);
//	abinfo_list[0] = NULL;
//	abinfo_list.Delete(0);
//	n = abinfo_list.Count();
//	SortList(abinfo_list, SortCompare);

//	LISTX<ABINFO*>	abinfo_listx;
//	abinfo_listx.Add(NULL);
//	abinfo_listx.Insert(NULL, 0);
//	abinfo_listx[0] = NULL;
//	abinfo_listx.Delete(0);
//	n = abinfo_listx.Count();
//	SortList(abinfo_listx, SortCompare);

//	LIST<ABNAME>	abname_list;
//	ABNAME		abname;
//	abname_list.Add(abname);
//	abname_list.Insert(abname, 0);
//	abname_list[0] = abname;
//	abname_list.Delete(0);
//	n = abname_list.Count();

//	LIST<_ATTR*>	_attr_list;
//	_attr_list.Add(NULL);
//	_attr_list.Insert(NULL, 0);
//	_attr_list[0] = NULL;
//	_attr_list.Delete(0);
//	n = _attr_list.Count();

//	LIST<BOOK*>	book_list;
//	book_list.Add(NULL);
//	book_list.Insert(NULL, 0);
//	book_list[0] = NULL;
//	book_list.Delete(0);
//	n = book_list.Count();

//	LISTX<BOOK*>	book_listx;
//	book_listx.Add(NULL);
//	book_listx.Insert(NULL, 0);
//	book_listx[0] = NULL;
//	book_listx.Delete(0);
//	n = book_listx.Count();

//	LIST<BOOKMARK*>	bookmark_list;
//	bookmark_list.Add(NULL);
//	bookmark_list.Insert(NULL, 0);
//	bookmark_list[0] = NULL;
//	bookmark_list.Delete(0);
//	n = bookmark_list.Count();

//	LISTX<BOOKMARK*>	bookmark_listx;
//	bookmark_listx.Add(NULL);
//	bookmark_listx.Insert(NULL, 0);
//	bookmark_listx[0] = NULL;
//	bookmark_listx.Delete(0);
//	n = bookmark_listx.Count();

//	LIST<BOOKNAME>	bookname_list;
//	BOOKNAME	bookname;
//	bookname_list.Add(bookname);
//	bookname_list.Insert(bookname, 0);
//	bookname_list[0] = bookname;
//	bookname_list.Delete(0);
//	n = bookname_list.Count();

//	LISTX<CARDCAT*>	cardcat_listx;
//	cardcat_listx.Add(NULL);
//	cardcat_listx.Insert(NULL, 0);
//	cardcat_listx[0] = NULL;
//	cardcat_listx.Delete(0);
//	n = cardcat_listx.Count();

//	LIST<DOCNAME>	docname_list;
//	DOCNAME		docname;
//	docname_list.Add(docname);
//	docname_list.Insert(docname, 0);
//	docname_list[0] = docname;
//	docname_list.Delete(0);
//	n = docname_list.Count();

//	LIST<FILE*>	file_list;
//	file_list.Add(NULL);
//	file_list.Insert(NULL, 0);
//	file_list[0] = NULL;
//	file_list.Delete(0);
//	n = file_list.Count();

//	LIST<PSLINK*>	pslink_list;
//	pslink_list.Add(NULL);
//	pslink_list.Insert(NULL, 0);
//	pslink_list[0] = NULL;
//	pslink_list.Delete(0);
//	n = pslink_list.Count();

//	LISTX<PSLINK*>	pslink_listx;
//	pslink_listx.Add(NULL);
//	pslink_listx.Insert(NULL, 0);
//	pslink_listx[0] = NULL;
//	pslink_listx.Delete(0);
//	n = pslink_listx.Count();

//	LIST<SCOPEKEY*>	scopekey_list;
//	scopekey_list.Add(NULL);
//	scopekey_list.Insert(NULL, 0);
//	scopekey_list[0] = NULL;
//	scopekey_list.Delete(0);
//	n = scopekey_list.Count();

//	LISTX<SCOPEKEY*>	scopekey_listx;
//	scopekey_listx.Add(NULL);
//	scopekey_listx.Insert(NULL, 0);
//	scopekey_listx[0] = NULL;
//	scopekey_listx.Delete(0);
//	n = scopekey_listx.Count();

//	LIST<SEARCHDOC*>	searchdoc_list;
//	searchdoc_list.Add(NULL);
//	searchdoc_list.Insert(NULL, 0);
//	searchdoc_list[0] = NULL;
//	searchdoc_list.Delete(0);
//	n = searchdoc_list.Count();
//
//	LISTX<SEARCHDOC*>	searchdoc_listx;
//	searchdoc_listx.Add(NULL);
//	searchdoc_listx.Insert(NULL, 0);
//	searchdoc_listx[0] = NULL;
//	searchdoc_listx.Delete(0);
//	n = searchdoc_listx.Count();

	LIST<STRING>	string_list;
	string_list.Add(NULL_STRING);
	string_list.Insert(NULL_STRING, 0);
	string_list[0] = NULL_STRING;
	string_list.Delete(0);
	n = string_list.Count();

//	LIST<ZONEWGHT*>	zonewght_list;
//	zonewght_list.Add(NULL);
//	zonewght_list.Insert(NULL, 0);
//	zonewght_list[0] = NULL;
//	zonewght_list.Delete(0);
//	n = zonewght_list.Count();
//
//	LISTX<ZONEWGHT*>	zonewght_listx;
//	zonewght_listx.Add(NULL);
//	zonewght_listx.Insert(NULL, 0);
//	zonewght_listx[0] = NULL;
//	zonewght_listx.Delete(0);
//	n = zonewght_listx.Count();
}
