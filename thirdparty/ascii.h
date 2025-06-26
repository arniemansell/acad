// SixteenSegmentASCII licensed as below:
/*
 *  Project     Segmented LED Display - ASCII Library
 *  @author     David Madison
 *  @link       github.com/dmadison/Segmented-LED-Display-ASCII
 *  @license    MIT - Copyright (c) 2017 David Madison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#pragma once

#include "object_oo.h"
#include <cstdint>

#define ASTERISK 10
#define MIN_CHAR 32
#define MAX_CHAR 127

// ASCII engine for writing 16 segment text to a vector object
// Supports all readable ascii characters plus newline and carriage return
// Non-readable characters are replaced by asterisks
class asciivec
{
private:
	double ch, cw, cs;		// Character height, width and spacing (mm)
	double ls;				// Line spacing (mm)

	coord_t next_c;			// Start location of the next character

	coord_t text_block_st;	// Origin of the text block

	// Points array
	// 678
	// 345
	// 012 9 10
	coord_t rp[9] = {{ 0.0 }};

	// Pairs of indexes into pts defining the segments in order
	//                            a      b      c      d      e      f      g      h      k      m      n      p      r      s      t      u
	const size_t seg[16][2] = { {6,7}, {7,8}, {8,5}, {5,2}, {1,2}, {0,1}, {0,3}, {3,6}, {6,4}, {7,4}, {8,4}, {5,4}, {2,4}, {1,4}, {0,4}, {4,3} };

	const uint16_t SixteenSegmentASCII[96] = {
				0b0000000000000000, /* (space) */
				0b0000000000001100, /* ! */
				0b0000001000000100, /* " */
				0b1010101000111100, /* # */
				0b1010101010111011, /* $ */
				0b1110111010011001, /* % */
				0b1001001101110001, /* & */
				0b0000001000000000, /* ' */
				0b0001010000000000, /* ( */
				0b0100000100000000, /* ) */
				0b1111111100000000, /* * */
				0b1010101000000000, /* + */
				0b0100000000000000, /* , */
				0b1000100000000000, /* - */
				0b0001000000000000, /* . */
				0b0100010000000000, /* / */
				0b0100010011111111, /* 0 */
				0b0000010000001100, /* 1 */
				0b1000100001110111, /* 2 */
				0b0000100000111111, /* 3 */
				0b1000100010001100, /* 4 */
				0b1001000010110011, /* 5 */
				0b1000100011111011, /* 6 */
				0b0000000000001111, /* 7 */
				0b1000100011111111, /* 8 */
				0b1000100010111111, /* 9 */
				0b0010001000000000, /* : */
				0b0100001000000000, /* ; */
				0b1001010000000000, /* < */
				0b1000100000110000, /* = */
				0b0100100100000000, /* > */
				0b0010100000000111, /* ? */
				0b0000101011110111, /* @ */
				0b1000100011001111, /* A */
				0b0010101000111111, /* B */
				0b0000000011110011, /* C */
				0b0010001000111111, /* D */
				0b1000000011110011, /* E */
				0b1000000011000011, /* F */
				0b0000100011111011, /* G */
				0b1000100011001100, /* H */
				0b0010001000110011, /* I */
				0b0000000001111100, /* J */
				0b1001010011000000, /* K */
				0b0000000011110000, /* L */
				0b0000010111001100, /* M */
				0b0001000111001100, /* N */
				0b0000000011111111, /* O */
				0b1000100011000111, /* P */
				0b0001000011111111, /* Q */
				0b1001100011000111, /* R */
				0b1000100010111011, /* S */
				0b0010001000000011, /* T */
				0b0000000011111100, /* U */
				0b0100010011000000, /* V */
				0b0101000011001100, /* W */
				0b0101010100000000, /* X */
				0b1000100010111100, /* Y */
				0b0100010000110011, /* Z */
				0b0010001000010010, /* [ */
				0b0001000100000000, /* \ */
				0b0010001000100001, /* ] */
				0b0101000000000000, /* ^ */
				0b0000000000110000, /* _ */
				0b0000000100000000, /* ` */
				0b1010000001110000, /* a */
				0b1010000011100000, /* b */
				0b1000000001100000, /* c */
				0b0010100000011100, /* d */
				0b1100000001100000, /* e */
				0b1010101000000010, /* f */
				0b1010001010100001, /* g */
				0b1010000011000000, /* h */
				0b0010000000000000, /* i */
				0b0010001001100000, /* j */
				0b0011011000000000, /* k */
				0b0000000011000000, /* l */
				0b1010100001001000, /* m */
				0b1010000001000000, /* n */
				0b1010000001100000, /* o */
				0b1000001011000001, /* p */
				0b1010001010000001, /* q */
				0b1000000001000000, /* r */
				0b1010000010100001, /* s */
				0b1000000011100000, /* t */
				0b0010000001100000, /* u */
				0b0100000001000000, /* v */
				0b0101000001001000, /* w */
				0b0101010100000000, /* x */
				0b0000101000011100, /* y */
				0b1100000000100000, /* z */
				0b1010001000010010, /* { */
				0b0010001000000000, /* | */
				0b0010101000100001, /* } */
				0b1100110000000000, /* ~ */
				0b0000000000000000 }; /* (del) */

	
public:
	asciivec();											               // Default to character height of 6mm and a (0,0) starting position
	explicit asciivec(double height_mm);							// Set character height (mm), starting position defaults to (0,0)
	asciivec(double height_mm, coord_t start_point);	      // Set character height (mm) and starting coordinates (mm)
	
	void add(obj& obj, coord_t st, const char *str);	      // Reset starting position to st, then write str to obj
	void add(obj& obj, const char *str);				         // Write str to obj
														                  // Write str to obj; if it overlaps, move it steps of movement until it doesn't
	void add_no_overlap(obj& obj, coord_t st, const char *str, vector_t movement);
};
