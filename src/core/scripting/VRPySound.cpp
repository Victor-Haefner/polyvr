#include "VRPySound.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSoundManager>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Sound",             /*tp_name*/
    sizeof(VRPySound),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRSound binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySound::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMethodDef VRPySound::methods[] = {
    {"play", (PyCFunction)VRPySound::play, METH_VARARGS, "Play the sound with the filename given as first parameter. \n The second paramater to 1 for loop, to 0 for no loop." },
    {"stop", (PyCFunction)VRPySound::stop, METH_NOARGS, "Stops the currently playing sound." },
    {"stopAllSounds", (PyCFunction)VRPySound::stopAllSounds, METH_NOARGS, "Stops all currently playing sounds." },
    {"setVolume", (PyCFunction)VRPySound::setVolume, METH_VARARGS, " Set music volume to the specified value ranging from 0.0 to 1.0 (= 100%)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySound::play(VRPySound* self, PyObject* args) {
    PyObject* path = NULL; long loop=0;
    if (! PyArg_ParseTuple(args, "Ol", &path, &loop)) return NULL;

    if (path == NULL) {
        PyErr_SetString(err, "Missing path parameter");
        return NULL;
    }

    string _path = PyString_AsString(path);

    //bool _loop = false;
    //if(loop==1)_loop = true;

    //if(_loop) OSG::VRSoundManager::get().playMusic(_path);
    //else
    OSG::VRSoundManager::get().playSound(_path);
    Py_RETURN_TRUE;
}


PyObject* VRPySound::stop(VRPySound* self) {

     OSG::VRSoundManager::get().stopMusic();
    Py_RETURN_TRUE;
}

PyObject* VRPySound::stopAllSounds(VRPySound* self) {
     OSG::VRSoundManager::get().stopAllSounds();
    Py_RETURN_TRUE;
}

PyObject* VRPySound::setVolume(VRPySound* self, PyObject* args) {
    float vol = 0;
    if (! PyArg_ParseTuple(args, "f", &vol)) return NULL;


    //float _vol = PyFloat_AsDouble(vol); //Richtig so?

    OSG::VRSoundManager::get().setMusicVolume(vol);
    OSG::VRSoundManager::get().setSoundVolume(vol);
    Py_RETURN_TRUE;
}
