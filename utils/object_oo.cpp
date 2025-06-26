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

#include "debug.h"
#include "hpgl.h"
#include "object_oo.h"
#include <assert.h>

char lineStr[256]; // Not thread safe

bool isEqualWithinMargin(double arg1, double arg2, double margin) {
   double maxVal = arg1 + margin;
   double minVal = arg1 - margin;
   return ((arg2 <= maxVal) && (arg2 >= minVal));
}

bool isEqualWithinPercentage(double arg1, double arg2, double percentage) {
   double margin = arg1 * (percentage / 100.0);
   return (isEqualWithinMargin(arg1, arg2, margin));
}

bool isSamePoint(coord_t pt1, coord_t pt2) {
   return (distTwoPoints(pt1, pt2) <= SNAP_LEN ? 1 : 0);
}

double distTwoPoints(coord_t c1, coord_t c2) {
   double dx = c1.x - c2.x;
   double dy = c1.y - c2.y;
   return (sqrt((dx * dx) + (dy * dy)));
}

coord_t averageTwoPoints(coord_t c1, coord_t c2) {
   return coord_t{ (c1.x + c2.x) / 2.0, (c1.y + c2.y) / 2.0 };
}

double dotprodRaw(double x1, double y1, double x2, double y2) {
   return ((x1 * x2) + (y2 * y1));
}

double dotprod(vector_t pt1, vector_t pt2) {
   return (dotprodRaw(pt1.dx, pt1.dy, pt2.dx, pt2.dy));
}

double perpprodRaw(double x1, double y1, double x2, double y2) {
   return ((x1 * y2) - (x2 * y1));
}

double perpprod(vector_t pt1, vector_t pt2) {
   return (perpprodRaw(pt1.dx, pt1.dy, pt2.dx, pt2.dy));
}

double slotWidth(const line& crossLine, const line& slottedLine, double crossThck, double slottedThck) {
   double wCross = crossLine.angle();
   double wSlotd = slottedLine.angle();
   double theta = fabs(wCross - wSlotd);

   if (theta > (M_PI / 2)) {
      theta = M_PI - theta;
   }

   PR_ANY("slotWidth: wCross %.1lf  wSlotd %.1lf  Theta %.1lf\n", TO_DEGS(wCross), TO_DEGS(wSlotd), TO_DEGS(theta));
   return ((slottedThck / tan(theta)) + (crossThck / sin(theta)));
}

// Rotate point ptx,pty about pivot
void rotatePoint(coord_t* pt, coord_t pivot, double rads) {
   coord_t unit = { cos(rads), sin(rads) };
   coord_t vec = { (pt->x - pivot.x), (pt->y - pivot.y) };

   pt->x = pivot.x + (vec.x * unit.x) - (vec.y * unit.y);
   pt->y = pivot.y + (vec.x * unit.y) + (vec.y * unit.x);
}

bool obj_sort_left_right(const obj& a, const obj& b) {
   obj cpya = a;
   obj cpyb = b;
   return (cpya.find_avg_centre().x < cpyb.find_avg_centre().x);
}

bool obj_sort_top_bottom(const obj& a, const obj& b) {
   obj cpya = a;
   obj cpyb = b;
   return (cpya.find_avg_centre().y < cpyb.find_avg_centre().y);
}

bool intersect_sort(const obj_line_intersect& a, const obj_line_intersect& b) {
   return (a.T < b.T);
}

// LINVAR
linvar::linvar(double x0, double y0, double x1, double y1)
   : m{ (y1 - y0) / (x1 - x0) },
   c{ y0 - (x0 * (y1 - y0) / (x1 - x0)) },
   x0{ x0 },
   x1{ x1 } {
}

double linvar::v(double x) const {
   return ((m * x) + c);
}

double linvar::vl(double x) const {
   if (x1 >= x0) {
      x = (x < x0) ? x0 : x;
      x = (x > x1) ? x1 : x;
   }
   else {
      x = (x < x1) ? x1 : x;
      x = (x > x0) ? x0 : x;
   }

   return ((m * x) + c);
}

// SQVAR
sqvar::sqvar(double x0, double y0, double x1, double y1, double squaredness)
   : x0{ x0 },
   y0{ y0 },
   x1{ x1 },
   y1{ y1 },
   cdenom{ (1.0 / (x1 - x0)) },
   yd{ (y1 - y0) },
   m{ squaredness },
   k{ (1.0 - squaredness) } {
}

double sqvar::vl(double x) const {
   if (x1 >= x0) {
      x = (x < x0) ? x0 : x;
      x = (x > x1) ? x1 : x;
   }
   else {
      x = (x < x1) ? x1 : x;
      x = (x > x0) ? x0 : x;
   }

   double c = (x - x0) * cdenom;
   return (((c * c) * yd) + y0);
}

// LINE
line::line(coord_t s0, vector_t v0)
   : S0(s0),
   V(v0) {
}

line::line(coord_t s0, coord_t s1)
   : S0(s0),
   V(vector_t{ (s1.x - s0.x), (s1.y - s0.y) }) {
}

line::line(coord_t s0, double length, double angle) {
   S0 = s0;
   V = vector_t{ length, 0.0 };
   rotate(S0, angle);
}

// PRIVATE METHODS
bool line::is_small(double val) const {
   return (fabs(val) < SNAP_LEN);
}

// SET METHODS
void line::set(coord_t s0, vector_t v0) {
   S0 = s0;
   V = v0;
}

void line::set(coord_t s0, double length, double angle) {
   S0 = s0;
   V = vector_t{ length, 0.0 };
   rotate(S0, angle);
}

void line::set(coord_t s0, coord_t s1) {
   S0 = s0;
   V.dx = s1.x - s0.x;
   V.dy = s1.y - s0.y;
}

// DIMENSION METHODS
double line::len() const {
   return sqrt((V.dx * V.dx) + (V.dy * V.dy));
}

double line::angle() const {
   if (is_small(V.dx)) {
      return ((V.dy > 0) ? (M_PI / 2.0) : (-M_PI / 2.0));
   }
   return (atan2(V.dy, V.dx));
}

double line::angle(line& l2) {
   if ((len() > 0.0) && (l2.len() > 0.0))
      return atan2(perpprod(V, l2.get_V()), dotprod(V, l2.get_V()));
   else
      return 0.0;
}

// Get a coordinate point on a line based on T
coord_t line::get_pt(double T) const {
   coord_t pt = { S0.x + (T * V.dx), S0.y + (T * V.dy) };
   return (pt);
}

vector_t line::get_V() const {
   return V;
}

coord_t line::get_S0() const {
   return S0;
}

coord_t line::get_S1() const {
   return get_pt(T_S1);
}

double line::T_for_y(double y) const {
   if (is_horizontal())
      return (0.0);

   return ((y - S0.y) / V.dy);
}

double line::T_for_x(double x) const {
   if (is_vertical())
      return (0.0);

   return ((x - S0.x) / V.dx);
}

double line::T_for_pt(coord_t pt) const {
   if (std::abs(V.dx) > std::abs(V.dy))
      return T_for_x(pt.x);

   return T_for_y(pt.y);
}

// INTERROGATION METHODS
char* line::print_str() {
   sprintf(lineStr, "S0(%.3lf, %.3lf) V(%.3lf, %.3lf)", S0.x, S0.y, V.dx, V.dy);
   return (lineStr);
}

bool line::is_vertical() const {
   return (is_small(V.dx));
}

bool line::is_horizontal() const {
   return (is_small(V.dy));
}

