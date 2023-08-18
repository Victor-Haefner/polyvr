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


Partitiontree::Partitiontree(float res, float s, string n) : resolution(res), firstSize(s), name(n) { if (s < res) firstSize = res; }
Partitiontree::~Partitiontree() { ; }

PartitiontreePtr Partitiontree::ptr() { return shared_from_this(); }


