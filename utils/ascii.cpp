/*
Copyright(C) 2019-2025 Adrian Mansell

This program is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <cmath>

#include "ascii.h"
#include "debug.h"
#include "object_oo.h"
#include "string.h"

// See https://github.com/dmadison/LED-Segment-ASCII

asciivec::asciivec() {
   next_c =
   { 0.0, 0.0 };
   text_block_st =
   { 0.0, 0.0 };

   ch = 6.0;       // Character height
   cw = 0.5 * ch;  // Character width
   cs = 0.75 * ch; // Character spacing
   ls = 1.5 * ch;  // Line spacing
   for (size_t a = 0; a <= 2; a++) {
      for (size_t b = 0; b <= 2; b++) {
         size_t ind = b + (a * 3);
         rp[ind].x = (double)b * (ch / 4.0);
         rp[ind].y = (double)a * (ch / 2.0);
      }
   }
}

asciivec::asciivec(double height_mm) {
   next_c =
   { 0.0, 0.0 };
   text_block_st =
   { 0.0, 0.0 };

   ch = height_mm; // Character height
   cw = 0.5 * ch;  // Character width
   cs = 0.75 * ch; // Character spacing
   ls = 1.5 * ch;  // Line spacing

   for (size_t a = 0; a <= 2; a++) {
      for (size_t b = 0; b <= 2; b++) {
         size_t ind = b + (a * 3);
         rp[ind].x = (double)b * (ch / 4.0);
         rp[ind].y = (double)a * (ch / 2.0);
      }
   }
}

asciivec::asciivec(double height_mm, coord_t start_point) {
   next_c = start_point;
   text_block_st = start_point;

   ch = height_mm; // Character height
   cw = 0.5 * ch;  // Character width
   cs = 0.75 * ch; // Character spacing
   ls = 1.5 * ch;  // Line spacing
   for (size_t a = 0; a <= 2; a++) {
      for (size_t b = 0; b <= 2; b++) {
         size_t ind = b + (a * 3);
         rp[ind].x = (double)b * (ch / 4.0);
         rp[ind].y = (double)a * (ch / 2.0);
      }
   }
}

void asciivec::add(obj& obj, coord_t st, const char* str) {
   next_c = st;
   text_block_st = st;
   add(obj, str);
}

void asciivec::add(obj& obj, const char* str) {
   for (size_t i = 0; i < strlen(str); i++) { // Work through character by character
      if (str[i] == '\n') {
         next_c.x = text_block_st.x;
         next_c.y = next_c.y - ls;
      }
      else if (str[i] == '\r') {
         next_c.x = text_block_st.x;
      }
      else {
         // If character is out of range then replace with an asterisk
         size_t index =
            (((unsigned)str[i] < MIN_CHAR) || ((unsigned)str[i] > MAX_CHAR)) ? ASTERISK : ((size_t)str[i] - MIN_CHAR);
         uint16_t mask = SixteenSegmentASCII[index];
         for (size_t bt = 0; bt < 16; bt++) {
            // Draw each segment in turn if relevant mask bit is set
            if ((mask >> bt) & 1) {
               size_t pt0 = seg[bt][0];
               size_t pt1 = seg[bt][1];
               obj.add(rp[pt0], rp[pt1]);
               obj.last()->add_offset(next_c.x, next_c.y);
               //PR_ANY("%.1lf %.1lf  %.1lf %.1lf\n", obj.last()->get_S0().x, obj.last()->get_S0().y, obj.last()->get_S1().x, obj.last()->get_S1().y);
            }
         }
         next_c.x = next_c.x + cs;
      }
   }
}

void asciivec::add_no_overlap(obj& objd, coord_t st, const char* str, vector_t movement) {
   obj text;

   add(text, st, str);

   while (objd.obj_intersect(text)) {
      text.add_offset(movement.dx, movement.dy);
   }

   objd.splice(text);
}
