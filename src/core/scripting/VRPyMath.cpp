#include "VRPyMath.h"
#include "VRPyBaseT.h"
#include "core/utils/toString.h"

using namespace OSG;

PyMethodDef VRPyMath::methods[] = {
    {"cos", (PyCFunction)VRPyMath::cos, METH_VARARGS, "Cosine" },
    {"sin", (PyCFunction)VRPyMath::sin, METH_VARARGS, "Sine" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMath::cos(VRPyMath* self, PyObject* args) { float v = VRPyBase::parseFloat(args); return PyFloat_FromDouble(::cos(v)); }
PyObject* VRPyMath::sin(VRPyMath* self, PyObject* args) { float v = VRPyBase::parseFloat(args); return PyFloat_FromDouble(::sin(v)); }

template<> PyTypeObject VRPyBaseT<Vec2d>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Math.Vec3",             /*tp_name*/
    sizeof(VRPyVec2f),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    VRPyVec2f::Print,                         /*tp_repr*/
    &VRPyVec2f::nMethods,                         /*tp_as_number*/
    &VRPyVec2f::sMethods,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Vec3 binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    VRPyVec2f::iter,		               /* tp_iter */
    VRPyVec2f::iternext,		               /* tp_iternext */
    VRPyVec2f::methods,             /* tp_methods */
    0,                      /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    VRPyVec2f::New,                 /* tp_new */
};

PyNumberMethods VRPyVec2f::nMethods = {
    (binaryfunc)VRPyVec2f::add,
    (binaryfunc)VRPyVec2f::sub,
    (binaryfunc)VRPyVec2f::mul,
    (binaryfunc)VRPyVec2f::div,
    0,               /* binaryfunc nb_remainder;    __mod__ */
    0,            /* binaryfunc nb_divmod;       __divmod__ */
    0,               /* ternaryfunc nb_power;       __pow__ */
    (unaryfunc)VRPyVec2f::neg,
    0,               /* unaryfunc nb_positive;      __pos__ */
    (unaryfunc)VRPyVec2f::abs,
    0,           /* inquiry nb_nonzero;         __nonzero__ */
    0,            /* unaryfunc nb_invert;        __invert__ */
    0,            /* binaryfunc nb_lshift;       __lshift__ */
    0,            /* binaryfunc nb_rshift;       __rshift__ */
    0,               /* binaryfunc nb_and;          __and__ */
    0,               /* binaryfunc nb_xor;          __xor__ */
    0,                /* binaryfunc nb_or;           __or__ */
    0,            /* coercion nb_coerce;         __coerce__ */
    0,               /* unaryfunc nb_int;           __int__ */
    0,              /* unaryfunc nb_long;          __long__ */
    0,             /* unaryfunc nb_float;         __float__ */
    0,               /* unaryfunc nb_oct;           __oct__ */
    0,               /* unaryfunc nb_hex;           __hex__ */
};

PyMethodDef VRPyVec2f::methods[] = {
    {"normalize", (PyCFunction)VRPyVec2f::normalize, METH_NOARGS, "Normalize the vector - normalize()" },
    {"normalized", (PyCFunction)VRPyVec2f::normalized, METH_NOARGS, "Return normalized vector - normalized()" },
    {"length", (PyCFunction)VRPyVec2f::length, METH_NOARGS, "Compute the length - float length()" },
    {"dot", (PyCFunction)VRPyVec2f::dot, METH_VARARGS, "Compute the dot product - float dot( Vec3 )" },
    {"cross", (PyCFunction)VRPyVec2f::cross, METH_VARARGS, "Compute the cross product - Vec3 cross( Vec3 )" },
    {"asList", (PyCFunction)VRPyVec2f::asList, METH_NOARGS, "Return as python list - [x,y,z] asList()" },
    {"distance", (PyCFunction)VRPyVec2f::distance, METH_VARARGS, "Return the distance to another point - float distance([x,y,z])" },
    {NULL}  /* Sentinel */
};

VRPyVec2f* toPyVec2f(const Vec2d& v) {
    VRPyVec2f* pv = (VRPyVec2f*)VRPyVec2f::typeRef->tp_alloc(VRPyVec2f::typeRef, 0);
    pv->owner = false;
    pv->v = v;
    return pv;
}

PyObject* toPyObject(const Vec2d& v) { return (PyObject*)toPyVec2f(v); }

PyObject* VRPyVec2f::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject* v = 0;
    float a,b,c;
    if (! PyArg_ParseTuple(args, "O", &v))
        if (! PyArg_ParseTuple(args, "fff", &a, &b, &c)) { setErr("Bad Constructor to Vec3"); return NULL; }
    VRPyVec2f* pv = (VRPyVec2f*)allocPtr( type, 0 );
    if (!v) pv->v = Vec2d(a,b,c);
    else pv->v = parseVec2dList(v);
    return (PyObject*)pv;
}

