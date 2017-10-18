#include "VRPyMeasure.h"
#include "VRPyGeometry.h"
#include "VRPyPose.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Measure, New_VRObjects_ptr);

PyMethodDef VRPyMeasure::methods[] = {
    {"setPoint", PyWrap(Measure, setPoint, "Set one of the three points, where i is 0,1 or 2 and p the position", void, int, PosePtr ) },
    {"rollPoints", PyWrap(Measure, rollPoints, "Roll through the points", void, PosePtr ) },
    {NULL}  /* Sentinel */
};

