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

#include "rib.h"
#include "airfoil.h"
#include "ascii.h"
#include "object_oo.h"
#include "part.h"
#include "planform.h"
#include "spar.h"

bool Rib::addKeepout(
   double stX,
   double stY,
   double enX,
   double enY,
   double width) {
   line kpln(coord_t{ stX, stY }, coord_t{ enX, enY });

   coord_t planIs = { 0, 0 };
   if (kpln.lines_intersect(refLn, &planIs, 0)) {
      double xpos = distTwoPoints(refLn.get_S0(), planIs);
      double left = xpos - (width / 2.0);
      double right = xpos + (width / 2.0);
      userKpos.push_back(Keepout{ left, right });
      DBGLVL1("Added keepout to rib %d: left=%lf right=%lf", index, left, right);
      return true;
   }
   return false;
}

void Rib::createRibPart(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log) {
   // Draw the reference airfoil and choord line, with and without trailing edge thickness blends
   obj& aorgo = addRole(aorg);
   aorgo = af.generate_airfoil(refLn, te_thck, te_blend, pl.getRole(Planform::LE), pl.getRole(Planform::TE));
   aorgo.regularise();
   achd.set(coord_t{ 0.0, 0.0 }, coord_t{ aorgo.find_extremity(RIGHT), 0.0 });

   obj& anoto = addRole(anot);
   anoto = af.generate_airfoil(refLn, 0.0, te_blend, pl.getRole(Planform::LE), pl.getRole(Planform::TE));

   DBGLVL2("aorg and anot drawn OK");

   // Process in stages to generate rib and jig references
   obj& apcdo = addRole(apcd);
   apcdo.copy_from(aorgo);

   if (washout != 0.0) {
      // Find the pivot point
      switch (wo_pivot) {
      case LE:
         wo_pivot_pt = achd.get_S1();
         break;
      case TE:
         wo_pivot_pt = achd.get_S0();
         break;
      case CENTRE:
         wo_pivot_pt = line(achd.get_S0(), achd.get_S1()).get_pt(0.5);
         break;
      default:
         assert(0);
         break;
      }
      // Rotate
      apcdo.rotate(wo_pivot_pt, TO_RADS(-washout));

      DBGLVL1("Washout applied: %lf", washout);
   }

   obj& rjigo = addRole(rjig);
   rjigo.copy_from(apcdo);
   obj& rorgo = getRawPart();
   rorgo.copy_from(apcdo);
   DBGLVL2("rjig and rorg drawn OK");

   if (w_sh_thck != 0.0) {
      rorgo.trace_at_offset(-w_sh_thck);
      draftMode ? rorgo.simplify(0.1) : rorgo.simplify();
      DBGLVL2("Wing sheeting applied: %.2lf", w_sh_thck);
   }

   obj& rpcdo = getPart();
   rpcdo.copy_from(rorgo);

   obj& rorgholeso = addRole(rorgholes);
   rorgholeso.copy_from(rorgo);

   // Truncate the rib & jig reference based on defined LE and TE
   if (teW != 0.0) {
      double pos = apcdo.find_extremity(LEFT) + teW;
      autoKpos.push_back(Keepout{ 0.0, pos });

      if (!rpcdo.remove_extremity_rejoin(pos, LEFT))
         log.append(
            SS("Rib ") + TS(index) + "Error applying TE when processing rib.rpcd for TE cut at x=" + TS(pos) + ", remove_extremity_rejoin failed. This is usually because you have not set the trailing edge thickness large enough to allow for the wing skin thickness.\n");
      if (!rjigo.remove_extremity_rejoin(pos, LEFT))
         log.append(
            SS("Rib ") + TS(index) + "Error applying TE when processing rib.rjig for TE cut at x=" + TS(pos) + ", remove_extremity_rejoin failed. This is usually because you have not set the trailing edge thickness large enough to allow for the wing skin thickness.\n");

      rpcdo.regularise();
      rjigo.regularise();
      DBGLVL2("TE position applied: %.2lf", pos);
   }
   if (leW != 0.0) {
      double pos = apcdo.find_extremity(RIGHT) - leW;
      autoKpos.push_back(Keepout{ pos, pos + leW });

      if (!rpcdo.remove_extremity_rejoin(pos, RIGHT))
         log.append(
            SS("Rib ") + TS(index) + "Error applying LE when processing rib.rpcd for LE cut at x=" + TS(pos) + ", remove_extremity_rejoin failed. This is usually because you have not set the leading edge thickness large enough to allow for the wing skin thickness.\n");
      if (!rjigo.remove_extremity_rejoin(pos, RIGHT))
         log.append(
            SS("Rib ") + TS(index) + "Error applying LE when processing rib.rjig for LE cut at x=" + TS(pos) + ", remove_extremity_rejoin failed. This is usually because you have not set the leading edge thickness large enough to allow for the wing skin thickness.\n");

      rpcdo.regularise();
      rjigo.regularise();
      DBGLVL2("LE position applied: %.2lf", pos);
   }

   // Draw the 2D planform line of the rib
   redrawObjLine();
}

