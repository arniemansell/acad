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
#include <cmath>
#include <iterator>
#include <list>

// Constants
#define SMALL_RATIO 1e-5  //!< Ratio below which someting is insignificant
#define SNAP_LEN 0.0001   //!< Used by make_path() - snap distance for joining to nearby line ends
#define SIMPLIFY_ERR 0.01 //!< Allowable error when simplifying objects
#define SMALL_NUM (SNAP_LEN * 1.0e-3)
#define LARGE 3e3
#define TRACE_STEP_MM 0.5 //!< For trace_at_offset, maximum size of a trace step in mm
#define MIN_TRACE_STEPS 4 //!< For trace_at_offset, minimum number of steps per line element

//!< Angle macros
#define TO_RADS(x) ((x)*M_PI / 180.0)
#define TO_DEGS(x) ((x)*180.0 / M_PI)
#define N_x_NINETYDEG(n) ((n)*M_PI / 2.0)

//!< Points on a line
#define T_S0 0.0     //!< S0, the start of the line
#define T_S1 1.0     //!< S1, the end of the line
#define T_CENTER 0.5 //!< The middle of the line

// Direction enumerations
typedef enum {
   LEFT = 0,
   RIGHT,
   UP,
   DOWN
} direction_e;

//! Locations for pivot point
typedef enum {
   LE = 0,
   CENTRE,
   TE
} pivot_e;
#define pivot_e_txt(X) (X == LE) ? "Leading Edge" : (X == CENTRE) ? "Centre"        \
                                                : (X == TE)       ? "Trailing Edge" \
                                                                  : "???"

//! Slot types
typedef enum {
   VERTICAL,
   CENGRAD,
} slot_style_e;

//! States for the makepath function
typedef enum {
   MP_INIT,
   MP_PROCESS_PATH,
   MP_PATH_OPEN,
   MP_PATH_CLOSED
} mpState_e;

//! COORDINATE - a point in space referred to by [x,y] coordinates
class coord_t {
public:
   coord_t(double x = 0.0, double y = 0.0)
      : x(x),
      y(y) {
   }
   char* prstr() {
      snprintf(prbuf, 256, "(%.2lf, %.2lf)", x, y);
      return prbuf;
   };
   double x;
   double y;

private:
   char prbuf[256];
};

//! VECTOR - a movement referred to by change in x and change in y
typedef struct vector_s {
   vector_s(double dx = 0.0, double dy = 0.0)
      : dx(dx),
      dy(dy) {
   }
   double dx;
   double dy;
} vector_t;

//! Check if a values are the same within +/-margin
bool isEqualWithinMargin(double arg1, double arg2, double margin);

//! Check if arg2 is within a margin of (percentage /100) * arg1
bool isEqualWithinPercentage(double arg1, double arg2, double percentage);

//! Distance between two points
double distTwoPoints(coord_t c1, coord_t c2);

//! Average of two points
coord_t averageTwoPoints(coord_t c1, coord_t c2);

//! Perpendicular and dot products
double dotprodRaw(double x1, double y1, double x2, double y2);
double dotprod(vector_t pt1, vector_t pt2);
double perpprodRaw(double x1, double y1, double x2, double y2);
double perpprod(vector_t pt1, vector_t pt2);

//! Point rotation
void rotatePoint(coord_t* pt, coord_t pivot, double rads);

//! Simple linearly iterated variable
class linvar {
private:
   //!< y = (m * x) + c
   double m = 0.0;
   double c = 0.0;
   //!< Normal input range of values
   double x0 = 0.0;
   double x1 = 0.0;

public:
   linvar(double x0, double y0, double x1, double y1);

   double v(double x) const;  //!< Return the value at x position = x
   double vl(double x) const; //!< Return the value at x position = x but limit to range [x0...x1]
};

// Simple square law iterated variable where x[0.0..1.0] is transformed to x^2[0.0...1.0] before interpolation
class sqvar {
private:
   double x0 = 0.0;
   double y0 = 0.0;
   double x1 = 0.0;
   double y1 = 0.0;
   double cdenom = 0.0;
   double yd = 0.0;
   double m = 0.0; //!< Squared term coefficient [0..1]
   double k = 0.0; //!< Linear term coefficent

public:
   sqvar(double x0, double y0, double x1, double y1, double squaredness); //!< 1 = square law through to 0 = linear

   double vl(double x) const; //!< Return the value at x position = x but limit to range [x0...x1]
};

// LINE
class line {
private:
   // Line is stored in parametric form y = S0 + t.V
   coord_t S0 = { 0.0, 0.0 };
   vector_t V = { 0.0, 0.0 };

   bool is_small(double val) const;

public:
   line() = default;
   line(coord_t s0, vector_t v0);
   line(coord_t s0, coord_t s1);
   line(coord_t s0, double length, double angle);

