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
#include "former.h"

#include <QCoreApplication>

#include <array>
#include <cmath>
#include <list>
#include <vector>

LiteEngine::LiteEngine(
   double rimSpacing,
   double outerWidth,
   double innerWidth,
   double girderWidth,
   double anchorSpan,
   double minAngle,
   double hSplitY,
   direction_e startAtDir,
   mode_e mode,
   QProgressBar* prog) : rs(rimSpacing),
   ow(outerWidth),
   iw(innerWidth),
   gw(girderWidth),
   as(anchorSpan),
   ma(TO_RADS(minAngle)),
   sp(hSplitY),
   mode(mode),
   di(startAtDir),
   progressBar(prog) {
};

bool LiteEngine::run(obj& in,
   obj& out,
   bool lighten,
   bool notchDetect,
   bool girder,
   bool showConstruction,
   bool anchorAtNotches,
   bool hSplit,
   bool vSplit) {
   out.del();

   // Take a copy of the input and tidy up
   inp = in;
   inp.regularise();
   if (inp.size() <= 2) {
      dbg::alert("This does not appear to be a useful shape!");
      out.copy_from(inp);
      return false;
   }
   DBGLVL2("Object sp at %s", inp.get_sp().prstr());

   // Create a version without notches
   createOuterRimOuter(notchDetect);
   progress();

   if (girder)
      createOuterRimInner();
   progress();

   if (lighten) {
      if (girder)
         createInnerRimOuter(ori); // Need to use the inner to check for crossings
      else
         createInnerRimOuter(oro);
   }
   progress();

   if (girder)
      createInnerRimInner(iw);
   else
      createInnerRimInner(); // For straight lightening there is no further offset
   progress();

   if (girder) {
      if (generateAnchorPoints(anchorAtNotches)) {
         progress();
         if (generateBraces()) {
            progress();
            invalidateNarrowBracePairs();
            invalidateCrossingBraces();
            drawValidBraces();
            openBraceGaps(false);
            openBraceGaps(true);
         }
         else {
            progress();
         }
      }
      else {
         progress();
      }
   }
   progress();

   obj outline = oro;
   outline.copy_from(iri);

   obj inners = ori;
   inners.copy_from(iro);
   inners.copy_from(bro);
   progress();

   if (girder) {
      if (hSplit)
         girderHsplit(outline, inners, sp);
      if (vSplit)
         girderVsplit(outline, inners);
   }
   else {
      if (hSplit)
         simpleHsplit(outline);
      if (vSplit)
         simpleVsplit(outline);
   }

   out = outline;
   if (girder)
      out.copy_from(inners);

   if (showConstruction)
      out.copy_from(construct);

   return true;
}

int LiteEngine::notchDetect(obj& o) {
   notches.clear();

   // The angle at which two lines are said to form a corner
   static constexpr double detAng = TO_RADS(20.0);

   for (line_iter l = o.begin(); l != o.end(); ++l) {
      // Get the angles between this and the following 4 lines
      std::array<double, 4> angs;
      std::array<line_iter, 5> lns;
      lns[0] = l;
      for (int k = 0; k < 4; ++k) {
         lns[k + 1] = o.nextc(lns[k]);
         if ((lns[k]->len() > 0.0) && (lns[k + 1]->len() > 0.0))
            angs[k] = atan2(perpprod(lns[k]->get_V(), lns[k + 1]->get_V()), dotprod(lns[k]->get_V(), lns[k + 1]->get_V()));
         else
            angs[k] = 0.0;
      }

      // Is this a square notch?
      if ((angs[0] < -detAng) && (angs[1] > detAng) && (angs[2] > detAng) && (angs[3] < -detAng)) {
         notch_t n = {
             .notchLines{lns[1], lns[2], lns[3]},
             .beg = lns[0]->get_S1(),
             .end = lns[4]->get_S0() };
         notches.push_back(n);
      }
      else {
         // Is this a triangular notch?
         if ((angs[0] < -detAng) && (angs[1] > detAng) && (angs[2] < -detAng)) {
            notch_t n = {
                .notchLines{lns[1], lns[2]},
                .beg = lns[0]->get_S1(),
                .end = lns[3]->get_S0() };
            notches.push_back(n);
         }
      }
   }
   return (int)notches.size();
}

