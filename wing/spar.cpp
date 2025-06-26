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

#include "spar.h"
#include "ascii.h"
#include "debug.h"
#include "object_oo.h"
#include "part.h"
#include "rib.h"
#include "tabs.h"

bool sp_rib_is_sort(const intersect_t& a, const intersect_t& b) {
   return (a.intersect.x < b.intersect.x);
}

bool Spar::create(Rib_set& ribs, std::string& log) {
   DBGLVL2("stX: %lf  stY: %lf  enX: %lf  enY: %lf", stX, stY, enX, enY);

   objLn.set(coord_t{ stX, stY }, coord_t{ enX, enY });
   objLn.extend_mm(REFLN_EXT_mm);

   switch (mytype) {
   case jigspar:
      jigSpar(ribs, log);
      break;

   case sheetspar:
   case websslotted:
      sheetSpar(ribs, log);
      break;

   case ribsupport:
      ribSupport(ribs, log);
      break;

   case singlespar:
      singleSpar(ribs, log);
      break;

   case ribtabs:
      ribTabs(ribs, log);
      break;

   case hspar:
   case hsspar:
   case boxspar:
      topBotSpar(ribs, log);
      break;

   case webs:
      sparWebs(ribs, log);
      break;

   default:
      dbg::fatal(SS("Unrecognised spar type in createSpar: ") + TS((int)mytype));
      break;
   }

   createPartText();
   return true;
}

obj& Spar::getPlan() {
   obj& plan = Part::getPlan();

   // Clear the plan
   plan.del();

   if (objLn.len() == 0.0)
      return plan;

   if (isSheetType()) {
      plan.add_rect(objLn, spW, markspace);
      if (mytype == spartype_e::jigspar)
         plan.add_dotted(objLn, 2.0, 8.0);
   }
   else if (isStripType())
      plan.add_rect(objLn, spW, markspace);
   else if (isWebType())
      plan.add_rect(objLn, wThck, markspace);

   char str[10];
   asciivec tp(4.0);
   snprintf(str, 10, "%d", index);
   coord_t txt = { objLn.get_S0().x - 10, objLn.get_S0().y - 2.0 };
   if (index & 1)
      txt.x = txt.x - 10;
   tp.add(plan, txt, str);

   return plan;
}

void Spar::ribSupport(Rib_set& ribs, std::string& log) {
   DBGLVL1("Rib support %d...", index);

   markspace = RIBSUPPORT_MARKSPACE;

   sparRibIntersect(ribs, log);

   if (iss.size() < 2) {
      log.append(SS("Rib support type spar ") + TS(index) + "must intersect at least two ribs to be rendered\n");
      return;
   }

   // Draw the outline of the rib support
   obj topObj = {};
   obj botObj = {};

   for (auto ist = iss.begin(); ist != iss.end(); ++ist) {
      ist->posSpr = planToXpos(ist->intersect);

      if (ist == iss.begin()) {
         // First slot so extend to the left by 100mm
         coord_t top = { ist->posSpr - 100.0, ist->rib_bot.y + RIB_SUPPORT_MIN_SLOT };
         coord_t bot = { ist->posSpr - 100.0, -height };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }

      {
         coord_t top = { ist->posSpr, ist->rib_bot.y + RIB_SUPPORT_MIN_SLOT };
         coord_t bot = { ist->posSpr, -height };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }

      if (ist == std::prev(iss.end())) {
         // Last slot so extend to the right
         coord_t top = { ist->posSpr + 100.0, ist->rib_bot.y + RIB_SUPPORT_MIN_SLOT };
         coord_t bot = { ist->posSpr + 100.0, -height };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }
   }

   // Make a part from it
   obj& p = getPart();
   p.copy_from(topObj);
   p.copy_from(botObj);
   p.add(topObj.get_sp(), botObj.get_sp());
   p.add(topObj.get_ep(), botObj.get_ep());
   p.make_path();
   getRawPart().copy_from(p);

   // Now cut the slots for the ribs
   for (auto ist = iss.begin(); ist != iss.end(); ++ist)
      cutSlot(ist->intersect, true, true, false, ist->rib_bot.y, ist->wSpr, 0.0, log);

   // Trim part to length - some overhang is left to support the end ribs
   trimByAutoKeepouts(-Part::OVC + JIG_EXTEND_END);

   redrawObjLine();
}

