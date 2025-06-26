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

#include <cmath>
#include <exception>
#include <iterator>

#include "part.h"

bool Keepout::isInKeepout(double val, double offset) {
   return ((val >= (l - offset)) && (val <= (r + offset)));
}

bool Keepout::isInKeepout(Keepout& kp, double offset) {
   return (isInKeepout(kp.l, offset) || isInKeepout(kp.r, offset));
}

void Keepout::set(double left, double right) {
   l = left;
   r = right;
}

void Keepout::set(const obj& o) {
   if (!o.empty()) {
      l = o.find_extremity(LEFT);
      r = o.find_extremity(RIGHT);
   }
}

bool Part::isInSparKeepout(double val, double offset) {
   for (auto& it : sparKpos) {
      if (it.isInKeepout(val, offset))
         return true;
   }

   return false;
}

bool Part::isInUserKeepout(double val, double offset) {
   for (auto& it : userKpos) {
      if (it.isInKeepout(val, offset))
         return true;
   }

   return false;
}

bool Part::isInAutoKeepout(double val, double offset) {
   for (auto& it : autoKpos) {
      if (it.isInKeepout(val, offset))
         return true;
   }

   return false;
}

bool Part::isInKeepout(double val, double offset) {
   return (isInUserKeepout(val, offset) || isInAutoKeepout(val, offset));
}

obj& Part::addRole(int role, bool isUserRole) {
   if (isUserRole && (role > MAX_USER_ROLE))
      dbg::fatal(std::string("Exceeded maximum USER ROLE value "));

   o.emplace(role, obj{});
   return o.at(role);
}

obj& Part::getRole(int role) {
   if (!o.count(role))
      o.emplace(role, obj{});

   return o.at(role);
}

obj& Part::getRawPart() {
   if (!o.count(RAWPART))
      o.emplace(RAWPART, obj{});

   return o.at(RAWPART);
}

obj& Part::getPlan() {
   if (!o.count(PLFM))
      o.emplace(PLFM, obj{});

   return o.at(PLFM);
}

obj& Part::getPart() {
   if (!o.count(PART))
      o.emplace(PART, obj{});

   return o.at(PART);
}

obj& Part::getPartText() {
   if (!o.count(PARTTEXT))
      o.emplace(PARTTEXT, obj{});

   return o.at(PARTTEXT);
}

obj& Part::getPrettyPart(int role) {
   if (!o.count(PRETTYPART))
      o.emplace(PRETTYPART, obj{});

   if (!o.count(role))
      o.emplace(role, obj{});

   obj& pp = o.at(PRETTYPART);
   obj& p = o.at(role);
   pp.del();
   if (!p.empty()) {
      // Take a copy of the part into pretty part, do choord splitting if required
      pp.copy_from(p);

      if (splitAtChoord) {
         obj top = {};
         obj bot = {};
         pp.split_along_line_rejoin(line(coord_t{ 0.0, 0.0 }, vector_t{ 1.0, 0.0 }), top, bot, NULL);
         top.add_offset(0.0, 5.0);
         pp.del();
         pp.copy_from(top);
         pp.copy_from(bot);
      }
   }

   return pp;
}

void Part::createPartText(const char* typeTxtOverride) {
   std::string finalTypeText = strlen(typeTxtOverride) ? SS(typeTxtOverride) : typeTxt;

   asciivec tp(5.0);
   char str[256];
   obj& pt = getPartText();
   pt.del();

   if (!finalTypeText.empty()) {
      obj name = {};
      snprintf(str, 256, "%s %2d", finalTypeText.c_str(), index);
      tp.add(name, coord_t{ 0.0, 10 }, str);
      pt.splice(name);
   }

   if (!notes.empty() && !strlen(typeTxtOverride)) {
      obj note = {};
      if (notes.size() > 39)
         snprintf(str, 40, "%s...", notes.c_str());
      else
         snprintf(str, 40, "%s", notes.c_str());
      tp.add(note, coord_t{ 0.0, 0.0 }, str);
      pt.splice(note);
   }
}