int LiteEngine::removeNotches(obj& o) {
   notchDetect(o);

   line_iter l = o.begin();
   auto n = notches.begin();

   while (n != notches.end()) {
      if (l == n->notchLines.front()) {
         // Found the notch
         // Work out the distance to its start
         n->distance = 0.0;
         for (auto dl = o.begin(); dl != l; ++dl)
            n->distance += dl->len();

         bool first = true;
         for (auto& nl : n->notchLines) {
            l = o.nextc(nl);
            if (first) {
               nl->set(n->beg, n->end); // Replace the first line element with the joining line
               n->notchReplacedLineIter = nl;
               n->notchReplacedLine = *nl;
               first = false;
            }
            else {
               o.del(nl); // Delete the remaining notch line elements
            }
         }
         DBGLVL2("Notch %lld replaced with line %s",
            std::distance(notches.begin(), n),
            n->notchReplacedLine.print_str());
         // Move on the notches
         n = std::next(n);
      }
      else {
         l = o.nextc(l);
      }
   }

   o.regularise(); // Necessary to get everything back in order

   // Find the notch distances - must be done after the above regularise
   int cnt = 0;
   for (auto& n : notches) {
      n.distance = 0.0;
      for (auto l = o.begin(); l != o.end(); ++l) {
         if (l != n.notchLines.front()) {
            n.distance += l->len();
         }
         else {
            n.distance += l->len() / 2.0; // Halfway along the notch replacement line
            break;
         }
      }
      DBGLVL2("Notch %d distance around object %.1lf",
         ++cnt,
         n.distance);
   }
   return (int)notches.size();
}

void LiteEngine::createOuterRimOuter(bool notchDetect) {
   oro = inp;
   non = inp;
   removeNotches(non);

   // Create an outer reference with notches removed if required
   if (notchDetect) {
      reforo = non;
      construct.copy_from(reforo);
   }
   else {
      reforo = inp;
   }
}

void LiteEngine::createOuterRimInner() {
   DBGLVL2("Trace original outline at %.1lfmm to create ori", -ow);
   ori = inp;
   ori.trace_at_offset(-ow);

   // Create an inner reference from the outer reference
   DBGLVL2("Trace reforo at %.1lfmm to create refori, set start direction and regularise", -ow);
   refori = reforo;
   refori.trace_at_offset(-ow);
   refori.startAtDirection(di);
   refori.regularise();
}

void LiteEngine::createInnerRimOuter(obj& clearanceObject) {
   double additionalSpacing = 0.0;
   // If necessary, increase the rim spacing until the inner rim
   // is clear of the clearance object
   while (true) {
      double spc = -(rs + additionalSpacing);
      DBGLVL2("Trace refo at %.1lfmm to create iro and check for clearance", spc);
      iro = reforo;
      iro.trace_at_offset(spc);
      if (!iro.obj_intersect(clearanceObject))
         break;
      additionalSpacing = additionalSpacing + 1.0;
   }
   if (additionalSpacing > 0.0)
      DBGLVL1("Additional rim spacing %.1lf required to avoid intersections", additionalSpacing);
}

void LiteEngine::createInnerRimInner(double sep) {
   DBGLVL2("Trace iro at %.1lfmm to create iri", -sep);
   iri = iro;
   iri.regularise();
   iri.trace_at_offset(-sep);
}

