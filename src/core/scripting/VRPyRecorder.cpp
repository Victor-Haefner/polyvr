#include "VRPyRecorder.h"
#include "VRPyBaseT.h"
#include "VRPyTransform.h"
#include "VRPyImage.h"

template<> PyTypeObject VRPyBaseT<OSG::VRRecorder>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Recorder",             /*tp_name*/
    sizeof(VRPyRecorder),             /*tp_basicsize*/
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
    "VRRecorder binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyRecorder::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef VRPyRecorder::methods[] = {
    {"capture", (PyCFunction)VRPyRecorder::capture, METH_NOARGS, "Capture the current view as an image - capture()" },
    {"compile", (PyCFunction)VRPyRecorder::compile, METH_VARARGS, "Compile the recorded frames to a video file on disk - compile(string path)" },
    {"setView", (PyCFunction)VRPyRecorder::setView, METH_VARARGS, "Set the view to record - setView(int ID)" },
    {"clear", (PyCFunction)VRPyRecorder::clear, METH_NOARGS, "Clear the captures frames - clear()" },
    {"getRecordingSize", (PyCFunction)VRPyRecorder::getRecordingSize, METH_NOARGS, "Get the number of captured frames - int getRecordingSize()" },
    {"getRecordingLength", (PyCFunction)VRPyRecorder::getRecordingLength, METH_NOARGS, "Get the length in seconds fromt he first to last of the captured frames - float getRecordingLength()" },
    {"setMaxFrames", (PyCFunction)VRPyRecorder::setMaxFrames, METH_VARARGS, "Set the maximum number of frames" },
    {"frameLimitReached", (PyCFunction)VRPyRecorder::frameLimitReached, METH_NOARGS, "Check if the frame limit has been reached" },
    {"get", (PyCFunction)VRPyRecorder::get, METH_VARARGS, "Get capture i - get(int i)" },
    {"setTransform", (PyCFunction)VRPyRecorder::setTransform, METH_VARARGS, "Apply the transform of the camera pose of frame i - setTransform(transform t, int i)" },
    {"getFrom", (PyCFunction)VRPyRecorder::getFrom, METH_VARARGS, "Get the position of the camera pose of frame i - getFrom(int i)" },
    {"getDir", (PyCFunction)VRPyRecorder::getDir, METH_VARARGS, "Get the direction of the camera pose of frame i - getDir(int i)" },
    {"getAt", (PyCFunction)VRPyRecorder::getAt, METH_VARARGS, "Get the at vector of the camera pose of frame i - getAt(int i)" },
    {"getUp", (PyCFunction)VRPyRecorder::getUp, METH_VARARGS, "Get the up vector of the camera pose of frame i - getUp(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRecorder::setTransform(VRPyRecorder* self, PyObject* args) {
	VRPyTransform* t; int i;
    if (!PyArg_ParseTuple(args, "Oi", &t, &i)) return NULL;
    self->obj->setTransform(t->objPtr,i);
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::get(VRPyRecorder* self, PyObject* args) {
    int i = parseInt(args);
    return VRPyImage::fromSharedPtr( self->obj->get(i) );
}

PyObject* VRPyRecorder::getFrom(VRPyRecorder* self, PyObject* args) { return toPyTuple(self->obj->getFrom( parseInt(args) ) ); }
PyObject* VRPyRecorder::getDir(VRPyRecorder* self, PyObject* args) { return toPyTuple(self->obj->getDir( parseInt(args) ) ); }
PyObject* VRPyRecorder::getAt(VRPyRecorder* self, PyObject* args) { return toPyTuple(self->obj->getAt( parseInt(args) ) ); }
PyObject* VRPyRecorder::getUp(VRPyRecorder* self, PyObject* args) { return toPyTuple(self->obj->getUp( parseInt(args) ) ); }

PyObject* VRPyRecorder::setMaxFrames(VRPyRecorder* self, PyObject* args) {
    if (self->obj) self->obj->setMaxFrames( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::frameLimitReached(VRPyRecorder* self) {
    return PyBool_FromLong( self->obj->frameLimitReached() );
}

PyObject* VRPyRecorder::getRecordingSize(VRPyRecorder* self) {
    int size = 0;
    if (self->obj) size = self->obj->getRecordingSize();
    return PyInt_FromLong(size);
}

PyObject* VRPyRecorder::getRecordingLength(VRPyRecorder* self) {
    float length = 0;
    if (self->obj) length = self->obj->getRecordingLength();
    return PyFloat_FromDouble(length);
}

PyObject* VRPyRecorder::clear(VRPyRecorder* self) {
    if (self->obj) self->obj->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::setView(VRPyRecorder* self, PyObject* args) {
    if (self->obj) self->obj->setView( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::capture(VRPyRecorder* self) {
    if (self->obj) self->obj->capture();
    int size = 0;
    if (self->obj) size = self->obj->getRecordingSize();
    return PyInt_FromLong(size-1);
}

PyObject* VRPyRecorder::compile(VRPyRecorder* self, PyObject* args) {
    if (self->obj) self->obj->compile( parseString(args) );
    Py_RETURN_TRUE;
}
