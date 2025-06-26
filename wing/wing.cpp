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
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <stdio.h>
#include <string.h>

#include <QDebug>

#include "airfoil.h"
#include "ascii.h"
#include "debug.h"
#include "dxf.h"
#include "hpgl.h"
#include "wing.h"

void Wing::setDraftMode() {
   aifs.draft_mode();
   ribs.draft_mode();
}

obj& Wing::getPlan() {
   plan.del();
   plan.splice(plnf.getPlan());
   plan.splice(ribs.getPlan());
   plan.splice(sprs.getPlan());
   plan.splice(elms.getPlan());
   return plan;
}

obj& Wing::getParts() {
   coord_t loc = { 0.0, 0.0 };
   parts.del();

   // Retrieve the part objects and associated text
   std::list<std::reference_wrapper<obj>> objects = {};
   std::list<std::reference_wrapper<obj>> texts = {};
   ribs.getPrettyParts(objects, texts);
   sprs.getPrettyParts(objects, texts);
   lets.getPrettyParts(objects, texts);

   // Layout parts and texts in space
   for (auto obs = objects.begin(), txs = texts.begin();
      (obs != objects.end()) && (txs != texts.end());
      ++obs, ++txs) {
      obj ob = *(obs);
      obj tx = *(txs);
      ob.move_origin_to(loc);
      tx.move_origin_to(coord_t(loc.x - tx.find_extremity(RIGHT) - 30.0, loc.y));
      double obextr = ob.find_extremity(UP);
      double txextr = tx.find_extremity(UP);
      loc.y = 30.0 + ((obextr > txextr) ? obextr : txextr);
      parts.splice(ob);
      parts.splice(tx);
   }

   return parts;
}

void Wing::exportToHpgl(QFileInfo& fi) {
   FILE* fd;
   if (!exportFileOpen(fi, &fd))
      return;

   obj exp;
   exp.copy_from(getPlan());
   exp.move_origin_to(coord_t{ getParts().find_extremity(RIGHT) + 100, 0 });
   exp.copy_from(getParts());

   exportObjHpglFile(fd, exp);
   fclose(fd);
}

void Wing::exportToDxf(QFileInfo& fi) {
   FILE* fd;
   if (!exportFileOpen(fi, &fd))
      return;

   // Retrieve the part objects and associated text
   std::list<std::reference_wrapper<obj>> objects = {};
   std::list<std::reference_wrapper<obj>> texts = {};
   ribs.getPrettyParts(objects, texts);
   sprs.getPrettyParts(objects, texts);
   lets.getPrettyParts(objects, texts);

   // Create correctly position blocks in the DXF for each part
   dxf_export dxf = {};
   coord_t loc = { 0, 0 };
   for (auto obs = objects.begin(), txs = texts.begin();
      (obs != objects.end()) && (txs != texts.end());
      ++obs, ++txs) {
      obj ob = *(obs);
      obj tx = *(txs);
      ob.move_origin_to(loc);
      tx.move_origin_to(coord_t(loc.x - tx.find_extremity(RIGHT) - 30.0, loc.y));
      dxf.add_object(ob);
      dxf.add_object(tx);
      double obextr = ob.find_extremity(UP);
      double txextr = tx.find_extremity(UP);
      loc.y = 30.0 + ((obextr > txextr) ? obextr : txextr);
   }

   // Add the plan to the right
   obj pl = {};
   pl.copy_from(getPlan());
   pl.move_origin_to(coord_t(dxf.get_limmax().x + 50.0, 0));
   dxf.add_object(pl);

   // Finish
   dxf.write(fd);
   fclose(fd);
}

bool Wing::exportFileOpen(QFileInfo& fi, FILE** fd) {
   *fd = fopen(fi.absoluteFilePath().toStdString().c_str(), "w");
   if (!*fd) {
      dbg::alert(SS("Unable to open file for writing:"), fi.absoluteFilePath().toStdString());
      return false;
   }
   DBGLVL1("File opened (export): %s", fi.absoluteFilePath().toStdString().c_str());
   return true;
}

// Parse a text file defining a wing
void loadWingFiles(FILE** wfp) {
   (void)wfp;
}
