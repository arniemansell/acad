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
#include "planform.h"
#include "bezier.h"
#include "part.h"

Planform::Planform()
{
   (void)addRole(LE);
   (void)addRole(TE);
   (void)addRole(BOX);
}

bool Planform::add(GenericTab* T, std::string& log)
{
   DBGLVL2("Number planform parts %d", T->GetNumParts());
   for (int r = 0; r < T->GetNumParts(); r++) {
      DBGLVL1("Processing row %d of %s", r, T->GetKey().c_str());

      const bool leNotTe = (T->gqst(r, "LEORTE") == QString("LE"));
      if (!leNotTe && (T->gqst(r, "LEORTE") != QString("TE")))
         dbg::fatal(std::string("Unrecognised planform point type ") + __FILE__);

      if (T->gqst(r, "meta") == QString("Planform Point")) {
         if (T->gqst(r, "LEORTE") == QString("LE"))
            addLePoint(T->gdbl(r, "X"), T->gdbl(r, "Y"));

         else if (T->gqst(r, "LEORTE") == QString("TE"))
            addTePoint(T->gdbl(r, "X"), T->gdbl(r, "Y"));

         else
            dbg::fatal(std::string("Unrecognised planform point type ") + __FILE__);
      }

      else if (T->gqst(r, "meta") == QString("Planform X/Y File")) {
         QStringList xVals = T->Get(r, "XYFILE", planformXRole).toStringList();
         QStringList yVals = T->Get(r, "XYFILE", planformYRole).toStringList();
         for (int e = 0; e < xVals.size(); e++) {
            if (leNotTe)
               addLePoint(xVals.at(e).toDouble(), yVals.at(e).toDouble());
            else
               addTePoint(xVals.at(e).toDouble(), yVals.at(e).toDouble());
         }
      }

      else if (T->gqst(r, "meta") == QString("Cubic Bezier")) {
         CubicBezier c = { coord_t{T->gdbl(r, "P1X"), T->gdbl(r, "P1Y")},
                          coord_t{T->gdbl(r, "P2X"), T->gdbl(r, "P2Y")},
                          coord_t{T->gdbl(r, "P3X"), T->gdbl(r, "P3Y")},
                          coord_t{T->gdbl(r, "P4X"), T->gdbl(r, "P4Y")} };
         obj co = c.curve(0.0, 1.0, 1.0 / (T->gint(r, "NPTS") - 1));
         if (leNotTe)
            getRole(LE).copy_from(co);
         else
            getRole(TE).copy_from(co);
      }
   }

   // Sanity checking
   if ((getRole(LE).size() == 0) || (getRole(LE).len() < 1.0)) {
      log.append("You have not defined a leading edge for the wing in the "
         "planform tab.\n");
      return false;
   }
   if ((getRole(TE).size() == 0) || (getRole(TE).len() < 1.0)) {
      log.append("You have not defined a trailing edge for the wing in the "
         "planform tab.\n");
      return false;
   }

   // Tidy up
   getRole(LE).del_zero_lens();
   getRole(LE).extend1mm();
   getRole(LE).extend1mm();
   getRole(TE).del_zero_lens();
   getRole(TE).extend1mm();
   getRole(TE).extend1mm();

   // Create the planform "Box"
   getRole(BOX).del();
   getRole(BOX).copy_from(getRole(LE));
   getRole(BOX).copy_from(getRole(TE));
   getRole(BOX).add(getRole(LE).get_sp(), getRole(TE).get_sp());
   getRole(BOX).add(getRole(LE).get_ep(), getRole(TE).get_ep());
   getRole(BOX).regularise();

   isDefined = true;
   return true;
}

void Planform::addLePoint(double x, double y)
{
   getRole(LE).add(coord_t{ x, y });
}

void Planform::addTePoint(double x, double y)
{
   getRole(TE).add(coord_t{ x, y });
}

obj& Planform::getPlan()
{
   Part::getPlan().del();
   obj le(getRole(LE));
   obj te(getRole(TE));
   Part::getPlan().splice(le);
   Part::getPlan().splice(te);
   return Part::getPlan();
}

line Planform::get_airfoil_line(double leX, double teX)
{
   coord_t le_pt, te_pt;
   line_iter dln;
   if ((!getRole(LE).top_intersect(leX, &le_pt, dln))
      || (!getRole(TE).top_intersect(teX, &te_pt, dln)))
      return line(coord_t{ 0, 0 }, vector_t{ 0, 0 });

   return line(te_pt, le_pt);
}

bool Planform::isInPlanform(coord_t pt)
{
   if (getRole(BOX).size())
      return getRole(BOX).surrounds_point(pt);

   return false;
}

bool Planform::isInPlanform(const line& ln)
{
   if (getRole(BOX).size()) {
      if (!getRole(BOX).surrounds_point(ln.get_S0()))
         return false;
      if (!getRole(BOX).surrounds_point(ln.get_S1()))
         return false;
      return true;
   }

   return false;
}
