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

#include "app.h"
#include "debug.h"
#include "version.h"

#include <QApplication>

int main(int argc, char* argv[]) {
   Q_INIT_RESOURCE(application);

   QApplication a(argc, argv);

   QCoreApplication::setOrganizationName(AUTHOR);

   QString appname = { APP_NAME };
   appname.append(" ");
   appname.append(VERSION);
   QCoreApplication::setApplicationName(appname);
   QCoreApplication::setApplicationVersion(VERSION);

   // Debug
   // int current_debug_level = dbg::NO_DEBUG;
   int current_debug_level = dbg::LVL2;
   dbg::init(current_debug_level);

   App app;
   app.setWindowIcon(QIcon(":images/mum.ico"));
   app.showMaximized();
   return a.exec();
}