void Spar::jigSpar(Rib_set& ribs, std::string& log) {
   DBGLVL1("Jig-sheet spar %d...", index);

   // Start off with a sheet spar
   sheetSpar(ribs, log, JIG_EXTEND_END);
   obj& p = getPart();

   // We now need to work along the bottom line of the part,
   // creating the jig piece and adding tabs to join the two together.
   // Two tabs are created between each pair of ribs where there is enough room.
   // Start a top line for the jig piece
   obj jt = {};
   double partExt[4];

   // Find the starting point of the top of the jig piece based on the bottom-left corner of the spar
   p.find_extremity(partExt);
   jt.add(coord_t{ partExt[LEFT], partExt[DOWN] - JIG_SEP_SLOT_WIDTH });

   auto firstIst = iss.begin();
   for (auto ist = std::next(iss.begin()); ist != iss.end(); ++ist) {
      double x0 = planToXpos(firstIst->intersect);
      double x1 = planToXpos(ist->intersect);
      double dX = (x1 - x0);

      // Skip this rib pair if there is not enough room for tabs
      if (dX < JIG_MIN_TAB_IST_SEP) {
         firstIst = ist;
         continue;
      }

      // Create two tabs centered on 25% and 75% of the gap between the ribs
      double tabFactor = tabpc / 100.0;
      double w = (tabFactor / 2.0) * dX;
      for (double factor = 0.25; factor < 1.0; factor = factor + 0.5) {
         double l = (factor * dX) + x0 - (w / 2.0);
         double r = (factor * dX) + x0 + (w / 2.0);

         // Open a gap in the bottom of the spar section
         coord_t lpt = {}, rpt = {};
         line_iter lln = {}, rln = {};
         if (!p.bot_intersect(l, &lpt, lln) || !p.bot_intersect(r, &rpt, rln)) {
            log.append(
               SS("Jigspar ") + TS(index) + ": Failed to find an intersect in order to draw a tab between the spar and the jig sections.\n");
            break;
         }
         openGap(rpt, rln, lpt, lln);

         // Draw on the jig piece top line
         jt.add(coord_t{ lpt.x, lpt.y - JIG_SEP_SLOT_WIDTH });
         jt.add(lpt);
         jt.add(rpt, coord_t{ rpt.x, rpt.y - JIG_SEP_SLOT_WIDTH });
      }

      firstIst = ist;
   }

   // Complete the jig piece
   jt.add(coord_t{ partExt[RIGHT], partExt[DOWN] - JIG_SEP_SLOT_WIDTH });
   coord_t topLeft = jt.get_sp();
   coord_t topRigh = jt.get_ep();
   coord_t botLeft = { topLeft.x, -height };
   coord_t botRigh = { topRigh.x, -height };
   jt.add(topLeft, botLeft);
   jt.add(botLeft, botRigh);
   jt.add(botRigh, topRigh);

   p.copy_from(jt);
   p.make_path();
   redrawObjLine();
}

