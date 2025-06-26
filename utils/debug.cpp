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
#include "debug.h"
#include <assert.h>
#include <stdio.h>

/*
 * New Debug code for GUI
 */
int dbg::dbglvl = dbg::NO_DEBUG;
bool dbg::dbgOpen = false;
std::ofstream dbg::fdbg = {};

void dbg::init(int lvl) {
   dbglvl = lvl;

   if (dbglvl > NO_DEBUG) {
      if (!dbgOpen) {
         std::string fname = (QDir::homePath().append("/Documents/acad/acad.log")).toStdString();
         dbg::fdbg.open(fname, std::ios::out | std::ios::trunc);
         if (!dbg::fdbg.is_open()) {
            throw std::runtime_error(std::string("Unable to open debug log file ") + fname);
         }
         dbgOpen = true;
      }
   }
}

void dbg::lvl1(std::string const& str) {
   if (dbglvl >= LVL1)
      if (dbgOpen)
         fdbg << str << std::endl;
}

void dbg::lvl1(char* str) {
   if (dbglvl >= LVL1)
      if (dbgOpen)
         fdbg << std::string(str) << std::endl;
}

void dbg::lvl2(char* str) {
   if (dbglvl >= LVL2)
      if (dbgOpen)
         fdbg << std::string(str) << std::endl;
}

void dbg::lvl2(std::string const& str) {
   if (dbglvl >= LVL2)
      if (dbgOpen)
         fdbg << str << std::endl;
}

void dbg::alert(std::string const& str, std::string const& details) {
   QMessageBox mb;
   mb.setText(QString::fromStdString(str));
   if (!details.empty())
      mb.setDetailedText(QString::fromStdString(details));
   mb.setIcon(QMessageBox::Warning);
   mb.exec();
}

void dbg::fatal(std::string const& str, std::string const& details) {
   QMessageBox mb;
   mb.setText(QString::fromStdString(str));
   mb.setInformativeText(QString("ACAD will exit..."));
   if (!details.empty())
      mb.setDetailedText(QString::fromStdString(details));
   mb.setIcon(QMessageBox::Critical);
   mb.exec();
   if (dbgOpen)
      fdbg.close();
   exit(0);
}
