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
#ifndef BEZIER_H
#define BEZIER_H

#include "object_oo.h"

class CubicBezier
{
public:
   CubicBezier() = default;
   CubicBezier(coord_t p1, coord_t p2, coord_t p3, coord_t p4);

   coord_t point(double t) const; // Return a single point at bezier interval value t
   obj curve() const;             // The curve as a default of 1000 points
   obj curve(
      double maxSegLen) const; // The curve with enough points to guarantee maximum segment length
   obj curve(double tBeg,
      double tEnd,
      double tStep) const; // The curve for a range of bezier interval values

private:
   coord_t p1 = { 0.0, 0.0 };
   coord_t p2 = { 0.0, 0.0 };
   coord_t p3 = { 0.0, 0.0 };
   coord_t p4 = { 0.0, 0.0 };
};

#endif // BEZIER_H