void Spar::sheetSpar(Rib_set& ribs, std::string& log, double extendEnd) {
   DBGLVL1("Sheet spar %d...", index);

   sparRibIntersect(ribs, log);

   if (!drawSheetSparOutline(log))
      return;

   for (auto ist = iss.begin(); ist != iss.end(); ++ist) {
      if (tabsNotSlots) {
         cutTabSlot(ist->intersect, ist->wSpr, slotDepthPercent, 0.0, log);
         // Half the slot above, half below for the rib
         if (ist->slotRib) {
            double ribPercent = (100 - slotDepthPercent) / 2.0;
            ist->rib->cutSheetStyleSlot(ist->intersect, true, true, ist->wRib, ribPercent, 0.0, fe, log);
            ist->rib->redrawObjLine();
         }
      }
      else {
         cutSheetStyleSlot(ist->intersect, inFromBelow, !inFromBelow, ist->wSpr, slotDepthPercent, 0.0, CENTRE, log);
         if (ist->slotRib) {
            double ribPercent = 100.0 - slotDepthPercent;
            ist->rib->cutSheetStyleSlot(ist->intersect, !inFromBelow, inFromBelow, ist->wRib, ribPercent, 0.0, fe, log);
            ist->rib->redrawObjLine();
         }
      }
   }

   trimByAutoKeepouts(-Part::OVC + extendEnd);
   addHoles(log);
   getPart().make_path();
}

bool Spar::drawSheetSparOutline(std::string& log) {
   if (iss.size() < 2) {
      log.append(SS("Sheet type spar ") + TS(index) + "must intersect at least two ribs to be rendered\n");
      return false;
   }

   // Draw the top and bottom of the part by working along the spar rib intersects
   obj topObj = {};
   obj botObj = {};

   for (auto ist = iss.begin(); ist != iss.end(); ++ist) {
      ist->posSpr = planToXpos(ist->intersect);

      if (ist == iss.begin()) {
         // Extend to the left to allow room for a leaning first rib
         double height = ist->rib_top.y - ist->rib_bot.y;
         coord_t top = { ist->posSpr - height, ist->rib_top.y };
         coord_t bot = { ist->posSpr - height, ist->rib_bot.y };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }

      {
         coord_t top = { ist->posSpr, ist->rib_top.y };
         coord_t bot = { ist->posSpr, ist->rib_bot.y };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }

      if (ist == std::prev(iss.end())) {
         // Extend to the right to allow room for a leaning last rib
         double height = ist->rib_top.y - ist->rib_bot.y;
         coord_t top = { ist->posSpr + height, ist->rib_top.y };
         coord_t bot = { ist->posSpr + height, ist->rib_bot.y };
         topObj.add(top);
         botObj.add(bot);
         DBGLVL2("Adding Top Point: %s  Bot Point: %s", top.prstr(), bot.prstr());
      }
   }

   // Complete the part by joining the ends
   obj& p = getPart();
   p.copy_from(topObj);
   p.copy_from(botObj);
   p.add(topObj.get_sp(), botObj.get_sp());
   p.add(topObj.get_ep(), botObj.get_ep());
   p.make_path();
   getRawPart().copy_from(p);

   return true;
}

void Spar::singleSpar(Rib_set& ribs, std::string& log) {
   DBGLVL1("Single strip spar %d:", index);

   sparRibIntersect(ribs, log);

   for (auto& ist : iss)
      if (ist.slotRib)
         ist.rib->cutSnappedStripSparSlot(ist.intersect, ribTop, ist.wRib, spD, log);
}

void Spar::ribTabs(Rib_set& ribs, std::string& log) {
   DBGLVL1("Rib tab set %d:", index);

   sparRibIntersect(ribs, log);

   for (auto& ist : iss) {
      if (ist.rib->typeTxt == "RIB") // Don't add tabs to doublers or geodetics
      {
         if (!ist.rib->cutSlot(ist.intersect, false, false, false, -height, ribTabW, 0.0, log))
            log.append(
               SS("Problem adding a rib tab to rib ") + TS(index) + " at plan point " + TScoord(ist.intersect) + "\n");
         else {
            // Rib tabs don't have a keepout, they shouldn't affect holes
            ist.rib->autoKpos.pop_back();

            // Add small markers to help with separation
            const double markerSep = 10.0; // Spacing of markers
            int numMarkers = floor(ribTabW / markerSep);
            double markerOffset = ((numMarkers % 2) == 0) ? (markerSep / 2.0) : 0;
            for (int marker = 0; marker < numMarkers; ++marker) {
               const double markerPos = ((marker - (numMarkers / 2)) * markerSep) + markerOffset + ist.posRib;
               coord_t pt = {};
               line_iter ln = {};
               if (!ist.rib->getRawPart().bot_intersect(markerPos, &pt, ln))
                  log.append(
                     SS("Problem adding a rib tab marker dot ") + TS(index) + " at plan point " + TScoord(ist.intersect) + "\n");
               else {
                  line rectLine = {};
                  const double dotDepth = 1.55; // Depth of the marker dot cutout
                  const double dotWidth = 2.0;
                  rectLine.set(pt, (dotWidth / 2.0), ln->angle());
                  rectLine.set(rectLine.get_pt(-1), rectLine.get_pt(1));
                  rectLine.move_sideways(dotDepth / 2.0);
                  ist.rib->getPart().add_rect(rectLine, dotDepth);
               }
            }
         }
      }
   }
}