bool line::is_contiguous_with(const line& l) const {
   coord_t l1s0 = l.get_S0();
   coord_t l1s1 = l.get_S1();
   coord_t l2s0 = get_S0();
   coord_t l2s1 = get_S1();

   return (isSamePoint(l1s0, l2s0) || isSamePoint(l1s0, l2s1) || isSamePoint(l1s1, l2s0) || isSamePoint(l1s1, l2s1));
}

bool line::is_in_collinear_seg(coord_t pt) const {
   double line_min, line_max, line_point1, line_point2, point;

   if (!is_vertical()) {
      // Segment is not vertical, just check the x value
      line_point1 = get_S0().x;
      line_point2 = get_S1().x;
      point = pt.x;
   }
   else {
      // Segment is vertical, so just check the y value
      line_point1 = get_S0().y;
      line_point2 = get_S1().y;
      point = pt.y;
   }

   line_min = (line_point1 < line_point2) ? line_point1 : line_point2;
   line_max = (line_point1 >= line_point2) ? line_point1 : line_point2;

   return ((point <= line_max) && (point >= line_min));
}

bool line::is_same_as(const line& ln) const {
   return ((ln.get_S0().x == S0.x) &&
      (ln.get_S0().y == S0.y) &&
      (ln.get_V().dx == V.dx) &&
      (ln.get_V().dy == V.dy));
}

bool line::lines_intersect(const line& l2, coord_t* i, int allowExtrapolation) const {
   vector_t w = { (S0.x - l2.S0.x), (S0.y - l2.S0.y) };
   double perpl1l2 = perpprod(V, l2.V);
   coord_t dummy = {};

   if (!i)
      i = &dummy;

   // Safe initialisation
   i->x = 0;
   i->y = 0;

   // Check for lines being parallel
   if (((perpl1l2 < 0) && (perpl1l2 > (-SMALL_NUM))) || ((perpl1l2 >= 0) && (perpl1l2 < SMALL_NUM))) {
      double dotl1 = dotprod(V, V);
      double dotl2 = dotprod(l2.V, l2.V);
      double perpl1w = perpprod(V, w);
      double perpl2w = perpprod(l2.V, w);

      // They are parallel, but are they collinear?
      if ((perpl1w != 0.0) || (perpl2w != 0.0)) {
         return (false); // They are not collinear
      }

      // they are parallel and collinear; if extrapolation is allowed they must intersect
      if (allowExtrapolation) {
         i->x = (S0.x + l2.S0.x + (get_S1()).x + (l2.get_S1()).x) / 4.0;
         i->y = (S0.y + l2.S0.y + (get_S1()).y + (l2.get_S1()).y) / 4.0;
         return (true);
      }

      // they are collinear or they are degenerate point(s)
      if (dotl1 == 0 && dotl2 == 0) {
         // both lines are points
         if (is_contiguous_with(l2)) {
            i->x = S0.x;
            i->y = S0.y;
            return (true); // they are the same point
         }
         else {
            return (false); // they are distinct points
         }
      }

      if (dotl1 == 0) {
         // line 1 is a single point
         if (!l2.is_in_collinear_seg(S0)) {
            // but is not in the line S2
            return (false);
         }
         i->x = S0.x;
         i->y = S0.y;
         return (true);
      }

      if (dotl2 == 0) {
         // line 2 is a single point
         if (!is_in_collinear_seg(l2.S0)) {
            // but is not in the line S1
            return (false);
         }
         i->x = l2.S0.x;
         i->y = l2.S0.y;
         return (true);
      }

      // they are collinear segments - get  overlap (or not)
      {
         double t1, t2;
         vector_t w2 = { (get_S1().x - l2.S0.x), (get_S1().y - l2.S0.y) };

         if (l2.V.dx != 0) {
            t1 = w.dx / l2.V.dx;
            t2 = w2.dx / l2.V.dx;
         }
         else {
            t1 = w.dy / l2.V.dy;
            t2 = w2.dy / l2.V.dy;
         }

         if (t1 > t2) {
            // Need to swap t1 and t2
            double tmp = t1;
            t1 = t2;
            t2 = tmp;
         }

         if ((t1 > 1.0) || (t2 < 0.0)) {
            // There is no overlap
            return (false);
         }

         // Clip the factors
         t1 = (t1 < 0) ? 0 : t1;

         // Find valid overlap point
         *i = l2.get_pt(t1);
         return (true);
      }
   }

   // the segments are skew and may intersect in a point
   {
      // get the intersect parameter for seg1
      double s1I = perpprod(l2.V, w) / perpl1l2;
      double s2I = perpprod(V, w) / perpl1l2;

      if (((s1I < 0) || (s1I > 1) || (s2I < 0) || (s2I > 1)) && !allowExtrapolation) {
         // No intersection
         return (false);
      }

      // Calculate the intersection
      *i = get_pt(s1I);
      return (true);
   }
}

double line::distance_to_point(coord_t p) const {
   coord_t S1 = get_S1();

   // Find where perpendicular line to point intersects the actual line
   double t = (((p.x - S0.x) * V.dx) + ((p.y - S0.y) * V.dy)) / ((V.dx * V.dx) + (V.dy * V.dy));

   if (t < 0.0) // Nearest point is the S0 end
   {
      return (distTwoPoints(S0, p));
   }
   else if (t > 1.0) // Nearest point is the S1 end
   {
      return (distTwoPoints(S1, p));
   }
   // Nearest point is the point of intersect
   return (distTwoPoints(p, get_pt(t)));
}

// MANIPULATION
void line::add_offset(double xOffset, double yOffset) {
   S0.x = S0.x + xOffset;
   S0.y = S0.y + yOffset;
}

void line::move_sideways(double dist) {
   coord_t perp = { -V.dy, V.dx };
   double scale = (1 / sqrt((perp.x * perp.x) + (perp.y * perp.y))) * dist;
   add_offset((perp.x * scale), (perp.y * scale));
}

void line::rotate(coord_t pivot, double rads) {
   coord_t S0i = get_S0();
   coord_t S1i = get_S1();
   rotatePoint(&S0i, pivot, rads);
   rotatePoint(&S1i, pivot, rads);
   set(S0i, S1i);
}

void line::mirror_x() {
   S0.x = -S0.x;
   V.dx = -V.dx;
}

void line::mirror_y() {
   S0.y = -S0.y;
   V.dy = -V.dy;
}

void line::reverse() {
   S0 = get_S1();
   V.dx = -V.dx;
   V.dy = -V.dy;
}

void line::set_length(double length) {
   double scale = length / len();
   V.dx = V.dx * scale;
   V.dy = V.dy * scale;
}

void line::extend_S0_mm(double mm) {
   // Can't do anything to a stupidly short line
   if (is_small(len()))
      return;

   double factor = -mm / len();
   set(get_pt(factor), get_S1());
}

void line::extend_S1_mm(double mm) {
   // Can't do anything to a stupidly short line
   if (is_small(len()))
      return;

   double factor = 1.0 + (mm / len());
   set(get_S0(), get_pt(factor));
}

