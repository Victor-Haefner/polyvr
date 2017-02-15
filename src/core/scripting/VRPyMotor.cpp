#include "VRPyMotor.h"
#include "core/scripting/VRPyBaseT.h"

#include "core/scene/sound/VRSoundManager.h"

using namespace OSG;
using namespace rpmTool;

simpleVRPyType( Motor , New_ptr );

PyMethodDef VRPyMotor::methods[] = {
    {"load", (PyCFunction)VRPyMotor::load, METH_VARARGS, "Load spectrum data - load( str path )" },
    {"stopAllSounds", (PyCFunction)VRPyMotor::stopAllSounds, METH_NOARGS, "Stops all currently playing sounds." },
    {"setVolume", (PyCFunction)VRPyMotor::setVolume, METH_VARARGS, "Set sound volume - setVolume( float ) \n\tfrom 0.0 to 1.0 (= 100%)" },
    {"play", (PyCFunction)VRPyMotor::play, METH_VARARGS, "play( rpm )\t\n rpm is the motor speed to synthesize" },
    {"playCurrent", (PyCFunction)VRPyMotor::playCurrent, METH_NOARGS, "Synthesize the current stored rpm value" },
    {"setRPM", (PyCFunction)VRPyMotor::setRPM, METH_VARARGS, "Set the motor rpm value" },
    {"getRPM", (PyCFunction)VRPyMotor::getRPM, METH_NOARGS, "Return the motor's current rpm value" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMotor::load(VRPyMotor* self, PyObject *args) {
    const char* path = 0;
    if (! PyArg_ParseTuple(args, "s", (char*)&path)) return NULL;
    if (path) self->objPtr->load(path);
    Py_RETURN_TRUE;
}

PyObject* VRPyMotor::play(VRPyMotor* self, PyObject* args) {
    float rpm;
    if (! PyArg_ParseTuple(args, "f", &rpm)) return NULL;
    self->objPtr->play(rpm);
    Py_RETURN_TRUE;
}

PyObject* VRPyMotor::playCurrent(VRPyMotor* self) {
    if (!self->valid()) return NULL;
    self->objPtr->play();
    Py_RETURN_TRUE;
}

PyObject* VRPyMotor::setRPM(VRPyMotor* self, PyObject* args) {
    float rpm;
    if (! PyArg_ParseTuple(args, "f", &rpm)) return NULL;
    self->objPtr->setRPM(rpm);
    Py_RETURN_TRUE;
}

PyObject* VRPyMotor::getRPM(VRPyMotor* self) {
    if (!self->valid()) return NULL;
    return PyFloat_FromDouble( self->objPtr->getRPM() );
}

PyObject* VRPyMotor::stopAllSounds(VRPyMotor* self) {
    VRSoundManager::get().stopAllSounds();
    Py_RETURN_TRUE;
}

PyObject* VRPyMotor::setVolume(VRPyMotor* self, PyObject* args) {
    float vol = parseFloat(args);
    if ((vol < 0.0f) || (vol > 1.0f)) { PyErr_SetString(err, "Volume ranging from 0.0 to 1.0"); return NULL; }

    VRSoundManager::get().setSoundVolume(vol);
    Py_RETURN_TRUE;
}