bool LiteEngine::generateAnchorPoints(bool anchorAtNotches) {
   bool ok = true;
   if (anchorAtNotches) {
      if (getNumNotches() == 0) {
         DBGLVL1("Unable to find any notches to anchor at, defaulting to generic placement");
         anchorAtNotches = false;
      }
      else {
         // Work through the set of detected notches.
         // For each pair, find the distance around the reference between them
         // and work out how many anchor points there should be and their separation.
         nAnchors = 0;
         for (auto nc = notches.begin(); nc != notches.end(); ++nc) {
            auto nn = std::next(nc);
            if (nn == notches.end())
               nn = notches.begin();
            double distBetween = (nc == nn) ? non.len() : nn->distance - nc->distance;
            distBetween = (distBetween < 0.0) ? distBetween + non.len() : distBetween;
            int nAnchorsBetween = (int)round(distBetween / as);
            double dAnchor = distBetween / nAnchorsBetween;
            // Layout the anchor points
            for (int k = 0; k < nAnchorsBetween; ++k) {
               double d = nc->distance + (k * dAnchor);
               double dummyT;
               line_iter dummyLi;
               anchors.emplace_back();
               anchors[nAnchors].rimPt = non.get_pt_along_length(d, dummyLi, dummyT);
               anchors[nAnchors].rimLine = *dummyLi;
               DBGLVL2("Notch %lld: Anchor %d: distance %.1lf  pt %s  line %s",
                  std::distance(notches.begin(), nc), nAnchors, d, anchors[nAnchors].rimPt.prstr(), anchors[nAnchors].rimLine.print_str());
               nAnchors++;
            }
         }
      }
   }
   if (!anchorAtNotches) {
      // Work out the number of outer anchor points and an even spacing
      // around the whole inner reference
      nAnchors = (int)round(reforo.len() / as);
      double dAnchor = refori.len() / nAnchors;
      DBGLVL1("Using %d anchor points spaced by %.1lf mm", nAnchors, dAnchor);

      for (int k = 0; k < nAnchors; ++k) // Instead of relying on std::dynarray
         anchors.emplace_back();

      // Find the point and line element each anchor on the outer reference
      for (int k = 0; k < nAnchors; ++k) {
         double cDist = k * dAnchor;
         double dummyT;
         line_iter dummyLi;
         anchors[k].rimPt = refori.get_pt_along_length(cDist, dummyLi, dummyT);
         anchors[k].rimLine = *dummyLi;
         DBGLVL2("Anchor %d: distance %.1lf  pt %s  line %s", k, cDist, anchors[k].rimPt.prstr(), anchors[k].rimLine.print_str());
      }
   }

   // For each anchor point, create two construction lines at +/- the initial
   // estimate of the brace angles
   double aBrace = atan2(as, (2.0 * rs));
   DBGLVL2("Brace angle initial estimate %.1lfdegs", TO_DEGS(aBrace));
   for (int k = 0; k < nAnchors; ++k) {
      for (int b = 0; b < 2; ++b) {
         anchors[k].brace[b].refLn.set(anchors[k].rimPt,
            1.0,
            anchors[k].rimLine.angle() - N_x_NINETYDEG(1) + (b == 0 ? -aBrace : aBrace));
      }
   }

   // For each pair of anchor points, calculate the average line between the two
   // relevant construction lines. This line points to the correct intersect with
   // the inner rim. Redraw the construction lines using the intersect.
   for (int k = 0; k < nAnchors; ++k) {
      anchor_t& a0 = anchors[k];
      anchor_t& a1 = anchors[(k + 1) % nAnchors];

      // There is a corner case where the construction lines end up colinear,
      // resulting in a zero length average line.  If this happens, it can be
      // fudged by partly rotating on of the construction lines.
      line cRefOrig;
      do {
         cRefOrig.set(averageTwoPoints(a0.brace[1].refLn.get_S0(), a1.brace[0].refLn.get_S0()),
            averageTwoPoints(a0.brace[1].refLn.get_S1(), a1.brace[0].refLn.get_S1()));
         if (cRefOrig.len() < SNAP_LEN)
            a0.brace[1].refLn.rotate(a0.brace[1].refLn.get_S0(), TO_RADS(1.0));
      } while (cRefOrig.len() < SNAP_LEN);

      DBGLVL2("Anchor %d: cRefOrig %s", k, cRefOrig.print_str());

      // Find the intersections between cRef and the inner rim outer.
      // If there is no intersection, progressively rotate the reference
      // in each direction and try again.
      std::list<obj_line_intersect> isects;
      line cRef;
      bool done = false;
      for (int offsetAngle = 0; !done; ++offsetAngle) {
         cRef = cRefOrig;
         double rAngDeg = (offsetAngle % 2) ? offsetAngle : -offsetAngle;
         cRef.rotate(cRef.get_S0(), TO_RADS(rAngDeg));
         isects.clear();
         iro.line_intersect(cRef, &isects, true);
         if (isects.size() >= 2) {
            done = true; // We have found our intersect
         }
         else if (offsetAngle >= maxRefLineRotationDegs) {
            // We have failed to find an intersect
            if (ok)
               dbg::alert(SS("Less than two intersects found between inner rim and extrapolated cRef line"));
            DBGLVL1("Failed to find two cRef intersects for anchor point %d", nAnchors);
            done = true;
            ok = false;
            cRef = cRefOrig;
         }
      }
      line constructRef = cRef;
      constructRef.extend_S1_mm(rs);
      construct.add_dotted(constructRef, 0.2, 1.2);
      DBGLVL2("Anchor %d: %u intersects, cRef %s", k, (unsigned)isects.size(), cRef.print_str());

      if (isects.size() >= 2) {
         // First intersect is the point we are interested in.
         // However, there are some shapes which might cause a reveresed reference,
         // so here we check and take the shortest. Then redraw the construction lines.
         if (line(a0.rimPt, isects.front().pt).len() <= line(a0.rimPt, isects.back().pt).len()) {
            a0.brace[1].refLn.set(a0.rimPt, isects.front().pt);
            a1.brace[0].refLn.set(a1.rimPt, isects.front().pt);
         }
         else {
            a0.brace[1].refLn.set(a0.rimPt, isects.back().pt);
            a1.brace[0].refLn.set(a1.rimPt, isects.back().pt);
         }
         construct.add_dotted(a0.brace[1].refLn, 0.2, 1.2);
         construct.add_dotted(a1.brace[0].refLn, 0.2, 1.2);
         DBGLVL2("Anchor %d: iro intersect at %s", k, isects.begin()->pt.prstr());
      }
   }
   return ok;
}