void Part::splitAtY(double y, int role) {
   obj& p = getRole(role);

   obj top = {};
   obj bot = {};
   p.split_along_line_rejoin(line(coord_t{ 0.0, y }, vector_t{ 1.0, 0.0 }), top, bot, NULL);
   top.add_offset(0.0, SPLIT_SEPARATION);
   p.del();
   p.copy_from(top);
   p.copy_from(bot);
}

void Part::splitAtX(double x, int role) {
   obj& p = getRole(role);

   obj lft = {};
   obj rgt = {};
   p.split_along_line_rejoin(line(coord_t{ x, 0.0 }, vector_t{ 0.0, 1.0 }), lft, rgt, NULL);
   rgt.add_offset(SPLIT_SEPARATION, 0.0);
   p.del();
   p.copy_from(rgt);
   p.copy_from(lft);
}

void Part::maxLenSplitX(double maxlen, int role) {
   obj& p = getRole(role);

   if (p.empty())
      return;
   if (maxlen == 0.0)
      return;

   // Find the initial extremities and length
   double ext[4];
   p.find_extremity(ext);
   double l = ext[RIGHT] - ext[LEFT];
   if (l == 0.0)
      return;

   // Find the number of segments
   int numsegs = (int)ceil(l / maxlen);
   double seglen = l / numsegs;

   // Split along boundaries at the edge of segments
   for (int seg = 1; seg < numsegs; ++seg) {
      obj lft = {};
      obj rgt = {};
      double xSplit = (seg * seglen) + ((seg - 1) * SPLIT_SEPARATION) + ext[LEFT];
      splitAtX(xSplit, role);
   }
}

