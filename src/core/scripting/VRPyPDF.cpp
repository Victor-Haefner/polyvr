#include "VRPyPDF.h"

#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( PDF, New_named_ptr);

PyMethodDef VRPyPDF::methods[] = {
    {"write", PyWrap(PDF, write, "Write to file", void, string ) },
    {"project", PyWrap(PDF, project, "Project scene on 2D plane, dir is plane normal", void, VRObjectPtr, PosePtr ) },
    {"slice", PyWrap(PDF, slice, "Generate a 2D slice of the scene, dir is plane normal", void, VRObjectPtr, PosePtr ) },
    {NULL}  /* Sentinel */
};
