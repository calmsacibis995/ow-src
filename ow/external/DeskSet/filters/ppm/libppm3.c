/* libppm3.c - ppm utility library part 3
**
** Colormap routines.
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "ppm.h"
#include "ppmcmap.h"
#include "libppm.h"

#define HASH_SIZE 20023

#ifdef PPM_PACKCOLORS
#define ppm_hashpixel(p) ( ( (int) (p) & 0x7fffffff ) % HASH_SIZE )
#else /*PPM_PACKCOLORS*/
#define ppm_hashpixel(p) ( ( ( (long) PPM_GETR(p) * 33023 + (long) PPM_GETG(p) * 30013 + (long) PPM_GETB(p) * 27011 ) & 0x7fffffff ) % HASH_SIZE )
#endif /*PPM_PACKCOLORS*/

colorhist_vector
ppm_computecolorhist( pixels, cols, rows, maxcolors, colorsP )
    pixel** pixels;
    int cols, rows, maxcolors;
    int* colorsP;
    {
    colorhash_table cht;
    colorhist_vector chv;

    cht = ppm_computecolorhash( pixels, cols, rows, maxcolors, colorsP );
    if ( cht == (colorhash_table) 0 )
	return (colorhist_vector) 0;
    chv = ppm_colorhashtocolorhist( cht, maxcolors );
    ppm_freecolorhash( cht );
    return chv;
    }

void
ppm_addtocolorhist( chv, colorsP, maxcolors, colorP, value, position )
    colorhist_vector chv;
    pixel* colorP;
    int* colorsP;
    int maxcolors, value, position;
    {
    int i, j;

    /* Search colorhist for the color. */
    for ( i = 0; i < *colorsP; ++i )
	if ( PPM_EQUAL( chv[i].color, *colorP ) )
	    {
	    /* Found it - move to new slot. */
	    if ( position > i )
		{
		for ( j = i; j < position; ++j )
		    chv[j] = chv[j + 1];
		}
	    else if ( position < i )
		{
		for ( j = i; j > position; --j )
		    chv[j] = chv[j - 1];
		}
	    chv[position].color = *colorP;
	    chv[position].value = value;
	    return;
	    }
    if ( *colorsP < maxcolors )
	{
	/* Didn't find it, but there's room to add it; so do so. */
	for ( i = *colorsP; i > position; --i )
	    chv[i] = chv[i - 1];
	chv[position].color = *colorP;
	chv[position].value = value;
	++(*colorsP);
	}
    }

colorhash_table
ppm_computecolorhash( pixels, cols, rows, maxcolors, colorsP )
    pixel** pixels;
    int cols, rows, maxcolors;
    int* colorsP;
    {
    colorhash_table cht;
    register pixel* pP;
    colorhist_list chl;
    int col, row, hash;

    cht = ppm_alloccolorhash( );
    *colorsP = 0;

    /* Go through the entire image, building a hash table of colors. */
    for ( row = 0; row < rows; ++row )
	for ( col = 0, pP = pixels[row]; col < cols; ++col, ++pP )
	    {
	    hash = ppm_hashpixel( *pP );
	    for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
		if ( PPM_EQUAL( chl->ch.color, *pP ) )
		    break;
	    if ( chl != (colorhist_list) 0 )
		++(chl->ch.value);
	    else
		{
		if ( ++(*colorsP) > maxcolors )
		    {
		    ppm_freecolorhash( cht );
		    return (colorhash_table) 0;
		    }
		chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
		if ( chl == 0 )
		    pm_error( "out of memory computing hash table" );
		chl->ch.color = *pP;
		chl->ch.value = 1;
		chl->next = cht[hash];
		cht[hash] = chl;
		}
	    }
    
    return cht;
    }

colorhash_table
ppm_alloccolorhash( )
    {
    colorhash_table cht;
    int i;

    cht = (colorhash_table) malloc( HASH_SIZE * sizeof(colorhist_list) );
    if ( cht == 0 )
	pm_error( "out of memory allocating hash table" );

    for ( i = 0; i < HASH_SIZE; ++i )
	cht[i] = (colorhist_list) 0;

    return cht;
    }

int
ppm_addtocolorhash( cht, colorP, value )
    colorhash_table cht;
    pixel* colorP;
    int value;
    {
    register int hash;
    register colorhist_list chl;

    chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
    if ( chl == 0 )
	return -1;
    hash = ppm_hashpixel( *colorP );
    chl->ch.color = *colorP;
    chl->ch.value = value;
    chl->next = cht[hash];
    cht[hash] = chl;
    return 0;
    }

colorhist_vector
ppm_colorhashtocolorhist( cht, maxcolors )
    colorhash_table cht;
    int maxcolors;
    {
    colorhist_vector chv;
    colorhist_list chl;
    int i, j;

    /* Now collate the hash table into a simple colorhist array. */
    chv = (colorhist_vector) malloc( maxcolors * sizeof(struct colorhist_item) );
    /* (Leave room for expansion by caller.) */
    if ( chv == (colorhist_vector) 0 )
	pm_error( "out of memory generating histogram" );

    /* Loop through the hash table. */
    j = 0;
    for ( i = 0; i < HASH_SIZE; ++i )
	for ( chl = cht[i]; chl != (colorhist_list) 0; chl = chl->next )
	    {
	    /* Add the new entry. */
	    chv[j] = chl->ch;
	    ++j;
	    }

    /* All done. */
    return chv;
    }

colorhash_table
ppm_colorhisttocolorhash( chv, colors )
    colorhist_vector chv;
    int colors;
    {
    colorhash_table cht;
    int i, hash;
    pixel color;
    colorhist_list chl;

    cht = ppm_alloccolorhash( );

    for ( i = 0; i < colors; ++i )
	{
	color = chv[i].color;
	hash = ppm_hashpixel( color );
	for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
	    if ( PPM_EQUAL( chl->ch.color, color ) )
		pm_error(
		    "same color found twice - %d %d %d", PPM_GETR(color),
		    PPM_GETG(color), PPM_GETB(color) );
	chl = (colorhist_list) malloc( sizeof(struct colorhist_list_item) );
	if ( chl == (colorhist_list) 0 )
	    pm_error( "out of memory" );
	chl->ch.color = color;
	chl->ch.value = i;
	chl->next = cht[hash];
	cht[hash] = chl;
	}

    return cht;
    }

int
ppm_lookupcolor( cht, colorP )
    colorhash_table cht;
    pixel* colorP;
    {
    int hash;
    colorhist_list chl;

    hash = ppm_hashpixel( *colorP );
    for ( chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next )
	if ( PPM_EQUAL( chl->ch.color, *colorP ) )
	    return chl->ch.value;

    return -1;
    }

void
ppm_freecolorhist( chv )
    colorhist_vector chv;
    {
    free( (char*) chv );
    }

void
ppm_freecolorhash( cht )
    colorhash_table cht;
    {
    int i;
    colorhist_list chl, chlnext;

    for ( i = 0; i < HASH_SIZE; ++i )
	for ( chl = cht[i]; chl != (colorhist_list) 0; chl = chlnext )
	    {
	    chlnext = chl->next;
	    free( (char*) chl );
	    }
    free( (char*) cht );
    }
