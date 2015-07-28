#include "VRPyNumberingEngine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"


template<> PyTypeObject VRPyBaseT<VRNumberingEngine>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.NumberEngine",             /*tp_name*/
    sizeof(VRPyNumberingEngine),             /*tp_basicsize*/
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
    "NumberingEngine binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyNumberingEngine::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_unnamed,                 /* tp_new */
};

PyMethodDef VRPyNumberingEngine::methods[] = {
    {"add", (PyCFunction)VRPyNumberingEngine::add, METH_VARARGS, "Add N numbers - add(int N)" },
    {"set", (PyCFunction)VRPyNumberingEngine::set, METH_VARARGS, "Set number - set(int i, [x,y,z] pos, float val)" },
    {"clear", (PyCFunction)VRPyNumberingEngine::clear, METH_NOARGS, "Clear numbers" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyNumberingEngine::add(VRPyNumberingEngine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyNumberingEngine::add - Object is invalid"); return NULL; }
    self->obj->add(OSG::Vec3f(), parseInt(args));
    Py_RETURN_TRUE;
}

PyObject* VRPyNumberingEngine::set(VRPyNumberingEngine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyNumberingEngine::set - Object is invalid"); return NULL; }

    int i;
    float f;
    PyObject* p;
    if (! PyArg_ParseTuple(args, "iOf", &i, &p, &f)) return NULL;

    OSG::Vec3f pos;
    vector<PyObject*> v = pyListToVector(p);
    pos[0] = PyFloat_AsDouble(v[0]);
    pos[1] = PyFloat_AsDouble(v[1]);
    pos[2] = PyFloat_AsDouble(v[2]);

    self->obj->set(i, pos, f);
    Py_RETURN_TRUE;
}

PyObject* VRPyNumberingEngine::clear(VRPyNumberingEngine* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyNumberingEngine::clear - Object is invalid"); return NULL; }
    self->obj->clear();
    Py_RETURN_TRUE;
}
