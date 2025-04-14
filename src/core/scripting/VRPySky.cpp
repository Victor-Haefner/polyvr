#include "VRPySky.h"
#include "VRPyBaseT.h"
#include "core/scene/VRScene.h"

using namespace OSG;

simpleVRPyType(Sky, New_toZero);
simpleVRPyType(Background, New_toZero);

template<> bool toValue(PyObject* o, VRBackground::TYPE& v) {
    if (!PyUnicode_Check(o)) return 0;
    string s = PyUnicode_AsUTF8(o);
    if (s == "SOLID") { v = VRBackground::SOLID; return 1; }
    if (s == "IMAGE") { v = VRBackground::IMAGE; return 1; }
    if (s == "SKYBOX") { v = VRBackground::SKYBOX; return 1; }
    if (s == "SKY") { v = VRBackground::SKY; return 1; }
    return 0;
}

PyMethodDef VRPyBackground::methods[] = {
    {"setBackground", PyWrap(Background, setBackground, "Set the background type", void, VRBackground::TYPE) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySky::methods[] = {
    {"setTime", PyWrap(Sky, setTime, "Set the current date and time for the sky model, flt seconds, int hours, int days, int year", void, double, int, int, int) },
    {"setClouds",  PyWrap(Sky, setClouds, "Set the overcast conditions, density, scale, height, speed", void, float, float, float, Vec2d, Color4f) },
    {"setLuminance",  PyWrap(Sky, setLuminance, "Set the luminance parameters, turbidity", void, float) },
    {"setGround",  PyWrap(Sky, setGround, "Set ground color", void, Color4f) },
    {"setPosition",  PyWrap(Sky, setPosition, "Set the latitude and longitude of the observer, flt latitude, flt longitude", void, float, float) },
    {"setSpeed",  PyWrap(Sky, setSpeed, "Set a time modifier", void, float) },
    {"getSpeed",  PyWrap(Sky, getSpeed, "Get the time modifier", float) },
    {"getHour",  PyWrap(Sky, getHour, "get Time, in hours", int, void) },
    {"getSunPos",  PyWrap(Sky, getSunPos, "get Poisition of sun", Vec3d, void) },
    {NULL}  /* Sentinel */
};




