/*
This source file is part of Rigs of Rods
Copyright 2009 Lefteris Stamatogiannakis

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PointColDetector_H__
#define __PointColDetector_H__

#include "Ogre.h"
#include <vector>
using namespace Ogre;
using namespace std;
#include "Beam.h"
#include "rormemory.h"

typedef struct _pointid {
	int nodeid;
	int truckid;
} pointid_t;

typedef struct _refelem {
	pointid_t* pidref;
	float* point;
} refelem_t;

typedef struct _kdnode {
	float min;
	int end;
	float max;
	refelem_t* ref;
	float middle;
	int begin;
} kdnode_t;

class PointColDetector : public MemoryAllocatedObject
{
public:
	std::vector< Vector3 > *object_list;
	std::vector< pointid_t* > hit_list;
	int hit_count;

	PointColDetector(std::vector< Vector3 > &o_list);
	PointColDetector();
	~PointColDetector();

	void reset();
	void update();
	void update(Beam** trucks, const int numtrucks);
	void update_structures();
	void update_structures_for_contacters(Beam** trucks, const int numtrucks);
	void querybb(const Vector3 &bmin, const Vector3 &bmax);
	void query(const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, float enlargeBB=0.0f);
	void query(const Vector3 &vec1, const Vector3 &vec2, const float enlargeBB=0.0f);
	inline void calc_bounding_box(Vector3 &bmin, Vector3 &bmax, const Vector3 &vec1, const Vector3 &vec2, const Vector3 &vec3, const float enlargeBB=0.0f);
	inline void calc_bounding_box(Vector3 &bmin, Vector3 &bmax, const Vector3 &vec1, const Vector3 &vec2, const float enlargeBB=0.0f);
private:
	int object_list_size;
	refelem_t *ref_list;
	pointid_t *pointid_list;
	std::vector< kdnode_t > kdtree;
	Vector3 bbmin;
	Vector3 bbmax;
	inline void queryrec(int kdindex, int axis);
	void build_kdtree(int begin, int end, int axis, int index);
	void build_kdtree_incr(int axis, int index);
	void partintwo(const int start, const int median, const int end, const int axis, float &minex, float &maxex);
};

#endif
