#include "VRPySound.h"
#include "VRPyBaseT.h"

#include "core/scene/sound/VRSoundManager.h"

using namespace OSG;

simpleVRPyType( SoundManager, 0 );
simpleVRPyType( Sound, New_ptr );

PyMethodDef VRPySound::methods[] = {
    {"play", PyWrap(Sound, play, "Play sound", void) },
    {"stop", PyWrap(Sound, stop, "Stop sound", void) },
    {"pause", PyWrap(Sound, pause, "Pause sound", void) },
    {"resume", PyWrap(Sound, resume, "Resume paused sound", void) },
    {"setPath", PyWrap(Sound, setPath, "Stop sound", void, string ) },
    {"setLoop", PyWrap(Sound, setLoop, "Stop sound", void, bool ) },
    {"setPitch", PyWrap(Sound, setPitch, "Stop sound", void, float ) },
    {"setVolume", PyWrap(Sound, setVolume, "Set sound volume, try setting master volume on soundmanager", void, float ) },
    {"setUser", PyWrap(Sound, setUser, "Stop sound", void, Vec3d, Vec3d ) },
    {"isRunning", PyWrap(Sound, isRunning, "Check if sound is running", bool) },
    {"synthesize", PyWrap(Sound, synthesize, "synthesize( Ac, wc, pc, Am, wm, pm, T)\t\n A,w,p are the amplitude, frequency and phase, c and m are the carrier sinusoid and modulator sinusoid, T is the packet duration in seconds", void, float, float, float, float, float, float, float) },
    {"synthBuffer", PyWrap(Sound, synthBuffer, "synthBuffer( [[f,A]], [[f,A]], T )\t\n [f,A] frequency/amplitude pairs, interpolate the two spectra, T is the packet duration in seconds", vector<short>, vector<Vec2d>, vector<Vec2d>, float) },
    {"synthBufferOnChannels", PyWrap(Sound, synthBufferOnChannels, "synthBufferOnChannels( [[[f,A]]], [[[f,A]]], T)\n\t [[f,A]] list of channels with each containing a list of frequency/amplitude pairs in channel order, interpolate the two spectra\n\tT is the packet duration in seconds\n\t", void, vector<vector<Vec2d>>, vector<vector<Vec2d>>, float) },
    {"synthSpectrum", PyWrap(Sound, synthSpectrum, "synthSpectrum( [A], int S, float T, float F, bool retBuffer )\t\n A amplitude, S sample rate, T packet duration in seconds, F fade in/out duration in s , specify if you want to return the generated buffer", vector<short>, vector<double>, uint, float, float, bool) },
    {"getQueuedBuffer", PyWrap(Sound, getQueuedBuffer, "Get the buffer currently queued", int) },
    {"recycleBuffer", PyWrap(Sound, recycleBuffer, "Recycle unused buffers", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySoundManager::methods[] = {
    {"setupSound", PyWrapOpt(SoundManager, setupSound, "Play sound, lopping and playing are optional", "0|0", VRSoundPtr, string, bool, bool) },
    {"stopAllSounds", PyWrap(SoundManager, stopAllSounds, "Stops all currently playing sounds.", void) },
    {"setVolume", PyWrap(SoundManager, setVolume, "Set sound volume from 0 to 1", void, float) },
    {"queueSounds", PyWrap(SoundManager, queueSounds, "Queue a list of sounds", void, vector<VRSoundPtr>) },
    {NULL}  /* Sentinel */
};

/*PyObject* VRPySound::play(VRPySound* self, PyObject* args) {
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

PyObject* VRPySound::getQueuedBuffer(VRPySound* self) {
    int qb = self->objPtr->getQueuedBuffer();
    return PyInt_FromLong(qb);
}

PyObject* VRPySound::recycleBuffer(VRPySound* self) {
    self->objPtr->recycleBuffer();
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
    PyObject *v1, *v2;
    if (! PyArg_ParseTuple(args, "OOf", &v1, &v2, &T)) return NULL;
    vector<Vec2d> data1, data2;
    pyListToVector<vector<Vec2d>, Vec2d>(v1, data1);
    pyListToVector<vector<Vec2d>, Vec2d>(v2, data2);

    auto buf = self->objPtr->synthBuffer(data1, data2, T);
    auto res = PyList_New(buf.size());
    for (uint i=0; i<buf.size(); i++) PyList_SetItem(res, i, PyInt_FromLong(buf[i]));
    return res;
}

PyObject* VRPySound::synthSpectrum(VRPySound* self, PyObject* args) {
    int S, doRB = 0;
    float T, F;
    PyObject* v;
    if (! PyArg_ParseTuple(args, "Oiff|i", &v, &S, &T, &F, &doRB)) return NULL;

    Py_ssize_t N = PyList_Size(v);
    vector<double> data(N);
    for (Py_ssize_t i=0; i<N; i++) {
        auto f = PyList_GetItem(v, i);
        data[i] = PyFloat_AsDouble(f);
    }

    auto buf = self->objPtr->synthesizeSpectrum(data, S, T, F, doRB);
    auto res = PyList_New(buf.size());
    for (uint i=0; i<buf.size(); i++) PyList_SetItem(res, i, PyInt_FromLong(buf[i]));
    return res;
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
}*/
