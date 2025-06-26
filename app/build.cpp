/*
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

#include <iostream>
#include <stdio.h>

#include "airfoil.h"
#include "app.h"
#include "object_oo.h"
#include "wing.h"

void App::buildHpgl() {
   // Get a filename using standard dialogue
   QString filename;
   filename = QFileDialog::getSaveFileName(this, tr("Export As"), currPath, tr("HPGL Files (*.plt)"));
   if (filename.isEmpty())
      return;

   QFileInfo fi(filename);
   // Enforce it has a .plt extension
   if (fi.suffix().isEmpty() || (fi.suffix() != "plt")) {
      filename.append(".plt");
      fi.setFile(filename);
   }

   QStatusBar* sb = statusBar();
   sb->clearMessage();
   sb->showMessage(QString("Building wing model"));
   Wing w;
   buildWingModel(w);
   sb->clearMessage();
   sb->showMessage(QString("Exporting to HPGL file"));
   w.exportToHpgl(fi);
   sb->clearMessage();
   sb->showMessage(QString("Export complete"), 10000);
}

void App::buildDxf() {
   // Get a filename using standard dialogue
   QString filename;
   filename = QFileDialog::getSaveFileName(this, tr("Export As"), currPath, tr("DXF Files (*.dxf)"));
   if (filename.isEmpty())
      return;

   QFileInfo fi(filename);
   // Enforce it has a .dxf extension
   if (fi.suffix().isEmpty() || (fi.suffix() != "dxf")) {
      filename.append(".dxf");
      fi.setFile(filename);
   }

   QStatusBar* sb = statusBar();
   sb->clearMessage();
   sb->showMessage(QString("Building wing model"));
   Wing w;
   buildWingModel(w);
   sb->clearMessage();
   sb->showMessage(QString("Exporting to DXF file"));
   w.exportToDxf(fi);
   sb->clearMessage();
   sb->showMessage(QString("Export complete"), 10000);
}

obj App::buildPlan() {
   obj plan;
   return plan;
}

obj App::buildPart(bool isDraft) {
   (void)isDraft;
   return obj();
}

void App::buildWingModel(Wing& w, bool inDraftMode) {
   // Set a busy cursor
   QApplication::setOverrideCursor(Qt::BusyCursor);

   if (inDraftMode) {
      w.aifs.draft_mode();
      w.ribs.draft_mode();
      w.elms.draft_mode();
      DBGLVL1("Building wing model in draft mode");
   }
   else
      DBGLVL1("Building wing model in full resolution mode");

   std::string log;
   do {
      if (!w.plnf.add(tabMap.at("PLANFORM"), log))
         break;
      if (!w.aifs.add(tabMap.at("AIRFOILS"), log))
         break;
      if (!w.ribs.add(tabMap.at("RIBS"), w.plnf, log))
         break;
      if (!w.ribs.addRibParams(tabMap.at("RIBPARAMS"), log))
         break;
      if (!w.ribs.create(w.plnf, w.aifs, log))
         break;
      if (!w.ribs.addGeodetics(tabMap.at("GEODETICS"), w.plnf, log))
         break;
      if (!w.ribs.addRibParams(tabMap.at("RIBPARAMS"), log))
         break;
      if (!w.ribs.create(w.plnf, w.aifs, log))
         break;
      if (!w.sprs.add(tabMap.at("SHEETSPARS"), log))
         break;
      if (!w.sprs.add(tabMap.at("STRIPSPARS"), log))
         break;
      if (!w.sprs.create(w.ribs, log))
         break;
      if (!w.ribs.addCreateJigs(tabMap.at("SJC1"), tabMap.at("SJC2"), log))
         break;
      if (!w.ribs.addCreateJigsType2(tabMap.at("SJCT2"), log))
         break;
      if (!w.sprs.addCreateJigsType2(tabMap.at("SJCT2"), w.ribs, log))
         break;
      if (!w.elms.add(tabMap.at("ELEMENTS"), log))
         break;
      if (!w.elms.create(w.ribs, log))
         break;
      if (!w.ribs.addHoles(log))
         break;
      if (!w.lets.add(tabMap.at("LETEMPLATES"), w.plnf, log))
         break;
      if (!w.lets.create(w.plnf, w.aifs, log))
         break;
   } while (false);

   if (log.length() != 0) {
      dbg::alert(SS("There are issues with your model; it has not been completely built"), log);
      QApplication::restoreOverrideCursor();
      return;
   }

   QApplication::restoreOverrideCursor();
}