bool Part::addHoles(std::string& log) {
   if (lhbw == 0.0)
      return true;

   DBGLVL1("Adding holes to part %d, lhbw=%lf mhl=%lf", index, lhbw, mhl);
   DBGLVL2("Keepout list lengths Auto: %zu  Manual: %zu", autoKpos.size(), userKpos.size());

   if (mhl <= (2.0 * lhbw)) {
      log.append(typeTxt + TS(index) + ": maximum hole length should be at least double the border width\n");
      return true;
   }

   // Find where valid holes can live by working across the part in x-axis steps, checking that
   //  1. there is no keepout in place
   //  2. the part outline exists
   //  3. there is enough part for a hole to be created
   // Rather than project an offset perpendicular to the part outline, an approximation is used
   // where the vertical offset is increased based on the deviation of the part outline from horizontal.
   bool x_in_hole = false;
   coord_t upper, lower, toppt = { 0, 0 }, botpt = { 0, 0 };
   line_iter upperln, lowerln;
   Hole hl;
   std::list<Hole> holes;
   obj hobj;
   size_t lastpr = 0;
   obj& p = getPart();
   obj& r = getRawPart();
   double leftlimit = p.find_extremity(LEFT);
   double rghtlimit = p.find_extremity(RIGHT);

   for (double x = leftlimit; x < rghtlimit; x = x + H_STEP) {
      if (!x_in_hole) {
         if (isInKeepout(x, lhbw)) { // Move on if x is in a keepout
            if (lastpr != 1) {
               DBGLVL2("%lf in keepout", x);
               lastpr = 1;
            }
            continue;
         }
         if (!r.top_bot_intersect(x, &upper, &lower, upperln, lowerln)) { // Move on if no part outline at x
            if (lastpr != 2) {
               DBGLVL2("%lf no rib outline", x);
               lastpr = 2;
            }
            continue;
         }
         if ((upperln->is_vertical()) || (lowerln->is_vertical())) { // Move on if part outline vertical
            if (lastpr != 3) {
               DBGLVL2("%lf outline is vertical", x);
               lastpr = 3;
            }
            continue;
         }
         double upperoffset = lhbw / fabs(cos(upperln->angle())); // Correct for the local slope of rib outline
         double loweroffset = lhbw / fabs(cos(lowerln->angle()));
         if ((upper.y - lower.y) <= (upperoffset + loweroffset)) { // Move on if not enough part at x for a hole
            if (lastpr != 4) {
               DBGLVL2("%lf rib not deep enough for hole", x);
               lastpr = 4;
            }
            continue;
         }

         // Start a new hole here
         x_in_hole = true;
         hl.l = x;
         DBGLVL2("%lf starting a new hole", x);
         lastpr = 0;
      }
      else //(x_in_hole)
      {
         if (isInKeepout(x, lhbw))
            DBGLVL2("%lf finishing hole - in keepout", x);
         else if (!r.top_bot_intersect(x, &upper, &lower, upperln, lowerln))
            DBGLVL2("%lf finishing hole - no rib outline", x);
         else if (upperln->is_vertical() || lowerln->is_vertical())
            DBGLVL2("%lf finishing hole - outline has a vertical", x);
         else {
            double upperoffset = lhbw / fabs(cos(upperln->angle())); // Correct for the local slope of part outline
            double loweroffset = lhbw / fabs(cos(lowerln->angle()));
            if ((upper.y - lower.y) > (upperoffset + loweroffset))
               continue; // Hole is still valid, so continue on
            DBGLVL2("%lf finishing hole - not enough height in rib", x);
         }

         // Hole is no longer valid; finish it at the previous step
         x_in_hole = false;
         hl.r = x - H_STEP;

         // Ditch the hole if it is not long enough
         if ((hl.r - hl.l) < MIN_HOLE_LENGTH) {
            DBGLVL2("    hole is too short, skipping");
            continue;
         }

         // Segment into multiple holes based on the length
         size_t numsegs = (size_t)ceil((hl.r - hl.l) / mhl);
         double segwidth = (hl.r - hl.l) / numsegs;
         for (size_t hls = 1; hls <= numsegs; hls++) {
            Hole seg;

            if (hls == 1)
               seg.l = hl.l;
            else
               seg.l = hl.l + (segwidth * (double)(hls - 1)) + (lhbw / 2.0);

            if (hls == numsegs)
               seg.r = hl.r;
            else
               seg.r = hl.l + (segwidth * (double)hls) - (lhbw / 2.0);

            DBGLVL2("   Segment hole %lf <-> %lf", seg.l, seg.r);
            holes.emplace_back(seg);
         }
      }
   }

   // Draw the holes
   for (std::list<Hole>::iterator hli = holes.begin(); hli != holes.end(); ++hli) {
      for (double x = hli->l; x <= hli->r; x = x + H_STEP) {
         if (!r.top_bot_intersect(x, &upper, &lower, upperln, lowerln))
            dbg::fatal(SS("Failed to find intersect whilst adding holes to rib ") + TS(index));
         double upperoffset = lhbw / fabs(cos(upperln->angle())); // Correct for the local slope of part outline
         double loweroffset = lhbw / fabs(cos(lowerln->angle()));

         if (x == hli->l) {
            // Start new hole
            toppt = coord_t{ x, upper.y - upperoffset };
            botpt = coord_t{ x, lower.y + loweroffset };

            if ((upper.y - lower.y) > (upperoffset + loweroffset + (2.0 * H_CORNER_SIZE))) {
               toppt.y = toppt.y - H_CORNER_SIZE; // Room for an angled corner
               botpt.y = botpt.y + H_CORNER_SIZE;
               x = x + (H_CORNER_SIZE * H_STEP);
            }
            hobj.add(toppt, botpt);
         }
         else {
            // Continue the hole from previous top and bottom points
            coord_t newtoppt = coord_t{ x, upper.y - upperoffset };
            coord_t newbotpt = coord_t{ x, lower.y + loweroffset };
            hobj.add(toppt, newtoppt);
            hobj.add(botpt, newbotpt);
            toppt = newtoppt;
            botpt = newbotpt;
            double lookahead = (x + (H_CORNER_SIZE * H_STEP));
            if (lookahead >= hli->r) {
               if (r.top_bot_intersect(lookahead, &upper, &lower, upperln, lowerln)) {
                  double upperoffset = lhbw / fabs(cos(upperln->angle()));
                  double loweroffset = lhbw / fabs(cos(lowerln->angle()));
                  if ((upper.y - lower.y) > (upperoffset + loweroffset + (2.0 * H_CORNER_SIZE))) {
                     newtoppt = coord_t{ lookahead, upper.y - upperoffset - H_CORNER_SIZE };
                     newbotpt = coord_t{ lookahead, lower.y + loweroffset + H_CORNER_SIZE };
                     hobj.add(toppt, newtoppt);
                     hobj.add(botpt, newbotpt);
                     toppt = newtoppt;
                     botpt = newbotpt;
                     break;
                  }
               }
            }
         }
      }

      // Close the hole
      hobj.add(toppt, botpt);
   }

   // Add the hole to the rib
   hobj.regularise();
   hobj.simplify(0.2); // Holes can be pretty approximate
   p.splice(hobj);

   return true;
}

