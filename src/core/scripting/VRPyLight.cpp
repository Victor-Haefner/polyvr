#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"
#include "VRPyTypeCaster.h"

template<typename sT, typename T, T> struct proxyWrap;
template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...)>
struct proxyWrap<sT, R (T::*)(Args...), mf> {
    static PyObject* exec(sT* self, PyObject* args);
};

template<typename sT, typename T, typename R, typename ...Args, R (T::*mf)(Args...)>
PyObject* proxyWrap<sT, R (T::*)(Args...), mf>::exec(sT* self, PyObject* args) {
    if (!self->valid()) return NULL;
    //pyT val;
    //if( !parseValue<pyT>(args, val) ) return NULL;
    //auto res = (self->objPtr.get()->*mf)(true, 0, OSG::Vec4f());
    (self->objPtr.get()->*mf)(true, 0, OSG::Vec4f());
    //return VRPyTypeCaster::cast( res );
}

//template <typename T, typename R, typename ...Args>
//VRSemanticBuiltinPtr addBuiltin(R (T::*callback)(Args...) );

#define PyWrap(X, Y) \
(PyCFunction)proxyWrap<VRPy ## X, void (OSG::VR ## X::*)(Args...), &OSG::VR ## X::Y>::exec \
, METH_VARARGS


using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", PySetter(Light, setOn, bool), "Set light state - setOn(bool)" },
    {"setBeacon", PySetter(Light, setBeacon, VRLightBeaconPtr), "Set beacon - setBeacon( beacon )" },
    {"setDiffuse", PySetter(Light, setDiffuse, Color4f), "Set diffuse light color - setDiffuse( [r,g,b,a] )" },
    {"setAmbient", PySetter(Light, setAmbient, Color4f), "Set ambient light color - setAmbient( [r,g,b,a] )" },
    {"setSpecular", PySetter(Light, setSpecular, Color4f), "Set specular light color - setSpecular( [r,g,b,a] )" },
    {"setAttenuation", PySetter(Light, setAttenuation, Vec3f), "Set light attenuation parameters - setAttenuation( [C,L,Q] )\n\twhere C is the constant attenuation, L the linear and Q the quadratic one" },
    {"setType", PySetter(Light, setType, string), "Set light type - setType( str type )\n\twhere type can be 'point', 'directional', 'spot'" },
    //{"setShadowParams", PyWrap(Light, setShadowParams), "Set shadow parameters - setShadowParams( bool toggle, int map resolution, [r,g,b,a] color )" },
    {"setShadowParams"
    , (PyCFunction) proxyWrap<VRPyLight, void (OSG::VRLight::*)(bool, int, Color4f), &OSG::VRLight::setShadowParams>::exec
    , METH_VARARGS
    , "Set shadow parameters - setShadowParams( bool toggle, int map resolution, [r,g,b,a] color )" },
    {NULL}  /* Sentinel */
};