void line::extend_mm(double mm) {
   extend_S0_mm(mm);
   extend_S1_mm(mm);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// OBJ
/////////////////////////////////////////////////////////////////////////////////////////////////
// Constructors
obj::obj(coord_t s0, coord_t s1) {
   e.emplace_back(s0, s1);
}

obj::obj(coord_t s0, vector_t v0) {
   e.emplace_back(s0, v0);
}

obj::obj(line ln) {
   e.emplace_back(ln);
}

// ADD NEW ELEMENT
line_iter obj::add(coord_t s0, coord_t s1) // Initialise using two points
{
   e.emplace_back(s0, s1);
   return last();
}

line_iter obj::add(coord_t s0, vector_t v0) // Initialise using point and a vector
{
   e.emplace_back(s0, v0);
   return last();
}

line_iter obj::add(const line& ln) // Initialise using two points
{
   e.emplace_back(ln);
   return last();
}

line_iter obj::add(double x1, double y1, double x2, double y2) {
   return add(coord_t{ x1, y1 }, coord_t{ x2, y2 });
}

line_iter obj::add(coord_t pt) {
   if (empty()) {
      return add(pt, vector_t{ 0.0, 0.0 }); // Object is empty so add zero length line ("a point")
   }
   else {
      return add(get_ep(), pt); // Else extend object from endpoint to pt
   }
}

void obj::add_dotted(const line& ln, double marklen, double splen) {
   double markT = (marklen / ln.len());
   double spaceT = (splen / ln.len());
   for (double T = 0.0; T <= (1.0 - markT); T = T + (markT + spaceT)) {
      e.emplace_back(ln.get_pt(T), ln.get_pt(T + markT));
   }
}

line_iter obj::add_rect(coord_t c1, coord_t c2, double ms) {
   line l1 = { coord_t{c1.x, c1.y}, coord_t{c1.x, c2.y} };
   line l2 = { coord_t{c1.x, c2.y}, coord_t{c2.x, c2.y} };
   line l3 = { coord_t{c2.x, c2.y}, coord_t{c2.x, c1.y} };
   line l4 = { coord_t{c2.x, c1.y}, coord_t{c1.x, c1.y} };
   if (ms != 1.0) {
      double mark = 4.0 * ms;
      double space = 4.0 * (1 - ms);
      add_dotted(l1, mark, space);
      add_dotted(l2, mark, space);
      add_dotted(l3, mark, space);
      add_dotted(l4, mark, space);
   }
   else {
      add(l1);
      add(l2);
      add(l3);
      add(l4);
   }

   return last();
}

line_iter obj::add_rect(const line& ln, double w, double ms) {
   line side1 = ln;
   line side2 = ln;
   side1.move_sideways(w / 2.0);
   side2.move_sideways(-w / 2.0);
   if (ms != 1.0) {
      double mark = 4.0 * ms;
      double space = 4.0 * (1 - ms);
      add_dotted(line(side1.get_S0(), side1.get_S1()), mark, space);
      add_dotted(line(side2.get_S0(), side2.get_S1()), mark, space);
      add_dotted(line(side1.get_S0(), side2.get_S0()), mark, space);
      add_dotted(line(side1.get_S1(), side2.get_S1()), mark, space);
   }
   else {
      add(side1.get_S0(), side1.get_S1());
      add(side2.get_S0(), side2.get_S1());
      add(side1.get_S0(), side2.get_S0());
      add(side1.get_S1(), side2.get_S1());
   }
   return last();
}

void obj::add_ellipse(coord_t centre, double rx, double ry) {
   for (double deg = 1; deg <= 360; deg = deg + 1) {
      double phi0 = (deg - 1.0) * M_PI / 180.0;
      double phi1 = deg * M_PI / 180.0;
      add(coord_t{ rx * cos(phi0), ry * sin(phi0) }, coord_t{ rx * cos(phi1), ry * sin(phi1) });
   }
   add_offset(centre.x, centre.y);
}

void obj::move_back_to_front() {
   if (!e.empty()) {
      line ln = *(std::prev(e.end()));
      e.pop_back();
      e.emplace_front(ln);
   }
}

// DELETE
void obj::del(line_iter& iter) {
   if (iter == end())
      FATAL("Cannot delete when iterator = end()");

   e.erase(iter);
}

void obj::del(line_iter& first, line_iter& last) {
   if (first == end())
      FATAL("Cannot delete when iterator = end()");

   e.erase(first, last);
}

void obj::del() {
   e.clear();
}

size_t obj::del_duplicates() {
   size_t cnt = 0;

   for (auto cur = begin(); cur != end(); ++cur) // Loop all iterators
   {
      for (auto cmp = next(cur); cmp != end(); ++cmp) // Loop through elements in front of cur
      {
         if (cur->is_same_as(*cmp)) {
            del(cmp);
            cmp = cur;
            cnt++;
         }
      }
   }
   return cnt;
}

size_t obj::del_zero_lens() {
   auto ln = begin();
   size_t cnt = 0;

   while (ln != end()) {
      if (ln->len() < SMALL_NUM) {
         del(ln);
         ln = begin();
         cnt++;
      }
      else {
         ++ln;
      }
   }
   return cnt;
}

bool obj::del_dupes_zeros() {
   size_t d = del_duplicates();
   size_t z = del_zero_lens();
   return ((d != 0) || (z != 0));
}

size_t obj::remove_verticals() {
   line_iter ln = begin();
   size_t cnt = 0;
   while (ln != end()) {
      line_iter nxt = next(ln);
      if (ln->is_vertical()) {
         del(ln);
         cnt++;
      }
      ln = nxt;
   }
   return cnt;
}

// POSITIONAL
line_iter obj::begin() const // First Element iterator
{
   return e.begin();
}

line_iter obj::last() const // Last element iterator
{
   if (e.empty())
      return e.end();
   return std::prev(e.end());
}

line_iter obj::end() const // End element iterator
{
   return e.end();
}

const_line_iter obj::cbegin() const // First Element iterator
{
   return e.cbegin();
}

const_line_iter obj::clast() const // Last element iterator

{
   if (e.empty())
      return e.cend();
   return std::prev(e.cend());
}

const_line_iter obj::cend() const // End element iterator
{
   return e.cend();
}

bool obj::is_begin(line_iter ln) const {
   return (ln == e.begin());
}

bool obj::is_last(line_iter ln) const {
   if (ln == e.end())
      return false;
   ++ln;
   return (ln == e.end());
}

bool obj::is_end(line_iter ln) const {
   return (ln == e.end());
}

line_iter obj::nextc(line_iter ln) const {
   if (empty())
      return end();
   if (is_last(ln)) {
      return begin();
   }
   return std::next(ln);
}

line_iter obj::prevc(line_iter ln) const {
   if (empty())
      return end();
   if (is_begin(ln)) {
      return last();
   }
   return std::prev(ln);
}

line_iter obj::next(line_iter ln) const {
   if (empty() || is_end(ln))
      return end();
   return std::next(ln);
}

line_iter obj::prev(line_iter ln) const {
   if (empty())
      return end();
   if (is_begin(ln))
      return begin();
   return std::prev(ln);
}

// INTERROGATION
bool obj::empty() const {
   return e.empty();
}

double obj::len() const {
   double l = 0.0;
   for (line_iter ln = begin(); ln != end(); ++ln) {
      l += ln->len();
   }
   return l;
}

size_t obj::size() const {
   return e.size();
}

coord_t obj::get_sp() const // Get the first point of the first element
{
   return begin()->get_S0();
}

coord_t obj::get_ep() const // Get the last point of the last element
{
   return last()->get_S1();
}

coord_t obj::get_pt_along_length(double dist, line_iter& ln, double& T) const {
   if (empty()) {
      T = 0.0;
      ln = end();
      return coord_t{ 0.0, 0.0 };
   }

   if (dist <= 0.0) {
      T = 0.0;
      ln = begin();
      return get_sp();
   }

   dist = std::fmod(dist, len()); // Continue around the object if dist > object length
   double lenSoFar = 0.0;
   for (ln = begin(); ln != end(); ++ln) {
      double llen = ln->len();
      T = (dist - lenSoFar) / llen;
      if (T <= T_S1)
         return ln->get_pt(T);
      else
         lenSoFar += llen;
   }
   dbg::fatal(SS("Internal error in get_pt_along_length"), TS(lenSoFar));
   return coord_t{ 0.0, 0.0 };
}

coord_t obj::get_pt_along_length(double dist, line_iter& ln) const {
   double T = 0.0;
   return get_pt_along_length(dist, ln, T);
}

coord_t obj::get_pt_along_length(double dist) const {
   double T = 0.0;
   line_iter ln;
   return get_pt_along_length(dist, ln, T);
}

size_t obj::index(line_iter ln) const {
   return std::distance(begin(), ln);
}

bool obj::sX_is_at(coord_t pt, line_iter& ln, double T) const {
   for (line_iter l = begin(); l != end(); ++l) {
      if (isSamePoint(l->get_pt(T), pt)) {
         ln = l;
         return true;
      }
   }

   return false;
}

bool obj::s1_is_at(coord_t pt, line_iter& ln) const {
   return (sX_is_at(pt, ln, T_S1));
}

bool obj::s0_is_at(coord_t pt, line_iter& ln) const {
   return (sX_is_at(pt, ln, T_S0));
}

coord_t obj::originIsAt() const {
   double ext[4];
   find_extremity(ext);
   return coord_t{ ext[LEFT], ext[DOWN] };
}

bool obj::findMarkerSquare(double size, coord_t* centre, bool deleteIt) {
   line_iter nxt = begin();
   line_iter sq[4]; // Store the four sides of the square
   unsigned int sideInd = 0;

   while (nxt != end()) {
      // Search for a side of the square
      if ((nxt->is_vertical() && isEqualWithinMargin(nxt->len(), size, 4 * SNAP_LEN)) ||
         (nxt->is_horizontal() && isEqualWithinMargin(nxt->len(), size, 4 * SNAP_LEN))) {
         PR_ANY("Found potential centre marker side: %u S0[%lf %lf] V[%lf %lf] - ", sideInd, nxt->get_S0().x, nxt->get_S0().y, nxt->get_V().dx, nxt->get_V().dy);

         // If this is the first side then just store it
         if (sideInd == 0) {
            PR_ANY("Marked as side 1\n");
            sq[sideInd++] = nxt;
         }
         else {
            // Otherwise see if it is connected before storing it
            unsigned int s;
            for (s = 0; s < sideInd; s++) {
               if (nxt->is_contiguous_with(*sq[s])) {
                  sq[sideInd++] = nxt;
                  PR_ANY("It is connected, marked as side %u\n", sideInd);
                  break;
               }
            }
         }
      }

      if (sideInd == 4) {
         // All four sides have been found, work out the centre point
         centre->x = (sq[0]->get_S0().x + sq[0]->get_S1().x +
            sq[1]->get_S0().x + sq[1]->get_S1().x +
            sq[2]->get_S0().x + sq[2]->get_S1().x +
            sq[3]->get_S0().x + sq[3]->get_S1().x) /
            8.0;
         centre->y = (sq[0]->get_S0().y + sq[0]->get_S1().y +
            sq[1]->get_S0().y + sq[1]->get_S1().y +
            sq[2]->get_S0().y + sq[2]->get_S1().y +
            sq[3]->get_S0().y + sq[3]->get_S1().y) /
            8.0;
         PR_ANY("Marker (%.2lf x %.2lf) found at x=%lf y=%lf\n", size, size, centre->x, centre->y);

         if (deleteIt) {
            e.erase(sq[0]);
            e.erase(sq[1]);
            e.erase(sq[2]);
            e.erase(sq[3]);
            PR_ANY("Marker deleted\n");
         }
         return (true);
      }

      // Still looking for sides, so move on to the next element
      if (is_last(nxt) && (sideInd > 0)) {
         // Last element and we did find a starting side, so return to that
         nxt = sq[0];
         sideInd = 0;
      }
      ++nxt;
   }

   PR_ANY("No marker square (%.2lf x %.2lf) found\n", size, size);
   return (false);
}

line_iter obj::line_intersect(const line& l2, coord_t* i, int allowExtrapolation) const {
   return line_intersect(begin(), l2, i, allowExtrapolation);
}

line_iter obj::line_intersect(line_iter l1, const line& l2, coord_t* i, int allowExtrapolation) const {
   while (l1 != end()) {
      if (l1->lines_intersect(l2, i, allowExtrapolation)) {
         break;
      }
      ++l1;
   }
   return l1;
}

bool obj::line_intersect(line l2, std::list<obj_line_intersect>* isects, int allowExtrapolation) const {
   coord_t iPt = {};
   bool retval = false;

   if (allowExtrapolation) {
      // Extrapolation refers to L2, not the line elements of the object
      // So we fudge it by extending the line L2 outside of the object extremities
      // Find the extremities of this object
      double ext[4];
      find_extremity(ext);

      // Keep expanding the line until both ends are outside of the bounding box of the object
      bool finished = false;
      do {
         coord_t S0 = l2.get_pt(-1.0);
         coord_t S1 = l2.get_pt(+2.0);
         l2.set(S0, S1);
         bool xOk = ((S0.x < ext[LEFT]) && (S1.x > ext[RIGHT])) || ((S1.x < ext[LEFT]) && (S0.x > ext[RIGHT]));
         bool yOk = ((S0.y < ext[DOWN]) && (S1.y > ext[UP])) || ((S1.y < ext[DOWN]) && (S0.y > ext[UP]));
         finished = xOk || yOk;
      } while (!finished);
   }

   // Work through the object elements and find all the intersections with L2
   for (line_iter ln = begin(); ln != end(); ++ln) {
      if (ln->lines_intersect(l2, &iPt, 0)) {
         retval = true;
         if (isects) {
            obj_line_intersect intersect = { l2.T_for_pt(iPt), ln, iPt };
            isects->push_back(intersect);
         }
      }
   }

   // Sort the intersects in order of increasing T
   if (isects)
      isects->sort(intersect_sort);

   return (retval);
}

bool obj::obj_intersect(obj& o) const {
   coord_t temp;
   for (line_iter ln = o.begin(); ln != o.end(); ++ln) {
      if (line_intersect(*ln, &temp, 0) != end())
         return true;
   }
   return false;
}

bool obj::top_bot_intersect(double xpos, coord_t* upper, coord_t* lower, line_iter& it_upper, line_iter& it_lower) const {
   std::list<obj_line_intersect> isects = {};
   line ref(coord_t{ xpos, 0.0 }, coord_t{ xpos, 1.0 });

   line_intersect(ref, &isects, 1);

   if (isects.size() == 0)
      return false;

   *lower = isects.front().pt;
   it_lower = isects.front().ln;

   *upper = isects.back().pt;
   it_upper = isects.back().ln;

   return true;
}

bool obj::top_bot_intersect(double xpos, coord_t* upper, coord_t* lower) const {
   line_iter ln0, ln1;
   return top_bot_intersect(xpos, upper, lower, ln0, ln1);
}

bool obj::top_intersect(double xpos, coord_t* pt, line_iter& ln) const {
   coord_t dpt;
   line_iter dln;

   return top_bot_intersect(xpos, pt, &dpt, ln, dln);
}

bool obj::bot_intersect(double xpos, coord_t* pt, line_iter& ln) const {
   coord_t dpt;
   line_iter dln;

   return top_bot_intersect(xpos, &dpt, pt, dln, ln);
}

bool obj::dir_intersect(direction_e dir, double xpos, coord_t* pt, line_iter& ln) const {
   switch (dir) {
   case UP:
      return top_intersect(xpos, pt, ln);
      break;
   case DOWN:
      return bot_intersect(xpos, pt, ln);
      break;
   default:
      break;
   }
   return false;
}

bool obj::test_extremity(direction_e dir, double var, double* res, coord_t* ptu, coord_t* ptd, coord_t inPt) const {
   // Test for a new best value
   if ((((dir == LEFT) || (dir == DOWN)) && (var < *res)) ||
      (((dir == RIGHT) || (dir == UP)) && (var > *res))) {
      *ptu = *ptd = inPt;
      *res = var;
      PR_ANY("B(%.1lf, %.1lf)", ptu->x, ptu->y);
      return true;
   }

   // Test for an equivalent to the best
   // if (isEqualWithinMargin(*res, var, SNAP_LEN))
   if (*res == var) {
      // Matching value, move the upper or lower point
      ptu->y = (((dir == LEFT) || (dir == RIGHT)) && (inPt.y > ptu->y)) ? inPt.y : ptu->y;
      ptd->y = (((dir == LEFT) || (dir == RIGHT)) && (inPt.y < ptd->y)) ? inPt.y : ptd->y;
      ptu->x = (((dir == UP) || (dir == DOWN)) && (inPt.x > ptu->x)) ? inPt.x : ptu->x;
      ptd->x = (((dir == UP) || (dir == DOWN)) && (inPt.x < ptd->x)) ? inPt.x : ptd->x;
      PR_ANY("M(%.1lf, %.1lf)(%.1lf, %.1lf)", ptu->x, ptu->y, ptd->x, ptd->y);
      return true;
   }

   PR_ANY("N");

   return false;
}

void obj::find_extremity(coord_t pt[4], double extremity[4], line_iter elm[4]) const {
   if (empty()) {
      for (int i = 0; i < 4; ++i) {
         pt[i].x = 0.0;
         pt[i].y = 0.0;
         extremity[i] = 0.0;
         elm[i] = end();
      }
      return;
   }
   coord_t ptu[4] = { 0, 0 }, ptd[4] = { 0, 0 };

   // Initialise best extremity estimate so far
   extremity[LEFT] = extremity[DOWN] = HUGE;
   extremity[RIGHT] = extremity[UP] = -HUGE;

   // Work through all the elements to find the extremity
   line_iter ln = begin();
   while (ln != end()) {
      for (size_t dir = LEFT; dir <= DOWN; dir++) {
         // Select the important axis
         double S0 = ((dir == UP) || (dir == DOWN)) ? ln->get_S0().y : ln->get_S0().x;
         double S1 = ((dir == UP) || (dir == DOWN)) ? ln->get_S1().y : ln->get_S1().x;

         // Test if this line immproves the estimate
         if (test_extremity((direction_e)dir, S0, &extremity[dir], &ptu[dir], &ptd[dir], ln->get_S0()))
            elm[dir] = ln;
         if (test_extremity((direction_e)dir, S1, &extremity[dir], &ptu[dir], &ptd[dir], ln->get_S1()))
            elm[dir] = ln;
      }
      ++ln;
   }

   // Actual extremity is halfway between ptu and ptd
   for (size_t dir = LEFT; dir <= DOWN; dir++) {
      line temp(ptu[dir], ptd[dir]);
      pt[dir] = temp.get_pt(0.5);
   }
   PR_ANY("\n");
}

double obj::find_extremity(direction_e dir) const {
   coord_t pt[4];
   line_iter ln[4];
   double ex[4];

   find_extremity(pt, ex, ln);
   return ex[dir];
}

void obj::find_extremity(coord_t pt[4]) const {
   line_iter ln[4];
   double ex[4];
   find_extremity(pt, ex, ln);
}

void obj::find_extremity(double val[4]) const {
   line_iter ln[4];
   coord_t pt[4];
   find_extremity(pt, val, ln);
}

bool obj::surrounds_point(coord_t pt) {
   line testLn = { pt, vector_t{LARGE, LARGE} };
   size_t crossings = 0;

   // Count the number of times a line from the point crosses this object
   for (line_iter ln = begin(); ln != end(); ++ln) {
      coord_t dummy = {};
      if (testLn.lines_intersect(*ln, &dummy, 0))
         ++crossings;
   }

   // Point is surrounded if we have an odd number of crossings
   return ((crossings & 1) == 1);
}
coord_t obj::find_extremity_pt(direction_e dir) const {
   coord_t pt[4];
   line_iter ln[4];
   double ex[4];

   find_extremity(pt, ex, ln);
   return pt[dir];
}

// Manipulation
void obj::split_along_line(
   const line ln,
   obj& left,
   obj& right,
   std::list<obj_line_intersect>* isects) {
   // Create a copy and rotate it so that the split-line would be horizontal
   obj ref = {};
   ref.copy_from(*this);
   ref.rotate(ln.get_S0(), -ln.angle());

   // Move it so that the split line would be at y = 0
   ref.add_offset(0.0, -ln.get_S0().y);

   // Now split it along the X axis
   line_iter elm = ref.begin();
   do {
      bool S0_left = (elm->get_S0().y >= 0.0);
      bool S1_left = (elm->get_S1().y >= 0.0);

      // Line is completely on the left
      if (S0_left && S1_left) {
         left.add(*elm);
         ++elm;
         continue;
      }

      // Line is completely on the right
      if (!S0_left && !S1_left) {
         right.add(*elm);
         ++elm;
         continue;
      }

      // Line is split; work out where it intersects and add to both halves
      double T = elm->T_for_y(0.0);
      if (isects) {
         obj_line_intersect intersect = { elm->get_pt(T).x, elm, elm->get_pt(T) };
         isects->push_back(intersect);
      }

      if (S0_left && !S1_left) {
         left.add(elm->get_S0(), elm->get_pt(T));
         right.add(elm->get_pt(T), elm->get_S1());
      }
      else {
         right.add(elm->get_S0(), elm->get_pt(T));
         left.add(elm->get_pt(T), elm->get_S1());
      }

      elm++;
   } while (elm != ref.end());

   // Reverse the offset and rotation for the left and right halves
   left.add_offset(0.0, ln.get_S0().y);
   right.add_offset(0.0, ln.get_S0().y);
   left.rotate(ln.get_S0(), +ln.angle());
   right.rotate(ln.get_S0(), +ln.angle());

   // And the same for the intersects
   if (isects) {
      isects->sort(intersect_sort);
      for (auto& is : *isects) {
         is.pt.y = is.pt.y + ln.get_S0().y;
         rotatePoint(&is.pt, ln.get_S0(), +ln.angle());
      }
   }
}

void obj::split_along_line_rejoin(
   const line ln,
   obj& left,
   obj& right,
   std::list<obj_line_intersect>* isects) {
   std::list<obj_line_intersect> localIsects = {};
   if (!isects)
      isects = &localIsects;

   split_along_line(ln, left, right, isects);

   // Rejoin the split ends
   // Assumes that the split ends should be joined in sequence
   // 0 to 1, 2 to 3 etc.
   for (int i = 1; i < (int)isects->size(); i = i + 2) {
      std::list<obj_line_intersect>::iterator is_a = isects->begin();
      std::list<obj_line_intersect>::iterator is_b = isects->begin();
      std::advance(is_a, i - 1);
      std::advance(is_b, i);
      left.add(is_a->pt, is_b->pt);
      right.add(is_a->pt, is_b->pt);
   }

   left.make_path();
   right.make_path();
}

bool obj::remove_extremity(double pos, direction_e dir, coord_t* ptu, coord_t* ptd, bool rejoin) {
   line ln = {};
   obj left = {};
   obj right = {};
   std::list<obj_line_intersect> isects = {};

   switch (dir) {
   case LEFT:
      ln.set(coord_t{ pos, 0.0 }, vector_t{ 0.0, -1.0 });
      break;
   case RIGHT:
      ln.set(coord_t{ pos, 0.0 }, vector_t{ 0.0, 1.0 });
      break;
   case UP:
      ln.set(coord_t{ 0.0, pos }, vector_t{ -1.0, 0.0 });
      break;
   case DOWN:
      ln.set(coord_t{ 0.0, pos }, vector_t{ 1.0, 0.0 });
      break;
   default:
      assert(0);
      break;
   }

   if (rejoin)
      split_along_line_rejoin(ln, left, right, &isects);
   else
      split_along_line(ln, left, right, &isects);

   if (!isects.empty()) {
      *ptd = isects.front().pt;
      *ptu = isects.back().pt;

      del();
      copy_from(left);
   }
   return (!isects.empty());
}

bool obj::remove_extremity_rejoin(double pos, direction_e dir, coord_t* ptu, coord_t* ptd) {
   return remove_extremity(pos, dir, ptu, ptd, true);
}

bool obj::remove_extremity(double pos, direction_e dir) {
   coord_t t1, t2;
   return remove_extremity(pos, dir, &t1, &t2);
}

bool obj::remove_extremity_rejoin(double pos, direction_e dir) {
   coord_t t1, t2;
   return remove_extremity(pos, dir, &t1, &t2, true);
}

void obj::add_offset(double xOffset, double yOffset) {
   for (line_iter ln = begin(); ln != end(); ++ln) {
      ln->add_offset(xOffset, yOffset);
   }
}

void obj::splice(line_iter pos_in_o, obj& o) {
   e.splice(end(), o.e, pos_in_o);
}

void obj::splice(obj& o) {
   e.splice(end(), o.e);
}

void obj::copy_from(obj& o) {
   obj dup = { o };
   e.splice(e.end(), dup.e);
}

void obj::rotate(coord_t pivot, double rads) {
   for (line_iter ln = begin(); ln != end(); ++ln) {
      ln->rotate(pivot, rads);
   }
}

void obj::mirror_x() {
   for (line_iter ln = begin(); ln != end(); ++ln) {
      ln->mirror_x();
   }
}

void obj::mirror_y() {
   for (line_iter ln = begin(); ln != end(); ++ln) {
      ln->mirror_y();
   }
}

void obj::make_path() {
   make_path(SNAP_LEN);
}

void obj::make_path(double snaplen) {
   std::list<obj> closed, open;
   make_path(snaplen, closed, open, false, false);
}

void obj::make_path(double snaplen, std::list<obj>& closed, std::list<obj>& open) {
   make_path(snaplen, closed, open, true, true);
}

// Try to close as many paths as possible in the object
// Return lists of objects for open or closed path
void obj::make_path(double snaplen, std::list<obj>& closed, std::list<obj>& open, bool listPaths, bool keepOpens) {
   PR_ANY("make_path: listPaths %u  keepOpens %u", listPaths, keepOpens);

   line_iter st = begin();
   line_iter en;
   while (st != end()) {
      // Get the next path, open or closed
      bool isClosed = trace_a_path(snaplen, st, en);

      PR_ANY(": isClosed %u  Size %zu  St %zu  En+1 %zu", isClosed, size(), index(st), index(en));

      // Delete open path
      if (!isClosed && !keepOpens) {
         line_iter delto = prev(en);
         del(st, delto);
      }
      else {
         // If it is closed, make it clockwise
         if (isClosed) {
            if (!is_clockwise(st, en)) {
               PR_ANY(": Reversing");
               for (line_iter ln = st; ln != en; ++ln)
                  ln->reverse();
               if (!trace_a_path(snaplen, st, en))
                  FATAL("Error: Unable to make a closed path after reversing elements");
            }
         }

         // Add to open or closed paths if needed
         if (listPaths) {
            obj dwg;
            for (line_iter ln = st; ln != en; ++ln)
               dwg.add(*ln);

            if (isClosed)
               closed.emplace_back(dwg);
            else
               open.emplace_back(dwg);
         }
      }
      st = en;
   }
   PR_ANY("\n");
}

bool obj::trace_a_path(double snaplen, line_iter& st, line_iter& en) {
   mpState_e state = MP_INIT;
   line_iter nx;

   while (1) {
      switch (state) {
      case MP_INIT:
         en = st;
         nx = st;
         // If empty object, return open path with both iterators -> end()
         state = (is_end(st)) ? MP_PATH_OPEN : MP_PROCESS_PATH;
         break;

      case MP_PROCESS_PATH:
         nx = next(nx);

         // If no more elements in object, path must be open
         if (is_end(nx)) {
            state = MP_PATH_OPEN;
            break;
         }

         // Is it connected to the end of the path but reversed?
         if (distTwoPoints(en->get_S1(), nx->get_S1()) <= snaplen) {
            nx->reverse();
         }

         // Is it correctly connected to the end of the path?
         if (distTwoPoints(en->get_S1(), nx->get_S0()) <= snaplen) {
            // Snap its S0 to the line end and splice it onto the path end
            nx->set(en->get_S1(), nx->get_S1());
            e.splice(next(en), e, nx);

            // Does it close the path?
            if (distTwoPoints(st->get_S0(), nx->get_S1()) <= snaplen) {
               // Snap its S1 to the line start and finish as closed
               nx->set(nx->get_S0(), st->get_S0());
               state = MP_PATH_CLOSED;
            }
            en = nx;
            break;
         }

         // Is it connected to the start of the path but reversed?
         if (distTwoPoints(st->get_S0(), nx->get_S0()) <= snaplen) {
            nx->reverse();
         }

         // Is it connected to the start of the path?
         if (distTwoPoints(st->get_S0(), nx->get_S1()) <= snaplen) {
            // Snap its S1 to the line start and move it to ahead of the path
            nx->set(nx->get_S0(), st->get_S0());
            e.splice(st, e, nx);

            // Update the start of the path then continue from the end
            st = nx;
            nx = en;
         }

         break;

      case MP_PATH_OPEN:
         en = next(en);
         return false;

      case MP_PATH_CLOSED:
         en = next(en);
         return true;

      default:
         assert(false);
      }
   }
}

bool obj::is_clockwise(line_iter st, line_iter en) const {
   double ang_acc = 0.0;
   int pos_neg = 0;
   line_iter prv = prev(en);

   // Acculmulate the relative angles between successive vectors
   for (line_iter ln = st; ln != en; ++ln) {
      if ((prv->len() > 0.0) && (ln->len() > 0.0)) {
         double ang = atan2(perpprod(prv->get_V(), ln->get_V()), dotprod(prv->get_V(), ln->get_V()));
         pos_neg = (ang >= 0.0) ? (pos_neg + 1) : (pos_neg - 1);
         ang_acc = ang_acc + ang;
         prv = ln;
      }
   }
   PR_ANY(": is_clockwise %.1lfrads  pos/neg %d", ang_acc, pos_neg);

   // The accumulation should be close to 360deg otherwise a local loop has been detected
   if (isEqualWithinPercentage(M_PI * 2.0, fabs(ang_acc), 5.0)) {
      return (ang_acc < 0.0);
   }
   else {
      PR_WARNING("\nWARNING: is_clockwise() angle is not a 2PI multiple (%.1lf) - using pos/neg counter instead (%d)\n",
         ang_acc,
         pos_neg);
      return (pos_neg < 0);
   }
}

void obj::regularise() {
   do {
      make_path();
   } while (del_dupes_zeros());
}

void obj::regularise_no_del() {
   std::list<obj> closed, open;
   make_path(SNAP_LEN, closed, open);
}

void obj::startAtDirection(direction_e dir) {
   make_path();

   // Find the intersects with lines at the vertical and horizontal centres
   double hpos = (find_extremity(UP) + find_extremity(DOWN)) / 2.0;
   double vpos = (find_extremity(LEFT) + find_extremity(RIGHT)) / 2.0;
   line hline(coord_t{ 0.0, hpos }, vector_t{ 1.0, 0.0 });
   line vline(coord_t{ vpos, 0.0 }, vector_t{ 0.0, 1.0 });
   std::list<obj_line_intersect> hisects, visects;
   if (!line_intersect(hline, &hisects, true))
      FATAL("Failed to find h-intersect");
   if (!line_intersect(vline, &visects, true))
      FATAL("Failed to find v-intersect");

   // Pick the relevant one
   std::list<obj_line_intersect>::iterator isect;
   switch (dir) {
   case LEFT:
      isect = hisects.begin();
      break;
   case RIGHT:
      isect = --hisects.end();
      break;
   case UP:
      isect = --visects.end();
      break;
   case DOWN:
      isect = visects.begin();
      break;
   default:
      FATAL("Unhandled direction");
      break;
   }

   // Build a new object
   obj dwg = {};
   dwg.add(isect->pt, isect->ln->get_S1());
   for (line_iter l = nextc(isect->ln); l != isect->ln; l = nextc(l))
      dwg.add(l->get_S0(), l->get_S1());
   dwg.add(isect->ln->get_S0(), isect->pt);

   // Replace me with the new object
   del();
   copy_from(dwg);
}

coord_t obj::find_avg_centre() const {
   line_iter ln = e.begin();
   coord_t c = { 0.0, 0.0 };
   size_t count = 0;

   while (ln != e.end()) {
      c.x = c.x + ln->get_pt(0.5).x;
      c.y = c.y + ln->get_pt(0.5).y;
      count++;
      ++ln;
   }
   if (count) {
      c.x = c.x / (double)count;
      c.y = c.y / (double)count;
   }
   return (c);
}

void obj::trace_at_offset(double ofs) {
   PR_ANY("\nTrace at offset %.1lf\n", ofs);

   // Ensure all my vectors are clockwise (around centre) and contiguous
   regularise();

   // Create the set of offsetting lines
   std::list<offset_line> osln;
   int cntOfsPts = 0, cntProxInv = 0, cntCrossing = 0, cntRadial = 0, cntRedundant = 0;
   for (line_iter ln = begin(); ln != end(); ++ln) {
      if (ln->len() < SNAP_LEN)
         continue;

      {
         // Linear offsetting lines
         line pl(*ln);
         pl.move_sideways(ofs);
         size_t nsteps = (size_t)ceil(ln->len() / TRACE_STEP_MM);
         nsteps = (nsteps < MIN_TRACE_STEPS) ? MIN_TRACE_STEPS : nsteps;
         for (size_t step = 0; step <= nsteps; step++) // Work along entire line length
         {
            // Find the offset point
            double T = (double)step / (double)nsteps;
            T = (T == 0.0) ? 0.000001 : T;
            T = (T == 1.0) ? 0.999999 : T;
            offset_line osl;
            osl.set(ln->get_pt(T), pl.get_pt(T));
            osl.valid = true;
            osl.radial = false;
            osl.src_index = index(ln);

            osln.emplace_back(osl);
            cntOfsPts++;
         }
      }

      {
         // Corner radial offsetting lines
         static constexpr double aStep = TO_RADS(3.0); // Angular steps at which to draw ofsetting lines

         // Find the line angles
         line_iter nx = nextc(ln);
         if (nx->len() == 0.0)
            continue;
         double a0 = ln->angle();
         double a1 = nx->angle();
         double ad = ln->angle(*nx);

         // Number of steps between the lines
         int nSteps = (int)std::floor(std::abs(ad) / aStep);

         // Decide if this is a convex corner and needs processing
         // If it does find the line angles at the correct perpendicular
         // We will need to rotate a set of lines from a0 to a1, the easiest
         // way to deal with wrapping of phase is to adjust in steps of 2Pi
         // to get the correct relationship.
         double aStepSigned;
         if ((ad > 0.0) && (ofs < 0.0)) {
            a0 -= M_PI_2;
            a1 -= M_PI_2;
            while (a0 < 0.0)
               a0 += M_PI * 2.0;
            while (a1 < a0)
               a1 += M_PI * 2.0;
            aStepSigned = aStep;
         }
         else if ((ad < 0.0) && (ofs > 0.0)) {
            a0 += M_PI_2;
            a1 += M_PI_2;
            while (a0 > 0.0)
               a0 -= M_PI * 2.0;
            while (a1 > a0)
               a1 -= M_PI * 2.0;
            aStepSigned = -aStep;
         }
         else
            nSteps = 0;

         if (0 && nSteps)
            DBGLVL2("nSteps %d a0 %.1lf a1 %.1lf ad %.1lf aStepSigned %.1lf", nSteps, TO_DEGS(a0), TO_DEGS(a1), TO_DEGS(ad), TO_DEGS(aStepSigned));

         for (int k = 1; k < nSteps; ++k) {
            offset_line osl;
            osl.set(ln->get_S1(), vector_t{ std::abs(ofs), 0.0 }); // Horizontal line of correct offsetting length
            osl.valid = true;
            osl.radial = true;
            osl.src_index = index(ln);
            osl.rotate(osl.get_S0(), a0 + (k * aStepSigned)); // Rotate to correct angle
            osl.set(osl.get_pt(0.001), osl.get_S1());         // Shorten at the origin end to prevent crossing
            osln.emplace_back(osl);                           // with the other radial lines
            cntRadial++;
         }
      }
   }

   // Invalidate lines whose endpoint is too close to the original object
   const double test = std::abs(ofs);
   for (offset_line_iter osl = osln.begin(); osl != osln.end(); ++osl) {
      for (line_iter ln = begin(); ln != end(); ++ln) {
         if (osl->src_index != index(ln)) { // Do not self compare
            if (ln->distance_to_point(osl->get_S1()) < test) {
               osl->valid = false;
               cntProxInv++;
               break;
            }
         }
      }
   }

   // Invalidate the offsetting lines that cross any other offsetting line
   for (offset_line_iter ref = osln.begin(); ref != osln.end(); ++ref) {
      if (std::next(ref) != osln.end()) {
         for (offset_line_iter cmp = std::next(ref); cmp != osln.end(); ++cmp) {
            if (cmp->valid && ref->valid) {
               coord_t dp;
               if (ref->lines_intersect(*cmp, &dp, 0)) {
                  ref->valid = false;
                  cntCrossing++;
                  break;
               }
            }
         }
      }
   }

   // Remove intermediate steps from the same source line
   for (offset_line_iter ref = osln.begin(); ref != osln.end(); ++ref) {
      offset_line_iter nxt = std::next(ref);
      if ((ref != osln.begin()) && (nxt != osln.end())) {
         offset_line_iter prv = std::prev(ref);
         if ((prv->valid && (prv->src_index == ref->src_index)) &&
            (nxt->valid && (nxt->src_index == ref->src_index)) &&
            (!prv->radial && !ref->radial && !nxt->radial)) {
            ref->valid = false;
            cntRedundant++;
         }
      }
   }

   DBGLVL2("Created %d linear offsetting lines, %d radial, %d proximity invalidated, %d crossing invalidated, %d redundant", cntOfsPts, cntRadial, cntProxInv, cntCrossing, cntRedundant);

   // Join the dots
   obj tr;
   for (offset_line_iter ref = osln.begin(); ref != osln.end(); ++ref) {
      if (ref->valid) {
         tr.add(ref->get_S1());
      }
   }

   // join start and finish off path
   tr.add(tr.get_sp(), tr.get_ep());
   tr.regularise();

   if (testFlag) { // Test code to draw the offsetting lines
      tr.del();
      for (offset_line_iter ref = osln.begin(); ref != osln.end(); ++ref) {
         if (ref->valid) {
            tr.add(line{ ref->get_pt(0.8), ref->get_S1() });
         }
      }
   }

   e = tr.e;
   PR_ANY(": Done\n");
}

void obj::scale_x_lr(double factor) {
   double left_x = find_extremity(LEFT);
   line_iter ln = e.begin();
   while (ln != e.end()) {
      coord_t new_S0 = { (ln->get_S0().x - left_x) * factor, ln->get_S0().y };
      coord_t new_S1 = { (ln->get_S1().x - left_x) * factor, ln->get_S1().y };
      ln->set(new_S0, new_S1);
      ++ln;
   }
}

void obj::scale(double factor) {
   for (line_iter ln = e.begin(); ln != e.end(); ++ln) {
      coord_t new_S0 = { ln->get_S0().x * factor, ln->get_S0().y * factor };
      coord_t new_S1 = { ln->get_S1().x * factor, ln->get_S1().y * factor };
      ln->set(new_S0, new_S1);
   }
}

void obj::move_extremity_to(direction_e dir, double pos) {
   if (!empty()) {
      double offset = pos - find_extremity(dir);
      if ((dir == UP) || (dir == DOWN)) {
         add_offset(0.0, offset);
      }
      else {
         add_offset(offset, 0.0);
      }
   }
}

void obj::move_origin_to(coord_t loc) {
   move_extremity_to(LEFT, loc.x);
   move_extremity_to(DOWN, loc.y);
}

void obj::make_gap(line_iter l0, coord_t p0, line_iter l1, coord_t p1, bool noNewLines) {
   // Delete any whole segments bridging the gap
   if (l0 != l1) {
      line_iter ln = nextc(l0);
      while (ln != l1) {
         line_iter nxt = nextc(ln);
         del(ln);
         ln = nxt;
      }
   }

   // Create the partial segments to pt0 and from pt1
   if (l0 == l1) {
      // pt0 and pt1 are in the same segment
      coord_t s1 = l0->get_S1();

      // Shorten segment to reach only pt0
      l0->set(l0->get_S0(), p0);

      if (noNewLines) {
         // Instead of inserting new segment, lengthen the next segment
         line_iter n = nextc(l0);
         n->set(p1, n->get_S1());
      }
      else {
         // Insert new segment from pt1
         l1 = add(line(p1, s1));
      }
   }
   else {
      // Shorten to reach only their respective points
      l0->set(l0->get_S0(), p0);
      l1->set(p1, l1->get_S1());
   }
}

bool obj::cut_slot(direction_e dir, double xpos, double width, double depth, slot_style_e style) {
   coord_t d0, d1;
   return cut_slot(dir, xpos, width, depth, style, &d0, &d1);
}

bool obj::cut_slot(direction_e dir, double xpos, double width, double depth, slot_style_e style, coord_t* pt0,
   coord_t* pt1) {
   regularise_no_del();

   // Find the reference centre point and centre line segment
   coord_t ptc;
   line_iter lnc;
   if (!dir_intersect(dir, xpos, &ptc, lnc))
      return false;

   // create a slot-width length vector in the direction of the slot bottom centred on refpc
   line slotref(ptc, lnc->get_V());
   if (style == VERTICAL) {
      slotref.set(slotref.get_S0(), vector_t{ slotref.get_V().dx, 0.0 }); // set reference to be horizontal for a vertical slot
   }
   slotref.set_length(width / 2.0);
   slotref.set(slotref.get_pt(-1.0), slotref.get_pt(1.0));

   line_iter l0, l1;

   // Find the corner intersect points and lines
   if (!dir_intersect(dir, slotref.get_S0().x, pt0, l0))
      return false;
   if (!dir_intersect(dir, slotref.get_S1().x, pt1, l1))
      return false;

   PR_ANY("cut_slot: PT0 %.1lf %.1lf   PT1 %.1lf %.1lf   LN0 %s   LN1 %s\n", pt0->x, pt0->y, pt1->x, pt1->y, l0->print_str(), l1->print_str());
   // Open a gap between pt0 and pt1
   make_gap(l0, *pt0, l1, *pt1);

   // Draw the slot
   line slot_bottom(slotref);
   slot_bottom.move_sideways(-depth);
   add(*pt0, slot_bottom.get_S0());
   add(slot_bottom.get_S1(), *pt1);
   add(slot_bottom);
   regularise_no_del();

   return true;
}

double obj::get_max_line_distance(line_iter st, line_iter en, const line& ln) const {
   double err = 0.0;
   line_iter term = next(en);
   for (line_iter seg = st; seg != term; ++seg) {
      double d0 = ln.distance_to_point(seg->get_S0());
      double d1 = ln.distance_to_point(seg->get_S1());
      err = (d0 > err) ? d0 : err;
      err = (d1 > err) ? d1 : err;
   }
   return err;
}

size_t obj::simplify() {
   return simplify(SIMPLIFY_ERR);
}

size_t obj::simplify(double error) {
   size_t start_size = size();
   line_iter st = begin();
   line_iter en = begin();

   while (!is_end(st)) {
      while (true) {
         line_iter ca = next(en);

         // Finished the object
         if (is_end(ca)) {
            break;
         }

         // Terminate if it isn't actually connected
         if (distTwoPoints(en->get_S1(), ca->get_S0()) > SNAP_LEN) {
            break;
         }

         // Create the simplified line and check its error to each point
         line sl(st->get_S0(), ca->get_S1());
         if (get_max_line_distance(st, ca, sl) >= error) {
            break;
         }

         // It passes - update end segment and carry on
         en = ca;
      }

      // Perform simplification
      en->set(st->get_S0(), en->get_S1());
      del(st, en);

      // Next iteration
      en = next(en);
      st = en;
   }

   return (size_t)(start_size - size());
}

void obj::extend1mm() {
   // Can't do anything to a closed path or empty path
   if (empty() || ((get_sp().x == get_ep().x) && (get_sp().y == get_ep().y)))
      return;

   // Move the start point 1mm
   line_iter st = begin();
   if (st->len() > 0.0) {
      double factor = -1.0 / st->len();
      st->set(st->get_pt(factor), st->get_S1());
   }

   // Same for the endpoint
   st = last();
   if (st->len() > 0.0) {
      double factor = 1.0 + (1.0 / st->len());
      st->set(st->get_S0(), st->get_pt(factor));
   }
}
