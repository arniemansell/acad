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

#define _USE_MATH_DEFINES
#include <climits>
#include <cmath>
#include <stdio.h>
#include <unordered_map>
#include <vector>

#include "airfoil.h"
#include "element.h"
#include "le_template.h"
#include "object_oo.h"
#include "planform.h"
#include "rib.h"
#include "spar.h"

class Wing {
public:
   /**
    * @brief Get the plan for the wing
    */
   obj& getPlan();
   obj plan;

   /**
    * @brief Get the parts for the wing
    */
   obj& getParts();
   obj parts;

   /**
    * @brief Tell the wing it is to draw in draft mode
    */
   void setDraftMode();

   /**
    * @brief Export to HPGL
    */
   void exportToHpgl(QFileInfo& fi);

   /**
    * @brief Export to DXF
    */
   void exportToDxf(QFileInfo& fi);

   /**
    * @brief Export file basics
    */
   bool exportFileOpen(QFileInfo& fi, FILE** fd);

   Planform plnf;
   Airfoil_set aifs;
   Rib_set ribs;
   Spar_set sprs;
   Element_set elms;
   LeTemplate_set lets;

private:
};

void loadWingFiles(FILE** wfp);
