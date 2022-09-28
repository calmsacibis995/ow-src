/*
 * ISAM Schema for AnswerBook book database record.
 */

declare table ISAMREC (

	id		char (47),	/* document id */
	title		char (127),	/* document title */
	label		char (47),	/* document label (section/chapter) */
	view_method	char (63),	/* view method */
	print_method	char (63),	/* print method */
	flags		long,		/* NODISPLAY, FILE, etc. */
	first_page	int,		/* first page of page range */
	last_page	int,		/* last page of page range */

	/*
	 * Pointers to related documents in this book.
	 */
	parent_rec	long,		/* rec# of parent document */
	next_sibling_rec long,		/* rec# of next sibling document */
	prev_sibling_rec long,		/* rec# of previous sibling document */
	first_child_rec	long,		/* rec# of first child document */
	last_child_rec	long,		/* rec# of last child document */

	/*
	 * For future expansion.
	 */
	spare_long_1	long,		/* spare */
	spare_long_2	long,		/* spare */
	spare_char63_1	char (63),	/* spare */
	spare_char63_2	char (63)	/* spare */
)

declare primary key	ISAMREC_MASTER as (id)