PyObject* VRPyVec2f::Print(PyObject* self) {
    string s = "[" + toString(((VRPyVec2f*)self)->v) + "]";
    std::replace( s.begin(), s.end(), ' ', ',');
    return PyString_FromString( s.c_str() );
}

PyObject* VRPyVec2f::normalize(VRPyVec2f* self) {
    self->v.normalize();
    return (PyObject*) toPyVec2f(self->v);
}

PyObject* VRPyVec2f::normalized(VRPyVec2f* self) {
    auto v = self->v;
    v.normalize();
    return (PyObject*) toPyVec2f(v);
}

PyObject* VRPyVec2f::asList(VRPyVec2f* self) {
    auto l = PyList_New(3);
    PyList_SetItem(l,0,PyFloat_FromDouble(self->v[0]));
    PyList_SetItem(l,1,PyFloat_FromDouble(self->v[1]));
    PyList_SetItem(l,2,PyFloat_FromDouble(self->v[2]));
    return l;
}

PyObject* VRPyVec2f::length(VRPyVec2f* self) {
    return PyFloat_FromDouble( self->v.length() );
}

PyObject* VRPyVec2f::dot(VRPyVec2f* self, PyObject* args) {
    PyObject* v;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return PyFloat_FromDouble( self->v.dot( parseVec2dList(v) ) );
}

PyObject* VRPyVec2f::distance(VRPyVec2f* self, PyObject* args) {
    PyObject* v;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return PyFloat_FromDouble( (self->v - parseVec2dList(v)).length() );
}

PyObject* VRPyVec2f::cross(VRPyVec2f* self, PyObject* args) {
    PyObject* v = 0;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return ::toPyObject( self->v.cross( parseVec2dList(v)) );
}


PyObject* VRPyVec2f::add(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec2f*)self)->v + parseVec2dList(v) );
}

PyObject* VRPyVec2f::sub(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec2f*)self)->v - parseVec2dList(v) );
}

PyObject* VRPyVec2f::mul(PyObject* self, PyObject* F) {
    if (PyNumber_Check(self) && check(F)) swap(self, F);
    if (!PyNumber_Check(F)) { setErr("Vector multiplication needs number"); return NULL; }
    float f = PyFloat_AsDouble(F);
    return ::toPyObject( ((VRPyVec2f*)self)->v * f );
}

PyObject* VRPyVec2f::div(PyObject* self, PyObject* F) {
    if (!PyNumber_Check(F)) { setErr("Dividing by a vector is not allowed"); return NULL; }
    float f = PyFloat_AsDouble(F);
    return ::toPyObject( ((VRPyVec2f*)self)->v * (1.0/f) );
}

PyObject* VRPyVec2f::neg(PyObject* self) {
    return ::toPyObject( -((VRPyVec2f*)self)->v);
}

PyObject* VRPyVec2f::abs(PyObject* self) {
    Vec2d v = ((VRPyVec2f*)self)->v;
    return ::toPyObject( Vec2d(::abs(v[0]), ::abs(v[1]), ::abs(v[2])) );
}

PySequenceMethods VRPyVec2f::sMethods = {
    VRPyVec2f::len,       /* inquiry sq_length;              __len__ */
    0,    /* binaryfunc sq_concat;           __add__ */
    0,    /* intargfunc sq_repeat;           __mul__ */
    VRPyVec2f::getItem,   /* intargfunc sq_item;             __getitem__ */
    VRPyVec2f::getSlice,  /* intintargfunc sq_slice;         __getslice__ */
    VRPyVec2f::setItem,   /* intobjargproc sq_ass_item;      __setitem__ */
    0,  /* intintobjargproc sq_ass_slice;  __setslice__ */
};

Py_ssize_t VRPyVec2f::len(PyObject* self) {
    return 3;
}

PyObject* VRPyVec2f::getItem(PyObject* self, Py_ssize_t i) {
    Vec2d v = ((VRPyVec2f*)self)->v;
    return PyFloat_FromDouble(v[i]);
}

int VRPyVec2f::setItem(PyObject* self, Py_ssize_t i, PyObject* val) {
    Vec2d& v = ((VRPyVec2f*)self)->v;
    v[i] = PyFloat_AsDouble(val);
    return 0;
}

PyObject* VRPyVec2f::getSlice(PyObject* self, Py_ssize_t ilow, Py_ssize_t ihigh) {
    if (ilow < 0) ilow += 2;
    if (ihigh < 0) ihigh += 2;
    if (ilow >= 2) ilow = 2-1;
    if (ihigh > 2) ihigh = 2;

    Vec2d v2;
    if (0 <= ilow && ihigh <= 2 && ilow < ihigh) {
        for (int i=ilow; i < ihigh; i++) v2[i] = ((VRPyVec2f*)self)->v[i];
    } else v2 = ((VRPyVec2f*)self)->v;
    return toPyObject(v2);
}

