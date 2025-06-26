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

class Element : public Part {
public:
   enum shape_e {
      TUBE,
      BAR,
      DOT,
      NONE
   };

   enum z_e {
      CHOORD_L,
      SNAP_BOTTOM,
      SNAP_TOP,
      ROTATE_BOTTOM,
      ROTATE_TOP
   };

   shape_e shape = shape_e::NONE;
   double angle = 0.0;
   double diameter = 0.0;
   double width = 0.0;
   double depth = 0.0;
   double stX = 0.0;
   double stY = 0.0;
   double stZ = 0.0;
   double enX = 0.0;
   double enY = 0.0;
   double enZ = 0.0;
   bool widenSlots = false;
   z_e ztype = z_e::CHOORD_L;

   line yLn = {};
   line zLn = {};

   bool create(Rib_set& rbs, std::string& log, bool draftmode);

   obj& getPlan();
};

class Element_set {
public:
   std::list<Element> elms = {};
   bool draft = false;
   obj plan = {};

   /**
    * @brief Add elements from the entry model
    */
   bool add(GenericTab* T, std::string& log);

   /**
    * @brief Create the elements and add them to the ribs
    */
   bool create(Rib_set& rbs, std::string& log);

   /**
    * @brief Configure to work in draft mode
    */
   void draft_mode() {
      draft = true;
   }

   /**
    * @brief Get the plan view of the elements
    */
   obj& getPlan();
};
