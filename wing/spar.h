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

#include "debug.h"
#include "object_oo.h"
#include "part.h"
#include "rib.h"
#include "tabs.h"

enum spartype_e {
   sheetspar,
   jigspar,
   ribsupport,
   singlespar,
   boxspar,
   hspar,
   hsspar,
   webs,
   websslotted,
   ribtabs,
   sheetJigType2,
   nospartype
};

class intersect_t {
public:
   rib_iter rib = {};               //!< The rib that this intersect relates to
   coord_t intersect = {};          //!< Location in planform of the intersect
   double posRib = 0.0;             //!< x location along the rib
   double posSpr = 0.0;             //!< x location along the spar
   coord_t rib_top = {};            //!< Rib outline top at intersect
   coord_t rib_bot = {};            //!< Rib outline bottom at intersect
   double minYforRibSupport = HUGE; //!< Lowest point on the rib for a rib support
   double wRib = 0.0;               //!< Width of slot in rib
   double aRib = 0.0;               //!< Lean angle of slot in rib
   double wSpr = 0.0;               //!< Width of slot in spar
   double aSpr = 0.0;               //!< Lean angle of slot in spar
   bool slotRib = true;             //!< Used to identify that the rib is already slotted here; automates the No Slot Last Rib feature
};

typedef typename std::list<intersect_t>::iterator intersect_iter;
bool sp_rib_is_sort(const intersect_t& a, const intersect_t& b);

class Spar : public Part {
public:
   static constexpr double JIG_EXTEND_END = 4.0;       //!< Leave this much material at the end of the jig spar
   static constexpr double JIG_MIN_TAB_IST_SEP = 15.0; //!< Intersects need to be this far apart to get tabs between them
   static constexpr double JIG_SEP_SLOT_WIDTH = 2.0;   //!< For jigSpar, the vertical gap separating the spar from the jig
   static constexpr double SPAR_MIN_HOLE_LENGTH = 7.0;
   static constexpr double RIB_SUPPORT_MIN_SLOT = 3.0;
   static constexpr double MIN_WEB_WIDTH_mm = 2.0;      //!< Do not draw a web if it is shorter than this
   static constexpr double WEB_SEG_OS_mm = 4.0;         //!< When splitting webs, put this much space between the segments
   static constexpr double REFLN_EXT_mm = 1.0;          //!< How much to extend the spar line to ensure intersects
   static constexpr double HSPAR_MARKSPACE = 0.45;      //!< Dotting line for drawing HSpar webs on plan
   static constexpr double BOXSPAR_MARKSPACE = 0.75;    //!< Dotting line for drawing Box Spar webs on plan
   static constexpr double RIBTAB_MARKSPACE = 0.25;     //!< Dotting line for drawing rib tab reference on plan
   static constexpr double RIBSUPPORT_MARKSPACE = 0.25; //!< Dotting line for drawing rib suport reference on plan

   static constexpr int WEB_ROLE = 1; //!< Internal role for storing a single spar web

   double stX = 0.0, stY = 0.0, enX = 0.0, enY = 0.0; //!< The spar line in plan
   bool widenSlots = false;                           //!< Widen slots
   bool noLastRibSlot = false;                        //!< Don't slot the last rib
   bool noRibKeepouts = false;                        //!< Don't add hole keepouts to ribs
   double thickness = 0.0;                            //!< Sheet Spars - sheet thickness
   pivot_e fe = CENTRE;                               //!< Sheet Spars - false edge type
   double slotDepthPercent = 0.0;                     //!< Sheet Spars - rib slot depth
   double ribTabW = 0.0;                              //!< Rib support tabs - width of tab
   double spW = 0.0, spD = 0.0;                       //!< Strip Spars - width and depth
   double wThck = 0.0, mlen = 0.0;                    //!< Strip Spars - Web thickness and maximum web length
   bool ribTop = false;                               //!< Single Spar - Place spar in rib top
   bool tabsNotSlots = false;                         //!< Sheet Spars - use front tabs
   enum spartype_e mytype = nospartype;               //!< What sort of spar is this?
   double height = 0.0;                               //!< Height of centreline above building board
   double tabpc = 0.0;                                //!< Percentage of joint to jig which is wood
   bool inFromBelow = false;                          //!< For sheet spars, insert the spar from below

   std::list<intersect_t> iss = {}; //!< Intersect information for rib/spar intersects

   bool create(Rib_set& ribs, std::string& log);

   obj& getPlan();

   bool isSheetType() {
      return ((mytype == sheetspar) || (mytype == websslotted) || (mytype == jigspar) || (mytype == ribsupport));
   };
   bool isStripType() {
      return ((mytype == boxspar) || (mytype == hspar) || (mytype == hsspar) || (mytype == singlespar) || (mytype == ribtabs));
   };
   bool isWebType() {
      return (mytype == webs);
   };

   void jigSpar(Rib_set& ribs, std::string& log);

   void ribSupport(Rib_set& ribs, std::string& log);

   void sheetSpar(Rib_set& ribs, std::string& log, double extendEnd = 0.0);

   void sparRibIntersect(Rib_set& ribs, std::string& log);

   bool drawSheetSparOutline(std::string& log);

   void sparWebs(Rib_set& ribs, std::string& log);

   void singleSpar(Rib_set& ribs, std::string& log);

   void ribTabs(Rib_set& ribs, std::string& log);

   void topBotSpar(Rib_set& ribs, std::string& log);
};

typedef typename std::list<Spar>::iterator spar_iter;

class Spar_set {
public:
   obj plan = {};
   obj pparts = {};
   std::list<Spar> spars = {};

   spar_iter begin() {
      return spars.begin();
   }
   spar_iter end() {
      return spars.end();
   }

   /**
    * @brief Add spars to the set from a generic tab
    */
   bool add(GenericTab* T, std::string& log);

   /**
    * @brief Create the spars each in turn
    */
   bool create(Rib_set& ribs, std::string& log);

   /**
    * @brief Generate sheeting jigs spars - Type 2
    */
   bool addCreateJigsType2(GenericTab* T1, Rib_set& ribs, std::string& log);

   /**
    * @brief Get the plan view of all the spars
    */
   obj& getPlan();

   /**
    * @brief Return an index for a given spar iterator
    */
   unsigned sparItInd(std::list<Spar>::iterator it) {
      return (1 + std::distance(spars.begin(), it));
   }

   /**
    * @brief Parts ready for display
    */
   void getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts);
};
