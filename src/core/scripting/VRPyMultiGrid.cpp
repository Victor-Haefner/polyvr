#include "VRPyMultiGrid.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(MultiGrid, New_VRObjects_ptr);

PyMethodDef VRPyMultiGrid::methods[] = {
    {"addGrid", PyWrap(MultiGrid, addGrid, "Add a grid, rectangle, resolution, ([x0,x1,y0,y1], [rx,ry])", void, Vec4d, Vec2d) },
    {"compute", PyWrapOpt(MultiGrid, compute, "Compute geometry", "0", bool, VRGeometryPtr) },
    {"clear", PyWrap(MultiGrid, clear, "Clear grids", void) },
    {NULL}  /* Sentinel */
};
