#include "VRPyAnnotationEngine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;
simpleVRPyType(AnnotationEngine, New_VRObjects_ptr);

PyMethodDef VRPyAnnotationEngine::methods[] = {
    {"set", PyWrap( AnnotationEngine, set,"Set label - set(int i, [x,y,z] pos, str val)", void, int, Vec3d, string ) },
    {"clear", PyWrap( AnnotationEngine, clear, "Clear numbers", void ) },
    {"setSize", PyWrap( AnnotationEngine, setSize, "Set font height - setSize( float )", void, float ) },
    {"setColor", PyWrap( AnnotationEngine, setColor, "Set font color - setColor( [r,g,b,a] )", void, Color4f ) },
    {"setBackground", PyWrap( AnnotationEngine, setBackground, "Set background color - setBackground( [r,g,b,a] )", void, Color4f ) },
    {"setBillboard", PyWrap( AnnotationEngine, setBillboard, "Set billboard - setBillboard( bool )", void, bool ) },
    {"setScreensize", PyWrap( AnnotationEngine, setScreensize, "Set screensize mode", void, bool ) },
    {NULL}  /* Sentinel */
};