void Spar::topBotSpar(Rib_set& ribs, std::string& log) {
   DBGLVL1("Top and bottom strip spars %d:", index);

   sparRibIntersect(ribs, log);

   for (auto& ist : iss) {
      if (ist.slotRib) {
         ist.rib->cutStripSparSlot(ist.intersect, true, ist.wRib, spD, log);
         ist.rib->cutStripSparSlot(ist.intersect, false, ist.wRib, spD, log);
      }
   }
}

void Spar::sparRibIntersect(Rib_set& ribs, std::string& log) {
   for (auto rib = ribs.begin(); rib != ribs.end(); rib++) {
      if (!rib->affectsSpars)
         continue;

      intersect_t is = {};

      // Find the spar/rib intersection point
      if (objLn.lines_intersect(rib->objLn, &is.intersect, 0)) {
         DBGLVL1("Spar %d Rib %d : Intersect is at %s", index, rib->index, is.intersect.prstr());
         is.rib = rib;
         is.posRib = rib->planToXpos(is.intersect);
         if (!rib->getPart().top_bot_intersect(is.posRib, &is.rib_top, &is.rib_bot)) {
            log.append(
               typeTxt + SS("Spar ") + TS(index) + " Rib " + TS(rib->index) + " Unable to find a top and bottom intersect to determine sheet spar depth at plan point" + TScoord(is.intersect) + "\n");
            continue;
         }
         DBGLVL2("X position on rib %.2lf  Rib Top %s  Rib Bottom %s", is.posRib, is.rib_top.prstr(), is.rib_bot.prstr());

         // Find the slot widths
         if (widenSlots) {
            is.wSpr = slotWidth(rib->objLn, objLn, rib->rib_thck, spW) + Part::OVC;
            is.wRib = slotWidth(objLn, rib->objLn, spW, rib->rib_thck) + Part::OVC;
         }
         else {
            is.wSpr = rib->rib_thck + Part::OVC;
            is.wRib = spW + Part::OVC;
         }
         DBGLVL2("  Slot widths: Spar %.2lf Rib %.2lf", is.wSpr, is.wRib);

         // Find the slot lean angles
         is.aRib = 0.0;
         is.aSpr = 0.0;

         // Check the rib slot is not in a keepout
         if ((mytype != websslotted) && (rib->isInSparKeepout(is.posRib - (is.wRib / 2.0)) || rib->isInSparKeepout(is.posRib + (is.wRib / 2.0)) || rib->isInSparKeepout(is.posRib))) {
            // A spar has already had its way with the rib, so we don't want to slot it again
            is.slotRib = false;
            DBGLVL2("X position on rib %.2lf is in keepout, will not be slotted", is.posRib);
            if ((mytype == sheetspar) || (mytype == jigspar)) {
               // For sheet spars, we'll have to work out the height of the part from an unslotted version
               if (!rib->getRawPart().top_bot_intersect(is.posRib, &is.rib_top, &is.rib_bot)) {
                  log.append(
                     typeTxt + SS("Spar ") + TS(index) + " Rib " + TS(rib->index) + " Unable to find a top and bottom intersect in raw part to determine sheet spar depth at plan point" + TScoord(is.intersect) + "\n");
                  continue;
               }
               DBGLVL2("X position on rib %.2lf  Rib Top %s  Rib Bottom %s  recalculated from raw part due to keepout",
                  is.posRib,
                  is.rib_top.prstr(), is.rib_bot.prstr());
            }
         }

         // Find the minimum y value for a rib support by intersecting at the corners of the slot 
         coord_t tmpPt = {};
         line_iter tmpLn = {};
         is.minYforRibSupport = is.rib_bot.y;
         if (rib->getPart().bot_intersect(is.posRib - (is.wRib / 2.0), &tmpPt, tmpLn))
            if (tmpPt.y < is.minYforRibSupport) {
               is.minYforRibSupport = tmpPt.y;
               DBGLVL2("Decreasing rib support y to %.2lf", is.minYforRibSupport);
            }

         if (rib->getPart().bot_intersect(is.posRib + (is.wRib / 2.0), &tmpPt, tmpLn))
            if (tmpPt.y < is.minYforRibSupport) {
               is.minYforRibSupport = tmpPt.y;
               DBGLVL2("Decreasing rib support y to %.2lf", is.minYforRibSupport);
            }

         // Add the slot set to the spar
         iss.push_back(is);
      }
      else
         DBGLVL1("Spar %d Rib %d : do not intersect", index, rib->index);
   }

   if (iss.size() == 0) {
      log.append(SS("Spar ") + TS(index) + " crosses no ribs; it will not be rendered.\n");
      return;
   }
   else if (iss.size() == 1) {
      // We don't have a spar line as such, so render only a small section
      objLn.set(
         coord_t{ iss.front().intersect.x - iss.front().rib->rib_thck, iss.front().intersect.y },
         coord_t{ iss.front().intersect.x + iss.front().rib->rib_thck, iss.front().intersect.y });
   }
   else {
      // Sort the intersects into order
      iss.sort(sp_rib_is_sort);

      // Redraw the spar in planform based on first and last intersects
      objLn.set(iss.front().intersect, iss.back().intersect);
   }

   // For a spar, reference and object lines are the same
   refLn = objLn;
}