bool Part::cutSlot(
   coord_t planIsect,
   bool topFlag,
   bool sheetSlot,
   bool snapOutline,
   double yAtBottom,
   double width,
   double leanAngle,
   std::string& log,
   int role) {
   obj& p = getRole(role);
   p.make_path();

   DBGLVL1("%s %d", typeTxt.c_str(), index);

   // Find the x position along the part
   coord_t refPt = { 0.0, 0.0 };
   if (!planToXpos(planIsect, refPt.x)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " based on reference line, slot requested at plan point" + TScoord(planIsect) + "is outside of the part outline.\n");
      return false;
   }

   // Find the y position in the part and the reference line where appropriate
   // For sheet slots, the reference point is on the chord line (y = 0.0)
   // Otherwise, the reference point is on the outline
   line_iter refLn;
   if (sheetSlot)
      refPt.y = 0.0;
   else if (!(topFlag ? p.top_intersect(refPt.x, &refPt, refLn) : p.bot_intersect(refPt.x, &refPt, refLn))) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " unable to determine the y position for the slot requested at plan point " + TScoord(planIsect) + "\n");
      return false;
   }

   DBGLVL1("Reference point %s  Y at bottom of slot %.2lf", refPt.prstr(), yAtBottom);

   // Update the lean angle if snap-to-outline is selected
   if (sheetSlot && snapOutline) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " Slot requested at plan point" + TScoord(planIsect) + "is both a sheet-slot and a snap-to-outline; this is not allowed\n");
      return false;
   }
   if (snapOutline) {
      if (topFlag)
         leanAngle = refLn->angle();
      else
         leanAngle = refLn->angle() + M_PI;
   }

   // Create the three slot line references (left, centre of the slot and right)
   SlotReference slRef[SLOT_R + 1];
   for (int i = SLOT_L; i <= SLOT_R; i++) {
      // Reference line
      slRef[i].ln.set(refPt, 1.0, N_x_NINETYDEG(1.0) + leanAngle);
      slRef[i].ln.move_sideways((double)(1 - i) * (width / 2.0));

      // Find the intersects in the part outline and redraw the reference between them
      std::list<obj_line_intersect> isects = {};
      p.line_intersect(slRef[i].ln, &isects, 1);

      // Need at least one intersect to do anything
      if (isects.size() == 0) {
         log.append(
            typeTxt + SS(" part index: ") + TS(index) + " slot side index: " + TS(i) + " no outline intersect found for slot requested at plan point" + TScoord(planIsect) + "; most likely you have a partial overlap of two parts at this point.\n");
         return false;
      }

      // If we only have one intersect...
      if (isects.size() == 1) {
         // ...we can't do a sheet slot
         if (sheetSlot) {
            log.append(
               typeTxt + SS(" part index: ") + TS(index) + " slot side index: " + TS(i) + " unable to find two part intersects for sheet-slot requested at plan point" + TScoord(planIsect) + "; most likely you have a partial overlap of two parts at this point.\n");
            return false;
         }
         // ...but we can do a normal slot
         {
            if (topFlag) {
               // Move reference line to finish at the intersect point
               coord_t isPt = isects.front().pt;
               slRef[i].ln.set(
                  coord_t{ isPt.x - slRef[i].ln.get_V().dx, isPt.y - slRef[i].ln.get_V().dy },
                  slRef[i].ln.get_V());
            }
            else {
               // Move the reference line to start at the intersect point
               slRef[i].ln.set(isects.front().pt, slRef[i].ln.get_V());
            }
         }
      }

      // Can do anything with two intersects
      if (isects.size() >= 2) {
         slRef[i].ln.set(isects.front().pt, isects.back().pt);
      }

      // Set the intersected lines in the part
      slRef[i].isect_ln_bot = isects.front().ln;
      slRef[i].isect_ln_top = isects.back().ln;
      DBGLVL2("%s: slref[%d]: %s", __func__, i, slRef[i].ln.print_str());
   }

   // Create the bottom of the slot
   line slotBottomLn = {};
   {
      coord_t centPt = slRef[SLOT_C].ln.get_pt(slRef[SLOT_C].ln.T_for_y(yAtBottom));
      coord_t leftPt = centPt;
      coord_t righPt = centPt;
      leftPt.x += -width / 2.0;
      righPt.x += +width / 2.0;
      slotBottomLn.set(leftPt, righPt);
      slotBottomLn.rotate(centPt, leanAngle);
   }

   // Open gap(s) in the outline and draw slot
   obj slot = {};
   //// Check if any of the slot bottom is outside the part
   //if (sheetSlot &&
   //   ((!p.surrounds_point(slotBottomLn.get_S0())) || (!p.surrounds_point(slotBottomLn.get_S1()))))
   // Check if the centre of the slot bottom is outside the part
   if (sheetSlot && (!p.surrounds_point(slotBottomLn.get_pt(T_CENTER)))) {
      // It is, so the slot is full depth
      openGap(p,
         slRef[SLOT_L].ln.get_S1(),
         slRef[SLOT_L].isect_ln_top,
         slRef[SLOT_R].ln.get_S1(),
         slRef[SLOT_R].isect_ln_top);
      openGap(p,
         slRef[SLOT_R].ln.get_S0(),
         slRef[SLOT_R].isect_ln_bot,
         slRef[SLOT_L].ln.get_S0(),
         slRef[SLOT_L].isect_ln_bot);
      slot.add(slRef[SLOT_L].ln.get_S0(), slRef[SLOT_L].ln.get_S1());
      slot.add(slRef[SLOT_R].ln.get_S0(), slRef[SLOT_R].ln.get_S1());
   }
   // Not a full depth slot
   else {
      if (topFlag) {
         openGap(p,
            slRef[SLOT_L].ln.get_S1(),
            slRef[SLOT_L].isect_ln_top,
            slRef[SLOT_R].ln.get_S1(),
            slRef[SLOT_R].isect_ln_top);
         slot.add(slotBottomLn);
         slot.add(slotBottomLn.get_S0(), slRef[SLOT_L].ln.get_S1());
         slot.add(slotBottomLn.get_S1(), slRef[SLOT_R].ln.get_S1());
      }
      else {
         openGap(p,
            slRef[SLOT_R].ln.get_S0(),
            slRef[SLOT_R].isect_ln_bot,
            slRef[SLOT_L].ln.get_S0(),
            slRef[SLOT_L].isect_ln_bot);
         slot.add(slotBottomLn);
         slot.add(slotBottomLn.get_S0(), slRef[SLOT_L].ln.get_S0());
         slot.add(slotBottomLn.get_S1(), slRef[SLOT_R].ln.get_S0());
      }
   }

   // Add a keepout
   if (sheetSlot) {
      // For a sheet slot, we must include the entirety if the angled region
      // not just the slot itself
      obj region = {};
      region.add(slRef[SLOT_L].ln);
      region.add(slRef[SLOT_R].ln);
      autoKpos.emplace_back(region);
      sparKpos.emplace_back(region);
   }
   else {
      autoKpos.emplace_back(slot);
      sparKpos.emplace_back(slot);
   }

   // Add the slot to the part
   p.copy_from(slot);
   p.make_path();
   return true;
}

