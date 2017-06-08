#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", PyWrap(Light, setOn, "Set light state", void, bool) },
    {"setBeacon", PyWrap(Light, setBeacon, "Set light beacon", void, VRLightBeaconPtr) },
    {"setDiffuse", PyWrap(Light, setDiffuse, "Set diffuse light color", void, Color4f) },
    {"setAmbient", PyWrap(Light, setAmbient, "Set ambient light color", void, Color4f) },
    {"setSpecular", PyWrap(Light, setSpecular, "Set specular light color", void, Color4f) },
    {"setAttenuation", PyWrap(Light, setAttenuation, "Set light attenuation parameters, [constant, linear, quadratic]", void, Vec3f) },
    {"setType", PyWrap(Light, setType, "Set light type: 'point', 'directional' or 'spot'", void, string) },
    {"setShadowParams", PyWrap(Light, setShadowParams, "Set shadow parameters", void, bool, int, Color4f) },
    {"isOn", PyWrap(Light, isOn, "Return the state of the light", bool) },
    {NULL}
};



