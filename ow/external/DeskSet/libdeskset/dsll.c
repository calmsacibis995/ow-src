#ifndef lint
static char sccsid [] = "@(#)dsll.c 1.1 93/01/10 Copy 1990 Sun Micro";
#endif

#include <sys/types.h>		/** for standard types */
#include <fcntl.h>

#ifdef DSLL_DEBUG
#define DP if(1)
#else
#define DP if(0)	
#endif

#define	NULL	0
#define	TRUE	1
#define FALSE	0

typedef unsigned int	ALIGN;
typedef unsigned int	bool;

struct dsll_item
{
	struct		dsll_item	*next;
	struct		dsll_item	*prev;
	struct		dsll_item	*head;
	unsigned		size;
	ALIGN 			_align;
};
typedef  struct dsll_item  *DSLL_TYPE;

#define NULLPTR (DSLL_TYPE)NULL
#define NULLCHR (ALIGN *)NULL

/* forward pointers */
ALIGN		*dsll_add_a(ALIGN *data, unsigned int size);
ALIGN		*dsll_add_b(ALIGN *data, unsigned int size);
ALIGN		*dsll_add_a_cp(ALIGN *data, unsigned int size, char *strng);
ALIGN		*dsll_add_b_cp(ALIGN *data, unsigned int size, char *strng);
ALIGN		*dsll_append(ALIGN *data1, ALIGN *data2);
DSLL_TYPE	dsll_brk_link(DSLL_TYPE rec);
DSLL_TYPE	dsll_ck_rec(ALIGN *data, bool ck_null_list);
void		dsll_debug(ALIGN *data, void (*func) ());
ALIGN		*dsll_del(ALIGN *data);
bool		dsll_del_list(ALIGN *data);
ALIGN		*dsll_first(ALIGN *data);
ALIGN		*dsll_get(char *file);
ALIGN		*dsll_gt_handle(ALIGN *data);
bool		dsll_if_first(ALIGN *data);
bool		dsll_if_last(ALIGN *data);
ALIGN		*dsll_init();
ALIGN		*dsll_last(ALIGN *data);
void		dsll_mk_link(DSLL_TYPE np, DSLL_TYPE rec);
bool		dsll_move(ALIGN *data1, ALIGN *data2);
ALIGN		*dsll_next(ALIGN *data);
ALIGN		*dsll_next_e(ALIGN *data);
ALIGN		*dsll_prev(ALIGN *data);
ALIGN		*dsll_prev_e(ALIGN *data);
bool		dsll_save(ALIGN *list, char *file);
ALIGN		*dsll_set_first(ALIGN *data);
int		dsll_size(ALIGN *data);
void		dsll_sort(ALIGN *data, int (*func) ());
bool		dsll_walk(ALIGN *data, bool(*func) (), int *p0, int *p1, int *p2, int *p3);

/*
 *  Functional Description:
 *      This routine adds an item to the list
 *     of the indicated size after the indicated data item.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_add_a(data,size)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   size
 *         TYPE:        unsigned
 *         I/O:         input
 *         DESCRIPTION: size of added item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec  - This routine is one of a set that maintains a
 *	dsll_mk_link - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (invalid record input)
 *         return (failure)
 *       if (size not given)
 *         return (failure)
 *       if (malloc fails)
 *         return (failure)
 *       save record size
 *       make links from record to list
 *       DSLL_MK_LINK - This routine is one of a set that maintains a
 *       return (pointer to records data area)
 *
 */


