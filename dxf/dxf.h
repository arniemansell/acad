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

#include "object_oo.h"

#include <cstdio>
#include <string>
#include <vector>

/**
 * @brief The grp_codes class
 *
 * Contains a vector of group codes which can be built up by addition.
 */
class dxf_grp_codes {
public:
   enum dxfgc_e {
      NOT_SET = -9999,
      TEXT_STRING = 0,
      NAME = 2,
      DESC = 3,
      BLOCK_HANDLE = 5,
      LINETYPE_NAME = 6,
      LAYER_NAME = 8,
      VARIABLE_NAME = 9,
      PRIMARY_POINT = 10,
      Y_VALUE = 20,
      Z_VALUE = 30,
      LTYPE_PATT_LEN = 40,
      COLOUR = 62,
      ENTITIES_FOLLOW = 66,
      INT70 = 70,
      LINE_ALIGNMENT = 72,
      LTYPE_NUM_ELM = 73,
      SUBCLASS_MARKER = 100
   };

   dxf_grp_codes() {};

   void add(dxfgc_e c, std::string v) {
      struct dxfgc_s new_code = {};
      new_code.code = c;
      new_code.valu = v;
      gcs.push_back(new_code);
   }

   void add(dxfgc_e c, int v) {
      add(c, std::to_string(v));
   }

   void add(dxfgc_e c, double v) {
      add(c, std::to_string(v));
   }

   void add(dxfgc_e c, const char* v) {
      if (v)
         add(c, std::string(v));
   }

protected:
   struct dxfgc_s {
      dxfgc_e code = NOT_SET;
      std::string valu = {};
   } dxfgc_t;

   std::vector<struct dxfgc_s> gcs = {};
};

/**
 * @brief The dxf_section class
 *
 * Extends the group codes to behave like a single DXF section
 */
class dxf_section : public dxf_grp_codes {
public:
   dxf_section(std::string name) {
      add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "SECTION");
      add(dxf_grp_codes::dxfgc_e::NAME, name.c_str());
   }

   void write(FILE* fd) {
      if (fd) {
         for (auto& gc : gcs) {
            fprintf(fd, "%d\n%s\n", gc.code, gc.valu.c_str());
         }
         fprintf(fd, "0\nENDSEC\n");
      }
   }
};

/**
 * @brief The dxf_export class
 *
 * A complete wrapper for a DXF file export of a set of objects.
 *
 * 1. Create an instance of this class
 * 2. Use the add_object() method for each obj in the drawing.
 *    Each call to add_object creates a new DXF block and an insertion
 *    of the block in the entities section.
 * 3. Finally, use write() method to create the DXF file
 */
class dxf_export {
public:
   dxf_export() {
      header.add(dxf_grp_codes::dxfgc_e::VARIABLE_NAME, "$ANGDIR");
      header.add(dxf_grp_codes::dxfgc_e::INT70, 0);

      // Line Types
      tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "TABLE");
      {
         tables.add(dxf_grp_codes::dxfgc_e::NAME, "LTYPE");
         tables.add(dxf_grp_codes::dxfgc_e::INT70, 14); // Not sure what this is for!
         tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "LTYPE");
         tables.add(dxf_grp_codes::dxfgc_e::NAME, "CONTINUOUS");
         tables.add(dxf_grp_codes::dxfgc_e::INT70, 0); // Flags
         tables.add(dxf_grp_codes::dxfgc_e::DESC, "Solid Line");
         tables.add(dxf_grp_codes::dxfgc_e::LINE_ALIGNMENT, 65); // Always ASCII A
         tables.add(dxf_grp_codes::dxfgc_e::LTYPE_NUM_ELM, 0);
         tables.add(dxf_grp_codes::dxfgc_e::LTYPE_PATT_LEN, 0);
      }
      tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "ENDTAB");

      // Layers
      tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "TABLE");
      {
         tables.add(dxf_grp_codes::dxfgc_e::NAME, "LAYER");
         tables.add(dxf_grp_codes::dxfgc_e::INT70, 3); // Not sure what this is for!
         tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "LAYER");
         tables.add(dxf_grp_codes::dxfgc_e::NAME, "ACAD_PARTS");
         tables.add(dxf_grp_codes::dxfgc_e::INT70, 0); // Flags
         tables.add(dxf_grp_codes::dxfgc_e::COLOUR, 7);
         tables.add(dxf_grp_codes::dxfgc_e::LINETYPE_NAME, "CONTINUOUS");
      }
      tables.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "ENDTAB");
   }

   // Create a new DXF block containing a drawing object
   void add_object(obj& addObj) {
      new_block();

      // Sort into Open and Closed paths for conversion to polylines
      std::list<obj> open, closed;
      addObj.make_path(SNAP_LEN, closed, open);
      // Add the polylines
      for (auto& o : closed) {
         add_path(o, true);
      }
      for (auto& o : open) {
         add_path(o, false);
      }

      end_block();
   }

   // Finish up the export preparation and write the DXF to a file
   void write(FILE* fd) {
      if (fd) {
         // Update the header with the boundary limits
         header.add(dxf_grp_codes::dxfgc_e::VARIABLE_NAME, "$EXTMIN");
         header.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, limmin.x);
         header.add(dxf_grp_codes::dxfgc_e::Y_VALUE, limmin.y);
         header.add(dxf_grp_codes::dxfgc_e::VARIABLE_NAME, "$EXTMAX");
         header.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, limmax.x);
         header.add(dxf_grp_codes::dxfgc_e::Y_VALUE, limmax.y);

         // Write
         header.write(fd);
         tables.write(fd);
         blocks.write(fd);
         entities.write(fd);

         // Terminate
         fprintf(fd, "0\nEOF\n");
      }
   }

   // Get the bottom left coordinate of the drawing extent
   coord_t get_limmin() {
      return limmin;
   }

   // Get the top right coordinate of the drawing extent
   coord_t get_limmax() {
      return limmax;
   }

