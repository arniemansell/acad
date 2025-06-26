#pragma once
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

#include <assert.h>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdio.h>

#include <QDir>
#include <QMessageBox>
#include <QString>

typedef enum {
   DBG_ANY, // Most detailed debug information
   DBG_CHATTY,
   DBG_INFO, // Default for normal debug information
   DBG_WARNING,
   DBG_ERROR, // Will always be printed and will cause an assert()
   DBG_NONE   // Will always be printed
} dbgPrintLevel_e;

/*
 * New Debug code for GUI
 */
class dbg {
public:
   static constexpr int NO_DEBUG = 0;
   static constexpr int LVL1 = 1;
   static constexpr int LVL2 = 2;
   static constexpr int STRLENGTH = 2048;

   static std::ofstream fdbg;
   static bool dbgOpen;
   static int dbglvl;

   static void init(int lvl);

   static void lvl1(std::string const& str);
   static void lvl1(char* str);
   static void lvl2(std::string const& str);
   static void lvl2(char* str);
   static void alert(std::string const& str, std::string const& details = std::string());
   static void fatal(std::string const& str, std::string const& details = std::string());

protected:
};

#define SS(x) std::string(x)
#define TS(x) std::to_string(x)
#define TScoord(c) " (" + TS(c.x) + ", " + TS(c.y) + ") "

#define ALERT(fmt, ...)                                \
  do {                                                 \
    char s[1024];                                      \
    snprintf(s, 1024, fmt __VA_OPT__(, ) __VA_ARGS__); \
    dbg::alert(SS(s), SS(__FILE__) + SS(__func__));    \
  } while (false)

#define FATAL(fmt, ...)                                \
  do {                                                 \
    char s[1024];                                      \
    snprintf(s, 1024, fmt __VA_OPT__(, ) __VA_ARGS__); \
    dbg::fatal(SS(s), SS(__FILE__) + SS(__func__));    \
  } while (false)

#ifdef NDEBUG

#define DBGLVL1(fmt, ...)
#define DBGLVL2(fmt, ...)

#else

#define DBGLVL1(fmt, ...)                                               \
  do {                                                                  \
    char s[1024];                                                       \
    snprintf(s, 1024, "%s: " fmt, __func__ __VA_OPT__(, ) __VA_ARGS__); \
    dbg::lvl1(s);                                                       \
  } while (false)

#define DBGLVL2(fmt, ...)                                               \
  do {                                                                  \
    char s[1024];                                                       \
    snprintf(s, 1024, "%s: " fmt, __func__ __VA_OPT__(, ) __VA_ARGS__); \
    dbg::lvl2(s);                                                       \
  } while (false)

#endif

// Legacy support for object code
//#define PR_ANY(fmt,...)  DBGLVL2(fmt,__VA_ARGS__)
#define PR_ANY(fmt, ...)
#define PR_CHATTY(fmt, ...) DBGLVL2(fmt, __VA_ARGS__)
#define PR_INFO(fmt, ...) DBGLVL1(fmt, __VA_ARGS__)
#define PR_WARNING(fmt, ...) DBGLVL1(fmt, __VA_ARGS__)
#define PR_ERROR(fmt, ...) DBGLVL1(fmt, __VA_ARGS__)
