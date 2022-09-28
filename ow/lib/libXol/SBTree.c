#pragma ident	"@(#)SBTree.c	1.6    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

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
 *	Implementation of the Sorted Binary Tree module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#include <stdio.h>		/* NULL, printf(), scanf(), putchar(), EOF */
#include <errno.h>		/* errno */
#include <sys/types.h>		/* boolean_t */

#include <Xol/Datum.h>
#include <Xol/diags.h>		/* _OL_MALLOC(), _OL_FREE() */

#include <Xol/SBTree.h>		/* Interface of this module */
#include <Xol/SBTreeP.h>


/************************************************************************
 *
 *      Module Private Global Storage Allocation
 *
 ************************************************************************/

static _OlDatumComparisonFunc	the_comparison_func = 
					(_OlDatumComparisonFunc)NULL;


/************************************************************************
 *
 *      Forward Declarations
 *
 ************************************************************************/

static int		get_insertion_node(const _OlSBTree top,
	const _OlDatum datum, _OlSBTNode* match_npp);

static _OlSBTNode	new_leaf(const _OlDatum datum);


/************************************************************************
 *
 *      Implementation of this module's public functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlSBTreeConstruct -- Initialize the tree
 *
 ************************************************************************/

void
_OlSBTreeConstruct(_OlSBTree* topp, _OlDatumComparisonFunc cmp_func)
{

	*topp = (_OlSBTree)NULL;
	the_comparison_func = cmp_func;
}


/************************************************************************
 *
 *      _OlSBTreeDestruct -- Destroy the tree
 *
 ************************************************************************/

void
_OlSBTreeDestruct(_OlSBTree* topp)
{

	if (NULL == topp && NULL != *topp) {
		_OlSBTNode	np = *topp;
		_OlSBTree	lt;
		_OlSBTree	gt;
	
		_OlDatumDestruct(&(np->datum));
		lt = (_OlSBTree)np->lesser_node;
		gt = (_OlSBTree)np->greater_node;
		_OlSBTreeDestruct(&lt);
		_OlSBTreeDestruct(&gt);
		_OL_FREE(np);
	}
	
	*topp = (_OlSBTree)NULL;
}


/************************************************************************
 *
 *      _OlSBTreeInsert -- Insert datum into the tree
 *
 ************************************************************************/

void
_OlSBTreeInsert(_OlSBTree* topp, const _OlDatum datum)
{
	_OlSBTNode	np;
	int		cmp_val;

	if (NULL == topp)
		_OlAbort(NULL, "Invalid tree pointer passed to "
			"_OlSBTreeInsert().\n");
		/*NOTREACHED*/
	else if ((cmp_val = get_insertion_node(*topp, datum, &np)) <
			0)
		np->lesser_node = new_leaf(datum);
	else if (cmp_val > 0)
		np->greater_node = new_leaf(datum);
	else if (NULL == np) {
		*topp = (_OlSBTree)new_leaf(datum);
	} else if (0 == np->reference_count) {
		/* Data with same key was previously deleted. */
		_OlDatumDestruct(&np->datum);
		_OlDatumConstruct(&np->datum, datum);
		np->reference_count = 1;
	} else {
		/* Data with same key already exists. */
		#ifdef	_OLDATUM_MERGE
		_OlDatumMerge(np->datum, &datum);
		#endif
		np->reference_count++;
	}
}


/************************************************************************
 *
 *      _OlSBTreeGet -- Returns pointer to the datum matching a key
 *	(Does not expose the opacity of the module's data structures)
 *
 ************************************************************************/

_OlDatum
_OlSBTreeGet(const _OlSBTree top, const _OlDatum datum)
{
	_OlSBTNode	np;
	
	if (get_insertion_node(top, datum, &np) == 0 &&
	    NULL != np &&
	    np->reference_count != 0)
		return (np->datum);
	else
		return ((_OlDatum)NULL);
}


/************************************************************************
 *
 *      _OlSBTreeDelete -- Mark node with datum as deleted 
 *			(by decrementing its reference count)
 *
 ************************************************************************/

boolean_t
_OlSBTreeDelete(const _OlSBTree top, const _OlDatum datum)
{
	_OlSBTNode	np;
	boolean_t	ok = B_FALSE;

	if (get_insertion_node(top, datum, &np) == 0 &&
			NULL != np && 
			np->reference_count != 0) {
		np->reference_count--;
		ok = B_TRUE;
	}
	
	return ok;
	
}


/************************************************************************
 *
 *      _OlSBTreeIsEmpty -- B_TRUE if tree is empty
 *
 ************************************************************************/

boolean_t
_OlSBTreeIsEmpty(const _OlSBTree top)
{

	return (boolean_t)((_OlSBTree)NULL == top);
}


/************************************************************************
 *
 *      _OlSBTreeTraverse -- In order traversal of the tree
 *
 ************************************************************************/

/*ARGSUSED2*/
void
_OlSBTreeTraverse(const _OlSBTree top, _OlDatumTraversalFunc tr_func,
	void* arg1, void* arg2, void* arg3, void* arg4)
{
	_OlSBTNode	np = (_OlSBTNode)top;
	
	if (NULL != np) {
		_OlSBTreeTraverse(np->lesser_node, tr_func, arg1, arg2, arg3,
			arg4);
		(*tr_func)(np->datum, arg1, arg2, arg3, arg4);
		_OlSBTreeTraverse(np->greater_node, tr_func, arg1, arg2, arg3,
			arg4);
	}
}


