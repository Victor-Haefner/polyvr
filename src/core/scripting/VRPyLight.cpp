#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", PySetter(Light, setOn, bool), "Set light state - setOn(bool)" },
    {"setBeacon", PySetter(Light, setBeacon, VRLightBeaconPtr), "Set beacon - setBeacon( beacon )" },
    {"setDiffuse", PySetter(Light, setDiffuse, Color4f), "Set diffuse light color - setDiffuse( [r,g,b,a] )" },
    {"setAmbient", PySetter(Light, setAmbient, Color4f), "Set ambient light color - setAmbient( [r,g,b,a] )" },
    {"setSpecular", PySetter(Light, setSpecular, Color4f), "Set specular light color - setSpecular( [r,g,b,a] )" },
    {"setAttenuation", PySetter(Light, setAttenuation, Vec3f), "Set light attenuation parameters - setAttenuation( [C,L,Q] )\n\twhere C is the constant attenuation, L the linear and Q the quadratic one" },
    {NULL}  /* Sentinel */
};



