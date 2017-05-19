#include "VRPySky.h"
#include "VRPyBaseT.h"
#include "core/scene/VRScene.h"

using namespace OSG;

simpleVRPyType(Sky, New_toZero);

PyMethodDef VRPySky::methods[] = {
    {"setDateTime", (PyCFunction)VRPySky::setDateTime, METH_VARARGS, "Set the current date and time for the sky model - setDate( flt seconds, int hours, int days, int year)" },
    {"setWeather", (PyCFunction)VRPySky::setWeather, METH_VARARGS, "Set the wind and cloud conditions - setWeather( flt cloudCover (in [0,1]), flt cloudHeight (in [0,1]), flt wind (in [0,1]), flt haze (in [0,1]) ) " },
    {"setPosition", (PyCFunction)VRPySky::setPosition, METH_VARARGS, "Set the latitude and longitude of the observer -setPosition( flt latitude, flt longitude) " },
    {"setSpeed", (PyCFunction)VRPySky::setSpeed, METH_VARARGS, "Set the rate at which time passes in the sky model (default=1) -setSpeed( flt speed) " },
    {NULL}  /* Sentinel */
};

PyObject* VRPySky::setDateTime(VRPySky* self, PyObject* args) {
    int h,d,y;
    float s;
    if (! PyArg_ParseTuple(args, "fiii", &s, &h, &d, &y)) return NULL;
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setTime(s, h, d, y);
    Py_RETURN_TRUE;
}

PyObject* VRPySky::setWeather(VRPySky* self, PyObject* args) {
    float cc, ch, w, h;
    if (! PyArg_ParseTuple(args, "ffff", &cc, &ch, &w, &h)) return NULL;
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setWeather(cc, ch, w, h);
    Py_RETURN_TRUE;
}

PyObject* VRPySky::setPosition(VRPySky* self, PyObject* args) {
    float lt, lg;
    if (! PyArg_ParseTuple(args, "ff", &lt, &lg)) return NULL;
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setPosition(lt, lg);
    Py_RETURN_TRUE;
}

PyObject* VRPySky::setSpeed(VRPySky* self, PyObject* args) {
    float s;
    if (! PyArg_ParseTuple(args, "f", &s)) return NULL;
    auto sky = VRScene::getCurrent()->getSky();
    if (sky) sky->setSpeed(s);
    Py_RETURN_TRUE;
}




