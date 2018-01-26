#include "VRPyWeather.h"

#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Rain, New_ptr);
simpleVRPyType(RainCarWindshield, New_ptr);

PyMethodDef VRPyRain::methods[] = {
    {"start", PyWrap(Rain, start, "Starts Rain", void) },
    {"stop", PyWrap(Rain, stop, "Stops Rain", void) },
    {"setScale", PyWrap(Rain, setScale, "Sets Scale", void, float) },
    {"get", PyWrap(Rain, get, "Gets Scale", float ) },
    {"overrideParameters", PyWrap(Rain, overrideParameters, "7 parameters: tTransition, densRain, densCL, vX, vY, clCL, light \n\t", void, float, float, float, float, float, float, float ) },
    //{"doTestFunction", PyWrap(Rain, doTestFunction, "doTestFunction", void) },
    {"getRenderer", PyWrap(Rain, getRenderer, "Get renderer", VRTextureRendererPtr) },
    //{"getTexMat", PyWrap(Rain, getTexMat, "getTexMat", VRMaterialPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRainCarWindshield::methods[] = {
    {"doTestFunction", PyWrap(RainCarWindshield, doTestFunction, "do Test Function", void ,void) },
    {"setWindshield", PyWrap(RainCarWindshield, setWindshield, "setWindshield", void, VRGeometryPtr) },
    {"setScale", PyWrap(RainCarWindshield, setScale, "sets scale", void, float) },
    {"start", PyWrap(RainCarWindshield, start, "starts rain on Windshield", void, void) },
    {"stop", PyWrap(RainCarWindshield, stop, "stops rain on Windshield", void, void) },
    {NULL}  /* Sentinel */
};

