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

#include <climits>
#include <list>
#include <stdint.h>
#include <unordered_map>

#include "ascii.h"
#include "debug.h"
#include "object_oo.h"

/**
 * @brief Simple left right pair forming a keepout region for holes
 */
class Keepout {
public:
   Keepout(double l = 0.0, double r = 0.0)
      : l(l),
      r(r) {
   }
   explicit Keepout(const obj& o) {
      set(o);
   }
   double l;
   double r;

   bool isInKeepout(double val, double offset = 0.0);  //!< True if val is in this keepout
   bool isInKeepout(Keepout& kp, double offset = 0.0); //!< True if kp overlaps with or is contained by this keepout

   void set(double left, double right); //!< Set from a left and right value
   void set(const obj& o);              //!< Set from extremities of an object
};

/**
 * @brief Simple left right pair defining the boundaries of a hole
 */
class Hole {
public:
   Hole(double l = 0.0, double r = 0.0)
      : l(l),
      r(r) {
   }
   double l;
   double r;
};

/**
 * @brief Combination of a reference line and intersected lines for top and bottom instersect when cutting a slot
 */
class SlotReference {
public:
   line ln = {};
   line_iter isect_ln_top = {};
   line_iter isect_ln_bot = {};
};

/**
 * @brief Common base for parts of a structure
 *
 * Reuses the Qt concept of Roles to allow multiple drawing
 * objects to be stored.  Default roles for the PART and the
 * PLAN view are created.
 *
 * Other commonly used values are also stored.
 *
 * A part has two lists of keepout regions - one for manually added keepouts
 * and one for automatically added keepouts.
 *
 * A part has a reference line which may be of arbitrary length but is along the line
 * of the part in the plan view. It also has an object line which is the actual centre
 * line of the part in the plan view.
 */
class Part {
public:
   double lhbw = 0.0;                //!< Lightening hole border width
   double mhl = 0.0;                 //!< Maximum length of a single lightening hole
   bool splitAtChoord = false;       //!< Split pretty part along the chord
   bool doesNotInteract = false;     //!< Part should not be considered to interact with others
   double markspace = 1.0;           //!< Mark space ratio for the plan view
   std::list<Keepout> userKpos = {}; //!< User defined keepouts for lightening holes
   std::list<Keepout> autoKpos = {}; //!< Auto defined keepouts for lightening holes
   std::list<Keepout> sparKpos = {}; //!< To handle but joining of spars

   int index = -1;           //!< Part index for display
   std::string typeTxt = {}; //!< The print type of the part
   std::string notes = {};   //!< Notes to display with the part
   line refLn = {};          //!< The reference line in the plan view
   line objLn = {};          //!< The object line in the plan view (i.e. the actual line occupied by the part)

   std::unordered_map<int, obj> o; //!< Roles of the part

   // Keepout related methods
   bool isInSparKeepout(double val, double offset = 0.0); //!< True if value is in a spar related keepout
   bool isInUserKeepout(double val, double offset = 0.0); //!< True if value is in any user defined keepouts
   bool isInAutoKeepout(double val, double offset = 0.0); //!< True if value is in any automatically generated keepouts
   bool isInKeepout(double val, double offset = 0.0);     //!< True if value is in any of the keepouts

   // Part Role related methods
   obj& addRole(int role,
      bool isUserRole = true); //!< Create a new object and map it to "role"
   obj& getRole(int role);               //!< Return reference to the object for "role"

   obj& getPlan();                                        //!< Get the default plan view object
   obj& getPart();                                        //!< Get the default part object
   obj& getPartText();                                    //!< Get the text object for the part
   obj& getRawPart();                                     //!< Get the raw, unprocessed part
   obj& getPrettyPart(int role = PART);                   //!< Get the part ready for display, based on role
   void createPartText(const char* typeTxtOverride = ""); //!< Create a vector text object for the part

   // Interrogation methods
   double planToXpos(coord_t planPt); //!< Convert a plan point to an x position along the part (without checking)
   bool planToXpos(                   //!< As above, but return false if the point appears to be outside of the part
      coord_t planPt,
      double& xpos);

   // Manipulation methods
   bool addHoles(std::string& log); //!< Add holes to the part
   void trimByAutoKeepouts(         //!< Truncate the part at each end based on the keepouts
      double margin,
      int role = PART);
   void redrawObjLine(); //!< Using the X extremities and the refline, redraw the object line