   char* print_str(); //!< Return a string for display

   void set(coord_t s0, coord_t s1);                  //!< Set using two points
   void set(coord_t s0, vector_t v0);                 //!< Set using point and a vector
   void set(coord_t s0, double length, double angle); //!< Set using start point, length and angle

   double len() const;     //!< Return length
   double angle() const;   //!< Return angle in radians
   double angle(line& l2); //!< Return the angle between two lines

   coord_t get_pt(double T) const;    //!< Get a coordinate point on a line based on (T[0.0..1.0] to be in the bounds of the line)
   vector_t get_V() const;            //!< Return the line vector
   coord_t get_S0() const;            //!< Return the first end point of the line
   coord_t get_S1() const;            //!< Return the second end point of the line
   double T_for_y(double y) const;    //!< Return the ratio value T which matches Y=y
   double T_for_x(double x) const;    //!< Return the ratio value T which matches X=x
   double T_for_pt(coord_t pt) const; //!< Return the ratio value T which matches pt

   bool is_vertical() const;                     //!< Test if a line is vertical
   bool is_horizontal() const;                   //!< Test if a line is horizontal
   bool is_same_as(const line& ln) const;        //!< Test if lines are identical
   bool is_contiguous_with(const line& l) const; //!< Test if it is connected with another line
   bool is_in_collinear_seg(coord_t pt) const;   //!< Test if the point is in the collinear region of this line
   bool lines_intersect(const line& l2, coord_t* i,
      int allowExtrapolation) const; //!< Test if a line intersects with this line
   double distance_to_point(coord_t p) const;          //!< Minimum distance from the line to a point

   void add_offset(double xOffset, double yOffset); //!< Move in x/y
   void move_sideways(double dist);                 //!< Move in a direction at +90deg to the line
   void rotate(coord_t pivot, double rads);         //!< Rotate around a point
   void mirror_x();                                 //!< Mirror around a vertical line at x=0
   void mirror_y();                                 //!< Mirror around a horizontal line at y=0
   void reverse();                                  //!< Swap ends
   void set_length(double length);                  //!< Leave S0 where it is, expand the line length
   void extend_S0_mm(double mm);                    //!< If possible, extend the start of the line by mm millimetres
   void extend_S1_mm(double mm);                    //!< If possible, extend the end   of the line by mm millimetres
   void extend_mm(double mm);                       //!< Extend by mm millimetres at both ends
};

typedef typename std::list<line>::iterator line_iter;
typedef typename std::list<line>::const_iterator const_line_iter;

double slotWidth(const line& crossLine,
   const line& slottedLine,
   double crossThck, double slottedThck); //!< Calculate a slot width for an angled intersect

// Used for the trace offseting algorithm
class offset_line : public line {
public:
   bool valid = true;
   bool radial = false;
   size_t src_index = 0;
};

typedef typename std::list<offset_line>::iterator offset_line_iter;

// Used for storing information about an object's intersections with a line
class obj_line_intersect {
public:
   obj_line_intersect(double T, line_iter ln, coord_t pt)
      : T(T),
      ln(ln),
      pt(pt) {
   }
   double T;     //!< Ratio along intersecting line
   line_iter ln; //!< The line iterator for the intersected element in obj
   coord_t pt;   //!< The pt of intersection
};

//!< OBJECT
class obj {
private:
   mutable std::list<line> e; //!< Lines comprising this object

   bool test_extremity(direction_e dir, double var, double* res, coord_t* ptu,
      coord_t* ptd,
      coord_t inPt) const;
   bool sX_is_at(coord_t pt, line_iter& ln, double T) const;
   bool del_dupes_zeros();
   void make_path(double snaplen, std::list<obj>& closed, std::list<obj>& open,
      bool listPaths,
      bool keepOpens);
   bool trace_a_path(double snaplen, line_iter& st, line_iter& en);                //!< Make a path starting at st, return true if it is a closed path. st points to first element, en to element after the last
   double get_max_line_distance(line_iter st, line_iter en, const line& ln) const; //!< Max distance of ln from the points of all elements st to en inclusive
   void startAtDirection(direction_e dir, line_iter st, line_iter en);             //!< See public function comments

public:
   obj() = default;
   obj(coord_t s0, coord_t s1);  //!< Initialise with a line between two points
   obj(coord_t s0, vector_t v0); //!< Initialise with a line of a vector from a point
   explicit obj(line ln);        //!< Initialise with a copy of a line

