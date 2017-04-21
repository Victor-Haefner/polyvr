#include "VRPyMath.h"
#include "VRPyBaseT.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> PyTypeObject VRPyBaseT<Vec3f>::type = {
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
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
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
    {"length", (PyCFunction)VRPyVec3f::length, METH_NOARGS, "Compute the length - float length()" },
    {"dot", (PyCFunction)VRPyVec3f::dot, METH_VARARGS, "Compute the dot product - float dot( Vec3 )" },
    {"cross", (PyCFunction)VRPyVec3f::cross, METH_VARARGS, "Compute the cross product - Vec3 cross( Vec3 )" },
    {"asList", (PyCFunction)VRPyVec3f::asList, METH_NOARGS, "Return as python list - [x,y,z] asList()" },
    {NULL}  /* Sentinel */
};

VRPyVec3f* toPyVec3f(const Vec3f& v) {
    VRPyVec3f* pv = (VRPyVec3f*)VRPyVec3f::typeRef->tp_alloc(VRPyVec3f::typeRef, 0);
    pv->owner = false;
    pv->v = v;
    return pv;
}

PyObject* toPyObject(const Vec3f& v) { return (PyObject*)toPyVec3f(v); }

PyObject* VRPyVec3f::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject* v = 0;
    float a,b,c;
    if (! PyArg_ParseTuple(args, "O", &v))
        if (! PyArg_ParseTuple(args, "fff", &a, &b, &c)) { setErr("Bad Constructor to Vec3"); return NULL; }
    VRPyVec3f* pv = (VRPyVec3f*)allocPtr( type, 0 );
    if (!v) pv->v = Vec3f(a,b,c);
    else pv->v = parseVec3fList(v);
    return (PyObject*)pv;
}

PyObject* VRPyVec3f::Print(PyObject* self) {
    string s = "[" + toString(((VRPyVec3f*)self)->v) + "]";
    return PyString_FromString( s.c_str() );
}

PyObject* VRPyVec3f::normalize(VRPyVec3f* self) {
    self->v.normalize();
    //Py_RETURN_TRUE;
    return (PyObject*) toPyVec3f(self->v);
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
    VRPyVec3f* v;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return PyFloat_FromDouble( self->v.dot(v->v) );
}

PyObject* VRPyVec3f::cross(VRPyVec3f* self, PyObject* args) {
    PyObject* v = 0;
    if (!PyArg_ParseTuple(args, "O", &v)) return NULL;
    return ::toPyObject( self->v.cross( parseVec3fList(v)) );
}


PyObject* VRPyVec3f::add(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec3f*)self)->v + parseVec3fList(v) );
}

PyObject* VRPyVec3f::sub(PyObject* self, PyObject* v) {
    return ::toPyObject( ((VRPyVec3f*)self)->v - parseVec3fList(v) );
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
    Vec3f v = ((VRPyVec3f*)self)->v;
    return ::toPyObject( Vec3f(::abs(v[0]), ::abs(v[1]), ::abs(v[2])) );
}

PySequenceMethods VRPyVec3f::sMethods = {
    (lenfunc)VRPyVec3f::len,       /* inquiry sq_length;             /* __len__ */
    0,    /* binaryfunc sq_concat;          /* __add__ */
    0,    /* intargfunc sq_repeat;          /* __mul__ */
    (ssizeargfunc)VRPyVec3f::getItem,   /* intargfunc sq_item;            /* __getitem__ */
    0,  /* intintargfunc sq_slice;        /* __getslice__ */
    (ssizeobjargproc)VRPyVec3f::setItem,   /* intobjargproc sq_ass_item;     /* __setitem__ */
    0,  /* intintobjargproc sq_ass_slice; /* __setslice__ */
};

PyObject* VRPyVec3f::len(PyObject* self) {
    /*if (!PyNumber_Check(F)) { setErr("Dividing by a vector is not allowed"); return NULL; }
    float f = PyFloat_AsDouble(F);
    return ::toPyObject( ((VRPyVec3f*)self)->v * (1.0/f) );*/
}

PyObject* VRPyVec3f::getItem(PyObject* self, PyObject* args) {
    //return ::toPyObject( -((VRPyVec3f*)self)->v);
}

PyObject* VRPyVec3f::setItem(PyObject* self, PyObject* args) {
    //Vec3f v = ((VRPyVec3f*)self)->v;
    //return ::toPyObject( Vec3f(::abs(v[0]), ::abs(v[1]), ::abs(v[2])) );
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
    L->l = Line( parseVec3fList(p), parseVec3fList(d) );
    return (PyObject*)L;
}

PyObject* VRPyLine::Print(PyObject* self) {
    auto l = ((VRPyLine*)self)->l;
    string s = "[" + toString(l) + "]";
    return PyString_FromString( s.c_str() );
}

PyObject* VRPyLine::pos(VRPyLine* self) {
    return ::toPyObject(Vec3f(self->l.getPosition()));
}

PyObject* VRPyLine::dir(VRPyLine* self) {
    return ::toPyObject(Vec3f(self->l.getDirection()));
}

PyObject* VRPyLine::intersect(VRPyLine* self, PyObject *args) {
    PyObject* l = 0;
    if (! PyArg_ParseTuple(args, "O", &l)) return NULL;
    VRPyLine* L = (VRPyLine*)l;

    auto insct = [&](const Pnt3f& p1, const Vec3f& n1, const Pnt3f& p2, const Vec3f& n2) -> Vec3f {
		Vec3f d = p2-p1;
		Vec3f n3 = n1.cross(n2);
		float N3 = n3.dot(n3);
		if (N3 == 0) N3 = 1.0;
		float s = d.cross(n2).dot(n1.cross(n2))/N3;
		return Vec3f(p1) + n1*s;
    };

    auto i = insct( self->l.getPosition(), self->l.getDirection(), L->l.getPosition(), L->l.getDirection() );
    return ::toPyObject( i );
}
