#include "VRPyWeather.h"
//#include "VRRain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRRainPtr& e) { return VRPyRain::fromSharedPtr(e); }

simpleVRPyType(Rain, New_ptr);

PyMethodDef VRPyRain::methods[] = {
    {"start", PyWrap(Rain, start, "Starts Rain", void) },
    {"stop", PyWrap(Rain, stop, "Stops Rain", void) },
    {"setScale", PyWrap(Rain, setScale, "Sets Scale", void, float) },
    {"get", PyWrap(Rain, get, "Gets Rain Parameters", Vec2d ) },
    {"overrideParameters", PyWrap(Rain, overrideParameters, "override Parameters \n overrideParameters(density, durationTransition, color, light) \n", void, float, float, float, float ) },
    {"reloadShader", PyWrap(Rain, reloadShader, "reloadShader", void) },
    {NULL}  /* Sentinel */
};