PyObject* VRPyVec2f::iter(PyObject *self) {
    Py_INCREF(self);
    auto p = (VRPyVec2f*)self;
    p->itr = 0;
    return self;
}

PyObject* VRPyVec2f::iternext(PyObject *self) {
    auto p = (VRPyVec2f*)self;
    Vec2d v = ((VRPyVec2f*)self)->v;
    if (p->itr < 2) {
        int i = p->itr;
        (p->itr)++;
        return PyFloat_FromDouble(v[i]);
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}


template<> PyTypeObject VRPyBaseT<Vec3d>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Math.Vec3",             /*tp_name*/
    sizeof(VRPyVec3f),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    VRPyVec3f::Print,                         /*tp_repr*/
    &VRPyVec3f::nMethods,                         /*tp_as_number*/
    &VRPyVec3f::sMethods,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Vec3 binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    VRPyVec3f::iter,		               /* tp_iter */
    VRPyVec3f::iternext,		               /* tp_iternext */
    VRPyVec3f::methods,             /* tp_methods */
    0,                      /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    VRPyVec3f::New,                 /* tp_new */
};

PyNumberMethods VRPyVec3f::nMethods = {
    (binaryfunc)VRPyVec3f::add,
    (binaryfunc)VRPyVec3f::sub,
    (binaryfunc)VRPyVec3f::mul,
    (binaryfunc)VRPyVec3f::div,
    0,               /* binaryfunc nb_remainder;    __mod__ */
    0,            /* binaryfunc nb_divmod;       __divmod__ */
    0,               /* ternaryfunc nb_power;       __pow__ */
    (unaryfunc)VRPyVec3f::neg,
    0,               /* unaryfunc nb_positive;      __pos__ */
    (unaryfunc)VRPyVec3f::abs,
    0,           /* inquiry nb_nonzero;         __nonzero__ */
    0,            /* unaryfunc nb_invert;        __invert__ */
    0,            /* binaryfunc nb_lshift;       __lshift__ */
    0,            /* binaryfunc nb_rshift;       __rshift__ */
    0,               /* binaryfunc nb_and;          __and__ */
    0,               /* binaryfunc nb_xor;          __xor__ */
    0,                /* binaryfunc nb_or;           __or__ */
    0,            /* coercion nb_coerce;         __coerce__ */
    0,               /* unaryfunc nb_int;           __int__ */
    0,              /* unaryfunc nb_long;          __long__ */
    0,             /* unaryfunc nb_float;         __float__ */
    0,               /* unaryfunc nb_oct;           __oct__ */
    0,               /* unaryfunc nb_hex;           __hex__ */
};

PyMethodDef VRPyVec3f::methods[] = {
    {"normalize", (PyCFunction)VRPyVec3f::normalize, METH_NOARGS, "Normalize the vector - normalize()" },
    {"normalized", (PyCFunction)VRPyVec3f::normalized, METH_NOARGS, "Return normalized vector - normalized()" },
    {"length", (PyCFunction)VRPyVec3f::length, METH_NOARGS, "Compute the length - float length()" },
    {"dot", (PyCFunction)VRPyVec3f::dot, METH_VARARGS, "Compute the dot product - float dot( Vec3 )" },
    {"cross", (PyCFunction)VRPyVec3f::cross, METH_VARARGS, "Compute the cross product - Vec3 cross( Vec3 )" },
    {"asList", (PyCFunction)VRPyVec3f::asList, METH_NOARGS, "Return as python list - [x,y,z] asList()" },
    {"distance", (PyCFunction)VRPyVec3f::distance, METH_VARARGS, "Return the distance to another point - float distance([x,y,z])" },
    {NULL}  /* Sentinel */
};

VRPyVec3f* toPyVec3f(const Vec3d& v) {
    VRPyVec3f* pv = (VRPyVec3f*)VRPyVec3f::typeRef->tp_alloc(VRPyVec3f::typeRef, 0);
    pv->owner = false;
    pv->v = v;
    return pv;
}

PyObject* toPyObject(const Vec3d& v) { return (PyObject*)toPyVec3f(v); }

PyObject* VRPyVec3f::fromVector(OSG::Vec3d v) { return toPyObject(v); }