bool Part::cutTabSlot(
   coord_t planIsect,
   double width,
   double percentDepth,
   double leanAngle,
   std::string& log,
   int role) {
   obj& p = getRole(role);

   // Find the x position along the part
   double x = 0;
   if (!planToXpos(planIsect, x)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " based on reference line, tab slot requested at plan point" + TScoord(planIsect) + "is outside of the part outline.\n");
      return false;
   }

   // Find the intersects with the outline
   coord_t top, bot;
   if (!p.top_bot_intersect(x, &top, &bot)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " unable to find a top and bottom intersect to determine slot bottom for slot requested at plan point" + TScoord(planIsect) + "\n");
      return false;
   }

   // Create a rectangle for the tab slot
   double hw = width / 2.0;
   double hh = ((percentDepth / 100.0) * (top.y - bot.y)) / 2.0;
   double y = (top.y + bot.y) / 2.0;
   coord_t topl = { (x - hw), (y + hh) };
   coord_t botr = { (x + hw), (y - hh) };

   obj slot = {};
   slot.add_rect(topl, botr);
   slot.rotate(coord_t{ x, y }, leanAngle);
   p.copy_from(slot);

   // Keepout
   autoKpos.emplace_back(slot);

   return true;
}

bool Part::_cutStripSparSlot(
   coord_t planIsect,
   bool topFlag,
   bool snapOutline,
   double width,
   double depth,
   std::string& log,
   int role) {
   obj& p = getRole(role);

   // Find the x position along the part
   double x = 0;
   if (!planToXpos(planIsect, x)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " based on reference line, slot requested at plan point" + TScoord(planIsect) + "is outside of the part outline.\n");
      return false;
   }

   //// Check it isn't in a keepout
   //if (isInAutoKeepout(x))
   //   return false;

   // Find the intersects with the outline
   coord_t top, bot;
   if (!p.top_bot_intersect(x, &top, &bot)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " unable to find a top and bottom intersect to determine slot bottom for slot requested at plan point" + TScoord(planIsect) + "\n");
      return false;
   }

   double y = topFlag ? (top.y - depth) : (bot.y + depth);

   return cutSlot(planIsect, topFlag, false, snapOutline, y, width, 0.0, log, role);
}

