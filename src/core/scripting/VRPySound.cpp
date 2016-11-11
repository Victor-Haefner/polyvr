#include "VRPySound.h"
#include "VRPyBaseT.h"

#include "core/scene/sound/VRSoundManager.h"

using namespace OSG;

simpleVRPyType( Sound , New_ptr );

PyMethodDef VRPySound::methods[] = {
    {"play", (PyCFunction)VRPySound::play, METH_VARARGS, "Play the sound with the filename given as first parameter. \n The second paramater to 1 for loop, to 0 for no loop." },
    {"stop", (PyCFunction)VRPySound::stop, METH_VARARGS, "Stops a sound - stop(myPath.mp3)" },
    {"stopAllSounds", (PyCFunction)VRPySound::stopAllSounds, METH_NOARGS, "Stops all currently playing sounds." },
    {"setVolume", (PyCFunction)VRPySound::setVolume, METH_VARARGS, "Set sound volume - setVolume( float ) \n\tfrom 0.0 to 1.0 (= 100%)" },
    {"synthesize", (PyCFunction)VRPySound::synthesize, METH_VARARGS, "synthesize( Ac, wc, pc, Am, wm, pm, T)\t\n A,w,p are the amplitude, frequency and phase, c and m are the carrier sinusoid and modulator sinusoid, T is the packet duration in seconds" },
    {"synthBuffer", (PyCFunction)VRPySound::synthBuffer, METH_VARARGS, "synthBuffer( [[f,A]], T )\t\n [f,A] frequency/amplitude pairs, T is the packet duration in seconds" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySound::play(VRPySound* self, PyObject* args) {
    PyObject* path = NULL;
    long loop = 0;

    if (! PyArg_ParseTuple(args, "Ol", &path, &loop)) return NULL;
    if (path == NULL) { PyErr_SetString(err, "Missing path parameter"); return NULL; }

    VRSoundManager::get().playSound( PyString_AsString(path), loop);
    Py_RETURN_TRUE;
}

PyObject* VRPySound::stop(VRPySound* self, PyObject* args) {
    VRSoundManager::get().stopSound(parseString(args));
    Py_RETURN_TRUE;
}

PyObject* VRPySound::synthesize(VRPySound* self, PyObject* args) {
    float Ac, wc, pc, Am, wm, pm, T;
    if (! PyArg_ParseTuple(args, "fffffff", &Ac, &wc, &pc, &Am, &wm, &pm, &T)) return NULL;
    self->objPtr->synthesize(Ac, wc, pc, Am, wm, pm, T);
    Py_RETURN_TRUE;
}

PyObject* VRPySound::synthBuffer(VRPySound* self, PyObject* args) {
    float T;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "Of", &v, &T)) return NULL;
    vector<Vec2d> data;
    pyListToVector<vector<Vec2d>, Vec2d>(v, data);
    self->objPtr->synthBuffer(data, T);
    Py_RETURN_TRUE;
}

PyObject* VRPySound::stopAllSounds(VRPySound* self) {
    VRSoundManager::get().stopAllSounds();
    Py_RETURN_TRUE;
}

PyObject* VRPySound::setVolume(VRPySound* self, PyObject* args) {
    float vol = parseFloat(args);
    if ((vol < 0.0f) || (vol > 1.0f)) { PyErr_SetString(err, "Volume ranging from 0.0 to 1.0"); return NULL; }

    VRSoundManager::get().setSoundVolume(vol);
    Py_RETURN_TRUE;
}