bool LiteEngine::generateBraces() {
   bool ok = true;
   for (int k = 0; k < nAnchors; ++k) {
      for (int b = 0; b < 2; ++b) {
         double aDir = (b == 0) ? 1.0 : -1.0; // Account for the direction of the brace

         for (int l = 0; l < 2; ++l) { // Each brace has two sides
            if (ok) {
               line ln = anchors[k].brace[b].refLn;
               ln.rotate((l == 0) ? ln.get_S0() : ln.get_S1(), aDir * atan2(gw, ln.len()));

               // First intersect between line and inner rim
               std::list<obj_line_intersect> iisects;
               if (!iro.line_intersect(ln, &iisects, true)) {
                  anchors[k].brace[b].isValid = false;
                  break;
               }
               ln.set(ln.get_S0(), iisects.begin()->pt);
               anchors[k].brace[b].brLine[l].iisectLn = iisects.begin()->ln;
               anchors[k].brace[b].brLine[l].iisectPt = iisects.begin()->pt;

               // First intersect between line and outer rim
               ln.reverse();
               ln.extend_S1_mm(1e4);
               std::list<obj_line_intersect> oisects;
               if (!ori.line_intersect(ln, &oisects, false)) {
                  anchors[k].brace[b].isValid = false;
                  break;
               }
               ln.set(ln.get_S0(), oisects.begin()->pt);
               anchors[k].brace[b].brLine[l].oisectLn = oisects.begin()->ln;
               anchors[k].brace[b].brLine[l].oisectPt = oisects.begin()->pt;
               ln.reverse();

               // Line is ready
               anchors[k].brace[b].brLine[l].brLn = ln;
            }
         }
      }
   }
   return ok;
}

void LiteEngine::invalidateNarrowBracePairs() {
   // Check for too-narrow-angle brace pairs
   for (int k = 0; k < nAnchors; ++k) {
      if ((anchors[k].brace[0].isValid) && (anchors[k].brace[1].isValid)) {
         line& ln0 = anchors[k].brace[0].refLn;
         line& ln1 = anchors[k].brace[1].refLn;
         if ((ln0.len() > 0.0) && (ln1.len() > 0.0)) {
            double ang = ln0.angle(ln1);
            if (std::abs(ang) < ma) {
               DBGLVL2("Anchor: %d included braces angle %.1lfdegs less than minimum allowed %.1lfdegs", k, TO_DEGS(ang), TO_DEGS(ma));
               // Invalidate the longest brace
               if (ln0.len() > ln1.len()) {
                  anchors[k].brace[0].isValid = false;
                  DBGLVL2("Anchor: %d Brace: 0 invalidated", k);
               }
               else {
                  anchors[k].brace[1].isValid = false;
                  DBGLVL2("Anchor: %d Brace: 1 invalidated", k);
               }
            }
         }
      }
   }
}