void Rib::createRibPlan(std::string& log) {
   (void)log;

   obj& plan = Part::getPlan();

   // Clear the plan
   plan.del();

   // Add the rib
   plan.add_rect(objLn, rib_thck);

   // Add manually added keepouts
   for (auto kp = userKpos.begin(); kp != userKpos.end(); ++kp) {
      double T = xposToAirfoilT(kp->l);
      coord_t plpt = refLn.get_pt(T);
      plan.add(coord_t{ plpt.x - rib_thck, plpt.y }, coord_t{ plpt.x + rib_thck, plpt.y });
      T = xposToAirfoilT(kp->r);
      plpt = refLn.get_pt(T);
      plan.add(coord_t{ plpt.x - rib_thck, plpt.y }, coord_t{ plpt.x + rib_thck, plpt.y });
   }

   char str[10];
   asciivec tp(4.0);
   snprintf(str, 10, "%d", index);
   coord_t txt = { objLn.get_S0().x + rib_thck + 1.0, objLn.get_S0().y - 5.0 };
   if (index & 1)
      txt.y = txt.y - 8;
   tp.add(plan, txt, str);
}

void Rib::createRibText(std::string& log) {
   (void)log;
   createPartText();
}

// Create the basic 2D drawings of ribs from their definitions
bool Rib::createRib(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log) {
   createRibPart(pl, af, draftMode, log);
   createRibPlan(log);
   createRibText(log);

   DBGLVL2("Rib complete");
   return true;
}

obj& Rib::getPlan() {
   return Part::getRole(Part::PLFM);
}

double Rib::xposToAirfoilT(double xpos) {
   return xpos / refLn.len();
}

double Rib::plnfmIntersectToXpos(line ln) {
   coord_t is = { 0, 0 };
   if (!refLn.lines_intersect(ln, &is, 1))
      dbg::fatal(SS("Failed to find intersect ") + SS(__func__),
         "This function is specifically called with intersecting rib and line.");
   double offset = distTwoPoints(is, refLn.get_S0());
   if (is.y < refLn.get_S0().y)
      offset = -offset;
   return offset;
}

