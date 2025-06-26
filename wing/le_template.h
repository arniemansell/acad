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

class LeTemplate : public Part {
public:
   static constexpr double LE_TEMPLATE_DEPTH = 0.15; //<! Fraction of choord to include in LE templates
   double xpos = 0.0;                                //!< The position of the LE template
   line airfLn = {};                                 //!< Line between airfTE and airfLE
   bool create(Planform& pl, Airfoil_set& af, bool draftMode, std::string& log);
};

class LeTemplate_set {
public:
   bool draftMode = false;
   obj pparts = {};
   std::list<LeTemplate> lets = {};

   std::list<LeTemplate>::iterator begin() {
      return lets.begin();
   };
   std::list<LeTemplate>::iterator end() {
      return lets.end();
   };

   /**
    * @brief Add LE templates to the set from a generic tab
    */
   bool add(GenericTab* T, Planform& plnf, std::string& log);

   /**
    * @brief Create each spacer
    */
   bool create(Planform& pl, Airfoil_set& af, std::string& log);

   /**
    * @brief Configure to work in draft mode
    */
   void draft_mode() {
      draftMode = true;
   }

   /**
    * @brief Parts and their texts
    */
   void getPrettyParts(std::list<std::reference_wrapper<obj>>& objects, std::list<std::reference_wrapper<obj>>& texts);
};
