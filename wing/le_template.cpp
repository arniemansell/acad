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

#include "le_template.h"
#include "airfoil.h"
#include "ascii.h"
#include "object_oo.h"
#include "part.h"
#include "planform.h"

// Leading Edge Templates
bool LeTemplate::create(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log) {
   obj& np = getPart();
   np.del();

   obj airf = { af.generate_airfoil(airfLn, 0, 0, pl.getRole(Planform::LE), pl.getRole(Planform::TE)) };
   airf.regularise();
   airf.trace_at_offset(0.2);
   draftMode ? airf.simplify(0.1) : airf.simplify();

   np.copy_from(airf);

   // Truncate to fraction of choord
   coord_t upt, lpt;
   double depth = airfLn.len() * (1.0 - LE_TEMPLATE_DEPTH);
   if (!np.top_bot_intersect(depth, &upt, &lpt)) {
      log.append(SS("Unable to find rib outline at x=") + TS(depth) + " for LE template " + TS(index) + "\n");
      return true;
   }
   np.remove_extremity(depth, LEFT);

   // Draw the rest of the template
   np.add(upt, coord_t{ depth - 3.0, upt.y + 3.0 });
   np.add(coord_t{ depth - 3.0, upt.y + 10.0 });
   np.add(coord_t{ airfLn.len() + 15.0, upt.y + 10.0 });
   np.add(coord_t{ airfLn.len() + 20.0, upt.y + 5.0 });
   np.add(coord_t{ airfLn.len() + 20.0, lpt.y - 10.0 });
   np.add(coord_t{ depth - 3.0, lpt.y - 10.0 });
   np.add(coord_t{ depth - 3.0, lpt.y - 3.0 });
   np.add(lpt);
   np.regularise();

   createPartText();
   return true;
}

bool LeTemplate_set::add(GenericTab* T, Planform& plnf, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      LeTemplate s = {};
      DBGLVL1("Processing row %d of %s", r, T->GetKey().c_str());

      s.index = r + 1;
      s.xpos = T->gdbl(r, "LEX");
      s.notes = T->gqst(r, "NOTES").toStdString();
      s.typeTxt.append("LE TEMPLATE");

      // Find the planform positions of its leading and trailing edge and draw the airfoil line
      s.airfLn = plnf.get_airfoil_line(s.xpos, s.xpos);
      if (s.airfLn.len() < 1.0) {
         log.append(SS("LE Template ") + TS(s.index) + " does not intersect the LE\n");
         continue;
      }

      lets.push_back(s);

      DBGLVL2("Processed row %d", r);
   }

   return true;
}

bool LeTemplate_set::create(Planform& pl, Airfoil_set& af, std::string& log) {
   for (auto& let : lets) {
      DBGLVL1("Creating LE Template: %d", let.index);
      if (!let.create(pl, af, draftMode, log))
         return false;
   }

   return true;
}

void LeTemplate_set::getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts) {
   for (auto& r : lets) {
      obj& p = r.getPrettyPart();
      if (!p.empty()) {
         objects.push_back(p);
         texts.push_back(r.getPartText());
      }
   }
}