void Spar::sparWebs(Rib_set& ribs, std::string& log) {
   DBGLVL1("Spar webs %d", index);

   sparRibIntersect(ribs, log);

   // Spar webs are created by drawing a sheet spar and the chopping it up into pieces
   if (!drawSheetSparOutline(log))
      return;

   obj webs = {};
   auto first = iss.begin();
   for (auto ist = std::next(iss.begin()); ist != iss.end(); ++ist) {
      double x0 = planToXpos(first->intersect) + (first->wSpr / 2.0);
      double x1 = planToXpos(ist->intersect) - (ist->wSpr / 2.0);
      double dX = (x1 - x0);

      if (dX < MIN_WEB_WIDTH_mm) {
         first = ist;
         continue;
      }

      // TODO: This method of chopping up doesn't support leaning ribs
      obj& web = getRole(WEB_ROLE);
      web.del();
      web.copy_from(getPart());
      web.remove_extremity_rejoin(x0, LEFT);
      web.remove_extremity_rejoin(x1, RIGHT);

      // Split the web into segments of maximum length
      maxLenSplitX(mlen, WEB_ROLE);

      // Rotate it to a grain-vertical position
      web.rotate(web.originIsAt(), M_PI_2);

      // Add it to the part
      web.move_origin_to(webs.find_extremity(RIGHT) + (SPLIT_SEPARATION * 2.0));
      webs.copy_from(web);
      first = ist;
   }

   getPart().del();
   getPart().copy_from(webs);
   getPart().make_path();
}