bool Part::cutStripSparSlot(
   coord_t planIsect,
   bool topFlag,
   double width,
   double depth,
   std::string& log,
   int role) {
   return _cutStripSparSlot(planIsect, topFlag, false, width, depth, log, role);
}

bool Part::cutSnappedStripSparSlot(
   coord_t planIsect,
   bool topFlag,
   double width,
   double depth,
   std::string& log,
   int role) {
   return _cutStripSparSlot(planIsect, topFlag, true, width, depth, log, role);
}

bool Part::cutSheetStyleSlot(
   coord_t planIsect,
   bool topFlag,
   bool botFlag,
   double width,
   double percentDepth,
   double leanAngle,
   pivot_e removeMaterial,
   std::string& log,
   int role) {
   obj& p = getRole(role);

   // Find the x position along the part
   double x = 0;
   if (!planToXpos(planIsect, x)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " based on reference line, slot requested at plan point" + TScoord(planIsect) + "is outside of the part outline.\n");
      return false;
   }

   // Find the intersects with the outline
   coord_t top, bot;
   if (!p.top_bot_intersect(x, &top, &bot)) {
      log.append(
         typeTxt + SS(" part index: ") + TS(index) + " unable to find a top and bottom intersect to determine slot bottom for slot requested at plan point" + TScoord(planIsect) + "\n");
      return false;
   }

   // Find slot bottoms
   linvar rDepthTop(0.0, top.y, 100.0, bot.y);
   linvar rDepthBot(0.0, bot.y, 100.0, top.y);
   double yTop = rDepthTop.v(percentDepth);
   double yBot = rDepthBot.v(percentDepth);

   // Make the slot
   bool retbool = true;
   if (topFlag)
      retbool &= cutSlot(planIsect, true, true, false, yTop, width, leanAngle, log, role);
   if (botFlag)
      retbool &= cutSlot(planIsect, false, true, false, yBot, width, leanAngle, log, role);

   // Trim the part
   if (removeMaterial == LE)
      retbool &= p.remove_extremity_rejoin(x + ((width - OVC) / 2.0), RIGHT);
   else if (removeMaterial == TE)
      retbool &= p.remove_extremity_rejoin(x - ((width - OVC) / 2.0), LEFT);

   return retbool;
}

