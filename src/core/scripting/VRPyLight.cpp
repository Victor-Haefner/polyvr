#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", PyWrap(Light, setOn, "Set light state", void, bool) },
    {"setBeacon", PyWrap(Light, setBeacon, "Set the light beacon", void, VRLightBeaconPtr) },
    {"getBeacon", PyWrap(Light, getBeacon, "Get the light beacon", VRLightBeaconPtr ) },
    {"setDiffuse", PyWrap(Light, setDiffuse, "Set diffuse light color", void, Color4f) },
    {"setAmbient", PyWrap(Light, setAmbient, "Set ambient light color", void, Color4f) },
    {"setSpecular", PyWrap(Light, setSpecular, "Set specular light color", void, Color4f) },
    {"setAttenuation", PyWrap(Light, setAttenuation, "Set light attenuation parameters, [constant, linear, quadratic]", void, Vec3d) },
    {"setType", PyWrap(Light, setType, "Set light type: 'point', 'directional', 'spot', 'photometric'", void, string) },
    {"setShadowParams", PyWrap(Light, setShadowParams, "Set shadow parameters", void, bool, int, Color4f) },
    {"setPhotometricMap", PyWrap(Light, setPhotometricMap, "Set map for photometric light, path to .ies file", void, VRTexturePtr) },
    {"loadPhotometricMap", PyWrap(Light, loadPhotometricMap, "Set map for photometric light, path to .ies file", void, string) },
    {"isOn", PyWrap(Light, isOn, "Return the state of the light", bool) },
    {NULL}
};



