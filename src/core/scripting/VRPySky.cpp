#include "VRPySky.h"
#include "VRPyBaseT.h"
#include "core/scene/VRScene.h"

using namespace OSG;

simpleVRPyType(Sky, New_toZero);

PyMethodDef VRPySky::methods[] = {
    {"setTime", PyWrap(Sky, setTime, "Set the current date and time for the sky model, flt seconds, int hours, int days, int year", void, double, int, int, int) },
    {"setClouds",  PyWrap(Sky, setClouds, "Set the overcast conditions, density, scale, height, speed", void, float, float, float, Vec2d, Color4f) },
    {"setLuminance",  PyWrap(Sky, setLuminance, "Set the luminance parameters, turbidity", void, float) },
    {"setPosition",  PyWrap(Sky, setPosition, "Set the latitude and longitude of the observer, flt latitude, flt longitude", void, float, float) },
    {"setSpeed",  PyWrap(Sky, setSpeed, "Set a time modifier", void, float) },
    {"getSpeed",  PyWrap(Sky, getSpeed, "Get the time modifier", float) },
    {"getHour",  PyWrap(Sky, getHour, "get Time, in hours", int, void) },
    {"getSunPos",  PyWrap(Sky, getSunPos, "get Poisition of sun", Vec3d, void) },
    {NULL}  /* Sentinel */
};




