#include "VRPyAnalyticGeometry.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(AnalyticGeometry, New_VRObjects_ptr);

PyMethodDef VRPyAnalyticGeometry::methods[] = {
    {"addVector", PyWrapOpt(AnalyticGeometry, addVector, "Add/set an annotated vector", "", int, Vec3d, Vec3d, Color3f, string) },
    {"setVector", PyWrapOpt(AnalyticGeometry, setVector, "Add/set an annotated vector", "", void, int, Vec3d, Vec3d, Color3f, string) },
    {"setCircle", PyWrapOpt(AnalyticGeometry, setCircle, "Add/set an annotated circle", "", void, int, Vec3d, Vec3d, float, Color3f, string) },
    {"setLabelParams", PyWrapOpt(AnalyticGeometry, setLabelParams, "Set the size of the labels", "0|0|0 0 0 1|0 0 0 0", void, float, bool, bool, Color4f, Color4f) },
    {"clear", PyWrap(AnalyticGeometry, clear, "Clear data", void) },
    {NULL}  /* Sentinel */
};
