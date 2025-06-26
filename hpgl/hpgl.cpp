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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "hpgl.h"
#include "object_oo.h"

typedef struct
{
   int penIsDown;
   int isAbsolute;
   struct
   {
      double x;
      double y;
   } pos;
} plotter_t;

typedef int (*cmdFnc)(FILE* fp, obj& tgt, plotter_t* plotter);

static int isTerminator(int c);
static void movePastNextTerminator(FILE* fp);
static cmdFnc getNextCmd(FILE* fp, cmdFnc* start, cmdFnc* param, cmdFnc* end);

// NULL command handlers - used for unrecognised commands
// Simply moves on to the next command by searching for a terminator
static int NULL_task(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)fp;
   (void)tgt;
   (void)plotter;
   return (0);
}

static int NULL_end(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)tgt;
   (void)plotter;
   movePastNextTerminator(fp);
   return (0);
}

// PLOT ABSOLUTE command handlers
static int PA_start(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)tgt;
   (void)fp;
   PR_ANY("Absolute Mode:\n");
   plotter->isAbsolute = 1;
   return (0);
}

// PLOT RELATIVE command handlers
static int PR_start(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)tgt;
   (void)fp;
   PR_ANY("Relative Mode:\n");
   plotter->isAbsolute = 0;
   return (0);
}

// PEN UP command handlers
static int PU_start(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)tgt;
   (void)fp;
   plotter->penIsDown = 0;
   return (0);
}

// PEN DOWN command handlers
static int PD_start(FILE* fp, obj& tgt, plotter_t* plotter) {
   (void)tgt;
   (void)fp;
   plotter->penIsDown = 1;
   return (0);
}

// MOVE PARAM commands
// This is a flexible command to read in x,y pairs of coordinates and move the
// plot point accordingly. If the pen is down, a line element is created.
static int MOVE_param(FILE* fp, obj& tgt, plotter_t* plotter) {
   double xpar, ypar, xend, yend;

   // Loop through all existing parameter pairs
   while (fscanf(fp, "%lf,%lf", &xpar, &ypar) == 2) {
      // Correct for HPGL units
      xpar = round(xpar) * HPGL_UNIT;
      ypar = round(ypar) * HPGL_UNIT;

      // Find the location to move to
      if (plotter->isAbsolute) {
         xend = xpar;
         yend = ypar;
         PR_ANY("M[%.0lf,%.0lf]:", xend, yend);
      }
      else {
         xend = xpar + plotter->pos.x;
         yend = ypar + plotter->pos.y;
         PR_ANY("M[%.0lf->%.0lf,%.0lf->%.0lf]:", xpar, xend, ypar, yend);
      }

      // If the pen is down, draw a line segment
      if (plotter->penIsDown) {
         tgt.add(coord_t{ plotter->pos.x, plotter->pos.y }, coord_t{ xend, yend });
         PR_ANY("LA:");
      }

      // Move the current point
      plotter->pos.x = xend;
      plotter->pos.y = yend;

      // Stop if the next character is terminator
      if (isTerminator(fgetc(fp))) {
         if (fseek((fp), -1L, SEEK_CUR) != 0)
            assert(0);
         break;
      }
   }
   return (0);
}

// Check if a character is a terminator
static int isTerminator(int c) {
   return ((c == ';') || (c == '\n') || (c == EOF) ? 1 : 0);
}

// Discard everything including the next terminator
static void movePastNextTerminator(FILE* fp) {
   int c;
   do {
      c = fgetc(fp);
   } while (!isTerminator(c));
}

// Read in the next command, search for it and set the handler function pointers
static cmdFnc getNextCmd(FILE* fp, cmdFnc* start, cmdFnc* param, cmdFnc* end) {
   char cmd[3];
   int ind;
#define NUM_CMDS 5
   struct
   {
      const char* cmd;
      cmdFnc start;
      cmdFnc param;
      cmdFnc end;
   } cmdMap[NUM_CMDS] = { {"PA", PA_start, MOVE_param, NULL_end},
                         {"PR", PR_start, MOVE_param, NULL_end},
                         {"SP", NULL_task, NULL_task, NULL_end},
                         {"PU", PU_start, MOVE_param, NULL_end},
                         {"PD", PD_start, MOVE_param, NULL_end} };

   // Get the two letter command string, return NULL if something goes wrong e.g., EOF
   if (fgets(cmd, 3, fp) == NULL) {
      return (NULL);
   }

   // Check the string against known commands
   *start = NULL;
   *param = NULL;
   *end = NULL;
   for (ind = 0; ind < NUM_CMDS; ind++) {
      if (!strcmp(cmdMap[ind].cmd, cmd)) {
         *start = cmdMap[ind].start;
         *param = cmdMap[ind].param;
         *end = cmdMap[ind].end;
         PR_ANY("%s:", cmd);
         return (*start);
      }
   }

   PR_WARNING("Failed to find command %s - skipping\n", cmd);
   *start = NULL_task;
   *param = NULL_task;
   *end = NULL_end;
   return (*start);
}

obj importHpglFile(FILE* fp) {
   plotter_t plotter;
   cmdFnc start, param, end;
   obj elements;

   // Initialise the plotter state
   plotter.penIsDown = 0;
   plotter.isAbsolute = 0;
   plotter.pos.x = 0;
   plotter.pos.y = 0;

   // Get next command loop
   while (getNextCmd(fp, &start, &param, &end) != NULL) {
      (start)(fp, elements, &plotter);
      (param)(fp, elements, &plotter);
      (end)(fp, elements, &plotter);
   }
   PR_ANY("\n");
   return (elements);
}

void exportObjHpglFile(FILE* fp, obj tgt) {
   plotter_t plotter;

   if (tgt.empty())
      return;

   // Initialise the plotter state
   plotter.penIsDown = 0;
   plotter.isAbsolute = 0;
   plotter.pos.x = 0;
   plotter.pos.y = 0;

   // Absolute coordinates, pen 1, start at origin
   fprintf(fp, "PA;SP1;PU0,0;");

   // Work through each line item in turn
   for (auto ln = tgt.begin(); ln != tgt.end(); ln++) {
      double xstr = round(ln->get_S0().x / HPGL_UNIT);
      double ystr = round(ln->get_S0().y / HPGL_UNIT);
      double xend = round(ln->get_S1().x / HPGL_UNIT);
      double yend = round(ln->get_S1().y / HPGL_UNIT);
      // Move pen to start of line if necessary
      if ((plotter.pos.x != xstr) || (plotter.pos.y != ystr)) {
         fprintf(fp, "PU%d,%d;", (int)xstr, (int)ystr);
         PR_ANY("PU%d,%d;", (int)xstr, (int)ystr);
      }
      // Add the line
      fprintf(fp, "PD%d,%d;", (int)xend, (int)yend);
      PR_ANY("PD%d,%d;", (int)xend, (int)yend);
      // Update the plotter
      plotter.pos.x = xend;
      plotter.pos.y = yend;
   }
}
