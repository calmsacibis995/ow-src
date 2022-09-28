#pragma ident "@(#)globals.h	1.7 92/06/07	SMI"

/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 */

/*
 * spider global variables
 */
 
Display	       *dpy;
int		screen;
Window		table;

#ifdef 		KITLESS
Window		message_win;
XFontStruct    *message_font;
#endif	/* KITLESS */

Pixmap		greenmap;
Pixmap		redmap;
Pixmap		logomap;

unsigned long	blackpixel;
unsigned long	whitepixel;
unsigned long	borderpixel;
unsigned long	greenpixel;

Bool		is_color;

CardList	deck;
CardList	stack[NUM_STACKS];		/* tableau */
CardList	piles[NUM_PILES];		/* full suits */

int		table_height;
int		table_width;

int		deck_index;

int		draw_count;

Bool		restart;
int		deal_number;

extern char    *version;
extern char    *build_date;

/* function decls */
void		shuffle_cards(void);
void		remove_card(CardPtr card);
void		add_card(CardPtr new, CardPtr old, int location, CardList list);
char	       *rank_name(Rank rank);
char	       *rnk_name( Rank rank);
char	       *suit_name(Suit suit);
#ifdef	DEBUG
char	       *type_name(Type type);
#endif	/* DEBUG */
char	       *get_selection(void);
char	       *remove_newlines(char *str);

Bool		can_move(   CardPtr card);
Bool		can_move_to(CardPtr card, CardList list);

CardList	best_card_move(CardPtr card);
CardPtr		last_card(     CardList list);

int		replay(void);

void		best_list_move(CardList list, CardPtr first_card);
void		move_to_list(CardPtr card, CardList list, Bool log);
void		move_to_pile(CardPtr card);
void		recompute_list_deltas(CardList list);

void		show_message( char *str);
void		card_message( char *str,  CardPtr card);
void		card2_message(char *str1, CardPtr card1,
			      char *str2, CardPtr card2);
void		clear_message(void);
void		print_version(void);


/*
 * Having prototype in place reveals fatal error in events.c
 *
 * From events.c:
 *
 *	#ifdef XVIEW
 *		show_play();
 *	#else
 *		show_play(0, 0, show_play_events, delay);
 *	#endif
 *
 *void		show_play(int start, int num_moves,
 *			  Bool (*event_check)(), void (*delay_func)());
 */
void		show_play();
void		locate(char *str);

void		advise_best_move(void);
void		delay(void);
void		force_redraw(void);

#ifndef KITLESS
void		key_press(     XKeyPressedEvent		*xev);
void		redraw_table(  XExposeEvent		*xev);
void		button_press(  XButtonPressedEvent	*xev);
void		button_release(XButtonReleasedEvent	*xev);
void		do_expand(void);
#endif KITLESS

#ifdef XAW
Bool		can_get_help_files(char helpfiles[6][256]);
#endif
