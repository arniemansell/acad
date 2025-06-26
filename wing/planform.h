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

#include "object_oo.h"
#include "part.h"
#include "tabs.h"

class Planform : public Part {
public:
   static constexpr int LE = 0;
   static constexpr int TE = 1;
   static constexpr int BOX = 2;

   Planform();

   /**
    * @brief Add planform points from the entry tab
    */
   bool add(GenericTab* T, std::string& log);

   /**
    * @brief Return a line joining the trailing edge to leading edge at X positions
    */
   line get_airfoil_line(double leX, double teX);

   /**
    * @brief Is point/line within the box defined by the planform
    */
   bool isInPlanform(coord_t pt);
   bool isInPlanform(const line& ln);

   /**
    * @brief get an outline of the plan
    */
   obj& getPlan();

private:
   bool isDefined = false;
   void addLePoint(double x, double y);
   void addTePoint(double x, double y);
};