ALIGN *
dsll_add_a(ALIGN *data, unsigned int size)
{
	register DSLL_TYPE np;
	register DSLL_TYPE rec;

	DP printf("*** add_a: data = %X size = %u\n", data, size, 0);

	/* invalid record input */
	if ((rec = dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* size not given */
	if (size <= 0)
	{
		/* failure */
		return (NULLCHR);
	}

	/* malloc fails */
	if ((np = (DSLL_TYPE) malloc(sizeof(*rec) + size)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	DP printf("*** add_a: passed checks, np = %X \n", np, 0, 0);

	/* save record size */
	np->size = size;

	/* make links from record to list */
	dsll_mk_link(np, rec);

	/* pointer to records data area */
	return (&np->_align);
}

/*
 *  Functional Description:
 *      This routine adds an item to the list
 *     of the indicated size after the indicated data item and copies
 *     the data pointed to by strng to the data area of the new record.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_add_a_cp(data,size,strng)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   size
 *         TYPE:        unsigned
 *         I/O:         input
 *         DESCRIPTION: size of added item
 *
 *         PARAMETER:   strng
 *         TYPE:        char *
 *         I/O:         input
 *         DESCRIPTION: string to add
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec  - This routine is one of a set that maintains a
 *	dsll_mk_link - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       if (no size given)
 *         return (failure)
 *       if (malloc fails)
 *         return (failure)
 *       save record size
 *       make links from record to list
 *       DSLL_MK_LINK - This routine is one of a set that maintains a
 *       copy strng to data area
 *       return (return pointer to records data area)
 *
 */


ALIGN *
dsll_add_a_cp(ALIGN *data, unsigned int size, char *strng)
{
	register DSLL_TYPE np;
	register DSLL_TYPE rec;

	DP printf("*** add_a_cp: data = %X size = %u\n", data, size, 0);

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* no size given */
	if (size <= 0)
	{
		/* failure */
		return (NULLCHR);
	}

	/* malloc fails */
	if ((np = (DSLL_TYPE) malloc(sizeof(*rec) + size)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	DP printf("*** add_a_cp: passed checks, np = %X \n", np, 0, 0);

	/* save record size */
	np->size = size;

	/* make links from record to list */
	dsll_mk_link(np, rec);

	/* copy strng to data area */
	memcpy(&np->_align, strng, size);

	/* return pointer to records data area */
	return (&np->_align);
}

/*
 *  Functional Description:
 *      This routine inserts an item of the
 *     indicated size BEFORE the indicated item (becomming the first
 *     record if needed).
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_add_b(data,size)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   size
 *         TYPE:        unsigned
 *         I/O:         input
 *         DESCRIPTION: size of added item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_add_a     - This routine is one of a set that maintains a
 *	dsll_ck_rec    - This routine is one of a set that maintains a
 *	dsll_gt_handle - This routine is one of a set that maintains a
 *	dsll_if_first  - This routine is one of a set that maintains a
 *	dsll_prev      - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       debug statement
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       if (the first item)
 *         add to top of list
 *         DSLL_ADD_A - This routine is one of a set that maintains a
 *         DSLL_GT_HANDLE - This routine is one of a set that maintains
 *       else (otherwise)
 *         if (not an empty list)
 *           go to previous item
 *           DSLL_PREV - This routine is one of a set that maintains a
 *         debug statement
 *         add item after
 *         DSLL_ADD_A - This routine is one of a set that maintains a
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_add_b(ALIGN *data, unsigned int size)
{
	register DSLL_TYPE rec;

	/** debug statement */
	DP printf("*** add_b: data = %X size = %u\n", data, size, 0);

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** add_b: rec = %X\n", rec, 0, 0);

	/* the first item */
	if (dsll_if_first(data))
	{
		/* add to top of list */
		data = dsll_add_a(dsll_gt_handle(data), size);
	}
	else			/* otherwise */
	{
		/* not an empty list */
		if (rec->head->next != NULLPTR)
		{
			/* go to previous item */
			data = dsll_prev(data);
		}

		/** debug statement */
		DP printf("*** add_b: prev = %X\n", data, 0, 0);

		/* add item after */
		data = dsll_add_a(data, size);
	}

	/** debug statement */
	DP printf("*** add_b: returned data = %X\n", data, 0, 0);

	/* pointer to data area */
	return (data);
}

/*
 *  Functional Description:
 *      This routine inserts an item of the
 *     indicated size BEFORE the indicated item (becomming the first
 *     record if needed) and copies the data pointed to by strng to
 *     the data area.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_add_b_cp(data,size,strng)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   size
 *         TYPE:        unsigned
 *         I/O:         input
 *         DESCRIPTION: size of added item
 *
 *         PARAMETER:   strng
 *         TYPE:        char *
 *         I/O:         input
 *         DESCRIPTION: string to add
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_add_a_cp  - This routine is one of a set that maintains a
 *	dsll_ck_rec    - This routine is one of a set that maintains a
 *	dsll_gt_handle - This routine is one of a set that maintains a
 *	dsll_if_first  - This routine is one of a set that maintains a
 *	dsll_prev      - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       debug statement
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       if (the first item)
 *         add to top of list
 *         DSLL_ADD_A_CP - This routine is one of a set that maintains a
 *         DSLL_GT_HANDLE - This routine is one of a set that maintains
 *       else (otherwise)
 *         if (not an empty list)
 *           go to previous item
 *           DSLL_PREV - This routine is one of a set that maintains a
 *         debug statement
 *         add item after
 *         DSLL_ADD_A_CP - This routine is one of a set that maintains a
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_add_b_cp(ALIGN *data, unsigned int size, char *strng)
{
	register DSLL_TYPE rec;

	/** debug statement */
	DP printf("*** add_b_cp: data = %X size = %u\n", data, size, 0);

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** add_b_cp: rec = %X\n", rec, 0, 0);

	/* the first item */
	if (dsll_if_first(data))
	{
		/* add to top of list */
		data = dsll_add_a_cp(dsll_gt_handle(data), size, strng);
	}
	else			/* otherwise */
	{
		/* not an empty list */
		if (rec->head->next != NULLPTR)
		{
			/* go to previous item */
			data = dsll_prev(data);
		}

		/** debug statement */
		DP printf("*** add_b_cp: prev = %X\n", data, 0, 0);

		/* add item after */
		data = dsll_add_a_cp(data, size, strng);
	}

	/** debug statement */
	DP printf("*** add_b_cp: returned data = %X\n", data, 0, 0);

	/* pointer to data area */
	return (data);
}

/*
 *  Functional Description:
 *      This routine appends list2 to list1
 *     with data2 following data1.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_append(data1,data2)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item 1
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item 1
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data1 or last of list1 if list1 header is used.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (check record 1 for valid input)
 *         return (failure)
 *       if (check record 2 for valid input)
 *         return (failure)
 *       debug statements
 *       if (both list are empty)
 *         null out old head information
 *         free space of old head
 *         return (list 1 head)
 *       if (first list is empty)
 *         debug statement
 *         change start pointer to start pointer of list 2
 *         save start pointer
 *         save head pointer
 *         while (for each item in list 2)
 *           change head pointer to point to list 1
 *           go to next item
 *         null out old head information
 *         free space of old head
 *         return (pointer to data area of first item in list 1)
 *       if (second list is empty)
 *         debug statement
 *         save head pointer
 *         null out old head information
 *         free space of old head
 *         return (pointer to data area of first item in list 2)
 *       if (head of list 1)
 *         reset rec1 to last item of list
 *       if (head of list 2)
 *         reset rec2 to first item of list
 *       save link pointers
 *       debug statements
 *       re-link items
 *       start at first record of list 2
 *       while (for each item in list 2)
 *         reset head record pointer to point to list 1
 *         go to next item in list 2
 *       null out values of head 2
 *       free space for head 2
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_append(ALIGN *data1, ALIGN *data2)
{
	register DSLL_TYPE rec1;
	register DSLL_TYPE rec2;
	register DSLL_TYPE next1;
	register DSLL_TYPE prev2;
	register DSLL_TYPE old_head;

	/* check record 1 for valid input */
	if ((rec1 = dsll_ck_rec(data1, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* check record 2 for valid input */
	if ((rec2 = dsll_ck_rec(data2, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statements */
	DP printf("*** append: data1 = %X\n", data1, 0, 0);
	DP printf("*** append: data2 = %X\n", data2, 0, 0);
	DP printf("*** append: rec1 = %X\n", rec1, 0, 0);
	DP printf("*** append: rec2 = %X\n", rec2, 0, 0);

	/* both list are empty */
	if (rec1->head->next == NULLPTR && rec2->head->next == NULLPTR)
	{
		/* null out old head information */
		rec2->head->head = NULLPTR;
		rec2->head->next = NULLPTR;
		rec2->head->prev = NULLPTR;

		/* free space of old head */
		free(rec2->head);

		/* list 1 head */
		return (&rec1->head->_align);
	}

	/* first list is empty */
	if (rec1->head->next == NULLPTR)
	{
		/** debug statement */
		DP printf("*** append: data1 is null\n", 0, 0, 0);

		/* change start pointer to start pointer of list 2 */
		rec1->head->next = rec2->head->next;

		/* save start pointer */
		next1 = rec2->head->next;

		/* save head pointer */
		old_head = rec2->head;

		/* for each item in list 2 */
		while (next1->head == old_head)
		{
			/* change head pointer to point to list 1 */
			next1->head = rec1->head;

			/* go to next item */
			next1 = next1->next;
		}

		/* null out old head information */
		old_head->head = NULLPTR;
		old_head->next = NULLPTR;
		old_head->prev = NULLPTR;

		/* free space of old head */
		free(old_head);

		/* pointer to data area of first item in list 1 */
		return (&rec1->head->next->_align);
	}

	/* second list is empty */
	if (rec2->head->next == NULLPTR)
	{
		/** debug statement */
		DP printf("*** append: data2 is null\n", 0, 0, 0);

		/* save head pointer */
		old_head = rec2->head;

		/* null out old head information */
		old_head->head = NULLPTR;
		old_head->next = NULLPTR;
		old_head->prev = NULLPTR;

		/* free space of old head */
		free(old_head);

		/* pointer to data area of first item in list 2 */
		return (data1);
	}

	/* head of list 1 */
	if (rec1 == rec1->head)
	{
		/* reset rec1 to last item of list */
		rec1 = rec1->head->next->prev;
	}

	/* head of list 2 */
	if (rec2 == rec2->head)
	{
		/* reset rec2 to first item of list */
		rec2 = rec2->head->next;
	}

	/* save link pointers */
	prev2 = rec2->prev;
	next1 = rec1->next;

	/** debug statements */
	DP printf("*** append: next1 = %X\n", next1, 0, 0);
	DP printf("*** append: prev2 = %X\n", prev2, 0, 0);

	/* re-link items */
	rec1->next = rec2;
	rec2->prev = rec1;
	prev2->next = next1;
	next1->prev = prev2;

	/* start at first record of list 2 */
	next1 = rec1->next;
	old_head = rec2->head;

	/* for each item in list 2 */
	while (next1->head == old_head)
	{
		/* reset head record pointer to point to list 1 */
		next1->head = rec1->head;

		/* go to next item in list 2 */
		next1 = next1->next;
	}

	/* null out values of head 2 */
	old_head->head = NULLPTR;
	old_head->next = NULLPTR;
	old_head->prev = NULLPTR;

	/* free space for head 2 */
	free(old_head);

	/* pointer to data area */
	return (&rec2->_align);
}

/*
 *  Functional Description:
 *      This routine breaks the link of an
 *     item to a list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_brk_link(rec)
 *
 *         PARAMETER:   rec
 *         TYPE:        register DSLL_TYPE
 *         I/O:         input
 *         DESCRIPTION: Input record
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to next rec after one broken.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:    None.
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (one item in list)
 *         debug statement
 *         null out list
 *         return (data pointer)
 *       else (more than one item in list)
 *         debug statement
 *         next item point to prev item
 *         prev item point to next item
 *         if (first item -- reset start pointer)
 *         return (next record)
 *
 */


DSLL_TYPE
dsll_brk_link(DSLL_TYPE rec)
{
	/* one item in list */
	if (rec->next == rec)
	{
		/** debug statement */
		DP printf("*** brk_link: breaking last, rec = %X\n", rec, 0, 0);

		/* null out list */
		rec->head->next = NULLPTR;

		/* data pointer */
		return (rec->head);
	}
	else			/* more than one item in list */
	{
		/** debug statement */
		DP printf("*** brk_link: breaking rec = %X\n", rec, 0, 0);

		/* next item point to prev item */
		rec->next->prev = rec->prev;

		/* prev item point to next item */
		rec->prev->next = rec->next;

		/* first item -- reset start pointer */
		if (rec->head->next == rec)
			rec->head->next = rec->next;

		/* next record */
		return (rec->next);
	}
}

/*
 *  Functional Description:
 *      This routine is used internally and
 *     checks to see if the item given to it is s valid linked list
 *     item and then returns the record pointer for use of other
 *     routines.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_ck_rec(data,ck_null_list)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   ck_null_list
 *         TYPE:        bool
 *         I/O:         input
 *         DESCRIPTION: flag for checking for a null list
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to record.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:    None.
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       debug statement
 *       if (data is null)
 *         return (failure)
 *       get record pointer
 *       debug statement
 *       if (record head does not point to itself)
 *         return (failure)
 *       else (null_list_check is specified and list is empty)
 *         return (failure)
 *       else (passed all checks)
 *         return (success)
 *
 */


DSLL_TYPE
dsll_ck_rec(ALIGN *data, bool ck_null_list)
{
	DSLL_TYPE rec;

	/** debug statement */
	DP printf("*** dsll_ck_rec: data = %X\n", data, 0, 0);

	/* data is null */
	if (data == NULLCHR)
	{
		/* failure */
		return (NULLPTR);
	}

	/* get record pointer */
	rec = (DSLL_TYPE)
	    ((char *) data - (sizeof(struct dsll_item) - sizeof(ALIGN)));

	/** debug statement */
	DP printf("*** dsll_ck_rec: rec = %X\n", rec, 0, 0);

	/* record head does not point to itself */
	if (rec->head != rec->head->head)
	{
		/* failure */
		return (NULLPTR);
	}
	else			/* null_list_check is specified and list is
				 * empty */
	{
		if (ck_null_list && rec->head->next == NULLPTR)
		{
			/* failure */
			return (NULLPTR);
		}
		else		/* passed all checks */
		{
			/* success */
			return (rec);
		}
	}
}

/*
 *  Functional Description:
 *      This routine prints out the pointer
 *     values used by the linked list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_debug(data, func)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   func
 *         TYPE:        void
 *         I/O:         input
 *         DESCRIPTION: print function
 *
 * RETURN VALUE:   None.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec   - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (debug flag is not set)
 *         return (exit)
 *       print input items
 *       if (good data pointer)
 *         print lables
 *         print head element
 *         if (list is empty)
 *           print warning
 *         else (otherwise)
 *           starting at the first element
 *           do (for each element)
 *             print information
 *             go to next record
 *             check for end of list
 *           not end of list
 *       else (bad data pointer is message printed)
 *         print bad pointer message
 *
 */


/* null print function */
void null_print()
{
	printf("\n");
}

void
dsll_debug(ALIGN *data, void (*func) ())
{
	register DSLL_TYPE rec;
	register int i;

	/* check for NULL function */
	if (func == NULL)
	{
		func = null_print;
	}
	/* print input items */
	printf("input item = %X\n", data);

	/* good data pointer */
	if ((rec = dsll_ck_rec(data, FALSE)) != NULLPTR)
	{
		/* print lables */
		printf("Elemnt record   next   prev   head   size   data\n");

		/* print head element */
		rec = rec->head;
		printf("  head %6X %6X %6X %6X %6u %6X\n",
		    rec,
		    rec->next,
		    rec->prev,
		    rec->head,
		    rec->size,
		    &rec->_align,
		    0);

		/* list is empty */
		if ((rec->head->next == NULLPTR))
		{
			/* print warning */
			printf("LIST IS EMPTY\n");
		}
		else		/* otherwise */
		{
			/* starting at the first element */
			rec = rec->next;
			i = 1;
			do	/* for each element */
			{
				/* print information */
				printf("%6d %6X %6X %6X %6X %6u %6X  ",
				    i++,
				    rec,
				    rec->next,
				    rec->prev,
				    rec->head,
				    rec->size,
				    &rec->_align);
				(*func) (&rec->_align);

				/* go to next record */
				rec = rec->next;

				/* check for end of list */
			} while (rec != rec->head->next);	/* not end of list */
		}
	}
	else			/* bad data pointer is message printed */
	{
		/* print bad pointer message */
		printf("bad LINKED LIST pointer\n");
	}
}

/*
 *  Functional Description:
 *      This routine deletes (dealloc's) an
 *     item from the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_del(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to next data area after one deleted.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_brk_link - This routine is one of a set that maintains a
 *	dsll_ck_rec   - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       if (given head record)
 *         return (failure)
 *       remove link
 *       DSLL_BRK_LINK - This routine is one of a set that maintains a
 *       free record storage
 *       return (pointer to next data area)
 *
 */


ALIGN *
dsll_del(ALIGN *data)
{
	DSLL_TYPE temp;
	DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* given head record */
	if (rec == rec->head)
	{
		/* failure */
		return (NULLCHR);
	}

	/* remove link */
	temp = dsll_brk_link(rec);

	/* free record storage */
	free(rec);

	/* pointer to next data area */
	return (&temp->_align);
}

/*
 *  Functional Description:
 *      This routine deletes (dealloc's)
 *     entire list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_del_list(data)
 *
 *         PARAMETER:   data
 *         TYPE:        register ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Null list pointer.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec    - This routine is one of a set that maintains a
 *	dsll_del       - This routine is one of a set that maintains a
 *	dsll_first     - This routine is one of a set that maintains a
 *	dsll_gt_handle - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (the argument is not a valid list element)
 *         return (failure)
 *       save the handle of the list
 *       DSLL_GT_HANDLE - This routine is one of a set that maintains a
 *       set temp to the first element of the list
 *       DSLL_FIRST - This routine is one of a set that maintains a
 *       while (temp points to a valid element of the list)
 *         remove that element from the list
 *         DSLL_DEL - This routine is one of a set that maintains a
 *       get the list structure associated with the handle
 *       DSLL_CK_REC - This routine is one of a set that maintains a
 *       NULL out the components of the list structure (so further
 *            calls
 *       that might try to reference this list will fail).
 *       release the storage
 *       return (SUCCESS)
 *
 */


bool
dsll_del_list(ALIGN *data)
{
	ALIGN *temp;
	ALIGN *handle;
	register DSLL_TYPE rec;

	/* the argument is not a valid list element */
	if ((dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (FALSE);
	}

	DP printf("*** del_list: initial data = %X\n", temp, 0, 0);

	/* save the handle of the list */
	handle = dsll_gt_handle(data);

	/* set temp to the first element of the list */
	temp = dsll_first(data);

	/* temp points to a valid element of the list */
	while (temp != NULLCHR)
	{
		/* remove that element from the list */
		temp = dsll_del(temp);
	}

	/* get the list structure associated with the handle */
	rec = dsll_ck_rec(handle, FALSE);

	/* NULL out the components of the list structure (so further calls */
	/* that might try to reference this list will fail).  */
	rec->head = NULLPTR;
	rec->next = NULLPTR;
	rec->prev = NULLPTR;

	/* release the storage */
	free(rec);

	/* SUCCESS */
	return (TRUE);
}

/*
 *  Functional Description:
 *      This routine returns the pointer to
 *     the first item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_first(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       move to first record
 *       return (pointer to data area of first record)
 *
 */


ALIGN *
dsll_first(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** first: initial rec = %X\n", rec, 0, 0);

	/* move to first record */
	rec = rec->head->next;

	/* pointer to data area of first record */
	return (&(rec->_align));
}
/*
 *  Functional Description:
 *	Gets a linked list from a file
 *
 *  Description of inputs:
 *	file_name
 *
 *  Description of outputs:
 *	head of linked list
 *
 *
 *  Errors:
 *
 *
 */

ALIGN *
dsll_get(char *file)
{
	register ALIGN *item;
	int fd;
	int size;

	if ((fd = open(file, O_RDONLY, 0664)) == -1)
	{
		return (0);
	}

	item = (ALIGN *) dsll_init();
	while (read(fd, &size, sizeof(size)) == sizeof(size))
	{
		item = (ALIGN *) dsll_add_a(item, size);

		if (read(fd, item, size) != size)
		{
			dsll_del_list(item);
			return (0);
		}
	}

	close(fd);
	return ((ALIGN *) dsll_gt_handle(item));
}

/*
 *  Functional Description:
 *      This routine returns a header record
 *     for a linked list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_gt_handle(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to linked list handle.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       get head
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_gt_handle(ALIGN *data)
{
	register DSLL_TYPE np;

	/* not valid input rec */
	if ((np = dsll_ck_rec(data, FALSE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* get head */
	np = np->head;

	/** debug statement */
	DP printf("*** gt_handle: handle = %X\n", np, 0, 0);

	/* pointer to data area */
	return (&np->_align);
}

/*
 *  Functional Description:
 *      This routine returns TRUE if the
 *     item given it is the first item.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_if_first(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        bool
 *     DESCRIPTION: TRUE - This is the first item in the list.
 *		   FALSE - Not the first item.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       return (TRUE if first record otherwise return FALSE)
 *
 */


bool
dsll_if_first(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULL);
	}

	/** debug statement */
	DP printf("*** if_first: initial rec = %X\n", rec, 0, 0);

	/* TRUE if first record otherwise return FALSE */
	return (rec == rec->head->next);
}

/*
 *  Functional Description:
 *      This routine returns TRUE if the item
 *     given it is the last item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_if_last(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        bool
 *     DESCRIPTION:  TRUE - This is the first item in the list.
 *		   FALSE - Not the first item.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       return (TRUE if record is the last one otherwise return FALSE)
 *
 */


bool
dsll_if_last(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULL);
	}

	/** debug statement */
	DP printf("*** if_last: initial rec = %X\n", rec, 0, 0);

	/* TRUE if record is the last one otherwise return FALSE */
	return (rec == rec->head->next->prev);
}

/*
 *  Functional Description:
 *      This routine creates a header record
 *     for a linked list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_init()
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to linked list handle.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:    None.
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (malloc space for header failed)
 *         return (failure)
 *       initalize header values
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_init()
{
	register DSLL_TYPE np;

	/* malloc space for header failed */
	if ((np = (DSLL_TYPE) malloc(sizeof(struct dsll_item))) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* initalize header values */
	np->next = NULLPTR;
	np->prev = np;
	np->head = np;

	/** debug statement */
	DP printf("*** init: starting new list rec = %X\n", np, 0, 0);

	/* pointer to data area */
	return (&np->_align);
}

/*
 *  Functional Description:
 *      This routine returns the pointer
 *     to the last data item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_last(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       set rec to last record
 *       debug statement
 *       return (pointer to data area of last record)
 *
 */


ALIGN *
dsll_last(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** last: initial rec = %X\n", rec, 0, 0);

	/* set rec to last record */
	rec = rec->head->next->prev;

	/** debug statement */
	DP printf("*** last: last rec = %X\n", rec, 0, 0);

	/* pointer to data area of last record */
	return (&(rec->_align));
}

/*
 *  Functional Description:
 *      This routine makes a link for an item
 *     at the given record.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_mk_link(np,rec)
 *
 *         PARAMETER:   np
 *         TYPE:        register DSLL_TYPE
 *         I/O:         input
 *         DESCRIPTION: new pointer
 *
 *         PARAMETER:   rec
 *         TYPE:        register DSLL_TYPE
 *         I/O:         input
 *         DESCRIPTION: Input record
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:    None.
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       debug statement
 *       set head pointer value
 *       if (list is empty)
 *         debug statment
 *         initialize the head values
 *         set pointers to point back to itself
 *       else (list not empty)
 *         if (old record is the head)
 *           debug statement
 *           change rec to last item
 *           change start record pointer
 *         rearrange next/prev pointers
 *
 */

void
dsll_mk_link(DSLL_TYPE np, DSLL_TYPE rec)
{
	/** debug statement */
	DP printf("*** mk_link: np = %X rec = %u\n", np, rec, 0);

	/* set head pointer value */
	np->head = rec->head;

	/* list is empty */
	if (rec->head->next == NULLPTR)
	{
		/** debug statment */
		DP printf("*** mk_link: make link to null list\n", 0, 0, 0);

		/* initialize the head values */
		np->head->next = np;

		/* set pointers to point back to itself */
		np->next = np;
		np->prev = np;
	}
	else			/* list not empty */
	{
		/* old record is the head */
		if (rec == rec->head)
		{
			/** debug statement */
			DP printf("*** mk_link: mk_linking at head\n", 0, 0, 0);

			/* change rec to last item */
			rec = rec->next->prev;

			/* change start record pointer */
			rec->head->next = np;
		}

		/* rearrange next/prev pointers */
		np->next = rec->next;
		np->prev = rec;
		np->next->prev = np;
		np->prev->next = np;
	}
}

/*
 *  Functional Description:
 *      This routine moves data1 to the
 *     location after data2.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_move(data1,data2)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item 1
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item 2
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area 1 in new position.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_brk_link - This routine is one of a set that maintains a
 *	dsll_ck_rec   - This routine is one of a set that maintains a
 *	dsll_mk_link  - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       debug statement
 *       if (rec 1 is a valid input rec)
 *         return (failure)
 *       if (rec 2 is a valid input rec)
 *         return (failure)
 *       if (record 1 is the head element)
 *         return (failure)
 *       if (record 1 is the head element)
 *         return (failure)
 *       break link for record 1
 *       DSLL_BRK_LINK - This routine is one of a set that maintains a
 *       add record 1 after record 2
 *       DSLL_MK_LINK - This routine is one of a set that maintains a
 *       return (success value)
 *
 */


bool
dsll_move(ALIGN *data1, ALIGN *data2)
{
	register DSLL_TYPE rec1;
	register DSLL_TYPE rec2;
	register DSLL_TYPE temp;

	/** debug statement */
	DP printf("*** move: data1 = %X data2 = %u\n", data1, data2, 0);

	/* rec 1 is a valid input rec */
	if ((rec1 = dsll_ck_rec(data1, TRUE)) == NULLPTR)
	{
		/* failure */
		return (FALSE);
	}

	/* rec 2 is a valid input rec */
	if ((rec2 = dsll_ck_rec(data2, TRUE)) == NULLPTR)
	{
		/* failure */
		return (FALSE);
	}

	/* record 1 is the head element */
	if (rec1 == rec1->head)
	{
		/* failure */
		return (FALSE);
	}

	/* record 1 is the head element */
	if (rec2 == rec2->head)
	{
		/* failure */
		return (FALSE);
	}

	/* break link for record 1 */
	dsll_brk_link(rec1);

	/* add record 1 after record 2 */
	dsll_mk_link(rec1, rec2);

	/* success value */
	return (TRUE);
}

/*
 *  Functional Description:
 *      This routine returns the pointer to
 *     the next data item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_next(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       set to next record
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_next(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** next: initial rec = %X\n", rec, 0, 0);

	/* set to next record */
	rec = rec->next;

	/** debug statement */
	DP printf("*** next: next rec = %X\n", rec, 0, 0);

	/* pointer to data area */
	return (&rec->_align);
}

/*
 *  Functional Description:
 *      This routine returns the pointer to
 *     the next data item in the list unless it is the "first" item
 *     and then it returns NULL.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_next_e(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (null list or not valid input rec)
 *         return (failure)
 *       if (one item in list and not head pointer)
 *         return (end of list)
 *       if (last item in list)
 *         return (end of list)
 *       set to next record
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_next_e(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* null list or not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* one item in list and not head pointer */
	if (rec->next == rec && rec->head != rec)
	{
		/* end of list */
		return (NULLCHR);
	}

	/* last item in list */
	if (rec == rec->head->next->prev)
	{
		/* end of list */
		return (NULLCHR);
	}

	/* set to next record */
	rec = rec->next;

	/* pointer to data area */
	return (&rec->_align);
}

/*
 *  Functional Description:
 *      This routine returns the pointer to
 *     the previous data item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_prev(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statements
 *       if (record is handle)
 *         move to first record
 *       move to previous record
 *       debug statement
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_prev(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statements */
	DP printf("*** prev: initial rec = %X\n", rec, 0, 0);

	/* record is handle */
	if (rec == rec->head)
	{
		/* move to first record */
		rec = rec->next;
	}

	/* move to previous record */
	rec = rec->prev;

	/** debug statement */
	DP printf("*** prev: prev rec = %X\n", rec, 0, 0);

	/* pointer to data area */
	return (&rec->_align);
}

/*
 *  Functional Description:
 *      This routine returns the pointer to
 *     the previous data item in the list.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_prev_e(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (null list or not valid input rec)
 *         return (failure)
 *       if (one item in list and not handle)
 *         return (end of list)
 *       if (last item in list)
 *         return (end of list)
 *       if (record is handle)
 *         move to first record
 *         (later changes will return the last value)
 *       move to previous record
 *       return (pointer to data area)
 *
 */


ALIGN *
dsll_prev_e(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* null list or not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* one item in list and not handle */
	if (rec == rec->next && rec != rec->head)
	{
		/* end of list */
		return (NULLCHR);
	}

	/* first item in list */
	if (rec == rec->head->next)
	{
		/* end of list */
		return (NULLCHR);
	}

	/* record is handle */
	if (rec == rec->head)
	{
		/* move to first record */
		/* (later changes will return the last value) */
		rec = rec->next;
	}

	/* move to previous record */
	rec = rec->prev;

	/* pointer to data area */
	return (&rec->_align);
}
/*
 *  Functional Description:
 *	Saves a linked list to a file
 *
 *  Description of inputs:
 *	linked list and file name
 *
 *  Description of outputs:
 *	status
 *
 *  Errors:
 *
 *
 */

bool
dsll_save(ALIGN *list, char *file)
{
	register ALIGN *item;
	int fd;
	int size;

	if ((fd = open(file, O_WRONLY | O_CREAT, 0664)) == -1)
	{
		return (FALSE);
	}

	item = (ALIGN *) dsll_gt_handle(list);
	while ((item = (ALIGN *) dsll_next_e(item)) != NULL)
	{
		size = dsll_size(item);
		if (write(fd, &size, sizeof(size)) != sizeof(size))
		{
			return (FALSE);
		}

		if (write(fd, item, size) != size)
		{
			return (FALSE);
		}
	}
	close(fd);
	return (TRUE);
}

/*
 *  Functional Description:
 *      This routine resets the first item
 *     in the list to be the one indicated.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_set_first(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *     TYPE:        Pointer
 *     DESCRIPTION: Pointer to data area.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       if (rec is head)
 *         return (failure)
 *       debug statement
 *       set record to first record
 *
 */


ALIGN *
dsll_set_first(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULLCHR);
	}

	/* rec is head */
	if (rec == rec->head)
	{
		/* failure */
		return (NULLCHR);
	}

	/** debug statement */
	DP printf("*** set_first: initial rec = %X\n", rec, 0, 0);

	/* set record to first record */
	rec->head->next = rec;
}

/*
 *  Functional Description:
 *     Returns size of item.
 *     
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_size(data)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 * RETURN VALUE:
 *
 *         TYPE:        NULLCHR
 *         DESCRIPTION: Return
 *
 *         TYPE:        expression
 *         DESCRIPTION: Return
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (not valid input rec)
 *         return (failure)
 *       debug statement
 *       return (set to size record)
 *
 *
 */


int
dsll_size(ALIGN *data)
{
	register DSLL_TYPE rec;

	/* not valid input rec */
	if ((rec = dsll_ck_rec(data, TRUE)) == NULLPTR)
	{
		/* failure */
		return (NULL);
	}

	/** debug statement */
	DP printf("*** size: initial rec = %X\n", rec, 0, 0);

	/* set to size record */
	return (rec->size);
}
/*
 *  Functional Description:
 *      This routine sorts the linked list
 *     useing a user supplied function.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_sort(list, func)
 *
 *         PARAMETER:   list
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: Input item
 *
 *         PARAMETER:   func
 *         TYPE:        void
 *         I/O:         input
 *         DESCRIPTION: Compare function
 *
 * RETURN VALUE:   None.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_ck_rec   - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 */

void
dsll_sort(ALIGN *data, int (*func) ())
{
	register ALIGN *bottom;
	register ALIGN *seek;
	register ALIGN *max;

	/* check for NULL function */
	if (func == NULL)
	{
		return;
	}

	bottom = (ALIGN *) dsll_last(data);
	while (!dsll_if_first(bottom))
	{
		max = bottom;
		seek = bottom;
		while ((seek = (ALIGN *) dsll_prev_e(seek)) != NULL)
		{
			if ((*func) (seek, max) > 0)
			{
				max = seek;
			}
		}
		if (max == bottom)
		{
			bottom = (ALIGN *) dsll_prev(bottom);
		}
		else
		{
			dsll_move(max, bottom);
		}
	}
}
/*
 *  Functional Description:
 *      This routine "walks" a list executing
 *     "func" function on each item of the list. If the handle is
 *     specified "func" is executed for EACH item and if another item
 *     is specified that item and all items following are used. This
 *     sequence is performed as long as the "func" returns a TRUE
 *     value. If it returns a FALSE value the process stops.
 *
 * CALLING SEQUENCE/USAGE:
 *         dsll_walk(data,func,p0,p1,p2,p3)
 *
 *         PARAMETER:   data
 *         TYPE:        ALIGN *
 *         I/O:         input
 *         DESCRIPTION: ptr to data item
 *
 *         PARAMETER:   func
 *         TYPE:        function pointer
 *         I/O:         input
 *         DESCRIPTION: ptr to processing routine
 *
 *         PARAMETER:   p
 *         TYPE:        tbits *
 *         I/O:         input
 *         DESCRIPTION: Processing routine argument 1
 *
 *         PARAMETER:   p
 *         TYPE:        tbits *
 *         I/O:         input
 *         DESCRIPTION: Processing routine argument 2
 *
 *         PARAMETER:   p
 *         TYPE:        tbits *
 *         I/O:         input
 *         DESCRIPTION: Processing routine argument 3
 *
 *         PARAMETER:   p
 *         TYPE:        tbits *
 *         I/O:         input
 *         DESCRIPTION: Processing routine argument 4
 *
 * RETURN VALUE:
 *
 *     TYPE:        bool
 *     DESCRIPTION: The last value returned from "func".
 *		   TRUE - It went through the whole list.
 *		   FALSE - It terminated early.
 *
 * FILES ACCESSED: None.
 *
 * GLOBAL VARIABLES: None.
 *
 * LIMITATIONS:
 *     None
 *
 * OPTIONAL INFORMATION:
 *
 *     SIDE EFFECTS:       None.
 *
 *     CALLED ROUTINES:
 *	dsll_gt_handle - This routine is one of a set that maintains a
 *	dsll_next_e    - This routine is one of a set that maintains a
 *
 *     FORKED PROCESS:     None.
 *
 *     ALGORITHM OVERVIEW:
 *       if (header)
 *         start at first item
 *         DSLL_NEXT_E - This routine is one of a set that maintains a
 *       while (not at end of list and "func" is still true)
 *         execute function
 *         get next data item
 *         DSLL_NEXT_E - This routine is one of a set that maintains a
 *       return (last return item from function)
 *
 */


bool
dsll_walk(ALIGN *data, bool(*func) (), int *p0, int *p1, int *p2, int *p3)
{
	bool end_func = TRUE;

	/* header */
	if (data == dsll_gt_handle(data))
	{
		/* start at first item */
		data = dsll_next_e(data);
	}

	/* not at end of list and "func" is still true */
	while ((data != NULLCHR) && (end_func != FALSE))
	{
		/* execute function */
		end_func = (*func) (data, p0, p1, p2, p3);

		/* get next data item */
		data = dsll_next_e(data);
	}

	/* last return item from function */
	return (end_func);
}