PyObject* VRPyVec3f::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject* v = 0;
    float a,b,c;
    if (! PyArg_ParseTuple(args, "O", &v))
        if (! PyArg_ParseTuple(args, "fff", &a, &b, &c)) { setErr("Bad Constructor to Vec3"); return NULL; }
    VRPyVec3f* pv = (VRPyVec3f*)allocPtr( type, 0 );
    if (!v) pv->v = Vec3d(a,b,c);
    else pv->v = parseVec3dList(v);
    return (PyObject*)pv;
}

PyObject* VRPyVec3f::Print(PyObject* self) {
    string s = "[" + toString(((VRPyVec3f*)self)->v) + "]";
    std::replace( s.begin(), s.end(), ' ', ',');
    return PyString_FromString( s.c_str() );
}

PyObject* VRPyVec3f::normalize(VRPyVec3f* self) {
    self->v.normalize();
    return (PyObject*) toPyVec3f(self->v);
}

PyObject* VRPyVec3f::normalized(VRPyVec3f* self) {
    auto v = self->v;
    v.normalize();
    return (PyObject*) toPyVec3f(v);
}

PyObject* VRPyVec3f::asList(VRPyVec3f* self) {
    auto l = PyList_New(3);
    PyList_SetItem(l,0,PyFloat_FromDouble(self->v[0]));
    PyList_SetItem(l,1,PyFloat_FromDouble(self->v[1]));
    PyList_SetItem(l,2,PyFloat_FromDouble(self->v[2]));
    return l;
}

PyObject* VRPyVec3f::length(VRPyVec3f* self) {
    return PyFloat_FromDouble( self->v.length() );
}

PyObject* VRPyVec3f::dot(VRPyVec3f* self, PyObject* args) {
    PyObject* v;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return PyFloat_FromDouble( self->v.dot( parseVec3dList(v) ) );
}

PyObject* VRPyVec3f::distance(VRPyVec3f* self, PyObject* args) {
    PyObject* v;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return PyFloat_FromDouble( (self->v - parseVec3dList(v)).length() );
}

PyObject* VRPyVec3f::cross(VRPyVec3f* self, PyObject* args) {
    PyObject* v = 0;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return ::toPyObject( self->v.cross( parseVec3dList(v)) );
}


PyObject* VRPyVec3f::add(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec3f*)self)->v + parseVec3dList(v) );
}

PyObject* VRPyVec3f::sub(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec3f*)self)->v - parseVec3dList(v) );
}

PyObject* VRPyVec3f::mul(PyObject* self, PyObject* F) {
    if (PyNumber_Check(self) && check(F)) swap(self, F);
    if (!PyNumber_Check(F)) { setErr("Vector multiplication needs number"); return NULL; }
    float f = PyFloat_AsDouble(F);
    return ::toPyObject( ((VRPyVec3f*)self)->v * f );
}

PyObject* VRPyVec3f::div(PyObject* self, PyObject* F) {
    if (!PyNumber_Check(F)) { setErr("Dividing by a vector is not allowed"); return NULL; }
    float f = PyFloat_AsDouble(F);
    return ::toPyObject( ((VRPyVec3f*)self)->v * (1.0/f) );
}

PyObject* VRPyVec3f::neg(PyObject* self) {
    return ::toPyObject( -((VRPyVec3f*)self)->v);
}

PyObject* VRPyVec3f::abs(PyObject* self) {
    Vec3d v = ((VRPyVec3f*)self)->v;
    return ::toPyObject( Vec3d(::abs(v[0]), ::abs(v[1]), ::abs(v[2])) );
}

PySequenceMethods VRPyVec3f::sMethods = {
    VRPyVec3f::len,       /* inquiry sq_length;              __len__ */
    0,    /* binaryfunc sq_concat;           __add__ */
    0,    /* intargfunc sq_repeat;           __mul__ */
    VRPyVec3f::getItem,   /* intargfunc sq_item;             __getitem__ */
    VRPyVec3f::getSlice,  /* intintargfunc sq_slice;         __getslice__ */
    VRPyVec3f::setItem,   /* intobjargproc sq_ass_item;      __setitem__ */
    0,  /* intintobjargproc sq_ass_slice;  __setslice__ */
};

Py_ssize_t VRPyVec3f::len(PyObject* self) {
    return 3;
}

PyObject* VRPyVec3f::getItem(PyObject* self, Py_ssize_t i) {
    if (i < 0 || i > 2) {
        setErr("Index i not in range [0-2] ("+toString(int(i))+")");
        return NULL;
    }
    Vec3d v = ((VRPyVec3f*)self)->v;
    return PyFloat_FromDouble(v[i]);
}

PyObject* VRPyVec3f::iter(PyObject *self) {
    Py_INCREF(self);
    auto p = (VRPyVec3f*)self;
    p->itr = 0;
    return self;
}

