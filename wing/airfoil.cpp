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

#include <assert.h>
#include <cmath>

#include "airfoil.h"
#include "debug.h"
#include "hpgl.h"

Airfoil::Airfoil(obj& dwg) {
   vec = dwg;

   // Move drawing to left edge at 0.0 and find the choord
   vec.add_offset(-vec.find_extremity(LEFT), 0.0);
   double choord = vec.find_extremity(RIGHT) - vec.find_extremity(LEFT);
   double factor = (1.0 + (2.0 * chord_os)) / choord;

   // Create the normalised airfoil outline
   vec.scale(factor);
   vec.add_offset(-chord_os, 0.0);
}

double Airfoil::interp(double v0, double v1, double r) {
   return (v0 + (r * (v1 - v0)));
}

double Airfoil::interp(double v0, double v1, double x0, double x1, double xpos) {
   return interp(v0, v1, (xpos - x0) / (x1 - x0));
}

void Airfoil::get_norm_y(double c, double* ty, double* by) {
   if (vec.empty())
      dbg::fatal(SS("Cannot get airfoil point from empty vector"));

   // Limit c [0.0, 1.0]
   c = (c < 0.0) ? 0.0 : c;
   c = (c > 1.0) ? 1.0 : c;

   // Find points
   coord_t upper, lower;

   if (!vec.top_bot_intersect(c, &upper, &lower))
      dbg::fatal(SS("Unable to find top/bottom intersect at ratio ") + TS(c));

   *ty = upper.y;
   *by = lower.y;
}

double Airfoil::get_norm_t(double c) {
   double t, b;
   get_norm_y(c, &t, &b);
   return t;
}

double Airfoil::get_norm_b(double c) {
   double t, b;
   get_norm_y(c, &t, &b);
   return b;
}

double Airfoil::get_t(double xpos, double choord) {
   return (choord * get_norm_t(xpos / choord));
}

double Airfoil::get_b(double xpos, double choord) {
   return (choord * get_norm_b(xpos / choord));
}

Airfoil_ref::Airfoil_ref(obj& dwg, double xpos)
   : Airfoil(dwg),
   xpos{ xpos } {
}

double Airfoil_ref::get_X() {
   return xpos;
}

bool airfoil_ref_sort_left_right(const Airfoil_ref& a, const Airfoil_ref& b) {
   Airfoil_ref cpya = a;
   Airfoil_ref cpyb = b;
   return (cpya.get_X() < cpyb.get_X());
}

Airfoil_set::Airfoil_set() {};

bool Airfoil_set::add(GenericTab* T, std::string& log) {
   if (T->GetNumParts() < 2) {
      log.append("You need to declare at least two airfoils (one at the root, one at the tip)\n");
      return false;
   }

   for (int r = 0; r < T->GetNumParts(); r++) {
      QStringList xVals = T->Get(r, "AIRFOIL", airfoilXRole).toStringList();
      QStringList yVals = T->Get(r, "AIRFOIL", airfoilYRole).toStringList();
      std::vector<double> xs, ys;
      for (int e = 0; e < xVals.size(); e++) {
         xs.emplace_back(xVals.at(e).toDouble());
         ys.emplace_back(yVals.at(e).toDouble());
      }
      add_af_from_vectors(T->gdbl(r, "X"), xs, ys);
   }
   return true;
}

void Airfoil_set::draft_mode() {
   draw_x_steps = draw_x_steps_draft;
   draw_x_step = 1.0 / (double)(draw_x_steps - 1);
}

bool Airfoil_set::add_af_from_vectors(double xpos, const std::vector<double>& xs, const std::vector<double>& ys) {
   if (xs.empty() || (xs.size() != ys.size()))
      return false;

   obj imp;
   for (size_t idx = 0; idx < xs.size(); idx++)
      imp.add(coord_t{ xs.at(idx), ys.at(idx) });

   imp.del_zero_lens();
   airfoils.emplace_back(Airfoil_ref(imp, xpos));
   airfoils.sort(airfoil_ref_sort_left_right);
   return true;
}

bool Airfoil_set::add_from_dat_file(FILE** fp, double xpos, bool invert) {
   obj imp;
   char line[256];
   bool doneAirfoilName = false;
   bool doneLednicer = false;

   while (fgets(line, 256, *fp) != NULL) {
      coord_t pt;
      if (sscanf(line, "%lf %lf", &pt.x, &pt.y) == 2) {
         if ((pt.x > 1.01) && (pt.y > 1.01) && !doneLednicer) {
            // Both values > 1 suggest a Lednicer format file
            DBGLVL1("Lednicer point counts: Top: %lf Bottom: %lf", pt.x, pt.y);
            doneLednicer = true;
         }

         if ((pt.x > 1.01) || (pt.y > 1.01) || (pt.x < -1.01) || (pt.y < -1.01))
            dbg::fatal(SS("Unparsable values in line: ") + line);

         // Point looks OK
         pt.x = 1.0 - pt.x;
         pt.y = (invert) ? -pt.y : pt.y;
         imp.add(pt);
      }
      else {
         if (!doneAirfoilName) {
            DBGLVL1("Airfoil name from .dat file: %s", line);
            doneAirfoilName = true;
         }
         else {
            DBGLVL1("Unknown line in .dat file: %s", line);
         }
      }
   }

   imp.del_zero_lens();

   airfoils.emplace_back(Airfoil_ref(imp, xpos));
   airfoils.sort(airfoil_ref_sort_left_right);

   DBGLVL1("Imported line elements # %zu", imp.size());

   return true;
}

