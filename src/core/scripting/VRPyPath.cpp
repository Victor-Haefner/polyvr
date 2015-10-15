#include "VRPyPath.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::path>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Path",             /*tp_name*/
    sizeof(VRPyPath),             /*tp_basicsize*/
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
    "Path binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyPath::methods,             /* tp_methods */
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

PyMethodDef VRPyPath::methods[] = {
    {"set", (PyCFunction)VRPyPath::set, METH_VARARGS, "Set the path - set(start pos, start dir, end pos, end dir, steps) \n       set(start pos, start dir, start up, end pos, end dir, end up, steps)" },
    {"setStart", (PyCFunction)VRPyPath::setStartPoint, METH_VARARGS, "Set the path start point" },
    {"setEnd", (PyCFunction)VRPyPath::setEndPoint, METH_VARARGS, "Set the path end point" },
    {"getStart", (PyCFunction)VRPyPath::getStartPoint, METH_NOARGS, "Get the path start point" },
    {"getEnd", (PyCFunction)VRPyPath::getEndPoint, METH_NOARGS, "Get the path end point" },
    {"invert", (PyCFunction)VRPyPath::invert, METH_NOARGS, "Invert start && end point of path" },
    {"compute", (PyCFunction)VRPyPath::compute, METH_VARARGS, "Compute path" },
    {"update", (PyCFunction)VRPyPath::update, METH_NOARGS, "Update path" },
    {"addPoint", (PyCFunction)VRPyPath::addPoint, METH_VARARGS, "Add a point to the path - int addPoint(vec3 pos, vec3 dir, vec3 col, vec3 up)" },
    {"getPoints", (PyCFunction)VRPyPath::getPoints, METH_NOARGS, "Return a list of the path points - [pos, dir, col, up] getPoints()" },
    {"close", (PyCFunction)VRPyPath::close, METH_NOARGS, "Close the path - close()" },
    {"getPositions", (PyCFunction)VRPyPath::getPositions, METH_NOARGS, "Return the positions from the computed path - [[x,y,z]] getPositions()" },
    {"getDirections", (PyCFunction)VRPyPath::getDirections, METH_NOARGS, "Return the directions from the computed path - [[x,y,z]] getDirections()" },
    {"getUpVectors", (PyCFunction)VRPyPath::getUpVectors, METH_NOARGS, "Return the up vectors from the computed path - [[x,y,z]] getUpVectors()" },
    {"getColors", (PyCFunction)VRPyPath::getColors, METH_NOARGS, "Return the colors from the computed path - [[x,y,z]] getColors()" },
    {"getSize", (PyCFunction)VRPyPath::getSize, METH_NOARGS, "Return the number of path nodes - int getSize()" },
    {"getLength", (PyCFunction)VRPyPath::getLength, METH_NOARGS, "Return the approximated path length - float getLength()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPath::getLength(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getLength - Object is invalid"); return NULL; }
    return PyFloat_FromDouble( self->obj->getLength() );
}

PyObject* VRPyPath::getSize(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getSize - Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->size() );
}

PyObject* VRPyPath::getPoints(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getPoints - Object is invalid"); return NULL; }
    auto pnts = self->obj->getPoints();
    if (pnts.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pnts.size());
    for (uint i=0; i<pnts.size(); i++) {
        PyObject* pnt = PyList_New(4);
        PyList_SetItem(pnt, 0, toPyTuple(pnts[i].p));
        PyList_SetItem(pnt, 1, toPyTuple(pnts[i].n));
        PyList_SetItem(pnt, 2, toPyTuple(pnts[i].c));
        PyList_SetItem(pnt, 3, toPyTuple(pnts[i].u));
        PyList_SetItem(res, i, pnt);
    }
    return res;
}

PyObject* VRPyPath::getPositions(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getPositions - Object is invalid"); return NULL; }
    auto pos = self->obj->getPositions();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getDirections(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getDirections - Object is invalid"); return NULL; }
    auto pos = self->obj->getDirections();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getUpVectors(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::close - Object is invalid"); return NULL; }
    auto pos = self->obj->getUpvectors();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::getColors(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::close - Object is invalid"); return NULL; }
    auto pos = self->obj->getColors();
    if (pos.size() == 0) return PyList_New(0);
    PyObject* res = PyList_New(pos.size());
    for (uint i=0; i<pos.size(); i++) PyList_SetItem(res, i, toPyTuple(pos[i]));
    return res;
}

PyObject* VRPyPath::set(VRPyPath* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::set, Object is invalid"); return NULL; }

    int i;
    PyObject *p1, *p2, *n1, *n2, *u1 = 0, *u2 = 0;
    if (PyList_GET_SIZE(args) <= 5) {
        if (! PyArg_ParseTuple(args, "OOOOi", &p1, &n1, &p2, &n2, &i)) return NULL;
    } else if (! PyArg_ParseTuple(args, "OOOOOOi", &p1, &n1, &u1, &p2, &n2, &u2, &i)) return NULL;

    OSG::Vec3f c, uv1(0,1,0), uv2(0,1,0);
    if (u1) uv1 = parseVec3fList(u1);
    if (u2) uv2 = parseVec3fList(u2);
    self->obj->addPoint(parseVec3fList(p1), parseVec3fList(n1), c, uv1);
    self->obj->addPoint(parseVec3fList(p2), parseVec3fList(n2), c, uv2);
    self->obj->compute(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::close(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::invert, Object is invalid"); return NULL; }
    self->obj->close();
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::invert(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::invert, Object is invalid"); return NULL; }
    self->obj->invert();
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::addPoint(VRPyPath* self, PyObject* args) {
    PyObject *_p, *_n, *_c, *_u; _p=_n=_c=_u=0;
    if (! PyArg_ParseTuple(args, "OOOO", &_p, &_n, &_c, &_u)) return NULL;

    OSG::Vec3f p, n, c, u;
    p = parseVec3fList(_p);
    n = parseVec3fList(_n);
    c = parseVec3fList(_c);
    u = parseVec3fList(_u);

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::setStartPoint, Object is invalid"); return NULL; }
    self->obj->addPoint(p,n,c,u);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::setStartPoint(VRPyPath* self, PyObject* args) {
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = parseVec3fList(_p);
    n = parseVec3fList(_n);
    c = parseVec3fList(_c);

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::setStartPoint, Object is invalid"); return NULL; }
    self->obj->addPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::setEndPoint(VRPyPath* self, PyObject* args) {
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = parseVec3fList(_p);
    n = parseVec3fList(_n);
    c = parseVec3fList(_c);

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::setEndPoint, Object is invalid"); return NULL; }
    self->obj->addPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::getStartPoint(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getStartPoint, Object is invalid"); return NULL; }
    OSG::Vec3f v[3];
    auto pnt = self->obj->getPoint(0);
    v[0] = pnt.p;
    v[1] = pnt.n;
    v[2] = pnt.c;

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::getEndPoint(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getEndPoint, Object is invalid"); return NULL; }
    OSG::Vec3f v[3];
    auto pnt = self->obj->getPoint(1);
    v[0] = pnt.p;
    v[1] = pnt.n;
    v[2] = pnt.c;

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::compute(VRPyPath* self, PyObject* args) {
    int N; N=0;
    if (! PyArg_ParseTuple(args, "i", &N)) return NULL;

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::compute, Object is invalid"); return NULL; }
    self->obj->compute(N);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::update(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::compute, Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}