bool Spar_set::add(GenericTab* T, std::string& log) {
   (void)log;

   for (int r = 0; r < T->GetNumParts(); r++) {
      Spar spr;
      DBGLVL1("Processing row %d of %s", r, T->GetKey().c_str());

      if (T->gqst(r, "meta") == QString("Sheet Spar")) {
         spr.mytype = spartype_e::sheetspar;
         spr.typeTxt.append("SHEET SPAR");
      }
      else if (T->gqst(r, "meta") == QString("Sheet Spar+Jig")) {
         spr.mytype = spartype_e::jigspar;
         spr.typeTxt.append("JIGGING SPAR");
      }
      else if (T->gqst(r, "meta") == QString("Rib Support")) {
         spr.mytype = spartype_e::ribsupport;
         spr.typeTxt.append("RIB SUPPORT");
      }
      else if (T->gqst(r, "meta") == QString("Box Spar")) {
         spr.mytype = spartype_e::boxspar;
         spr.typeTxt.append("BOX SPAR");
      }
      else if (T->gqst(r, "meta") == QString("H-Spar")) {
         spr.mytype = spartype_e::hspar;
         spr.typeTxt.append("H-SPAR");
      }
      else if (T->gqst(r, "meta") == QString("H-Sheet Spar")) {
         spr.mytype = spartype_e::hsspar;
         spr.typeTxt.append("H-SHEET SPAR");
      }
      else if (T->gqst(r, "meta") == QString("Strip Spar")) {
         spr.mytype = spartype_e::singlespar;
         spr.typeTxt.append("SINGLE SPAR");
      }
      else if (T->gqst(r, "meta") == QString("Rib Support Tabs")) {
         spr.mytype = spartype_e::ribtabs;
         spr.typeTxt.append("RIB TABS");
      }
      else
         dbg::fatal(SS("Spar at row ") + TS(r) + " is not of recognised type " + T->gqst(r, "meta").toStdString());

      spr.stX = T->gdbl(r, "STX");
      spr.stY = T->gdbl(r, "STY");
      spr.enX = T->gdbl(r, "ENX");
      spr.enY = T->gdbl(r, "ENY");
      spr.widenSlots = (T->gqst(r, "WIDENSLOTS") == QString("Yes")) ? true : false;
      //spr.noLastRibSlot= (T->gqst(r, "SLOTLASTRIB")  == QString("Yes")) ? false : true;
      spr.index = spars.size() + 1;
      spr.notes.append(T->gqst(r, "NOTES").toStdString());
      spr.fe = pivot_e::CENTRE;

      switch (spr.mytype) {
      case spartype_e::sheetspar:
      case spartype_e::jigspar:
      case spartype_e::ribsupport:
      case spartype_e::ribtabs:
         spr.spW = T->gdbl(r, "THK");
         spr.slotDepthPercent = T->gdbl(r, "SLOTDEPTH");
         spr.inFromBelow = (T->gqst(r, "INSFROM") == QString("Below")) ? true : false;
         if (T->gqst(r, "FALSEEDGE") == QString("In Front"))
            spr.fe = pivot_e::LE;
         else if (T->gqst(r, "FALSEEDGE") == QString("Behind"))
            spr.fe = pivot_e::TE;
         else
            spr.fe = pivot_e::CENTRE;
         spr.lhbw = (T->gqst(r, "LH") == QString("Yes")) ? T->gdbl(r, "LHBW") : 0.0;
         spr.mhl = T->gdbl(r, "MHL");
         spr.tabsNotSlots = (T->gqst(r, "TABSNOTSLOTS") == QString("Tabs")) ? true : false;
         spr.height = T->gdbl(r, "HEIGHT");
         spr.tabpc = T->gdbl(r, "TABW");
         spr.ribTabW = T->gdbl(r, "RSTABW");
         break;

      case spartype_e::boxspar:
      case spartype_e::hspar:
      case spartype_e::hsspar:
      case spartype_e::singlespar:
         spr.spW = T->gdbl(r, "SPW");
         spr.spD = T->gdbl(r, "SPD");
         spr.ribTop = (T->gqst(r, "TORB") == QString("Top")) ? true : false;
         spr.wThck = T->gdbl(r, "WTHK");
         spr.mlen = T->gdbl(r, "WLEN");
         break;

      default:
         dbg::fatal(SS("Switch - unrecognised spartype_e ") + TS((int)spr.mytype));
      }

      if (spr.mytype == spartype_e::boxspar) {
         // Add two new spars which are actually the webs
         // Calculate offset for front and rear web spar lines
         line webLn(coord_t{ spr.stX, spr.stY }, coord_t{ spr.enX, spr.enY });    // Box spar centre line
         double yoffset = ((spr.spW + spr.wThck) / 2.0) / cos(webLn.angle()); // Amount to move in y to centre line on the web

         spr.index = spars.size() + 1;
         spars.push_back(spr);

         Spar web = spr;
         web.typeTxt = std::string("BOX SPAR WEBS (F)");
         web.stY = spr.stY + yoffset;
         web.enY = spr.enY + yoffset;
         web.slotDepthPercent = 0.0;
         web.widenSlots = false;
         web.mytype = spartype_e::webs;
         web.index = spars.size() + 1;
         web.markspace = Spar::BOXSPAR_MARKSPACE;
         spars.push_back(web);

         web.stY = spr.stY - yoffset;
         web.enY = spr.enY - yoffset;
         web.index = spars.size() + 1;
         web.typeTxt = std::string("BOX SPAR WEBS (R)");
         spars.push_back(web);
      }
      else if (spr.mytype == spartype_e::hspar) {
         spr.index = spars.size() + 1;
         spars.push_back(spr);

         Spar web = spr;
         web.typeTxt = std::string("H-SPAR WEBS");
         web.mytype = spartype_e::webs;
         web.slotDepthPercent = 0.0;
         web.widenSlots = false;
         web.index = spars.size() + 1;
         web.markspace = Spar::HSPAR_MARKSPACE;
         spars.push_back(web);
      }
      else if (spr.mytype == spartype_e::hsspar) {
         spr.index = spars.size() + 1;
         spars.push_back(spr);

         Spar web = spr;
         line webLn(coord_t{ spr.stX, spr.stY }, coord_t{ spr.enX, spr.enY }); // Box spar centre line

         double yoffset = ((spr.spW - spr.wThck) / 2.0) / cos(webLn.angle()); // Amount to move in y to centre line on the sheet web
         if (T->gqst(r, "WPOS") == QString("Rear")) {
            yoffset = -yoffset;
         }
         else if (T->gqst(r, "WPOS") == QString("Centre")) {
            yoffset = 0.0;
         }

         web.stY = spr.stY + yoffset;
         web.enY = spr.enY + yoffset;
         web.spW = spr.wThck;
         web.typeTxt = std::string("H-SHEET SPAR");
         web.mytype = spartype_e::websslotted;
         web.slotDepthPercent = T->gdbl(r, "SLOTDEPTH");
         web.fe = pivot_e::CENTRE;
         web.lhbw = 0.0;
         web.mhl = 0.0;
         web.tabsNotSlots = false;
         web.height = 0.0;
         web.tabpc = 0.0;
         web.inFromBelow = false;
         web.widenSlots = true;
         web.index = spars.size() + 1;
         web.markspace = Spar::HSPAR_MARKSPACE;
         spars.push_back(web);
      }
      else if (spr.mytype == spartype_e::ribtabs) {
         spr.markspace = Spar::RIBTAB_MARKSPACE;
         spr.index = spars.size() + 1;
         spars.push_back(spr);
      }
      else {
         spr.index = spars.size() + 1;
         spars.push_back(spr);
      }
   }

   return true;
}