#ifdef	NOT_YET
#include <stdarg.h>
/*VARARGS3*/
void
_OlSBTreeTraverse(const _OlSBTree top, _OlDatumTraversalFunc tr_func, cnt_t nargs, ...)
{
	_OlSBTNode		np = (_OlSBTNode)top;
	va_list			args;
	
	if (NULL != np) {
		if (0 != nargs) {
			void*	array[MAXARGS];
			cnt_t	arg_no = 0;
			
			va_start(args, nargs);
			while (arg_no < nargs)
				array[arg_no++] = va_arg(args, void*);
			va_end(args);
		}

		_OlSBTreeTraverse(np->lesser_node, tr_func, nargs, array);
		(*tr_func)(np->datum);
		_OlSBTreeTraverse(np->greater_node, tr_func, nargs, array);
	}
}
#endif


#ifdef	DEBUG
/************************************************************************
 *
 *      _OlSBTreePrint -- In order dump of the tree
 *
 ************************************************************************/

void
_OlSBTreePrint(const _OlSBTree top)
{
	_OlSBTNode	np = (_OlSBTNode)top;
	
	if (NULL != np) {
		_OlSBTreePrint(np->lesser_node);
		_OlDatumPrint(np->datum);
		(void) printf("\tNode Count = %d\n", np->reference_count);
		_OlSBTreePrint(np->greater_node);
	}
}
#endif	/* DEBUG */


/************************************************************************
 *
 *      Implementation of the module's private functions
 *
 ************************************************************************/
 

/************************************************************************
 *
 *      get_insertion_node -- Find the location of the node where
 *		datum would be inserted.
 *
 * Returns the comparison value of the datum to be inserted to the datum 
 * at the insertion point.  *match_npp is set to NULL if no insertion
 * point is found, i.e., on an empty tree.
 *
 ************************************************************************/

static int
get_insertion_node(const _OlSBTree top, const _OlDatum datum, 
	_OlSBTNode* match_npp)
{
	_OlSBTNode	np = top;
	int		cmp_val = 0;
	boolean_t	done = B_FALSE;

	if (np != (_OlSBTree)NULL)
		while (!done) {
			if ((cmp_val = _OlDatumCompare(datum, np->datum, 
					the_comparison_func)) < 0) {
				if (NULL != np->lesser_node)
					np = np->lesser_node;
				else
					done = B_TRUE;
			} else if (cmp_val > 0) {
				if (NULL != np->greater_node)
					np = np->greater_node;
				else
					done = B_TRUE;
			} else
				done = B_TRUE;
		}
	
	*match_npp = np;
	return cmp_val;
}

/************************************************************************
 *
 *      new_leaf -- Create a new tree node
 *
 ************************************************************************/

static _OlSBTNode
new_leaf(const _OlDatum datum)
{
	_OlSBTNode	lp;

	_OL_MALLOC(lp, _OlSBTNodeRec);

	_OlDatumConstruct(&lp->datum, datum);
	lp->reference_count = 1;
	lp->lesser_node = NULL;
	lp->greater_node = NULL;

	return lp;
}	


#ifdef	DEBUG
/************************************************************************
 *
 *	Self-test
 *
 ************************************************************************/

/*ARGSUSED1*/
void
_OlSBTreeTest(
	const int		argc, 
	const char* const	argv[]		/* unused */
)
{
	_OlSBTree		tree;
	_OlDatumRec		value_buffer;
	char			string[64];
	int			i;
	int			nc = 0;
	const char *const	test_data[] = { "red", "green", "blue", "black",
		"white", "black", "red", "red", "yellow", "cyan", "magenta" };

	errno = 0;

	value_buffer.type = _OL_DATUM_TYPE_STRING;

	_OlSBTreeConstruct(&tree, NULL);
	for (i = 0; value_buffer.content = (XtPointer)test_data[i],
				i < XtNumber(test_data);
			++i) {
		_OlSBTreeInsert(&tree, (_OlDatum)&value_buffer);
	}
	if (!_OlSBTreeIsEmpty(tree))
		_OlSBTreePrint(tree);
	_OlSBTreeDestruct(&tree);

	if (argc > 1) {
		_OlSBTreeConstruct(&tree, NULL);
		
		(void) printf("Enter one string:  ");
		while  ((nc = scanf("%s", &string)) != EOF) {
			if (nc != 1)
				_OlAbort(NULL, "error -- read %d integers; "
					"expected 1.\n", nc);
			else {
				value_buffer.content = (XtPointer)string;
				_OlSBTreeInsert(&tree, (_OlDatum)&value_buffer);
			}
			(void) printf("\nEnter one string:  ");
	
		}
		(void) putchar('\n');
	
		if (!_OlSBTreeIsEmpty(tree))
			_OlSBTreePrint(tree);
		_OlSBTreeDestruct(&tree);
	}
}
#endif	/* DEBUG */


/* end of SBTree.c */
