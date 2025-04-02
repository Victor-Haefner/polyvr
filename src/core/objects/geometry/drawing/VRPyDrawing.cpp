#include "VRPyDrawing.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(TechnicalDrawing, New_VRObjects_ptr);

PyMethodDef VRPyTechnicalDrawing::methods[] = {
    {"getLayers", PyWrap( TechnicalDrawing, getLayers, "Get layers", vector<VRObjectPtr> ) },
    {NULL}  /* Sentinel */
};