bool Rib_set::add(GenericTab* T, Planform& plnf, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      Rib nrib;
      DBGLVL1("Processing row %d of %s (%s)", r + 1, T->GetKey().c_str(), T->gqst(r, "meta").toStdString().c_str());

      // Parameters common to spacer, rib and rib doubler
      nrib.rib_thck = T->gdbl(r, "THK");
      nrib.index = T->gint(r, "IDX");
      nrib.leW = T->gdbl(r, "LE");
      nrib.teW = T->gdbl(r, "TE");
      nrib.w_sh_thck = T->gdbl(r, "SHTTHK");
      nrib.lhbw = (T->gqst(r, "LH") == QString("Yes")) ? T->gdbl(r, "LHBW") : 0.0;
      nrib.mhl = T->gdbl(r, "MHL");
      nrib.jig = (T->gqst(r, "SHTJIG") == QString("Yes")) ? true : false;
      nrib.splitAtChoord = (T->gqst(r, "SAC") == QString("Yes")) ? true : false;
      nrib.notes = T->gqst(r, "NOTES").toStdString();
      nrib.affectsSpars = true;

      // Find the planform positions of its leading and trailing edge and draw the airfoil line
      nrib.refLn = plnf.get_airfoil_line(T->gdbl(r, "LEX"), T->gdbl(r, "TEX"));
      if (nrib.refLn.len() < 1.0) {
         log.append(SS("Rib ") + TS(nrib.index) + " does not intersect the LE/TE\n");
         continue;
      }

      // Type specific behaviour
      // RIB
      if (T->gqst(r, "meta") == QString("Rib")) {
         DBGLVL2("Adding a rib r=%d", r);
         nrib.typeTxt.append("RIB");

         ribs.push_back(nrib);
      }

      // DOUBLER
      else if (T->gqst(r, "meta") == QString("Doubler")) {
         DBGLVL2("Adding a rib doubler r=%d", r);
         nrib.typeTxt.append("DOUBLER");

         // Find the rib which is to be doubled
         int linkidx = T->gint(r, "LINK");
         auto ref = ribs.begin();
         while (ref != ribs.end()) {
            if (ref->index == linkidx)
               break;
            ++ref;
         }
         if (ref == ribs.end())
            dbg::fatal(SS("Adding rib doubler ") + TS(r) + " can't find linked rib index " + TS(linkidx));

         // Find the X positions of the doubler by moving the line of the reference sideways
         double offset = (ref->rib_thck + nrib.rib_thck) / 2.0;
         bool tipSide = (T->gqst(r, "LEFTORRIGHT") == QString("Right"));
         offset = (tipSide) ? -offset : offset;
         line doubler = ref->refLn;
         doubler.move_sideways(offset);

         nrib.refLn = plnf.get_airfoil_line(doubler.get_S1().x, doubler.get_S0().x);
         if (nrib.refLn.len() < 1.0) {
            log.append(SS("Rib doubler ") + TS(nrib.index) + " does not intersect the LE/TE\n");
            continue;
         }

         nrib.lhbw = ref->lhbw;
         nrib.mhl = ref->mhl;
         nrib.w_sh_thck = ref->w_sh_thck;
         nrib.jig = false; // Doublers have no jig by design

         // Add keepouts to the reference and the doubler based on the "allow holes" setting
         double tepos = nrib.teW;
         double lepos = nrib.refLn.len() - nrib.leW;
         if (T->gqst(r, "LH") == QString("Yes")) {
            // Place two keepouts in the rib and doubler, one at each end of where the doubler falls
            // (to provide a gluing surface if lightening holes are created in both)
            ref->autoKpos.push_back(Keepout(tepos, tepos + 2.0));
            ref->autoKpos.push_back(Keepout(lepos - 2.0, lepos));
            nrib.autoKpos.push_back(Keepout(tepos, tepos + 2.0));
            nrib.autoKpos.push_back(Keepout(lepos - 2.0, lepos));
         }
         else {
            // Place a single keepout covering the length of the doubler - no holes will be created
            ref->autoKpos.push_back(Keepout(tepos, lepos));
            nrib.autoKpos.push_back(Keepout(tepos, lepos));
         }

         // Put it in the correct place in the rib list
         if (tipSide)
            ribs.insert(std::next(ref), nrib);
         else
            ribs.insert(ref, nrib);
      }

      // SPACER
      else if (T->gqst(r, "meta") == QString("Spacer")) {
         nrib.typeTxt.append("SPACER");
         nrib.affectsSpars = false;
         nrib.jig = false; // Spacers have no jig by design
         ribs.push_back(nrib);
      }
      DBGLVL1("Processed row %d of %s (%s)", r + 1, T->GetKey().c_str(), T->gqst(r, "meta").toStdString().c_str());
   }

   return true;
}

bool Rib_set::addHoles(std::string& log) {
   for (auto it = begin(); it != end(); ++it)
      if (!it->addHoles(log))
         return false;

   return true;
}

bool Rib_set::addRibParams(GenericTab* T, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      DBGLVL1("Processing row %d of %s (%s)", r + 1, T->GetKey().c_str(), T->gqst(r, "meta").toStdString().c_str());

      if (T->gqst(r, "meta") == QString("Keep Out")) {
         bool doesIntersect = false;
         for (auto it = begin(); it != end(); ++it) {
            if (it->addKeepout(
               T->gdbl(r, "STX"),
               T->gdbl(r, "STY"),
               T->gdbl(r, "ENX"),
               T->gdbl(r, "ENY"),
               T->gdbl(r, "WIDTH")))
               doesIntersect = true;
         }
         if (!doesIntersect)
            log.append(SS("Keepout ") + TS(r + 1) + " does not affect any ribs\n");
      }
      else if (T->gqst(r, "meta") == QString("Washout")) {
         setWashout(r, T, log);
      }
      else if (T->gqst(r, "meta") == QString("TE Thickness")) {
         setTeThickness(r, T, log);
      }
      else
         log.append(SS("Unknown type of rib param ") + T->gqst(r, "meta").toStdString() + "\n");
   }

   return true;
}

obj& Rib_set::getPlan() {
   plan.del();
   for (auto it = begin(); it != end(); ++it)
      plan.copy_from(it->getPlan());

   // Add jig lines if they exist
   if ((jigLe.len() > 1.0) && (jigTe.len() > 1.0)) {
      plan.add_dotted(jigLe, 4.0, 4.0);
      plan.add_dotted(jigTe, 4.0, 4.0);
      asciivec tp(4.0);
      obj name = {};
      tp.add(name, "SH_JIG");
      double ls = -(name.find_extremity(RIGHT) + 5.0);
      name.move_origin_to(jigLe.get_S0());
      name.add_offset(ls, -2.0);
      plan.copy_from(name);
      name.move_origin_to(jigTe.get_S0());
      name.add_offset(ls, -2.0);
      plan.copy_from(name);
   }
   return plan;
}

