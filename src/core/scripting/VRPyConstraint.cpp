#include "VRPyConstraint.h"
#include "VRPyBaseT.h"
#include "core/objects/geometry/VRPhysics.h"

template<> PyTypeObject VRPyBaseT<OSG::VRConstraint>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Constraint",             /*tp_name*/
    sizeof(VRPyConstraint),             /*tp_basicsize*/
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
    "VRConstraint binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyConstraint::methods,             /* tp_methods */
    VRPyConstraint::members,             /* tp_members */
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

PyMemberDef VRPyConstraint::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyConstraint::methods[] = {
    {"constrain", (PyCFunction)VRPyConstraint::setDOFRange, METH_VARARGS, "Set the constraint range of one of the six degrees of freedom" },
    {"setLocal", (PyCFunction)VRPyConstraint::setLocal, METH_VARARGS, "Set the local flag of the constraints" },
    {"lock", (PyCFunction)VRPyConstraint::lock, METH_VARARGS, "Lock a list of DoFs" },
    {"free", (PyCFunction)VRPyConstraint::free, METH_VARARGS, "Free a list of DoFs" },
    {"setLocalOffset", (PyCFunction)VRPyConstraint::setLocalOffsetB, METH_VARARGS, "Set the offset (relative to the target transform), setLocalOffset(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {"setLocalOffsetA", (PyCFunction)VRPyConstraint::setLocalOffsetA, METH_VARARGS, "Set the offset in A, setLocalOffsetA(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {"setLocalOffsetB", (PyCFunction)VRPyConstraint::setLocalOffsetB, METH_VARARGS, "Set the offset in B, setLocalOffsetB(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyConstraint::setDOFRange(VRPyConstraint* self, PyObject* args) {
    int i;
    float f1, f2;
    if (! PyArg_ParseTuple(args, "iff", &i, &f1, &f2)) return NULL;
    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    self->obj->setMinMax(i,f1,f2);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setLocalOffsetA(VRPyConstraint* self, PyObject* args) {
    float x,y,z,roll,pitch,yaw;
    if (! PyArg_ParseTuple(args, "ffffff", &x,&y,&z,&yaw,&pitch,&roll)) return NULL;
    //translation
    OSG::Matrix m ;
    m.setTranslate(OSG::Vec3f(x,y,z));
    //rotation
    btQuaternion q ;
    q.setEuler(yaw,pitch,roll);
    OSG::Quaternion qtwo = OSG::Quaternion(q.x(),q.y(),q.z(),q.w());
    m.setRotate(qtwo);

    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    self->obj->setReferenceA(m);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setLocalOffsetB(VRPyConstraint* self, PyObject* args) {
    float x,y,z,roll,pitch,yaw;
    if (! PyArg_ParseTuple(args, "ffffff", &x,&y,&z,&yaw,&pitch,&roll)) return NULL;
    //translation
    OSG::Matrix m ;
    m.setTranslate(OSG::Vec3f(x,y,z));
    //rotation
    btQuaternion q ;
    q.setEuler(yaw,pitch,roll);
    OSG::Quaternion qtwo = OSG::Quaternion(q.x(),q.y(),q.z(),q.w());
    m.setRotate(qtwo);

    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    self->obj->setReferenceB(m);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setLocal(VRPyConstraint* self, PyObject* args) {
    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    bool b = parseBool(args);
    self->obj->setLocal(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::lock(VRPyConstraint* self, PyObject* args) {
    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    vector<PyObject*> o = parseList(args);
    for (unsigned int i=0; i<o.size(); i++) {
        int j=PyLong_AsLong(o[i]);
        self->obj->setMinMax(j,0,0);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::free(VRPyConstraint* self, PyObject* args) {
    if (self->obj == 0) self->obj = new OSG::VRConstraint();
    vector<PyObject*> o = parseList(args);
	for (unsigned int i = 0; i<o.size(); i++) {
        int j=PyLong_AsLong(o[i]);
        self->obj->setMinMax(j,1,-1);
    }
    Py_RETURN_TRUE;
}