void LiteEngine::invalidateCrossingBraces() {
   // Check for brace line crossings
   for (int ko = 0; ko < nAnchors; ++ko)
      for (int bo = 0; bo < 2; ++bo)
         for (int lo = 0; lo < 2; ++lo)
            for (int ki = 0; ki < nAnchors; ++ki)
               for (int bi = 0; bi < 2; ++bi)
                  for (int li = 0; li < 2; ++li)
                     if ((ko != ki) || (bo != bi) || (lo != li)) { // Don't self compare
                        if (anchors[ko].brace[bo].isValid && anchors[ki].brace[bi].isValid) {
                           static constexpr double endShrink = 0.001;
                           line lno = anchors[ko].brace[bo].brLine[lo].brLn;
                           line lni = anchors[ki].brace[bi].brLine[li].brLn;
                           lno.set(lno.get_pt(endShrink), lno.get_pt(1 - endShrink));
                           lni.set(lni.get_pt(endShrink), lni.get_pt(1 - endShrink));
                           coord_t i;
                           if (lno.lines_intersect(lni, &i, false)) {
                              DBGLVL2("Anchor: %d Brace: %d crosses Anchor: %d Brace: %d", ko, bo, ki, bi);
                              // Invalidate the longest brace
                              if (lno.len() > lni.len()) {
                                 anchors[ko].brace[bo].isValid = false;
                                 DBGLVL2("Anchor: %d Brace: %d invalidated", ko, bo);
                              }
                              else {
                                 anchors[ki].brace[bi].isValid = false;
                                 DBGLVL2("Anchor: %d Brace: %d invalidated", ki, bi);
                              }
                           }
                        }
                     }
}

void LiteEngine::drawValidBraces() {
   for (int k = 0; k < nAnchors; ++k) // !!!!!!!!!!!!!
      for (int b = 0; b < 2; ++b) {
         if (anchors[k].brace[b].isValid) {
            bro.add(anchors[k].brace[b].brLine[0].brLn);
            bro.add(anchors[k].brace[b].brLine[1].brLn);
         }
      }
}

void LiteEngine::openBraceGaps(bool iroNotOri) {
   // Create a set of gaps from the line and points defined with the anchors
   typedef struct gap_s {
      line_iter l0;
      coord_t p0;
      line_iter l1;
      coord_t p1;
   } gap_t;
   std::list<gap_t> gaps;

   DBGLVL2("iroNotOri %d", iroNotOri);

   for (int k = 0; k < nAnchors; ++k)
      for (int b = 0; b < 2; ++b)
         if (anchors[k].brace[b].isValid) {
            if (iroNotOri) {
               gaps.push_back(gap_t{ .l0 = anchors[k].brace[b].brLine[(b + 1) % 2].iisectLn,
                                    .p0 = anchors[k].brace[b].brLine[(b + 1) % 2].iisectPt,
                                    .l1 = anchors[k].brace[b].brLine[b].iisectLn,
                                    .p1 = anchors[k].brace[b].brLine[b].iisectPt });
            }
            else {
               gaps.push_back(gap_t{ .l0 = anchors[k].brace[b].brLine[(b + 1) % 2].oisectLn,
                                    .p0 = anchors[k].brace[b].brLine[(b + 1) % 2].oisectPt,
                                    .l1 = anchors[k].brace[b].brLine[b].oisectLn,
                                    .p1 = anchors[k].brace[b].brLine[b].oisectPt });
            }
         }

   // Work through each pair of adjacent gaps and determine those which overlap
   // These overlaps cause an issue in the make_gap function and must be
   // coalesced into one gap first.
   obj& ob = iroNotOri ? iro : ori;

   for (auto g = gaps.begin(); g != gaps.end(); ++g)
      DBGLVL2("Gap %lld: %lld %s %.2lf  %lld %s %.2lf", std::distance(gaps.begin(), g), ob.index(g->l0), g->p0.prstr(), g->l0->T_for_pt(g->p0), ob.index(g->l1), g->p1.prstr(), g->l1->T_for_pt(g->p1));

   int removedCount;
   do {

      removedCount = 0;
      auto g = gaps.begin();
      while (g != gaps.end()) {
         auto n = std::next(g);
         if (n == gaps.end())
            n = gaps.begin();

         int indn0 = ob.index(n->l0);
         int indg1 = ob.index(g->l1);

         if ((indg1 > (int)ob.size() / 2) && (indn0 < (int)ob.size() / 2)) { // Opposite sides of the border
            DBGLVL2("Pass gap %lld %d %d - opposite sides", std::distance(gaps.begin(), g), indg1, indn0);
            g = std::next(g);
            continue;
         }
         else if (indn0 > indg1) { // Not overlapped by line index
            DBGLVL2("Pass gap %lld %d %d - different line indexes", std::distance(gaps.begin(), g), indg1, indn0);
            g = std::next(g);
            continue;
         }
         else if ((indn0 == indg1) && (g->l1->T_for_pt(g->p1) < g->l1->T_for_pt(n->p0))) { // Not overlapped in same line index
            DBGLVL2("Pass gap %lld %d %d - same line not overlapped", std::distance(gaps.begin(), g), indg1, indn0);
            g = std::next(g);
            continue;
         }
         DBGLVL2("Removing gap after %lld %d %d", std::distance(gaps.begin(), g), indg1, indn0);
         // construct.add(g->p0, vector_t{-15, -15});
         g->l1 = n->l1;
         g->p1 = n->p1;
         gaps.erase(n); // Don't increment
         removedCount++;
      }
   } while (removedCount);

   for (auto g = gaps.begin(); g != gaps.end(); ++g)
      DBGLVL2("Gap %lld: %lld %s %.2lf  %lld %s %.2lf", std::distance(gaps.begin(), g), ob.index(g->l0), g->p0.prstr(), g->l0->T_for_pt(g->p0), ob.index(g->l1), g->p1.prstr(), g->l1->T_for_pt(g->p1));

   // Apply the gaps
   for (auto g = gaps.begin(); g != gaps.end(); ++g)
      ob.make_gap(g->l0, g->p0, g->l1, g->p1, true);
}