void Rib_set::getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts) {
   // Ribs first
   for (auto& r : ribs) {
      obj& p = r.getPrettyPart();
      if (!p.empty()) {
         objects.push_back(p);
         texts.push_back(r.getPartText());
      }
   }

   // Sheeting jigs next
   for (auto& r : ribs) {
      obj& pb = r.getRole(Rib::botjig);
      if (!pb.empty()) {
         objects.push_back(pb);
         texts.push_back(r.getRole(Rib::botjigtext));
      }

      obj& pt = r.getRole(Rib::topjig);
      if (!pt.empty()) {
         objects.push_back(pt);
         texts.push_back(r.getRole(Rib::topjigtext));
      }
   }
}

bool Rib_set::setWashout(int r, GenericTab* T, std::string& log) {
   linvar wo(T->gdbl(r, "STX"), T->gdbl(r, "STVAL"), T->gdbl(r, "ENX"), T->gdbl(r, "ENVAL"));
   pivot_e pivot;
   QString pivstr = T->gqst(r, "PIVOT");
   if (pivstr == QString("LE"))
      pivot = LE;
   else if (pivstr == QString("TE"))
      pivot = TE;
   else if (pivstr == QString("CENTRE"))
      pivot = CENTRE;
   else {
      pivot = CENTRE;
      dbg::fatal(std::string("Unmappable pivot string ") + __FILE__);
   }

   bool doesIntersect = false;
   for (auto rib = begin(); rib != end(); ++rib) {
      double xpos = (rib->refLn.get_S0().x + rib->refLn.get_S1().x) / 2.0;
      if ((xpos >= T->gdbl(r, "STX")) && (xpos <= T->gdbl(r, "ENX"))) {
         doesIntersect = true;
         rib->washout = wo.v(xpos);
         rib->wo_pivot = pivot;
      }
   }
   if (!doesIntersect)
      log.append(SS("Washout row ") + TS(r + 1) + " does not affect any ribs\n");

   return true;
}

bool Rib_set::setTeThickness(int r, GenericTab* T, std::string& log) {
   double x0 = T->gdbl(r, "STX");
   double t0 = T->gdbl(r, "STVAL");
   double b0 = T->gdbl(r, "BLEND");
   double x1 = T->gdbl(r, "ENX");
   double t1 = T->gdbl(r, "ENVAL");
   double b1 = T->gdbl(r, "BLEND");

   linvar te(x0, t0, x1, t1);
   linvar bl(x0, b0, x1, b1);

   bool doesIntersect = false;

   for (auto rib = begin(); rib != end(); ++rib) {
      double xpos = (rib->refLn.get_S0().x + rib->refLn.get_S1().x) / 2.0;
      if ((xpos >= x0) && (xpos <= x1)) {
         doesIntersect = true;
         rib->te_thck = te.v(xpos);
         rib->te_blend = (bl.v(xpos) == 0.0) ? te_blend_default : (bl.v(xpos) / 100.0);
      }
   }
   if (!doesIntersect)
      log.append(SS("TE thickness row ") + TS(r + 1) + " does not affect any ribs\n");

   return true;
}

bool Rib_set::create(Planform& pl, Airfoil_set& af, std::string& log) {
   for (auto rib = begin(); rib != end(); ++rib) {
      if (!rib->isCreated) {
         DBGLVL1("Creating rib: %d", rib->index);
         if (!rib->createRib(pl, af, draft, log))
            log.append(SS("Creation of rib ") + TS(rib->index) + " failed\n");
         else
            rib->isCreated = true;
      }
   }
   return true;
}

