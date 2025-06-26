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

#include <assert.h>
#include <float.h>
#include <iterator>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "hpgl.h"
#include "neutralpoint.h"
#include "object_oo.h"

// Find the neutral point of an outline shape, orientated horizontally
void neutralPoint(FILE** rfp) {
   obj dwg;
   obj debug;
   coord_t l_top, l_bot;
   bool isFirst = true;
   double leftx, rightx;
   slice_t* slices = (slice_t*)calloc(sizeof(slice_t), N_SLICE);
   assert(slices != NULL);
   double totalArea = 0;
   double neutralx = 0.0;

   assert(rfp != NULL);
   dwg = importHpglFile(*rfp);
   dwg.move_origin_to(coord_t{ 0.0, 0.0 });
   fclose(*rfp);

   // Find the left and right extremes
   leftx = dwg.find_extremity(LEFT);
   rightx = dwg.find_extremity(RIGHT);
   double xstep = ((rightx - leftx) / N_SLICE);
   PR_INFO("Left : Right extremes at %.1lf : %.1lf, iterating in %d slices\n", leftx, rightx, N_SLICE);

   // Calculate the slices
   {
      // Work through the slices in sequence
      for (size_t cnt = 0; cnt < N_SLICE; cnt++) {
         slices[cnt].x = leftx + (xstep * (double)cnt);

         // Find the top and bottom of the outline at this x value and work out the area
         coord_t upper, lower;
         if (dwg.top_bot_intersect(slices[cnt].x, &upper, &lower)) {
            slices[cnt].ymax = upper.y;
            slices[cnt].ymin = lower.y;
            slices[cnt].area = (slices[cnt].ymax - slices[cnt].ymin) * xstep;
         }
         else {
            slices[cnt].ymax = 0.0;
            slices[cnt].ymin = 0.0;
            slices[cnt].area = 0.0;
         }

         totalArea += slices[cnt].area;
         PR_CHATTY("%5zu: x % 8.1lf  ymax % 8.1lf  ymin % 8.1lf  area % 8.1lf\n",
            cnt + 1,
            slices[cnt].x, slices[cnt].ymax, slices[cnt].ymin, slices[cnt].area);

         // Add the information for this slice to the debug outline drawing
         if (isFirst) {
            l_top =
            { slices[cnt].x, slices[cnt].ymax };
            l_bot =
            { slices[cnt].x, slices[cnt].ymin };
            isFirst = false;
         }
         {
            coord_t tp = { slices[cnt].x, slices[cnt].ymax };
            coord_t bt = { slices[cnt].x, slices[cnt].ymin };
            debug.add(l_top, tp);
            debug.add(l_bot, bt);
            l_top = tp;
            l_bot = bt;
         }
      }
   }

   // Write the debug outline
   FILE* fpdbg;
   const char* fname = "debug_outline.plt";
   fpdbg = fopen(fname, "w");
   assert(fpdbg != NULL);
   if (!debug.empty()) {
      PR_INFO("Writing debug outline drawing to %s...\n", fname);
      exportObjHpglFile(fpdbg, debug);
   }
   else {
      PR_ERROR("ERROR: Outline vector redraw is empty\n");
   }
   fclose(fpdbg);

   // Find the Neutral point as the first slice with a negatve or zero volume
   {
      for (size_t cnt = 0; cnt < N_SLICE; cnt++) {
         double volume = 0;
         for (size_t cnt2 = 0; cnt2 < N_SLICE; cnt2++) {
            volume += slices[cnt2].area * (slices[cnt2].x - slices[cnt].x);
         }
         PR_ANY("%5zu: Volume %.1lf\n", cnt, volume);
         if (volume <= 0.0) {
            neutralx = slices[cnt].x;
            break;
         }
      }
   }

   // Display the result
   PR_INFO(
      "\nTotal Area: %.2lfdm2  \nNeutral point from left edge: %.1lf  \nNeutral point percentage from left edge %.1lf%%",
      totalArea / (100.0 * 100.0),
      (neutralx - leftx),
      100.0 * (neutralx - leftx) / (rightx - leftx));

   free(slices);
}