obj Airfoil_set::generate_airfoil(line planLine, double te_thck, double te_bl, obj& LE, obj& TE) const {
   if (airfoils.size() <= 1) {
      dbg::alert(SS("Need at least 2 airfoils defined, cannot generate rib"));
      return obj();
   }

   obj topln, botln;

   // Work along the planform line non-linearly
   for (size_t i = 0; i < draw_x_steps; i++) {
      // Get the x position in the wing that we are working at.  This uses a cosine transformation to concentrate the
      // points around the leading and trailing edges (similar to what Profili and the like seem to do).
      double c = 0.5 * (1 - cos(i * draw_x_step * M_PI));
      c = (c < 0.0) ? 0.0 : c;
      c = (c > 1.0) ? 1.0 : c;

      // xpos is the x position we are at in the wing
      // xpart is x position we are at along the part
      coord_t planPt = planLine.get_pt(c);
      double xpos = planPt.x;
      double xpart = distTwoPoints(planLine.get_S0(), planPt);

      // Find the wing choord at xpos
      coord_t le_pt, te_pt;
      line_iter dln;
      if (!LE.top_intersect(xpos, &le_pt, dln) || !TE.top_intersect(xpos, &te_pt, dln))
         dbg::fatal(SS("Failed to find LE/TE intersect at X position ") + TS(xpos));
      double choord = le_pt.y - te_pt.y;

      // Find the length ratio along the wing choord for the point we are interested in
      double wc = (planPt.y - te_pt.y) / choord;

      // Find the airfoil references that are in play and a position ratio between them
      std::list<Airfoil_ref>::iterator i0, i1;
      findEnclosingAirfoils(xpos, i0, i1);

      // Interpolate to find the top and bottom points of the airfoil at this location
      double top_y = i0->interp(i0->get_norm_t(wc), i1->get_norm_t(wc), i0->get_X(), i1->get_X(), xpos);
      double bot_y = i0->interp(i0->get_norm_b(wc), i1->get_norm_b(wc), i0->get_X(), i1->get_X(), xpos);
      top_y = top_y * choord;
      bot_y = bot_y * choord;

      // If we are in the trailing edge blend region, the adjust the top and bottom points
      if (te_thck != 0.0 && (wc < te_bl)) {
         // Find the top and bottom points of the airfoil at the trailing edge
         double topte = i0->interp(i0->get_norm_t(0.0), i1->get_norm_t(0.0), i0->get_X(), i1->get_X(), xpos);
         double botte = i0->interp(i0->get_norm_b(0.0), i1->get_norm_b(0.0), i0->get_X(), i1->get_X(), xpos); // Ha ha that spells botty
         double cente = (topte + botte) / 2.0;                                                                // Centre of the TE
         double haltk = te_thck / 2.0;                                                                        // Half the trailing edge thickness
         double ostp = cente + haltk - topte;                                                                 // Top offset for blending
         double osbt = cente - haltk - botte;                                                                 // Bottom offset for blending
         sqvar topos(te_bl, 0.0, 0.0, ostp, squareness);
         sqvar botos(te_bl, 0.0, 0.0, osbt, squareness);
         top_y += topos.vl(wc);
         bot_y += botos.vl(wc);
      }
      // Add to the airfoil lines
      topln.add(coord_t{ xpart, top_y });
      botln.add(coord_t{ xpart, bot_y });
   }

   topln.del_zero_lens();
   botln.del_zero_lens();

   // Combine into a drawing
   obj airf;
   airf.add(topln.get_sp(), botln.get_sp());
   airf.add(topln.get_ep(), botln.get_ep());
   airf.splice(topln);
   airf.splice(botln);
   airf.regularise();
   return airf;
}

void Airfoil_set::findEnclosingAirfoils(double xpos, std::list<Airfoil_ref>::iterator& i0,
   std::list<Airfoil_ref>::iterator& i1) const {
   std::list<Airfoil_ref>::iterator ix = airfoils.begin();
   i0 = airfoils.begin();
   i1 = airfoils.begin();

   if (xpos <= i0->get_X()) {
      // Out of range to left of wing
      i1 = i0;
      ++i1;
   }
   else if (xpos >= (--airfoils.end())->get_X()) {
      // Out of range to right of wing
      i1 = airfoils.end();
      --i1;
      i0 = i1;
      --i0;
   }
   else {
      while (ix != airfoils.end()) {
         if (ix->get_X() <= xpos) {
            // Can move up the leftmost and rightmost index
            i0 = ix;
            i1 = ix;
         }
         else if (ix->get_X() >= xpos) {
            i1 = ix;
            break;
         }
         ++ix;
      }
   }
}

void Airfoil_set::te_blend(obj& ob, const sqvar& os, double blend_to_x) const {
   for (auto ln = ob.begin(); ln != ob.end(); ln++) {
      coord_t s0 = ln->get_S0();
      coord_t s1 = ln->get_S1();

      if (s0.x < blend_to_x) {
         ln->set(coord_t{ s0.x, s0.y + os.vl(s0.x) }, s1);
         s0 = ln->get_S0();
      }
      if (s1.x < blend_to_x) {
         ln->set(s0, coord_t{ s1.x, s1.y + os.vl(s1.x) });
         s1 = ln->get_S1();
      }
   }
}