bool Rib_set::addGeodetics(GenericTab* T, Planform& plnf, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      DBGLVL1("Processing row %d of %s", r, T->GetKey().c_str());

      // Create the top and bottom line
      obj topobj(coord_t{ T->gdbl(r, "TOPSTX"), T->gdbl(r, "TOPSTY") },
         coord_t{ T->gdbl(r, "TOPENX"), T->gdbl(r, "TOPENY") });
      topobj.extend1mm();
      line topln = { topobj.get_sp(), topobj.get_ep() };

      obj botobj(coord_t{ T->gdbl(r, "BOTSTX"), T->gdbl(r, "BOTSTY") },
         coord_t{ T->gdbl(r, "BOTENX"), T->gdbl(r, "BOTENY") });
      botobj.extend1mm();
      line botln = { botobj.get_sp(), botobj.get_ep() };

      bool bot_to_top = (T->gqst(r, "STATBOT") == QString("Bottom")) ? true : false;

      // Sanity checking
      if (!plnf.isInPlanform(topln))
         log.append(SS("Geodetic top line falls outside of planform (row: ") + TS(r + 1) + ")\n");
      if (!plnf.isInPlanform(botln))
         log.append(SS("Geodetic bottom line falls outside of planform (row: ") + TS(r + 1) + ")\n");
      if (topln.lines_intersect(botln, NULL, 0))
         log.append(SS("Geodetic top and bottom lines cross; I doubt this will end well (row: ") + TS(r + 1) + ")\n");

      // Search for ribs that are intersected by the geodetic boundary lines
      for (rib_iter rib0 = ribs.begin(); rib0 != ribs.end(); ++rib0) {
         // Find a starting rib
         coord_t r0bp, r0tp, r1bp, r1tp;
         if (!check_geodetic_intersect(rib0, &topln, &botln, &r0tp, &r0bp))
            continue; // Rib does not intersect top and bottom line
         DBGLVL2("  Geodetic first reference rib index: %d at intersects: T%s  B%s", rib0->index, r0tp.prstr(),
            r0bp.prstr());

         // Have found a starting rib, find the next valid rib if there is one
         bool found_rib1 = false;
         rib_iter rib1 = rib0;
         while (++rib1 != ribs.end()) {
            if (check_geodetic_intersect(rib1, &topln, &botln, &r1tp, &r1bp)) {
               if ((r1tp.x - r0tp.x) > (GEODETIC_THICKNESS_TO_X_RATIO * T->gdbl(r, "THK"))) // Need enough room to create a geodetic
               {
                  found_rib1 = true;
               }
               break;
            }
         }
         if (!found_rib1)
            continue; // No valid second rib

         DBGLVL2("  Geodetic secon reference rib index: %d at intersects: T%s  B%s", rib1->index, r1tp.prstr(),
            r1bp.prstr());

         // Can create a geodetic
         ribs.emplace_back();
         Rib& geod = ribs.back();
         geod.typeTxt = SS("GEODETIC");
         geod.notes = T->gqst(r, "NOTES").toStdString();
         geod.jig = false;
         geod.rib_thck = T->gdbl(r, "THK");
         geod.w_sh_thck = rib0->w_sh_thck;
         geod.leW = 0.0;
         geod.teW = 0.0;
         geod.lhbw = (T->gqst(r, "LH") == QString("Yes")) ? T->gdbl(r, "LHBW") : 0.0;
         geod.mhl = T->gdbl(r, "MHL");
         geod.index = ribs.size();

         line r0TrkLn(r0bp, r0tp); // rib 0 line to trace along in planform
         line r1TrkLn(r1bp, r1tp); // rib 1 line to trace along in planform																	// Top and bottom drawing lines
         if (!bot_to_top) {
            // Need to work from top to bottom
            r0TrkLn.reverse();
            r1TrkLn.reverse();
         }
         geod.objLn.set(coord_t{ r0TrkLn.get_S0().x + (rib0->rib_thck / 2.0), r0TrkLn.get_S0().y },
            coord_t{ r1TrkLn.get_S1().x - (rib1->rib_thck / 2.0), r1TrkLn.get_S1().y }); // Line the geodesic will occupy in planform
         if (!bot_to_top) {
            geod.objLn.reverse();  // Ensure that ribs drawn top to bottom are not reversed in the part view
         }

         // This is subtle; when the rib is created it will have the wing skin thickness removed from it.
         // Unless we do something here, the geodesic will end up too short
         geod.objLn.extend_mm(geod.w_sh_thck);
         geod.refLn.set(geod.objLn.get_S0(), geod.objLn.get_S1());

         // Prevent hole approaching front and rear edge
         geod.autoKpos.push_back(Keepout{ geod.refLn.len(), geod.refLn.len() + 10.0 });
         geod.autoKpos.push_back(Keepout{ -10.0, 0.0 });

         // Reverse angle for next geodetic
         bot_to_top = !bot_to_top;
      }
   }

   return true;
}

bool Rib_set::check_geodetic_intersect(rib_iter rib, line* topln, line* botln, coord_t* top, coord_t* bot) {
   if (rib->typeTxt == std::string("GEODETIC"))
      return false;

   if (topln->lines_intersect(rib->objLn, top, 0)) {
      if (botln->lines_intersect(rib->objLn, bot, 0)) {
         return true;
      }
   }
   return false;
}