   // Add line elements
   line_iter add(double x1, double y1, double x2, double y2);            //!< Add a line using two raw points
   line_iter add(coord_t s0, coord_t s1);                                //!< Add a line using two points, return iterator
   line_iter add(coord_t s0, vector_t v0);                               //!< Add a line using point and a vector, return iterator
   line_iter add(const line& ln);                                        //!< Add a line
   line_iter add(coord_t pt);                                            //!< If empty, add a zero length line at pt; otherwise extend to pt from endpoint
   line_iter add_rect(coord_t c1, coord_t c2, double markspace = 1.0);   //!< Add a rectangle by opposite corners
   line_iter add_rect(const line& ln, double w, double markspace = 1.0); //!< Add a rectangle centered on line with width w
   void add_dotted(const line& ln, double marklen, double splen);        //!< Add line as a dotted
   void add_ellipse(coord_t centre, double rX, double rY);               //!< Add an ellipse at centre with X and Y radiuses
   void move_back_to_front();                                            //!< Move the last added element to the front of the object

   //!< Add elements from another object
   void splice(obj& o);                     //!< Concatenate o onto this object
   void splice(line_iter pos_in_o, obj& o); //!< Same, starting at pos_in_o in object o
   void copy_from(obj& o);                  //!< Same as splice but copies

   // Delete line elements
   void del(line_iter& iter);                   //!< Delete the referenced line
   void del(line_iter& first, line_iter& last); //!< Delete the referenced lines inclusive
   void del();                                  //!< Delete all lines
   size_t del_duplicates();                     //!< Remove all but one of duplicated lines, the first found is kept, return count
   size_t del_zero_lens();                      //!< Remove lines of zero length, return count
   size_t remove_verticals();                   //!< Remove vertical lines, return count

   // Position elements
   line_iter begin() const;             //!< First Element iterator
   line_iter last() const;              //!< Last element iterator
   line_iter end() const;               //!< End element iterator
   line_iter nextc(line_iter ln) const; //!< Next element (circular)
   line_iter prevc(line_iter ln) const; //!< Previous element (circular)
   line_iter next(line_iter ln) const;  //!< Next element
   line_iter prev(line_iter ln) const;  //!< Previous element
   const_line_iter cbegin() const;      //!< First Element iterator
   const_line_iter clast() const;       //!< Last element iterator
   const_line_iter cend() const;        //!< End Element iterator
   bool is_begin(line_iter ln) const;   //!< True if first element
   bool is_last(line_iter ln) const;    //!< True if last element
   bool is_end(line_iter ln) const;     //!< True if end of internal element list

   // Interrogation
   bool empty() const;                                  //!< True if empty (no elements)
   size_t size() const;                                 //!< Number of elements
   double len() const;                                  //!< Total length of all elements
   size_t index(line_iter ln) const;                    //!< Index of the element at ln, starting at 0
   bool is_clockwise(line_iter st, line_iter en) const; //!< The closed path bounded by the iterators is clockwise?
   coord_t get_sp() const;                              //!< Get the first point of the first element
   coord_t get_ep() const;                              //!< Get the last point of the last element
   coord_t get_pt_along_length(double dist) const;      //!< Return the point which is dist along the object
   coord_t get_pt_along_length(double dist, line_iter& ln) const;
   coord_t get_pt_along_length(double dist, line_iter& ln, double& T) const;
   bool s0_is_at(coord_t pt, line_iter& ln) const; //!< Find a line segment where S0 = pt
   bool s1_is_at(coord_t pt, line_iter& ln) const; //!< Find a line segment where S1 = pt
   coord_t originIsAt() const;                     //!< The bottom left corner of the bounding box

   bool findMarkerSquare(double size, coord_t* centre, bool deleteIt); //!< Find a marker square of side-dimension size

   bool obj_intersect(obj& o) const; //!< Find if o intersects this object
   bool surrounds_point(coord_t pt); //!< True if this object surrounds pt; assumes this object is a closed path

   /**
    * @brief Find and return all intersects between a line and this object
    * @param l2      The line to be tested
    * @param isects  Pointer to a list of intersect structures to be populated (or NULL)
    *                isects will be sorted in order of increasing T value along l2
    * @param allowExtrapolation Set nonzero to allow extrapolation of the line l2
    * @return True if an intersect was found
    */
   bool line_intersect(line l2, std::list<obj_line_intersect>* isects, int allowExtrapolation) const;
   line_iter line_intersect(const line& l2, coord_t* i, int allowExtrapolation) const;               //!< Find if l2 intersects this object
   line_iter line_intersect(line_iter l1, const line& l2, coord_t* i, int allowExtrapolation) const; //!< Same but search from l1 onwards

