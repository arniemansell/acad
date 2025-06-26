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

#include "airfoil.h"
#include "debug.h"
#include "object_oo.h"
#include "part.h"
#include "planform.h"

class Rib : public Part {
public:
   enum draft {
      aorg,       //!< Airfoil - original  - TE centered on (0, 0)
      anot,       //!< Same as aorg but with no trailing edge blend
      apcd,       //!< Airfoil - processed - Rib angle applied, washout applied
      rorg,       //!< Rib - original  - taken from apcd with sheeting thickness removed
      rorgholes,  //!< Copy of initial rorg, no bits and bobs applied, used for hole generation
      rjig,       //!< Rib - processed to enable sheeting jig creation
      topjig,     //!< Top sheeting jig
      botjig,     //!< Bottom sheeting jig
      topjigtext, //!< Text label for top jig
      botjigtext  //!< Text label for bottom jig
   };

   static constexpr double SH_JIG_CLAMPING_GAP = 4.0; //!< Total gap between the jig-ends when assembled
   static constexpr double SH_JIG_END_W = 22.0;       //!< Width of the jig end containing the clamping bar
   static constexpr double SH_JIG_BAR_W = 9.0;        //!< Clamping bar dimensions
   static constexpr double SH_JIG_BAR_T = 3.9;        //!< Clamping bar dimensions

   bool jig = false;             //!< Rib has an associated sheeting jig
   bool affectsSpars = true;     //!< True if this rib affects spars
   bool isCreated = false;       //!< Set true once created (to prevent duplicate create events)
   double leW = 0.0;             //!< Width of LE in line of rib
   double teW = 0.0;             //!< Width of TE in line of rib
   double rib_thck = 0.0;        //!< Material thickness of the rib
   double w_sh_thck = 0.0;       //!< Wing sheet thickness at the rib
   double te_thck = 0.0;         //!< Trailing edge thickness
   double jig_thck = 0.0;        //!< Thickness of sheeting jig material
   double te_blend = 0.0;        //!< Factor over which to blend TE thickness [0.0..1.0]
   double washout = 0.0;         //!< Washout angle
   pivot_e wo_pivot = CENTRE;    //!< Point about which washout is applied
   coord_t wo_pivot_pt = { 0, 0 }; //!< Point about which washout is applied

   // 2D planform information
   line achd = {}; //!<Choord line in side elevation

   //!< If the line intersects this rib add a keepout of "width" at the intersect
   bool addKeepout(
      double stX,
      double stY,
      double enX,
      double enY,
      double width);

   void createRibPart(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log);
   void createRibPlan(std::string& log);
   void createRibText(std::string& log);
   bool createRib(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log);

   obj& getPlan();

   bool operator==(const Rib& r) {
      return (r.index == index);
   };

   enum shJigEndType {
      jigType1,
      jigType2Simple
   };

   enum shJigType {
      type1,
      type2
   };

   enum shJigBarPos {
      inside,
      outside
   };

   bool sheetingJig(line& jigLe, line& jigTe, line& jigBotSpr, double jbsW, double jbsD,
      double lew,
      double let, double tew, double tet, double height, double thck, bool topFlag,
      bool draftMode, shJigType jigType, shJigEndType endType, shJigBarPos lepos, shJigBarPos tepos, int partIndex,
      std::string& log);

   coord_t drawJigEnd(Rib::shJigEndType endType, obj& jr, coord_t startHere, double offset, double height, direction_e dir);
   coord_t drawJigEndType1(obj& rib, coord_t startHere, double offset, double height, direction_e dir);
   coord_t drawJigEndType2Simple(obj& rib, coord_t startHere, double offset, double height, direction_e dir);

   double xposToAirfoilT(double xpos);

   double plnfmIntersectToXpos(line ln);
};

typedef typename std::list<Rib>::iterator rib_iter;

class Rib_set {
public:
   static constexpr double te_blend_default = 0.5;
   std::list<Rib> ribs = {};
   bool draft = false;
   obj plan = {};
   obj pparts = {};

   rib_iter begin() {
      return ribs.begin();
   };
   rib_iter end() {
      return ribs.end();
   };

   /**
    * @brief Add ribs to the set from a generic tab
    */
   bool add(GenericTab* T, Planform& plnf, std::string& log);

   /**
    * @brief Add all the geodetic sets defined
    */
   bool addGeodetics(GenericTab* T, Planform& plnf, std::string& log);

   /**
    * @brief Generate sheeting jigs fr the ribs that have them
    */
   bool addCreateJigs(GenericTab* T1, GenericTab* T2, std::string& log);

   /**
    * @brief Generate sheeting jigs fr the ribs that have them - Type 2
    */
   bool addCreateJigsType2(GenericTab* T1, std::string& log);

   /**
    * @brief Add lightening holes to all ribs
    */
   bool addHoles(std::string& log);

   /**
    * @brief Update ribs with manually placed keepouts, trailing edge thickness and washout
    */
   bool addRibParams(GenericTab* T, std::string& log);

   /**
    * @brief Create all the base components of each rib
    */
   bool create(Planform& pl, Airfoil_set& af, std::string& log);

   /**
    * @brief Configure to work in draft mode
    */
   void draft_mode() {
      draft = true;
   }

   /**
    * @brief Get plan objects
    */
   obj& getPlan();

   /**
    * @brief Get the parts ready for display or export
    */
   void getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts);

   /**
    * @brief Apply washout to a range of ribs, linearly interpolated between endpoints
    *
    */
   bool setWashout(int r, GenericTab* T, std::string& log);

   /**
    * @brief Return an index for a given rib iterator
    */
   unsigned ribItInd(std::list<Rib>::iterator it) {
      return (1 + std::distance(ribs.begin(), it));
   }

   /**
    * @brief Apply trailing edge thickness to a range of ribs, linearly interpolated between endpoints
    */
   bool setTeThickness(int r, GenericTab* T, std::string& log);

private:
   bool check_geodetic_intersect(rib_iter rib, line* topln, line* botln, coord_t* top, coord_t* bot);

   line jigLe = {};
   line jigTe = {};
   static constexpr double GEODETIC_THICKNESS_TO_X_RATIO = 5.0; //!< Ribs must be separated by at least this x thickness to place a geodetic between them
   static constexpr size_t GEODETIC_T_STEPS = 60;               //!< Number of x steps when estimating the shape of a geodetic
};