bool Rib_set::addCreateJigs(GenericTab* T1, GenericTab* T2, std::string& log) {
   if ((T1->GetNumParts() == 0) || (T2->GetNumParts() == 0)) {
      DBGLVL1("No sheeting jigs configured");
      return true; // OK to continue with wing build
   }

   if ((T1->GetNumParts() != 1) || (T2->GetNumParts() != 1)) {
      log.append(SS("Please define one sheeting jig configuration on each tab.  Sheeting jigs have not been drawn.\n"));
      return true; // OK to continue with wing build
   }

   // Parse the first configuration
   jigLe =
   { coord_t{T1->gdbl(0, "LESTX"), T1->gdbl(0, "LESTY")}, coord_t{T1->gdbl(0, "LEENX"), T1->gdbl(0, "LEENY")} };
   jigTe =
   { coord_t{T1->gdbl(0, "TESTX"), T1->gdbl(0, "TESTY")}, coord_t{T1->gdbl(0, "TEENX"), T1->gdbl(0, "TEENY")} };
   line jigBotSpr = { coord_t{T1->gdbl(0, "BSSTX"), T1->gdbl(0, "BSSTY")}, coord_t{T1->gdbl(0, "BSENX"), T1->gdbl(
                                                                                                            0, "BSENY")} };
   double jbsW = T1->gdbl(0, "BSWIDTH");
   double jbsD = T1->gdbl(0, "BSWIDTH");
   DBGLVL1("Sheeting jig configuration 1 loaded");

   // Parse the second configuration
   double leBarW = T2->gdbl(0, "LEBARWIDTH");
   double leBarD = T2->gdbl(0, "LEBARDEPTH");
   double teBarW = T2->gdbl(0, "TEBARWIDTH");
   double teBarD = T2->gdbl(0, "TEBARDEPTH");
   double height = T2->gdbl(0, "HEIGHT");
   double thckns = T2->gdbl(0, "THK");
   DBGLVL1("Sheeting jig configuration 2 loaded");

   // Work through the ribs
   for (auto& rb : ribs) {
      if (rb.jig) {
         rb.sheetingJig(jigLe, jigTe, jigBotSpr, jbsW, jbsD,
            leBarW,
            leBarD, teBarW, teBarD, height, thckns,
            false,
            draft, Rib::type1, Rib::jigType1, Rib::shJigBarPos::inside, Rib::shJigBarPos::inside, rb.index, log);
         rb.sheetingJig(jigLe, jigTe, jigBotSpr, jbsW, jbsD,
            leBarW,
            leBarD, teBarW, teBarD, height, thckns,
            true,
            draft, Rib::type1, Rib::jigType1, Rib::shJigBarPos::inside, Rib::shJigBarPos::inside, rb.index, log);
      }
   }
   return true;
}

bool Rib_set::addCreateJigsType2(GenericTab* T, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      if (T->gqst(r, "meta") == QString("Jig Configuration")) {
         // Parse the first configuration
         jigLe =
         { coord_t{T->gdbl(r, "LESTX"), T->gdbl(r, "LESTY")}, coord_t{T->gdbl(r, "LEENX"), T->gdbl(r, "LEENY")} };
         jigTe =
         { coord_t{T->gdbl(r, "TESTX"), T->gdbl(r, "TESTY")}, coord_t{T->gdbl(r, "TEENX"), T->gdbl(r, "TEENY")} };
         double leBarW = T->gdbl(r, "LEBARWIDTH");
         double leBarD = T->gdbl(r, "LEBARDEPTH");
         double teBarW = T->gdbl(r, "TEBARWIDTH");
         double teBarD = T->gdbl(r, "TEBARDEPTH");
         double height = T->gdbl(r, "HEIGHT");
         double thckns = T->gdbl(r, "THK");
         Rib::shJigBarPos lePos = (T->gqst(r, "LEBARPOS") == QString("Inside")) ? Rib::shJigBarPos::inside : Rib::shJigBarPos::outside;
         Rib::shJigBarPos tePos = (T->gqst(r, "TEBARPOS") == QString("Inside")) ? Rib::shJigBarPos::inside : Rib::shJigBarPos::outside;
         Rib::shJigEndType eType = (T->gqst(r, "ENDTYPE") == QString("Simple")) ? Rib::shJigEndType::jigType2Simple : Rib::shJigEndType::jigType1;
         line l;
         DBGLVL2("Type 2 Sheeting jig configuration %d", r);

         // Work through the ribs
         for (auto& rb : ribs) {
            if (rb.jig) {
               rb.sheetingJig(jigLe, jigTe, l, 0, 0,
                  leBarW,
                  leBarD, teBarW, teBarD, height, thckns,
                  false,
                  draft, Rib::type2, eType, lePos, tePos, rb.index, log);
               rb.sheetingJig(jigLe, jigTe, l, 0, 0,
                  leBarW,
                  leBarD, teBarW, teBarD, height, thckns,
                  true,
                  draft, Rib::type2, eType, lePos, tePos, rb.index, log);
            }
         }
      }
   }
   return true;
}

