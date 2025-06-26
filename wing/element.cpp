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

#include "element.h"
#include "ascii.h"
#include "debug.h"
#include "object_oo.h"
#include "part.h"
#include "rib.h"
#include "tabs.h"

bool Element::create(Rib_set& rbs, std::string& log, bool draftmode) {
   yLn.set(coord_t{ stX, stY }, coord_t{ enX, enY }); // Line of the part in (x,y)
   zLn.set(coord_t{ stX, stZ }, coord_t{ enX, enZ }); // Line of the part in (x,z)

   // Work through all ribs and process those that are intersected
   for (auto& rb : rbs) {
      if (!rb.affectsSpars)
         continue;

      // Find the element/rib intersection point
      coord_t planIs;
      if (yLn.lines_intersect(rb.objLn, &planIs, 0)) {
         DBGLVL2("   Element intersects rib %d at %s", rb.index, planIs.prstr());

         // Create a list of the roles that the element needs applying to, along with the Z types and a reference
         std::vector<obj*> applyToList;
         std::vector<obj*> refPartList;
         std::vector<enum z_e> applyToZ;

         if (shape == DOT) {
            // Dots needs to be applied to the rib and both sheeting jigs
            applyToList.push_back(&rb.getPart());
            refPartList.push_back(&rb.getRole(rb.rorgholes)); // Reference part needs to be the original rib outline
            applyToZ.push_back(SNAP_BOTTOM);

            applyToList.push_back(&rb.getPart());
            refPartList.push_back(&rb.getRole(rb.rorgholes));
            applyToZ.push_back(SNAP_TOP);

            if (!rb.getRole(Rib::topjig).empty()) {
               applyToList.push_back(&rb.getRole(Rib::topjig));
               refPartList.push_back(&rb.getRole(Rib::topjig));
               applyToZ.push_back(SNAP_BOTTOM);

               applyToList.push_back(&rb.getRole(Rib::topjig));
               refPartList.push_back(&rb.getRole(Rib::topjig));
               applyToZ.push_back(SNAP_TOP);
            }

            if (!rb.getRole(Rib::botjig).empty()) {
               applyToList.push_back(&rb.getRole(Rib::botjig));
               refPartList.push_back(&rb.getRole(Rib::botjig));
               applyToZ.push_back(SNAP_BOTTOM);

               applyToList.push_back(&rb.getRole(Rib::botjig));
               refPartList.push_back(&rb.getRole(Rib::botjig));
               applyToZ.push_back(SNAP_TOP);
            }
         }
         else {
            // Tubes and bars get applied to the rib part
            applyToList.push_back(&rb.getPart());
            refPartList.push_back(&rb.getRole(rb.rorgholes)); // Reference part needs to be the original rib outline
            applyToZ.push_back(ztype);
         }

         for (size_t i = 0; i < applyToList.size(); ++i) {
            obj* prt = applyToList[i];
            obj* ref = refPartList[i];
            z_e zt = applyToZ[i];

            // Find the element position in the rib
            double xpos = rb.plnfmIntersectToXpos(yLn);
            double ypos = ((distTwoPoints(yLn.get_S0(), planIs) / yLn.len()) * (enZ - stZ)) + stZ;

            // Find the width and height of the necessary opening
            double apHw = 0.0, apHd = 0.0, angRad = 0.0;
            switch (shape) {
            case TUBE:
               // Diameter is the width and height to start with
               apHw = widenSlots ? (slotWidth(yLn, rb.objLn, diameter, rb.rib_thck) / 2.0) : (diameter / 2.0);
               apHd =
                  widenSlots ? (slotWidth(zLn, line(coord_t{ 0, 0 }, vector_t{ 0, 1 }), diameter, rb.rib_thck) / 2.0) : (diameter / 2.0);
               angRad = 0.0;
               width = diameter;
               break;
            case BAR:
               apHw = widenSlots ? (slotWidth(yLn, rb.objLn, width, rb.rib_thck) / 2.0) : (width / 2.0);
               apHd =
                  widenSlots ? (slotWidth(zLn, line(coord_t{ 0, 0 }, vector_t{ 0, 1 }), depth, rb.rib_thck) / 2.0) : (depth / 2.0);
               angRad = TO_RADS(angle);
               break;
            case DOT:
               apHw = (width / 2.0);
               apHd = (depth / 2.0);
               angRad = 0.0;
               break;
            case NONE:
            default:
               dbg::fatal(SS("Unknown shape in ") + SS(__func__));
            }

            // Update ypos and angle based on ztype
            if (zt != CHOORD_L) {
               coord_t upper = { 0, 0 }, lower = { 0, 0 };
               line_iter upperln, lowerln;

               if (!ref->top_bot_intersect(xpos, &upper, &lower, upperln, lowerln))
                  dbg::fatal(SS("Failed to find intersect to snap to rib outline"),
                     SS("Rib ") + TS(rb.index) + " element " + TS(index));
               switch (zt) {
               case SNAP_TOP:
                  ypos = (upper.y - ypos - apHd);
                  break;
               case ROTATE_TOP:
                  ypos = (upper.y - ypos - apHd);
                  angRad = upperln->angle();
                  break;
               case SNAP_BOTTOM:
                  ypos = (lower.y + ypos + apHd);
                  break;
               case ROTATE_BOTTOM:
                  ypos = (lower.y + ypos + apHd);
                  angRad = lowerln->angle();
                  break;
               default:
                  dbg::fatal(SS("Unknown ztype ") + SS(__func__));
               }
            }

            // Draw the shape
            DBGLVL2("   (x,y)=(%lf,,%lf) width=%lf depth=%lf", xpos, ypos, apHw, apHd);

            getPart().del();
            switch (shape) {
            case TUBE:
               getPart().add_ellipse(coord_t{ xpos, ypos }, apHw, apHd);
               break;
            case BAR:
            case DOT: //ft
               getPart().add_rect(coord_t{ (xpos - apHw), (ypos + apHd) }, coord_t{ (xpos + apHw), (ypos - apHd) });
               getPart().rotate(coord_t{ xpos, ypos }, angRad);
               break;
            case NONE:
            default:
               dbg::fatal(SS("Section 2: Unknown shape in ") + SS(__func__));
            }

            draftmode ? getPart().simplify(0.1) : getPart().simplify();

            // Check if there is room in the rib for the element
            if (prt->obj_intersect(getPart()))
               log.append(
                  "Element " + TS(index) + " crosses the part outline of rib " + TS(rb.index) + ", please check this is what you wanted\n");

            // Apply to the rib
            prt->copy_from(getPart());
            if (shape != DOT)
               rb.autoKpos.push_back(Keepout{ xpos - apHw, xpos + apHw });
         }
      }
   }

   return true;
}

