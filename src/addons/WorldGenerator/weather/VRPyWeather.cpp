#include "VRPyWeather.h"
//#include "VRRain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRRainPtr& e) { return VRPyRain::fromSharedPtr(e); }

simpleVRPyType(Rain, New_ptr);

PyMethodDef VRPyRain::methods[] = {
    {"startRain", PyWrap(Rain, startRain, "Starts Rain", void) },
    {"stopRain", PyWrap(Rain, stopRain, "Stops Rain", void) },
    {"setRain", PyWrap(Rain, setRain, "Sets Rain Parameters", void, double , double) },
    {"getRain", PyWrap(Rain, getRain, "Gets Rain Parameters", Vec2d ) },
    {"overrideParameters", PyWrap(Rain, overrideParameters, "override Parameters", void, double, double, double, double ) },
    {"updateRain", PyWrap(Rain, updateRain, "update rain", void, float ) },
    {NULL}  /* Sentinel */
};
