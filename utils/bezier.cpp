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
#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include <cmath>

#include "bezier.h"
#include "object_oo.h"

CubicBezier::CubicBezier(coord_t pi1, coord_t pi2, coord_t pi3, coord_t pi4)
{
   p1 = pi1;
   p2 = pi2;
   p3 = pi3;
   p4 = pi4;
}

coord_t CubicBezier::point(double t) const
{
   const double mt = 1 - t;
   const double k1 = mt * mt * mt;
   const double k2 = 3 * mt * mt * t;
   const double k3 = 3 * mt * t * t;
   const double k4 = t * t * t;
   return (coord_t{ p1.x * k1 + p2.x * k2 + p3.x * k3 + p4.x * k4,
                   p1.y * k1 + p2.y * k2 + p3.y * k3 + p4.y * k4 });
}

obj CubicBezier::curve(double tBeg, double tEnd, double tStep) const
{
   obj o;
   double t = tBeg;
   while (t <= tEnd) {
      o.add(point(t));
      t += tStep;
   }
   return (o);
}

obj CubicBezier::curve() const
{
   return (curve(0.0, 1.0, 0.001));
}

obj CubicBezier::curve(double maxSegLen) const
{
   // Line joining p1, p2, p3, p4 is the worst case length
   obj lenRef;
   lenRef.add(p1);
   lenRef.add(p2);
   lenRef.add(p3);
   lenRef.add(p4);
   const int segments = (int)round(lenRef.len() / maxSegLen);
   return (curve(0.0, 1.0, 1.0 / segments));
}