   bool top_bot_intersect(double xpos, coord_t* upper, coord_t* lower,
      line_iter& it_upper,
      line_iter& it_lower) const; //!< Find highest and lowest intersects of a vertical line through xpos
   bool top_bot_intersect(double xpos, coord_t* upper, coord_t* lower) const;
   bool top_intersect(double xpos, coord_t* pt, line_iter& ln) const; //!< Find highest intersect of a vertical line through xpos
   bool bot_intersect(double xpos, coord_t* pt, line_iter& ln) const; //!< Find lowest intersect of a vertical line through xpos
   bool dir_intersect(direction_e dir, double xpos, coord_t* pt,
      line_iter& ln) const; //!< Find the UP or DOWN intersect of a vertical line through xpos

   void find_extremity(coord_t pt[4], double extremity[4],
      line_iter elm[4]) const;      //!< Find the furthest point/extremity in all four directions
   void find_extremity(coord_t pt[4]) const;         //!< Find the furthest point/extremity in all four directions
   void find_extremity(double val[4]) const;         //!< Find the furthest point/extremity in all four directions
   double find_extremity(direction_e dir) const;     //!< Find extremity as a single dimension in direction dir
   coord_t find_extremity_pt(direction_e dir) const; //!< Find extremity as a point in direction dir

   coord_t find_avg_centre() const; //!< Find the average coordinate of the object

   //!< Manipulation
   void add_offset(double xOffset, double yOffset); //!< Move in x and/or y
   void rotate(coord_t pivot, double rads);         //!< Rotate around a point
   void mirror_x();                                 //!< Mirror around a vertical line at x=0
   void mirror_y();                                 //!< Mirror around a horizontal line at y=0
   void trace_at_offset(double ofs);                //!< Replace the object with a trace at offset ofs - +ve->outside trace, -ve->inside trace
   void make_path(                                  //!< Sort an object into open and closed paths based on snaplen; closed paths are made clockwise
      double snaplen,
      std::list<obj>& closed,
      std::list<obj>& open);
   void make_path(double snaplen);                      //!< As above but discard open paths
   void make_path();                                    //!< As above but discard open paths and use default SNAPLEN
   void startAtDirection(direction_e dir);              //!< Starts with a make_path, then the elements of each closed path
   //!< are shuffled so that the path starts exactly at the compass point requested
   void regularise();                                   //!< Make_path, remove duplicates and zero length elements, closed paths to clockwise
   void regularise_no_del();                            //!< As above but no elements deleted
   void scale_x_lr(double factor);                      //!< Scale in x, keeping LEFT edge in same position
   void scale(double factor);                           //!< Scale all points by multiplying by factor
   void move_extremity_to(direction_e dir, double pos); //!< Move the object to put its extremity in direction dir at pos in the relevant axis
   void move_origin_to(coord_t loc);                    //!< Move the object's rectangular origin to point loc
   void make_gap(                                       //!< Make a gap in an object between p0 on element l0 and p1 on element l1
      line_iter l0,
      coord_t p0,
      line_iter l1,
      coord_t p1,
      bool noNewLines = false);
   bool cut_slot( //!< Cut a slot in top (UP) or bottom (DOWN) of closed path
      direction_e dir,
      double xpos,
      double width,
      double depth,
      slot_style_e style);
   bool cut_slot( //!< As above but set the outline corners of the slot
      direction_e dir,
      double xpos,
      double width,
      double depth,
      slot_style_e style,
      coord_t* pt0,
      coord_t* pt1);
   bool remove_extremity( //!< Remove everything beyond pos in the direction dir, setting the upper and lower points at the edge of removal
      double pos,
      direction_e dir,
      coord_t* ptu,
      coord_t* ptd,
      bool rejoin = false);
   bool remove_extremity_rejoin( //!< As above but rejoin ends
      double pos,
      direction_e dir,
      coord_t* ptu,
      coord_t* ptd);
   bool remove_extremity( //!< Remove everything beyond pos in the direction dir
      double pos,
      direction_e dir);
   bool remove_extremity_rejoin( //!< As above but rejoin ends
      double pos,
      direction_e dir);
   void split_along_line( //!< Split the object into two halves, one to the left of a line and one to the right
      const line ln,
      obj& left,
      obj& right,
      std::list<obj_line_intersect>* isects = NULL);
   void split_along_line_rejoin( //!< As above, but rejoin the split ends
      const line ln,
      obj& left,
      obj& right,
      std::list<obj_line_intersect>* isects = NULL);
   size_t simplify(double error); //!< Reduce number of line elements with a maximum allowable error distance
   size_t simplify();             //!< As above using default value for error; both functions return the count of deleted elements
   void extend1mm();              //!< Extend start and finish by 1mm if possible

   bool testFlag = false;
};

typedef typename std::list<obj>::iterator obj_iter;

bool obj_sort_left_right(const obj& a, const obj& b); //!< Used for list sort based on the position from find_avg_centre()
bool obj_sort_top_bottom(const obj& a, const obj& b);
bool intersect_sort(const obj_line_intersect& a, const obj_line_intersect& b);
