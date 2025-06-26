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

#include <vector>

#include "object_oo.h"
#include "tabs.h"

class Airfoil {
private:
   const double chord_os = 1e-6; //!<Oversize by this amount in order to prevent intersect misses at 0.0 and 1.0
   obj vec = {};                 //!<The normalised airfoil

public:
   explicit Airfoil(obj& dwg); //!<Parse dwg into an airfoil - choord line must be at y = 0

   void get_norm_y(double c, double* ty, double* by);                      //!<Return normalised bottom y and top y
   double get_norm_t(double c);                                            //!<Return normalised top line value
   double get_norm_b(double c);                                            //!<Return normalised bot line value
   double get_t(double xpos, double choord);                               //!<Return top line value for an x position along a choord length
   double get_b(double xpos, double choord);                               //!<Return bot line value for an x position along a choord length
   double interp(double v0, double v1, double r);                          //!<Between v0 and v1 by ratio r[0.0, 1.0]
   double interp(double v0, double v1, double x0, double x1, double xpos); //!<Between (x0, v0) and (x1, v1) at xpos
};

class Airfoil_ref : public Airfoil {
private:
   double xpos = 0.0; //!<x position of airfoil in planform

public:
   Airfoil_ref(obj& dwg, double xpos);
   double get_X();
};

bool airfoil_ref_sort_left_right(const Airfoil_ref& a, const Airfoil_ref& b);

class Airfoil_set {
private:
   const double centre_marker_size = 1.0;         //!<Side length of a centre marker square
   const double choord_line_max_angle_deg = 0.05; //!<Maximum allowed angle in the estimated choord line when importing airfoils
   const double squareness = 0.75;                //!<Blend 75% square law
   const size_t draw_x_steps_default = 200;       //!<Number of possible steps when drawing a new airfoil
   const size_t draw_x_steps_draft = 75;          //!<As above but when running in draft mode
   size_t draw_x_steps = draw_x_steps_default;
   double draw_x_step = 1.0 / (double)(draw_x_steps - 1); //!<Size of each linear step assuming total range [0.0, 1.0]
   mutable std::list<Airfoil_ref> airfoils = {};          //!<Our set of airfoil references

   void te_blend(obj& ob, const sqvar& os, double blend_to_x) const; //!<Apply trailing edge blend

public:
   Airfoil_set();
   void draft_mode(); //!<Change internals to draw ribs in a rough draft mode
   bool add(GenericTab* T, std::string& log);
   bool add_from_dat_file(FILE** fp, double xpos, bool invert); //!<Import from a standard .dat representation
   bool add_af_from_vectors(double xpos, const std::vector<double>& xs, const std::vector<double>& ys);
   obj generate_airfoil(line planLine, double te_thck, double te_bl, obj& LE, obj& TE) const; //!<Generate the airfoil that matches the planform line
   void findEnclosingAirfoils(double x, std::list<Airfoil_ref>::iterator& i0,
      std::list<Airfoil_ref>::iterator& i1) const; //!< Find the two airfoils that x is between
};
