#include "VRPyRecorder.h"
#include "VRPyBaseT.h"

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
    {NULL}  /* Sentinel */
};

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
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::compile(VRPyRecorder* self, PyObject* args) {
    if (self->obj) self->obj->compile( parseString(args) );
    Py_RETURN_TRUE;
}