bool Rib::sheetingJig(line& jigLe, line& jigTe, line& jigBotSpr, double jbsW, double jbsD,
   double lew,
   double let, double tew, double tet, double height, double thck, bool topFlag,
   bool draftMode, shJigType jigType, shJigEndType endType,
   shJigBarPos lepos, shJigBarPos tepos,
   int partIndex, std::string& log) {
   obj& jig = (topFlag) ? addRole(Rib::topjig) : addRole(Rib::botjig);
   obj& txt = (topFlag) ? addRole(Rib::topjigtext) : addRole(Rib::botjigtext);
   jig_thck = thck;

   // Create the text label
   asciivec tp(5.0);
   char str[256];
   txt.del();
   snprintf(str, 256, "%s %d", (topFlag) ? "TOP SHEETING JIG" : "BOTTOM SHEETING JIG", partIndex);
   tp.add(txt, coord_t{ 0.0, 0.0 }, str);

   // Find the extremeties of the jig rib reference
   coord_t pt[4];
   double ex[4];
   line_iter ln[4];
   getPart().find_extremity(pt, ex, ln);

   // Copy airfoil and enlarge by overcut
   jig.del();
   jig.copy_from(getRole(apcd));
   jig.trace_at_offset(0.2);
   draftMode ? jig.simplify(0.1) : jig.simplify();

   if (topFlag) {
      jig.mirror_y();
   }
   jig.regularise();

   // Add the slots for the LE and TE bar
   // Jigline is used to estimate the slot width
   double lesw = slotWidth(jigLe, refLn, lew, thck);
   double tesw = slotWidth(jigTe, refLn, tew, thck);

   // Estimate the xposition to align the edge of the bar with the extremity, then cut slot
   // This is a slightly complicated process:
   //   ept is found as the end point of the jig line; this is the top (l/r) corner of the bar
   //   tst is set to be the centre of the bar by offsetting from ept
   //   tst is then rotated around ept by the angle of the jig edge
   //		(the jig edge has to be reversed or have 180deg added to it to get an angle
   //       between +/-90deg given that this is the bottom of a clockwise rotating path)
   //   the x value of tst is now correct for the bar
   coord_t ept = { 0, 0 }, lept0, lept1, tept0, tept1, tst;
   line ref;
   line_iter eln, lel0, tel1;
   if (!jig.bot_intersect(ex[RIGHT], &ept, eln)) {
      log.append(SS("Sheeting Jig Generation: Failed to find intersection for LE bar for rib ") + TS(index) + "\n");
      return false;
   }
   const double lewOffset = (lepos == Rib::shJigBarPos::inside) ? (lew / 2.0) : -(lew / 2.0);
   tst = { ept.x + lewOffset, ept.y - (let / 2.0) };
   ref = *eln;
   rotatePoint(&tst, ept, ref.angle());
   if (!jig.cut_slot(DOWN, tst.x, lesw, -let, CENGRAD, &lept0, &lept1)) {
      log.append(
         SS("Sheeting Jig Generation: Failed to cut slot for LE bar for rib ") + TS(index) + " at x=" + TS(tst.x) + "\n");
      return false;
   }

   // Repeat for trailing edge bar
   if (!jig.bot_intersect(ex[LEFT], &ept, eln)) {
      log.append(SS("Sheeting Jig Generation: Failed to find intersection for TE bar for rib ") + TS(index) + "\n");
      return false;
   }
   const double tewOffset = (tepos == Rib::shJigBarPos::inside) ? (tew / 2.0) : -(tew / 2.0);
   tst = { ept.x + tewOffset, ept.y - (tet / 2.0) };
   ref = *eln;
   ref.reverse();
   rotatePoint(&tst, ept, ref.angle());
   if (!jig.cut_slot(DOWN, tst.x, tesw, -tet, CENGRAD, &tept0, &tept1)) {
      log.append(
         SS("Sheeting Jig Generation: Failed to cut slot for TE bar for rib ") + TS(index) + " at x=" + TS(tst.x) + "\n");
      return false;
   }

   // Recover the lines adjacent to the slots
   if (!jig.s1_is_at(lept0, lel0)) {
      log.append(SS("Sheeting Jig Generation: Failed to recover line adjacent to LE bar slot rib ") + TS(index) + "\n");
      return false;
   }
   if (!jig.s0_is_at(tept1, tel1)) {
      log.append(SS("Sheeting Jig Generation: Failed to recover line adjacent to TE bar slot rib ") + TS(index) + "\n");
      return false;
   }

   // Remove all line segments which are not part of the slot
   line_iter del = tel1;
   while (del != lel0) {
      line_iter nxt = jig.nextc(del);
      jig.del(del);
      del = nxt;
   }
   jig.del(lel0);

   // The following code works out where to put the end of the Jig so that it is on
   // line configured for the wing by sheetingJigConfig.
   double T, offset;
   coord_t ribEnd, jigEnd = { 0, 0 }, leftPt, righPt;
   // Where is it in the planform?
   T = tept1.x / refLn.len();
   ribEnd = refLn.get_pt(T);
   // How far is it from the jig line?
   if (!jigTe.lines_intersect(refLn, &jigEnd, 1)) {
      log.append(SS("Sheeting Jig Generation: Failed to find intersect between jigTE line and rib ") + TS(index) + "\n");
      return false;
   }
   offset = distTwoPoints(ribEnd, jigEnd);
   leftPt = drawJigEnd(endType, jig, tept1, offset, height, LEFT);

   // Repeat for the LE
   T = lept0.x / refLn.len();
   ribEnd = refLn.get_pt(T);
   if (!jigLe.lines_intersect(refLn, &jigEnd, 1)) {
      log.append(SS("Sheeting Jig Generation: Failed to find intersect between jigLE line and rib ") + TS(index) + "\n");
      return false;
   }
   offset = distTwoPoints(ribEnd, jigEnd);
   righPt = drawJigEnd(endType, jig, lept0, offset, height, RIGHT);

   // Join with a spar cut out if needed
   jig.add(righPt, leftPt);
   jig.move_back_to_front(); // Use the bottom line to start any regularise actions
   jig.regularise();
   if (jigType == Rib::type1) {
      if ((jbsD != 0.0) && (jbsW != 0.0)) {
         coord_t planIs = { 0, 0 };
         if (!jigBotSpr.lines_intersect(refLn, &planIs, 1)) {
            log.append(
               SS("Sheeting Jig Generation: Failed to find intersect between jig bottom spar line and rib ") + TS(index) + "\n");
            return false;
         }
         double pos = distTwoPoints(planIs, refLn.get_S0());
         double sw = slotWidth(jigBotSpr, refLn, jbsW, thck);
         if (!jig.cut_slot(DOWN, pos, sw, jbsD, VERTICAL)) {
            log.append(
               SS("Sheeting Jig Generation: Failed to cut slot for jig bottom spar line for rib ") + TS(index) + "\n");
            return false;
         }
      }
   }
   jig.regularise();
   return true;
}