PyObject* VRPyVec3f::iternext(PyObject *self) {
    auto p = (VRPyVec3f*)self;
    Vec3d v = ((VRPyVec3f*)self)->v;
    if (p->itr < 3) {
        int i = p->itr;
        (p->itr)++;
        return PyFloat_FromDouble(v[i]);
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

int VRPyVec3f::setItem(PyObject* self, Py_ssize_t i, PyObject* val) {
    if (i < 0 || i > 2) {
        setErr("Index i not in range [0-2] ("+toString(int(i))+")");
        return -1;
    }
    Vec3d& v = ((VRPyVec3f*)self)->v;
    v[i] = PyFloat_AsDouble(val);
    return 0;
}

PyObject* VRPyVec3f::getSlice(PyObject* self, Py_ssize_t ilow, Py_ssize_t ihigh) {
    if (ilow < 0) ilow += 3;
    if (ihigh < 0) ihigh += 3;
    if (ilow >= 3) ilow = 3-1;
    if (ihigh > 3) ihigh = 3;

    Vec3d v2;
    if (0 <= ilow && ihigh <= 3 && ilow < ihigh) {
        for (int i=ilow; i < ihigh; i++) v2[i] = ((VRPyVec3f*)self)->v[i];
    } else v2 = ((VRPyVec3f*)self)->v;
    return toPyObject(v2);
}

template<> PyTypeObject VRPyBaseT<Line>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Math.Line",             /*tp_name*/
    sizeof(VRPyLine),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    VRPyLine::Print,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Line binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyLine::methods,             /* tp_methods */
    0,                      /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    VRPyLine::New,                 /* tp_new */
};

PyMethodDef VRPyLine::methods[] = {
    {"intersect", (PyCFunction)VRPyLine::intersect, METH_VARARGS, "Intersect with another line - Vec3 intersect( Line )" },
    {"pos", (PyCFunction)VRPyLine::pos, METH_NOARGS, "Return position - Vec3 pos()" },
    {"dir", (PyCFunction)VRPyLine::dir, METH_NOARGS, "Return direction - Vec3 dir()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyLine::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject* p = 0;
    PyObject* d = 0;
    if (! PyArg_ParseTuple(args, "OO", &p, &d)) return NULL;
    VRPyLine* L = (VRPyLine*)allocPtr( type, 0 );
    L->l = Line( Pnt3f(parseVec3dList(p)), Vec3f(parseVec3dList(d)) );
    return (PyObject*)L;
}

PyObject* VRPyLine::Print(PyObject* self) {
    auto l = ((VRPyLine*)self)->l;
    string s = "[" + toString(l) + "]";
    std::replace( s.begin(), s.end(), ' ', ',');
    return PyString_FromString( s.c_str() );
}

PyObject* VRPyLine::pos(VRPyLine* self) {
    return ::toPyObject(Vec3d(self->l.getPosition()));
}

PyObject* VRPyLine::dir(VRPyLine* self) {
    return ::toPyObject(Vec3d(self->l.getDirection()));
}

PyObject* VRPyLine::intersect(VRPyLine* self, PyObject *args) {
    PyObject* l = 0;
    if (! PyArg_ParseTuple(args, "O", &l)) return NULL;
    VRPyLine* L = (VRPyLine*)l;

    auto insct = [&](const Pnt3f& p1, const Vec3f& n1, const Pnt3f& p2, const Vec3f& n2) -> Vec3d {
		Vec3f d = p2-p1;
		Vec3f n3 = n1.cross(n2);
		float N3 = n3.dot(n3);
		if (N3 == 0) N3 = 1.0;
		float s = d.cross(n2).dot(n1.cross(n2))/N3;
		return Vec3d(Vec3f(p1) + n1*s);
    };

    auto i = insct( self->l.getPosition(), self->l.getDirection(), L->l.getPosition(), L->l.getDirection() );
    return ::toPyObject( i );
}




// expression bindings

simplePyType(Expression, New_named_ptr);
simplePyType(MathExpression, New_named_ptr);
simplePyType(TSDF, VRPyTSDF::New );
simplePyType(OctreeNode, 0 );
simplePyType(Octree, VRPyOctree::New );
#ifndef WITHOUT_LAPACKE_BLAS
simplePyType(PCA, New_ptr);
#endif
simplePyType(Patch, New_ptr);
simplePyType(XML, New_ptr);
simplePyType(XMLElement, 0);
simpleVRPyType(Spreadsheet, New_ptr);

template<> string typeName(const Datarow& p) { return "Datarow"; }

template<> PyTypeObject VRPyBaseT<Datarow>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Datarow",             /*tp_name*/
    sizeof(VRPyDatarow),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &VRPyDatarow::sMethods,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRDatarow binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    VRPyDatarow::iter,		               /* tp_iter */
    VRPyDatarow::iternext,		              /* tp_iternext */
    VRPyDatarow::methods,             /* tp_methods */
    0,                      /* tp_members */
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

template<> bool toValue(PyObject* o, DatarowPtr& v) {
    if (VRPyBase::isNone(o)) { v = 0; return 1; }
    if (!VRPyDatarow::check(o)) return 0;
    v = ((VRPyDatarow*)o)->objPtr; return 1;
};

template<> bool toValue(PyObject* o, Datarow*& v) {
    if (VRPyBase::isNone(o)) { v = 0; return 1; }
    if (!VRPyDatarow::check(o)) return 0;
    v = ((VRPyDatarow*)o)->obj; return 1;
};

template<> PyObject* VRPyTypeCaster::cast(const DatarowPtr& e) {
    return VRPyDatarow::fromSharedPtr(e);
};

PyObject* VRPyDatarow::iter(PyObject *self) {
    Py_INCREF(self);
    auto p = (VRPyDatarow*)self;
    p->itr = 0;
    return self;
}

PyObject* VRPyDatarow::iternext(PyObject *self) {
    auto p = (VRPyDatarow*)self;
    auto v = p->objPtr;
    if (p->itr < v->length()) {
        int i = p->itr;
        (p->itr)++;
        return PyFloat_FromDouble(v->get(i));
    } else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

PySequenceMethods VRPyDatarow::sMethods = {
    VRPyDatarow::len,       /* inquiry sq_length;              __len__ */
    0,    /* binaryfunc sq_concat;           __add__ */
    0,    /* intargfunc sq_repeat;           __mul__ */
    VRPyDatarow::getItem,   /* intargfunc sq_item;             __getitem__ */
    VRPyDatarow::getSlice,  /* intintargfunc sq_slice;         __getslice__ */
    VRPyDatarow::setItem,   /* intobjargproc sq_ass_item;      __setitem__ */
    0,  /* intintobjargproc sq_ass_slice;  __setslice__ */
};

Py_ssize_t VRPyDatarow::len(PyObject* self) {
    auto v = ((VRPyDatarow*)self)->objPtr;
    return v->length();
}

PyObject* VRPyDatarow::getItem(PyObject* self, Py_ssize_t i) {
    auto v = ((VRPyDatarow*)self)->objPtr;
    if (i < 0 || i >= (int)v->length()) {
        setErr("VRPyDatarow::getItem, index i not in range [0-"+toString(v->length())+"] ("+toString(int(i))+")");
        return NULL;
    }
    return PyFloat_FromDouble(v->get(i));
}

int VRPyDatarow::setItem(PyObject* self, Py_ssize_t i, PyObject* val) {
    auto v = ((VRPyDatarow*)self)->objPtr;
    if (i < 0 || i >= (int)v->length()) {
        setErr("VRPyDatarow::setItem, index i not in range [0-"+toString(v->length())+"] ("+toString(int(i))+")");
        return -1;
    }
    v->set( PyFloat_AsDouble(val), i );
    return 0;
}

PyObject* VRPyDatarow::getSlice(PyObject* self, Py_ssize_t ilow, Py_ssize_t ihigh) {
    auto v = ((VRPyDatarow*)self)->objPtr;
    int N = v->length();
    if (ilow < 0) ilow += N;
    if (ihigh < 0) ihigh += N;
    if (ilow >= N) ilow = N-1;
    if (ihigh > N) ihigh = N;

    auto v2 = Datarow::create();
    if (0 <= ilow && ihigh <= N && ilow < ihigh) {
        for (int i=ilow; i < ihigh; i++) v2->append( v->get(i) );
    } else return NULL;
    return VRPyDatarow::fromSharedPtr(v2);
}

PyMethodDef VRPyDatarow::methods[] = {
    {"append", PyWrap2( Datarow, append, "Add value", void, double ) },
    {"set", PyWrap2( Datarow, set, "Set ith value", void, double, int ) },
    {"get", PyWrap2( Datarow, get, "Get ith value", double, int ) },
    {"asList", PyWrap2( Datarow, getDataCpy, "Get as list", vector<double> ) },
    {"fromList", PyWrap2( Datarow, setData, "Set from list", void, vector<double>& ) },
    {"length", PyWrap2( Datarow, length, "Get data size", size_t ) },
    {"resize", PyWrap2( Datarow, resize, "Resize data with value", void, int, double ) },
    {"add", PyWrap2( Datarow, add, "Add all elements of other datarow", void, DatarowPtr ) },
    {"insert", PyWrap2( Datarow, insert, "Add an element at ith place, element will be ith element, old ith element will shift to the right", void, int, double ) },
    {"getMinMax", PyWrap2( Datarow, getMinMax, "Return min and max value", vector<double> ) },
    {"getPCT", PyWrap2( Datarow, getPCT, "Get ith PCT", double, int ) },
    {"getLogRet", PyWrap2( Datarow, getLogRet, "Get ith log return", double, int ) },
    {"getPCTs", PyWrap2( Datarow, getPCTs, "Get PCTs", DatarowPtr ) },
    {"getLogRets", PyWrap2( Datarow, getLogRets, "Get log returns", DatarowPtr ) },
    {"computeCorrelation", PyWrapOpt2( Datarow, computeCorrelation, "Compute correlation between two series", "0|0", double, DatarowPtr, int, int ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyExpression::methods[] = {
    {"set", PyWrap2( Expression, set, "Set expression", void, string ) },
    {"segment", PyWrap2( Expression, segment, "Build tree structure", void ) },
    {"buildTree", PyWrap2( Expression, buildTree, "Compute tree structure", void ) },
    {"parse", PyWrap2( Expression, parse, "Compute tree structure", void ) },
    {"toString", PyWrap2( Expression, toString, "Return expression as string", string ) },
    {"treeToString", PyWrap2( Expression, treeToString, "Return tree as string", string ) },
    {"substitute", PyWrap2( Expression, substitute, "Substitute in tree", void, string, string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMathExpression::methods[] = {
    {"compute", PyWrap2( MathExpression, compute, "Compute expression", string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTSDF::methods[] = {
    {"get", PyWrap2( TSDF, get, "Get field", float, Vec3i ) },
    {"set", PyWrap2( TSDF, set, "Set field", void, float, Vec3i ) },
    {"extractMesh", PyWrap2( TSDF, extractMesh, "Extract surfaces as mesh", VRGeometryPtr ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTSDF::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject* s = 0;
    if (!PyArg_ParseTuple(args, "O", &s)) return NULL;
    Vec3i size = Vec3i(1,1,1);
    if (s) size = parseVec3iList(s);
    return allocPtr( type, TSDF::create(size) );
}


typedef OctreeNode* onPtr;
template<> PyObject* VRPyTypeCaster::cast(const onPtr& n) { if (n) return VRPyOctreeNode::fromPtr(n); Py_RETURN_NONE; }

PyMethodDef VRPyOctree::methods[] = {
    {"add", PyWrapOpt2( Octree, add, "Add to tree - will leak memory!", "0|-1|1|-1", OctreeNode*, Vec3d, void*, int, bool, int ) },
    {"get", PyWrapOpt2( Octree, get, "Get leaf node at position", "1", OctreeNode*, Vec3d, bool ) },
    {"getVisualization", PyWrapOpt2( Octree, getVisualization, "Get tree visual", "0", VRGeometryPtr, bool ) },
    {"getAllLeafs", PyWrap2( Octree, getAllLeafs, "Get all leafs", vector<OctreeNode*> ) },
    {"radiusSearch", PyWrapOpt2( Octree, radiusSearch, "Search for data within radius", "-1", vector<void*>, Vec3d, float, int) },
    {"radiusPointSearch", PyWrapOpt2( Octree, radiusPointSearch, "Search for points within radius", "-1|1", vector<Vec3d>, Vec3d, float, int, bool) },
    //{"boxSearch", PyWrapOpt2( Octree, boxSearch, "Search for points within bounding box", "-1", vector<void*>, const Boundingbox&, int) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyOctree::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    float res = 0.1;
    float size = 10;
    char* name = 0;
    if (!PyArg_ParseTuple(args, "f|fs", &res, &size, &name)) return NULL;
    string Name = name?name:"";
    return allocPtr( type, Octree::create(res, size, Name) );
}

PyMethodDef VRPyOctreeNode::methods[] = {
    {"getData", PyWrap2( OctreeNode, getData, "Get node data", vector<void*> ) },
    {"getPoints", PyWrap2( OctreeNode, getPoints, "Get node points", vector<Vec3d> ) },
    {"getChildren", PyWrap2( OctreeNode, getChildren, "Get node children", vector<OctreeNode*> ) },
    {"getCenter", PyWrap2( OctreeNode, getCenter, "Get node center", Vec3d ) },
    {"getLocalCenter", PyWrap2( OctreeNode, getLocalCenter, "Get node local center", Vec3d ) },
    {"getSize", PyWrap2( OctreeNode, getSize, "Get node size", float ) },
    {"getResolution", PyWrap2( OctreeNode, getResolution, "Get node resolution", float ) },
    {"getParent", PyWrap2( OctreeNode, getParent, "Get node parent", OctreeNode* ) },
    {"getRoot", PyWrap2( OctreeNode, getRoot, "Get node root", OctreeNode* ) },
    {NULL}  /* Sentinel */
};

#ifndef WITHOUT_LAPACKE_BLAS
PyMethodDef VRPyPCA::methods[] = {
    {"add", PyWrap2( PCA, add, "Add point", void, Vec3d ) },
    {"addMesh", PyWrap2( PCA, addMesh, "Add mesh vertex positions", void, VRObjectPtr ) },
    {"compute", PyWrap2( PCA, compute, "Compute, pose result contains: scale contains eigenvalues; dir, x, up contains eigenvectors; the position is the covariance parameters", Pose ) },
    {"size", PyWrap2( PCA, size, "Get amount of points", int ) },
    {"clear", PyWrap2( PCA, clear, "Clear points", void ) },
    {NULL}  /* Sentinel */
};
#endif

PyMethodDef VRPyPatch::methods[] = {
    {"getSurface", PyWrap2( Patch, getSurface, "Get surface object", VRObjectPtr ) },
    {"getDistance", PyWrap2( Patch, getDistance, "Get closest distance", float, Vec3d ) },
    {"getClosestPoint", PyWrap2( Patch, getClosestPoint, "Get closest point", Vec3d, Vec3d ) },
    {"fromTriangle", PyWrap2( Patch, fromTriangle, "Create patch from triangle data", VRObjectPtr, vector<Vec3d> positions, vector<Vec3d> normals, int, bool ) },
    {"fromQuad", PyWrap2( Patch, fromQuad, "Create patch from quad data", VRObjectPtr, vector<Vec3d> positions, vector<Vec3d> normals, int, bool ) },
    {"fromFullQuad", PyWrap2( Patch, fromFullQuad, "Create patch from quad data including handles", VRObjectPtr, vector<Vec3d> positions, vector<Vec3d> normals, vector<Vec3d> handles, int, bool ) },
    {"fromGeometry", PyWrap2( Patch, fromGeometry, "Create patch from geometry", VRObjectPtr, VRGeometryPtr, int, bool ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyXML::methods[] = {
    {"read", PyWrapOpt2(XML, read, "Read file", "1", void, string, bool) },
    {"parse", PyWrapOpt2(XML, parse, "Parse xml string", "1", void, string, bool) },
    {"write", PyWrap2(XML, write, "Write data to file", void, string) },
    {"toString", PyWrap2(XML, toString, "Return data as string", string) },
    {"getRoot", PyWrap2(XML, getRoot, "Return data root node", XMLElementPtr) },
    {"newRoot", PyWrap2(XML, newRoot, "Create new root node (name, ns_uri, ns_prefix)", XMLElementPtr, string, string, string) },
    {"printTree", PyWrapOpt2(XML, printTree, "Print subtree to console", "", void, XMLElementPtr, string) },
    {NULL}  /* Sentinel */
};

typedef map<string, string> ssDict;

PyMethodDef VRPyXMLElement::methods[] = {
    {"getName", PyWrap2(XMLElement, getName, "Return element name", string) },
    {"getNameSpace", PyWrap2(XMLElement, getNameSpace, "Return element namespace", string) },
    {"getText", PyWrap2(XMLElement, getText, "Return element text", string) },
    {"hasText", PyWrap2(XMLElement, hasText, "Check if element has text", bool) },
    {"setText", PyWrap2(XMLElement, setText, "Set element text", void, string) },
    {"getAttribute", PyWrap2(XMLElement, getAttribute, "Get attribute value", string, string) },
    {"getAttributes", PyWrap2(XMLElement, getAttributes, "Get attributes as dictionary", ssDict) },
    {"hasAttribute", PyWrap2(XMLElement, hasAttribute, "Return if element has an attribute", bool, string) },
    {"setAttribute", PyWrap2(XMLElement, setAttribute, "Set an attribute (name, value)", void, string, string) },
    {"getChildren", PyWrapOpt2(XMLElement, getChildren, "Get children, optional element name", "", vector<XMLElementPtr>, string) },
    {"getChild", PyWrap2(XMLElement, getChild, "Get child by name", XMLElementPtr, string) },
    {"addChild", PyWrap2(XMLElement, addChild, "Add child element", XMLElementPtr, string) },
    {"print", PyWrap2(XMLElement, print, "Print to console", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySpreadsheet::methods[] = {
    {"read", PyWrap(Spreadsheet, read, "Read file", void, string) },
    {"getSheets", PyWrap(Spreadsheet, getSheets, "Get list of sheet names", vector<string>) },
    {"getNColumns", PyWrap(Spreadsheet, getNColumns, "Get N columns of sheet", size_t, string) },
    {"getNRows", PyWrap(Spreadsheet, getNRows, "Get N rows of sheet", size_t, string) },
    {"getCell", PyWrap(Spreadsheet, getCell, "Get cell content (sheet, column, row)", string, string, size_t, size_t) },
    {NULL}  /* Sentinel */
};
