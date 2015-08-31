#include "VRPyStroke.h"
#include "VRPyPath.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"

template<> PyTypeObject VRPyBaseT<OSG::VRStroke>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Stroke",             /*tp_name*/
    sizeof(VRPyStroke),             /*tp_basicsize*/
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
    "Stroke binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyStroke::methods,             /* tp_methods */
    VRPyStroke::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMemberDef VRPyStroke::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyStroke::methods[] = {
    {"setPath", (PyCFunction)VRPyStroke::setPath, METH_VARARGS, "Set a single path" },
    {"addPath", (PyCFunction)VRPyStroke::addPath, METH_VARARGS, "Add a path" },
    {"setPaths", (PyCFunction)VRPyStroke::setPaths, METH_VARARGS, "Set a list of paths" },
    {"getPaths", (PyCFunction)VRPyStroke::getPaths, METH_NOARGS, "Get the list of paths" },
    {"strokeProfile", (PyCFunction)VRPyStroke::strokeProfile, METH_VARARGS, "Stroke along path using a profile" },
    {"strokeStrew", (PyCFunction)VRPyStroke::strokeStrew, METH_VARARGS, "Stew objects along path" },
    {"update", (PyCFunction)VRPyStroke::update, METH_NOARGS, "Update stroke" },
    {"convertToRope", (PyCFunction)VRPyStroke::convertToRope, METH_NOARGS, "converts this Stroke  to a rope (softbody)" },
    //{"getGeometry", (PyCFunction)VRPyStroke::getGeometry, METH_NOARGS, "Return the stroke's geometry" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyStroke::setPath(VRPyStroke* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::setPath - Object is invalid"); return NULL; }

    PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    VRPyPath* path = (VRPyPath*)o;

    OSG::VRStroke* e = (OSG::VRStroke*) self->obj;
    e->setPath(path->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::addPath(VRPyStroke* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::addPath - Object is invalid"); return NULL; }

    PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    VRPyPath* path = (VRPyPath*)o;

    OSG::VRStroke* e = (OSG::VRStroke*) self->obj;
    e->addPath(path->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::setPaths(VRPyStroke* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::setPaths - Object is invalid"); return NULL; }

    PyObject* vec;
    if (! PyArg_ParseTuple(args, "O", &vec)) return NULL;

    vector<OSG::path*> paths;
    for (int i=0; i<PyList_Size(vec); i++) {
        VRPyPath* path = (VRPyPath*)PyList_GetItem(vec, i);
        paths.push_back(path->obj);
    };

    OSG::VRStroke* e = (OSG::VRStroke*) self->obj;
    e->setPaths(paths);
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::getPaths(VRPyStroke* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::getPaths - Object is invalid"); return NULL; }

    vector<OSG::path*> _paths = self->obj->getPaths();
    PyObject* paths = PyList_New(_paths.size());

    for (uint i=0; i<_paths.size(); i++) {
        OSG::path* p = _paths[i];
        PyList_SetItem( paths, i, VRPyPath::fromPtr(p) );
    };

    return paths;
}

PyObject* VRPyStroke::strokeProfile(VRPyStroke* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::strokeProfile - Object is invalid"); return NULL; }

    int closed, lit;
    PyObject* vec;
    if (! PyArg_ParseTuple(args, "Oii", &vec, &closed, &lit)) return NULL;

    vector<OSG::Vec3f> profile;
    for (int i=0; i<PyList_Size(vec); i++) {
        OSG::Vec3f r;
        PyObject* v = PyList_GetItem(vec, i);
        for (int j=0; j<3; j++) {
            PyObject* vi = PyList_GetItem(v, j);
            r[j] = PyFloat_AsDouble(vi);
        }
        profile.push_back(r);
    };

    OSG::VRStroke* e = (OSG::VRStroke*) self->obj;
    e->strokeProfile(profile, closed);
    e->getMaterial()->setLit(lit);
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::strokeStrew(VRPyStroke* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::strokeStrew - Object is invalid"); return NULL; }

    PyObject* o = 0;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    VRPyGeometry* geo = (VRPyGeometry*)o;

    OSG::VRStroke* e = (OSG::VRStroke*) self->obj;
    e->strokeStrew(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::update(VRPyStroke* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::update - Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyStroke::convertToRope(VRPyStroke* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyStroke::convertToRope: C Object is invalid"); return NULL; }
    self->obj->getPhysics()->setDynamic(true);
    self->obj->getPhysics()->setShape("Rope");
    self->obj->getPhysics()->setSoft(true);
    self->obj->getPhysics()->setPhysicalized(true);
    Py_RETURN_TRUE;
}