   /**
    * @brief Cut a slot in the part
    * Types of slot:
    *    Absolute depth at a lean angle
    *    Absolute depth at the outline angle
    *    Fractional depth at a lean angle
    * @param planIsect     The intersect point in the plan view
    * @param topFlag       Cut the slot in the top of the part
    * @param sheetSlot     This is a slot in a sheet part; it will be rotated around y = 0 and tested for full-depth special case
    * @param snapOutline   Only valid when sheetSlot = FALSE; snap the angle of the slot to the outline angle
    * @param yAtBottom     The height at the centre of the bottom of the slot
    * @param width         The width of the slot
    * @param leanAngle     The lean angle of the slot
    * @return True if the slot was cut
    */
   bool cutSlot(
      coord_t planIsect,
      bool topFlag,
      bool sheetSlot,
      bool snapOutline,
      double yAtBottom,
      double width,
      double leanAngle,
      std::string& log,
      int role = PART);

   /**
    * @brief Cut a horizontal-bottomed slot for a strip spar
    */
   bool cutStripSparSlot(
      coord_t planIsect,
      bool topFlag,
      double width,
      double depth,
      std::string& log,
      int role = PART);

   /**
    * @brief Cut slot for a strip spar aligned to the outline
    */
   bool cutSnappedStripSparSlot(
      coord_t planIsect,
      bool topFlag,
      double width,
      double depth,
      std::string& log,
      int role = PART);

   /**
    * @brief Cut sheet style slot
    */
   bool cutSheetStyleSlot(
      coord_t planIsect,
      bool topFlag,
      bool botFlag,
      double width,
      double percentDepth,
      double leanAngle,
      pivot_e removeMaterial,
      std::string& log,
      int role = PART);

   /**
    * @brief For a sheet part cut a tab slot i.e., a rectangular hole which receive a tab
    */
   bool cutTabSlot(
      coord_t planIsect,
      double width,
      double percentDepth,
      double leanAngle,
      std::string& log,
      int role = PART);

   /**
    * @brief Open a gap in the outline of a part
    * Opens a gap from firstPt to seconPt assuming they exist in firstLn and seconLn
    */
   void openGap(obj& p, coord_t firstPt, line_iter firstLn, coord_t seconPt, line_iter seconLn);
   void openGap(int role, coord_t firstPt, line_iter firstLn, coord_t seconPt, line_iter seconLn);
   void openGap(coord_t firstPt, line_iter firstLn, coord_t seconPt, line_iter seconLn);

   /**
    * @brief Split the part along a horizontal line
    */
   void splitAtY(double y = 0.0, int role = PART);

   /**
    * @brief Split the part along a vertical line
    */
   void splitAtX(double x = 0.0, int role = PART);

   /**
    * @brief Split a part which is longer than maxlen into equal sections <= maxlen
    */
   void maxLenSplitX(double maxlen, int role = PART);

protected:
   static constexpr int PART = INT_MAX;
   static constexpr int PLFM = INT_MAX - 1;
   static constexpr int PRETTYPART = INT_MAX - 2;
   static constexpr int RAWPART = INT_MAX - 3;
   static constexpr int PARTTEXT = INT_MAX - 4;
   static constexpr int MAX_USER_ROLE = INT_MAX / 2;

   static constexpr double OVC = 0.1; //!< Default overcut for slots etc.

   static constexpr double MIN_HOLE_LENGTH = 7.0; //!< Minimum length of a rib lightening hole
   static constexpr double H_STEP = 1.0;          //!< X step size when scanning a rib to place holes
   static constexpr double H_CORNER_SIZE = 2.0;   //!< x/y offset for 45deg hole corners

   static constexpr double SPLIT_SEPARATION = 5.0; //!< Distance to separate halves of a split part by

   static constexpr int SLOT_L = 0; //!< Indexes into slot array for left side, centre and right side
   static constexpr int SLOT_C = 1;
   static constexpr int SLOT_R = 2;

   /**
    * @brief Cut a horizontal-bottomed slot for a strip spar
    */
   bool _cutStripSparSlot(
      coord_t planIsect,
      bool topFlag,
      bool snapOutline,
      double width,
      double depth,
      std::string& log,
      int role = PART);
};