double Part::planToXpos(coord_t planPt) {
   double x;
   planToXpos(planPt, x);
   return x;
}

bool Part::planToXpos(coord_t planPt, double& xpos) {
   // Find the x position along the part
   double refLnT = refLn.T_for_pt(planPt);
   if ((refLnT < 0.0) || (refLnT > 1.0)) // T must be between 0 and one if slot is in part
   {
      xpos = NAN;
      return false;
   }
   xpos = distTwoPoints(planPt, refLn.get_S0());
   return true;
}

void Part::redrawObjLine() {
   double te_c = getPart().find_extremity(LEFT) / refLn.len();
   double le_c = getPart().find_extremity(RIGHT) / refLn.len();
   objLn.set(refLn.get_pt(te_c), refLn.get_pt(le_c));
}

void Part::openGap(int role, coord_t firstPt, line_iter l0, coord_t seconPt, line_iter l1) {
   obj& p = getRole(role);
   openGap(p, firstPt, l0, seconPt, l1);
}

void Part::openGap(coord_t firstPt, line_iter firstLn, coord_t seconPt, line_iter seconLn) {
   openGap(PART, firstPt, firstLn, seconPt, seconLn);
}

void Part::openGap(obj& p, coord_t firstPt, line_iter l0, coord_t seconPt, line_iter l1) {
   // Delete any whole segments bridging the gap
   if (l0 != l1) {
      line_iter ln = p.nextc(l0);
      while (ln != l1) {
         line_iter nxt = p.nextc(ln);
         p.del(ln);
         ln = nxt;
      }
   }

   // Create the partial segments to pt0 and from pt1
   if (l0 == l1) {
      // pt0 and pt1 are in the same segment
      coord_t s1 = l0->get_S1();

      // Shorten segment to reach only pt0
      l0->set(l0->get_S0(), firstPt);

      // Insert new segment from pt1
      l1 = p.add(line(seconPt, s1));
   }
   else {
      // Shorten to reach only their respective points
      l0->set(l0->get_S0(), firstPt);
      l1->set(seconPt, l1->get_S1());
   }
}

void Part::trimByAutoKeepouts(double margin, int role) {
   DBGLVL2("Keepout list size %zd", autoKpos.size());

   // Can't do anything if there are no keepouts
   if (autoKpos.size() == 0)
      return;

   // Find the extremes of the part based on its keepout list and the margin
   double left = HUGE;
   double righ = -HUGE;
   for (auto& kpo : autoKpos) {
      left = (kpo.l < left) ? kpo.l : left;
      righ = (kpo.l > righ) ? kpo.r : righ;
   }
   left = left - margin;
   righ = righ + margin;

   // Truncate
   obj& p = getRole(role);
   p.remove_extremity_rejoin(left, LEFT);
   p.remove_extremity_rejoin(righ, RIGHT);

   DBGLVL1("Left %.2lf  Right %.2lf", left, righ);
}
