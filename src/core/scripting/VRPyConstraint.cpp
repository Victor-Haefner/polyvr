#include "VRPyConstraint.h"
#include "VRPyTransform.h"
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
    New_ptr,                 /* tp_new */
};

PyMemberDef VRPyConstraint::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyConstraint::methods[] = {
    {"constrain", (PyCFunction)VRPyConstraint::setDOFRange, METH_VARARGS, "Set the constraint range of one of the six degrees of freedom" },
    {"setLocal", (PyCFunction)VRPyConstraint::setLocal, METH_VARARGS, "Set the local flag of the constraints" },
    {"lock", (PyCFunction)VRPyConstraint::lock, METH_VARARGS, "Lock a list of DoFs" },
    {"free", (PyCFunction)VRPyConstraint::free, METH_VARARGS, "Free a list of DoFs" },
    {"setReference", (PyCFunction)VRPyConstraint::setReference, METH_VARARGS, "Set the reference matrix, setReference( transform )" },
    {"setReferential", (PyCFunction)VRPyConstraint::setReferential, METH_VARARGS, "Set the local referential, setReferential( transform )" },
    {"setLocalOffset", (PyCFunction)VRPyConstraint::setLocalOffsetB, METH_VARARGS, "Set the offset (relative to the target transform), setLocalOffset(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {"setLocalOffsetA", (PyCFunction)VRPyConstraint::setLocalOffsetA, METH_VARARGS, "Set the offset in A, setLocalOffsetA(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {"setLocalOffsetB", (PyCFunction)VRPyConstraint::setLocalOffsetB, METH_VARARGS, "Set the offset in B, setLocalOffsetB(offsetX,offsetY,offsetZ,yaw,pitch,roll)" },
    {"setRotationConstraint", (PyCFunction)VRPyConstraint::setRotationConstraint, METH_VARARGS, "Set a rotation constraint, setRotationConstraint([axis/normal], mode, bool globale)\n\tmode can be 'POINT', 'LINE', 'PLANE'" },
    {"setTranslationConstraint", (PyCFunction)VRPyConstraint::setTranslationConstraint, METH_VARARGS, "Set a translation constraint, setTranslationConstraint([axis/normal], mode, bool global)\n\tmode can be 'POINT', 'LINE', 'PLANE'" },
    {NULL}  /* Sentinel */
};


int getConstraintEnum(string s) {
    if (s == "POINT") return OSG::VRConstraint::POINT;
    if (s == "LINE") return OSG::VRConstraint::LINE;
    if (s == "PLANE") return OSG::VRConstraint::PLANE;
    return OSG::VRConstraint::NONE;
}

PyObject* VRPyConstraint::setRotationConstraint(VRPyConstraint* self, PyObject* args) {
    PyObject *v;
    const char* m;
    int g = 1;
    if (! PyArg_ParseTuple(args, "Osi", &v, &m, &g)) return NULL;
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setRConstraint(parseVec3fList(v), getConstraintEnum(m), g);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setTranslationConstraint(VRPyConstraint* self, PyObject* args) {
    PyObject *v;
    const char* m;
    int g = 1;
    if (! PyArg_ParseTuple(args, "Osi", &v, &m, &g)) return NULL;
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setTConstraint(parseVec3fList(v), getConstraintEnum(m), g);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setDOFRange(VRPyConstraint* self, PyObject* args) {
    int i;
    float f1, f2;
    if (! PyArg_ParseTuple(args, "iff", &i, &f1, &f2)) return NULL;
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setMinMax(i,f1,f2);
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

    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setReferenceA(m);
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

    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setReferenceB(m);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setReference(VRPyConstraint* self, PyObject* args) {
    VRPyTransform* t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setReference(t->objPtr->getWorldMatrix());
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setReferential(VRPyConstraint* self, PyObject* args) {
    VRPyTransform* t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    self->objPtr->setReferential(t->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::setLocal(VRPyConstraint* self, PyObject* args) {
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    bool b = parseBool(args);
    self->objPtr->setLocal(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::lock(VRPyConstraint* self, PyObject* args) {
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    vector<PyObject*> o = parseList(args);
    for (unsigned int i=0; i<o.size(); i++) {
        int j=PyLong_AsLong(o[i]);
        self->objPtr->setMinMax(j,0,0);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPyConstraint::free(VRPyConstraint* self, PyObject* args) {
    if (!self->objPtr) self->objPtr = OSG::VRConstraint::create();
    vector<PyObject*> o = parseList(args);
	for (unsigned int i = 0; i<o.size(); i++) {
        int j=PyLong_AsLong(o[i]);
        self->objPtr->setMinMax(j,1,-1);
    }
    Py_RETURN_TRUE;
}
