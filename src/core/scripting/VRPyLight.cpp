#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", PyWrap(Light, setOn, void, bool), "Set light state - setOn(bool)" },
    {"setBeacon", PyWrap(Light, setBeacon, void, VRLightBeaconPtr), "Set beacon - setBeacon( beacon )" },
    {"setDiffuse", PyWrap(Light, setDiffuse, void, Color4f), "Set diffuse light color - setDiffuse( [r,g,b,a] )" },
    {"setAmbient", PyWrap(Light, setAmbient, void, Color4f), "Set ambient light color - setAmbient( [r,g,b,a] )" },
    {"setSpecular", PyWrap(Light, setSpecular, void, Color4f), "Set specular light color - setSpecular( [r,g,b,a] )" },
    {"setAttenuation", PyWrap(Light, setAttenuation, void, Vec3f), "Set light attenuation parameters - setAttenuation( [C,L,Q] )\n\twhere C is the constant attenuation, L the linear and Q the quadratic one" },
    {"setType", PyWrap(Light, setType, void, string), "Set light type - setType( str type )\n\twhere type can be 'point', 'directional', 'spot'" },
    {"setShadowParams", PyWrap(Light, setShadowParams, void, bool, int, Color4f), "Set shadow parameters - setShadowParams( bool toggle, int map resolution, [r,g,b,a] color )" },
    {"isOn", PyWrap(Light, isOn, bool), "Return the state of the light - bool isOn()" },
    {NULL}
};



