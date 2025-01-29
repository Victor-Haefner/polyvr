#include "VRPyAnnotationEngine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;
simpleVRPyType(AnnotationEngine, New_VRObjects_ptr);

typedef map<int, string> mapIntStr;

PyMethodDef VRPyAnnotationEngine::methods[] = {
    {"set", PyWrap( AnnotationEngine, set,"Set label - set(int i, [x,y,z] pos, str val)", void, int, Vec3d, string ) },
    {"add", PyWrap( AnnotationEngine, add,"Append label - id add([x,y,z] pos, str val)", int, Vec3d, string ) },
    {"clear", PyWrap( AnnotationEngine, clear, "Clear numbers", void ) },
    {"setMask", PyWrap( AnnotationEngine, setMask, "Set mask to ith label - setMask(i, from, to)\n from, to go from 0.0 to 1.0", void, int, float, float ) },
    {"setSize", PyWrap( AnnotationEngine, setSize, "Set font height - setSize( float )", void, float ) },
    {"setColor", PyWrap( AnnotationEngine, setColor, "Set font color - setColor( [r,g,b,a] )", void, Color4f ) },
    {"setBackground", PyWrap( AnnotationEngine, setBackground, "Set background color - setBackground( [r,g,b,a] )", void, Color4f ) },
    {"setOutline", PyWrap( AnnotationEngine, setOutline, "Set outline - setOutline( radius, [r,g,b,a] )", void, int, Color4f ) },
    {"setBillboard", PyWrap( AnnotationEngine, setBillboard, "Set billboard - setBillboard( bool )", void, bool ) },
    {"setScreensize", PyWrap( AnnotationEngine, setScreensize, "Set screensize mode", void, bool ) },
    {"setOrientation", PyWrap( AnnotationEngine, setOrientation, "Set sprite orientation, dir and up", void, Vec3d, Vec3d ) },
    {"getLabels", PyWrap( AnnotationEngine, getLabels, "Get all labels", mapIntStr ) },
    {"getLabel", PyWrap( AnnotationEngine, getLabel, "Get ith label", string, int ) },
    {NULL}  /* Sentinel */
};