bool Spar_set::create(Rib_set& ribs, std::string& log) {
   for (spar_iter it = begin(); it != end(); it++) {
      DBGLVL1("Creating Spar Type %d index %d", (int)it->mytype, sparItInd(it));
      if (!it->create(ribs, log))
         return false;
   }
   return true;
}

bool Spar_set::addCreateJigsType2(GenericTab* T, Rib_set& ribs, std::string& log) {
   for (int r = 0; r < T->GetNumParts(); r++) {
      if (T->gqst(r, "meta") == QString("Jig Spar")) {
         Spar spr;
         spr.stX = T->gdbl(r, "LESTX");
         spr.stY = T->gdbl(r, "LESTY");
         spr.enX = T->gdbl(r, "LEENX");
         spr.enY = T->gdbl(r, "LEENY");
         spr.objLn.set(coord_t{ spr.stX, spr.stY }, coord_t{ spr.enX, spr.enY });

         // Ignore it if zero or short length
         if (spr.objLn.len() < SNAP_LEN) {
            continue;
         }
         spr.objLn.extend_mm(Spar::REFLN_EXT_mm);

         // Miscellaneous attributes
         spr.mytype = spartype_e::sheetJigType2;
         spr.spD = T->gdbl(r, "HEIGHT");
         spr.spW = T->gdbl(r, "THK");

         // Find all of the rib intersects
         DBGLVL1("Sheeting jig spar %d...", spr.index);
         spr.sparRibIntersect(ribs, log);

         // Remove those that have no jig
         auto nj = spr.iss.begin();
         while (nj != spr.iss.end()) {
            if (nj->rib->jig) {
               ++nj;
            }
            else {
               spr.iss.erase(nj);
               nj = spr.iss.begin();
            }
         }

         // Need at least two intersects left to form a spar
         if (spr.iss.size() < 2) {
            log.append(SS("Sheeting jig type spar ") + TS(spr.index) + "must intersect at least two ribs to be rendered\n");
            continue;
         }

         // Draw a blank spar
         obj& p = spr.getPart();
         auto ists = spr.iss.begin();
         ists->posSpr = spr.planToXpos(ists->intersect);
         double xSt = ists->posSpr - (ists->wRib / 2.0) - 10.0;
         auto iste = std::prev(spr.iss.end());
         iste->posSpr = spr.planToXpos(iste->intersect);
         double xEn = iste->posSpr + (iste->wRib / 2.0) + 10.0;

         p.add(xSt, 0, xSt, spr.spD);        // Vertical line at start
         p.add(xSt, spr.spD, xEn, spr.spD);  // Top line
         p.add(xEn, spr.spD, xEn, 0);        // Vertical line at end
         p.add(xEn, 0, xSt, 0);              // Bottom line

         // Cut slots in affected jigs and the spar
         for (auto ist = spr.iss.begin(); ist != spr.iss.end(); ++ist) {
            double cutHeight = (spr.spD / 2.0) + 0.3;
            double jigSlotW = slotWidth(spr.objLn, ist->rib->objLn, spr.spW, ist->rib->jig_thck);
            double sprSlotW = slotWidth(ist->rib->objLn, spr.objLn, ist->rib->jig_thck, spr.spW);
            spr.cutStripSparSlot(ist->intersect, true, sprSlotW, cutHeight, log);
            ist->rib->cutStripSparSlot(ist->intersect, false, jigSlotW, cutHeight, log, Rib::botjig);
            ist->rib->cutStripSparSlot(ist->intersect, false, jigSlotW, cutHeight, log, Rib::topjig);
         }

         // We need two copies of it (top and bottom jig)
         spr.getRawPart().copy_from(p);
         spr.index = r + 1;
         spr.typeTxt.append("SHEETING JIG SPAR BOT");
         spr.createPartText();
         spars.push_back(spr);
         spr.typeTxt.append("SHEETING JIG SPAR TOP");
         spr.createPartText();
         spars.push_back(spr);
      }
   }
   return true;
}

obj& Spar_set::getPlan() {
   plan.del();
   for (auto& it : spars)
      plan.copy_from(it.getPlan());

   return plan;
}

void Spar_set::getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts) {
   for (auto& r : spars) {
      obj& p = r.getPrettyPart();
      if (!p.empty()) {
         objects.push_back(p);
         texts.push_back(r.getPartText());
      }
   }
}
