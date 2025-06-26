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
#ifndef FORMER_H
#define FORMER_H

#include "debug.h"
#include "object_oo.h"

#include <QProgressBar>
#include <array>
#include <list>

class LiteEngine {
public:
   enum mode_e {
      FORMER,
      RIB
   };

   LiteEngine(
      double rimSpacing,
      double outerWidth,
      double innerWidth,
      double girderWidth,
      double anchorSpan,
      double minAngle,
      double hSplitY,
      direction_e startDir,
      mode_e mode,
      QProgressBar* prog = NULL);

   bool run(obj& in,
      obj& out,
      bool lighten,
      bool notchDetect,
      bool girder,
      bool showConstruction,
      bool anchorAtNotches,
      bool hSplit,
      bool vSplit);

   static constexpr int progressBarSteps = 8;

protected:
   void createOuterRimOuter(bool notchDetect);
   void createOuterRimInner();
   void createInnerRimOuter(obj& clearanceObject);
   void createInnerRimInner(double sep = 0.0);
   bool generateAnchorPoints(bool anchorAtNotches);
   bool generateBraces();
   void invalidateNarrowBracePairs();
   void invalidateCrossingBraces();
   void drawValidBraces();
   void openBraceGaps(bool iroNotOri);
   void simpleHsplit(obj& o);
   void girderHsplit(obj& ol, obj& in, double spy);
   void girderVsplit(obj& ol, obj& in);
   void simpleVsplit(obj& o);

   void progress();

   double rs, ow, iw, gw, as, ma, sp;
   mode_e mode;
   direction_e di;
   QProgressBar* progressBar;

   obj inp{};       // Copy of the input object
   obj non{};       // Object with notches removed
   obj oro{};       // Outer Rim Outer
   obj ori{};       // Outer Rim Inner
   obj reforo{};    // Reference Outer Rim Outer, may have had notches removed
   obj refori{};    // Reference Outer Rim Inner, derived from refo
   obj iro{};       // Inner Rim Outer, also the lightening hole when girdering is off
   obj iri{};       // Inner Rim Inner
   obj construct{}; // Construction lines
   obj bro{};       // Girder bracing lines

   static constexpr int maxRefLineRotationDegs = 60;

   typedef struct notch_s {
      std::list<line_iter> notchLines = {};
      line_iter notchReplacedLineIter = {};
      line notchReplacedLine = {};
      double distance = 0.0;
      coord_t beg = { 0.0, 0.0 };
      coord_t end = { 0.0, 0.0 };
   } notch_t;

   std::list<notch_t> notches{};
   int notchDetect(obj& o);
   int removeNotches(obj& o);
   int getNumNotches() {
      return (int)notches.size();
   }

   // Each anchor consists of
   //   two braces, each of which consists of
   //      two lines
   typedef struct brLine_s {
      line brLn;          // The line itself
      line_iter oisectLn; // The element where it intersects the outer
      coord_t oisectPt;   // The point where it intersects the outer
      line_iter iisectLn; // The element where it intersects the inner
      coord_t iisectPt;   // The point where it intersects the inner
   } brLine_t;

   typedef struct brace_s {
      double angle = 0.0;
      line refLn = {};
      std::array<brLine_t, 2> brLine = {};
      bool isValid = true;
   } brace_t;

   typedef struct anchor_s {
      brace_t brace[2];
      line rimLine;
      coord_t rimPt;
   } anchor_t;

   std::vector<anchor_t> anchors = {};
   int nAnchors;
};

#endif // FORMER_H
