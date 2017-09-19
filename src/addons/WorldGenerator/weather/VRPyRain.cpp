#include "VRPyRain.h"

using namespace OSG;

template<> PyObject* VRPyTypeCaster::cast(const VRRainPtr& e) { return VRPyRain::fromSharedPtr(e); }

//simpleVRPyType(Rain, New_VRObjects_ptr);

PyMethodDef VRPyRain::methods[] = {
    {"startRain", PyWrap(Rain, startRain, "Start Rain", void) },
    {"stopRain", PyWrap(Rain, stopRain, "Stop Rain", void) },

    {"setRain", PyWrap(Rain, setRain, "Sets Rain Parameters", void, double , double) },
    {"getRain", PyWrap(Rain, getRain, "Gets Rain Parameters", void, double ) },

    {"overrideParameters", PyWrap(Rain, overrideParameters, void, double, double, double, double ) },
    {NULL}  /* Sentinel */
};