// Draw an end of a jig
coord_t Rib::drawJigEnd(Rib::shJigEndType endType, obj& jr, coord_t startHere, double offset, double height, direction_e dir) {
   switch (endType) {
   case Rib::jigType1:
      return (drawJigEndType1(jr, startHere, offset, height, dir));
      break;
   case Rib::jigType2Simple:
      return (drawJigEndType2Simple(jr, startHere, offset, height, dir));
      break;
   default:
      DBGLVL1("Unknown Case Satement %d", endType);
      coord_t retval = {};
      return retval;
      break;
   }

}

// Draw the end of the jig with a slot for the clamp bar, starting at startHere and
// offset from the end by offset.
// dir defines whether it is the left or right hand end that gets drawn
coord_t Rib::drawJigEndType1(obj& jr, coord_t startHere, double offset, double height, direction_e dir) {
   coord_t retval;
   int ds = (dir == LEFT) ? -1 : +1;

   // Create a gap for clearance if needed (due to washout)
   double offset1, offset2;
   if (startHere.y > (0 - (SH_JIG_CLAMPING_GAP / 2.0))) {
      offset1 = (offset - 5.0) * ds;
      offset2 = 5.0 * ds;
   }
   else {
      offset1 = offset * ds;
      offset2 = 0;
   }
   jr.add(startHere, vector_t{ offset1, 0 });
   jr.add(jr.get_ep(), vector_t{ 0, 0 - (SH_JIG_CLAMPING_GAP / 2.0) - jr.get_ep().y });
   jr.add(jr.get_ep(), vector_t{ offset2, 0 });
   jr.add(jr.get_ep(), vector_t{ SH_JIG_END_W * ds, 0 });
   jr.add(jr.get_ep(), vector_t{ 0, -(height - (SH_JIG_CLAMPING_GAP / 2.0)) });
   retval = jr.get_ep();

   // Slot for clamp bar
   coord_t pt1 = { startHere.x + (offset * ds) + (SH_JIG_END_W * ds / 2.0) + (SH_JIG_BAR_W * ds / 2.0),
                  -(SH_JIG_CLAMPING_GAP / 2.0) - 2.0 };
   coord_t pt2 = { pt1.x - (SH_JIG_BAR_W * ds), pt1.y - SH_JIG_BAR_T };
   jr.add_rect(pt1, pt2);

   return (retval);
}

// Type 2 simple is just a square end with no thing else, although it is extended to the jig line
coord_t Rib::drawJigEndType2Simple(obj& jr, coord_t startHere, double offset, double height, direction_e dir) {
   int ds = (dir == LEFT) ? -1 : +1;
   double x = startHere.x + (offset * ds);
   jr.add(startHere, coord_t{ x, startHere.y });
   jr.add(jr.get_ep(), coord_t{ x, -height });
   return (jr.get_ep());
}