void LiteEngine::simpleHsplit(obj& o) {
   obj top, bot;
   o.split_along_line_rejoin(line{ coord_t{0.0, sp}, vector_t{1.0, 0.0} }, top, bot, NULL);
   top.add_offset(0.0, 5.0);
   o.del();
   o.copy_from(top);
   o.copy_from(bot);
}

void LiteEngine::girderHsplit(obj& ol, obj& in, double spy) {
   obj discard;

   // Split outline and rejoin it
   obj topOaI, botOaI; // OaI = "Outer and Inner"
   ol.split_along_line_rejoin(line{ coord_t{0.0, spy}, vector_t{1.0, 0.0} }, topOaI, botOaI, NULL);

   // Split and rejoin inners offset by outer rim width
   obj topRem, botRem;
   in.split_along_line_rejoin(line{ coord_t{0.0, spy + ow}, vector_t{1.0, 0.0} }, topRem, discard, NULL);
   in.split_along_line_rejoin(line{ coord_t{0.0, spy - ow}, vector_t{1.0, 0.0} }, discard, botRem, NULL);

   // Rebuild Objects
   topOaI.add_offset(0.0, 5.0);
   topRem.add_offset(0.0, 5.0);

   ol = topOaI;
   ol.copy_from(botOaI);

   in = topRem;
   in.copy_from(botRem);
}

void LiteEngine::girderVsplit(obj& ol, obj& in) {
   // Rotate through 90 degrees
   ol.rotate(coord_t{ 0.0, 0.0 }, M_PI_2);
   in.rotate(coord_t{ 0.0, 0.0 }, M_PI_2);

   // Split evenly horizontally
   double x = (ol.find_extremity(UP) + ol.find_extremity(DOWN)) / 2.0;
   girderHsplit(ol, in, x);

   // Rotate back
   ol.rotate(coord_t{ 0.0, 0.0 }, -M_PI_2);
   in.rotate(coord_t{ 0.0, 0.0 }, -M_PI_2);
}

void LiteEngine::simpleVsplit(obj& o) {
   double centre = (o.find_extremity(LEFT) + o.find_extremity(RIGHT)) / 2.0;
   obj left, right;
   o.split_along_line_rejoin(line{ coord_t{centre, 0.0}, vector_t{0.0, 1.0} }, left, right, NULL);
   right.add_offset(5.0, 0.0);
   o.del();
   o.copy_from(left);
   o.copy_from(right);
}

void LiteEngine::progress() {
   if (progressBar) {
      int cval = progressBar->value();
      progressBar->setValue(cval + 1);
      progressBar->update();
      qApp->processEvents();
   }
}
