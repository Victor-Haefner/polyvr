#include "VRPyRain.h"
#include "VRRain.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"
//#include "VRRain.h"

using namespace OSG;

//template<> PyObject* VRPyTypeCaster::cast(const VRRainPtr& e) { return VRPyRain::fromSharedPtr(e); }

//simpleVRPyType(Rain, New_VRObjects_ptr);
/*
PyMethodDef VRPyRain::methods[] = {
    {"startRain", PyWrap(Rain, startRain, "Start Rain", void, void) },
    {"stopRain", PyWrap(Rain, stopRain, "Stop Rain", void, void) },

    {"setRain", PyWrap(Rain, setRain, "Sets Rain Parameters", void, double , double) },
    {"getRain", PyWrap(Rain, getRain, "Gets Rain Parameters", double, void ) },

    {"overrideParameters", PyWrap(Rain, overrideParameters, "override Parameters", void, double, double, double, double ) },
    {NULL}  /* Sentinel */
//};

