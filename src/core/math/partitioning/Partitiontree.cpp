#include "Partitiontree.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <time.h>
#include <sstream> //for std::stringstream
#include <string>  //for std::string

#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

PartitiontreeNode::PartitiontreeNode(float res, float s, int lvl) : resolution(res), size(s), level(lvl) { ; }
PartitiontreeNode::~PartitiontreeNode() {}

// checkPosition avoids parent/child cycles due to float error
// partitionLimit sets a max amount of data points, tree is subdivided if necessary!

vector<Vec3d> PartitiontreeNode::getPoints() { return points; }

Vec3d PartitiontreeNode::getCenter() { return center; }

void PartitiontreeNode::remData(void* d) {
    data.erase(std::remove(data.begin(), data.end(), d), data.end());
}

int PartitiontreeNode::dataSize() { return data.size(); }
void* PartitiontreeNode::getData(int i) { return data[i]; }
Vec3d PartitiontreeNode::getPoint(int i) { return points[i]; }

float PartitiontreeNode::getSize() { return size; }
float PartitiontreeNode::getResolution() { return resolution; }
void PartitiontreeNode::setResolution(float res) { resolution = res; }

// sphere center, box center, sphere radius, box size
bool PartitiontreeNode::sphere_box_intersect(Vec3d Ps, Vec3d Pb, float Rs, float Sb)  {
    float r2 = Rs * Rs;
    Vec3d diag(Sb*0.5, Sb*0.5, Sb*0.5);
    Vec3d Bmin = Pb - diag;
    Vec3d Bmax = Pb + diag;
    float dmin = 0;
    if( Ps[0] < Bmin[0] ) dmin += ( Ps[0] - Bmin[0] )*( Ps[0] - Bmin[0] );
    else if( Ps[0] > Bmax[0] ) dmin += ( Ps[0] - Bmax[0] )*( Ps[0] - Bmax[0] );
    if( Ps[1] < Bmin[1] ) dmin += ( Ps[1] - Bmin[1] )*( Ps[1] - Bmin[1] );
    else if( Ps[1] > Bmax[1] ) dmin += ( Ps[1] - Bmax[1] )*( Ps[1] - Bmax[1] );
    if( Ps[2] < Bmin[2] ) dmin += ( Ps[2] - Bmin[2] )*( Ps[2] - Bmin[2] );
    else if( Ps[2] > Bmax[2] ) dmin += ( Ps[2] - Bmax[2] )*( Ps[2] - Bmax[2] );
    return dmin <= r2;
}

// box min, box max, octree box center, octree box size
bool PartitiontreeNode::box_box_intersect(Vec3d min, Vec3d max, Vec3d Bpos, float Sb)  {
    Vec3d Bdiag(Sb, Sb, Sb);
    Vec3d Bmin = Bpos - Bdiag*0.5;
    Vec3d Bmax = Bpos + Bdiag*0.5;

    Vec3d Apos = (max + min)*0.5;
    Vec3d Adiag = max-min;

    Vec3d diff = (Apos-Bpos)*2;
    Vec3d ABdiag = Adiag+Bdiag;
    return (abs(diff[0]) <= ABdiag[0]) && (abs(diff[1]) <= ABdiag[1]) && (abs(diff[2]) <= ABdiag[2]);
}

string PartitiontreeNode::toString(int indent) {
    auto pToStr = [](void* p) {
        const void * address = static_cast<const void*>(p);
        std::stringstream ss;
        ss << address;
        return ss.str();
    };

    string res = "\nOc ";
    for (int i=0; i<indent; i++) res += " ";
    res += "size: " + ::toString(size) + " center: " + ::toString(center);
    if (data.size() > 0) res += "\n";
    for (unsigned int i=0; i<data.size(); i++) if(data[i]) res += " " + pToStr(data[i]);
    return res;
}

vector<void*> PartitiontreeNode::getData() { return data; }


Partitiontree::Partitiontree(float res, float s, string n) : resolution(res), firstSize(s), name(n) { if (s < res) firstSize = res; }
Partitiontree::~Partitiontree() { ; }

PartitiontreePtr Partitiontree::ptr() { return shared_from_this(); }