obj& Element::getPlan() {
   obj& plan = Part::getPlan();

   // Clear the plan
   plan.del();

   if (yLn.len() > 0.1) {
      char str[20];
      asciivec tp(4.0);
      coord_t txt = { 0, 0 };

      switch (shape) {
      case TUBE:
         snprintf(str, 20, "TUBE %u", index);
         txt =
         { yLn.get_S0().x - 30, yLn.get_S0().y - 2.0 };
         tp.add(plan, txt, str);
         plan.add_rect(yLn, width);
         break;
      case BAR:
         snprintf(str, 20, "BAR %u", index);
         txt =
         { yLn.get_S0().x - 25, yLn.get_S0().y - 2.0 };
         tp.add(plan, txt, str);
         plan.add_rect(yLn, width);
         break;
      case DOT:
         snprintf(str, 20, "DOT %u", index);
         txt =
         { yLn.get_S0().x - 25, yLn.get_S0().y - 2.0 };
         tp.add(plan, txt, str);
         plan.add_dotted(yLn, 0.5, 0.5);
         break;
      default:
         dbg::fatal(SS("Unknown shape type ") + SS(__func__));
         break;
      }
   }

   return plan;
}

bool Element_set::add(GenericTab* T, std::string& log) {
   (void)log;

   for (int r = 0; r < T->GetNumParts(); r++) {
      Element s = {};
      DBGLVL1("Processing row %d of %s", r, T->GetKey().c_str());

      if (T->gqst(r, "meta") == QString("Rectangular Element")) {
         s.shape = Element::shape_e::BAR;
         s.typeTxt.append("RECT BAR");
      }
      else if (T->gqst(r, "meta") == QString("Tubular Element")) {
         s.shape = Element::shape_e::TUBE;
         s.typeTxt.append("TUBE");
      }
      else if (T->gqst(r, "meta") == QString("Alignment Dots")) {
         s.shape = Element::shape_e::DOT;
         s.typeTxt.append("CENTRE DOT");
      }
      else
         dbg::fatal(SS("Unrecognised element type - something has gone awry"),
            SS("Type = ") + T->gqst(r, "meta").toStdString());
      s.diameter = T->gdbl(r, "DIAMETER");
      s.width = T->gdbl(r, "WIDTH");
      s.depth = T->gdbl(r, "DEPTH");
      s.angle = T->gdbl(r, "ANGLE");
      s.stX = T->gdbl(r, "STX");
      s.stY = T->gdbl(r, "STY");
      s.stZ = T->gdbl(r, "STZ");
      s.enX = T->gdbl(r, "ENX");
      s.enY = T->gdbl(r, "ENY");
      s.enZ = T->gdbl(r, "ENZ");
      s.index = r + 1;
      s.widenSlots = (T->gqst(r, "WIDENSLOTS") == QString("Yes")) ? true : false;
      if (T->gqst(r, "ZMODE") == QString("Choord")) {
         s.ztype = Element::z_e::CHOORD_L;
      }
      else if (T->gqst(r, "ZMODE") == QString("Snap-Bottom")) {
         s.ztype = Element::z_e::SNAP_BOTTOM;
      }
      else if (T->gqst(r, "ZMODE") == QString("Snap-Top")) {
         s.ztype = Element::z_e::SNAP_TOP;
      }
      else if (T->gqst(r, "ZMODE") == QString("Rotate-Bottom")) {
         s.ztype = Element::z_e::ROTATE_BOTTOM;
      }
      else if (T->gqst(r, "ZMODE") == QString("Rotate-Top")) {
         s.ztype = Element::z_e::ROTATE_TOP;
      }
      else
         dbg::fatal(SS("Unrecognised Z type - something has gone awry"),
            SS("Mode = ") + T->gqst(r, "ZMODE").toStdString());

      // Overrides for alignment dots
      if (s.shape == Element::shape_e::DOT) {
         s.depth = T->gdbl(r, "DOTSIZE");
         s.width = T->gdbl(r, "DOTSIZE");
         s.stZ = T->gdbl(r, "DOTINSET");
         s.enZ = T->gdbl(r, "DOTINSET");
      }

      elms.push_back(s);
   }

   return true;
}

bool Element_set::create(Rib_set& rbs, std::string& log) {
   bool retbool = true;
   for (auto& el : elms) {
      DBGLVL1("Creating element %d", el.index);
      if (!el.create(rbs, log, draft))
         retbool = false;
   }

   return retbool;
}

obj& Element_set::getPlan() {
   plan.del();
   for (auto& it : elms)
      plan.copy_from(it.getPlan());

   return plan;
}