private:
   dxf_section header = { std::string("HEADER") };
   dxf_section tables = { std::string("TABLES") };
   dxf_section blocks = { std::string("BLOCKS") };
   dxf_section entities = { std::string("ENTITIES") };
   int blockCnt = 0;
   coord_t limmin = { HUGE, HUGE };
   coord_t limmax = { -HUGE, -HUGE };

   void add_vertex(coord_t c) {
      // Add the vertex point to the current block
      blocks.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "VERTEX");
      blocks.add(dxf_grp_codes::dxfgc_e::LAYER_NAME, "ACAD_PARTS");
      blocks.add(dxf_grp_codes::dxfgc_e::LINETYPE_NAME, "CONTINUOUS");
      blocks.add(dxf_grp_codes::dxfgc_e::COLOUR, 7);
      blocks.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, c.x);
      blocks.add(dxf_grp_codes::dxfgc_e::Y_VALUE, c.y);
      // Update the records of the extent of the drawing
      if (c.x < limmin.x)
         limmin.x = c.x;
      if (c.y < limmin.y)
         limmin.y = c.y;
      if (c.x > limmax.x)
         limmax.x = c.x;
      if (c.y > limmax.y)
         limmax.y = c.y;
   }

   void add_path(obj& ob, bool isClosed) {
      // Polyline header
      blocks.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "POLYLINE");
      blocks.add(dxf_grp_codes::dxfgc_e::LAYER_NAME, "ACAD_PARTS");
      blocks.add(dxf_grp_codes::dxfgc_e::LINETYPE_NAME, "CONTINUOUS");
      blocks.add(dxf_grp_codes::dxfgc_e::COLOUR, 7);
      blocks.add(dxf_grp_codes::dxfgc_e::ENTITIES_FOLLOW, 1);
      blocks.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, 0);
      blocks.add(dxf_grp_codes::dxfgc_e::Y_VALUE, 0);
      blocks.add(dxf_grp_codes::dxfgc_e::INT70, isClosed ? 129 : 128); // Flags
      // Vertexes of the start point of each segment
      for (auto ln = ob.begin(); ln != ob.end(); ++ln) {
         add_vertex(ln->get_S0());
      }
      // If this is an open line then we need to include the vertex
      // for the endpoint of the last segment
      if (!isClosed) {
         add_vertex(ob.get_ep());
      }
      // End of sequence
      blocks.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "SEQEND");
   }

   void new_block() {
      blocks.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "BLOCK");
      blocks.add(dxf_grp_codes::dxfgc_e::BLOCK_HANDLE, blockCnt);
      blocks.add(dxf_grp_codes::dxfgc_e::SUBCLASS_MARKER, "AcDbEntity");
      blocks.add(dxf_grp_codes::dxfgc_e::LAYER_NAME, "ACAD_PARTS");
      blocks.add(dxf_grp_codes::dxfgc_e::SUBCLASS_MARKER, "AcDbBlockBegin");
      blocks.add(dxf_grp_codes::dxfgc_e::NAME, blockCnt);
      blocks.add(dxf_grp_codes::dxfgc_e::INT70, 0); // Flags
      blocks.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, 0);
      blocks.add(dxf_grp_codes::dxfgc_e::Y_VALUE, 0);
      blocks.add(dxf_grp_codes::dxfgc_e::DESC, blockCnt);
   }

   void end_block() {
      blocks.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "ENDBLK");
      blocks.add(dxf_grp_codes::dxfgc_e::BLOCK_HANDLE, blockCnt);
      blocks.add(dxf_grp_codes::dxfgc_e::SUBCLASS_MARKER, "AcDbBlockEnd");
      // Add an insertion reference to the completed block to make it appear in the drawing
      entities.add(dxf_grp_codes::dxfgc_e::TEXT_STRING, "INSERT");
      entities.add(dxf_grp_codes::dxfgc_e::SUBCLASS_MARKER, "AcDbBlockReference");
      entities.add(dxf_grp_codes::dxfgc_e::NAME, blockCnt);
      entities.add(dxf_grp_codes::dxfgc_e::PRIMARY_POINT, 0);
      entities.add(dxf_grp_codes::dxfgc_e::Y_VALUE, 0);

      blockCnt++;
   }
};
