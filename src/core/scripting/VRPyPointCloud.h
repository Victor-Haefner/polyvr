#ifndef VRPYPOINTCLOUD_H_INCLUDED
#define VRPYPOINTCLOUD_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRPointCloud.h"

struct VRPyPointCloud : VRPyBaseT<OSG::VRPointCloud> {
    static PyMethodDef methods[];
};

#endif // VRPYPOINTCLOUD_H_INCLUDED
