#include "VRPySelection.h"
#include "VRPyGeometry.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "VRPyPose.h"

using namespace OSG;

simpleVRPyType(Selection, New_ptr);

PyMethodDef VRPySelection::methods[] = {
    {"add", (PyCFunction)VRPySelection::add, METH_VARARGS|METH_KEYWORDS, "add to the selection - add(object)" },
    {"sub", (PyCFunction)VRPySelection::sub, METH_VARARGS|METH_KEYWORDS, "substract from the selection - sub(object)" },
    {"append", (PyCFunction)VRPySelection::append, METH_VARARGS, "Append selection - append( selection)" },
    {"clear", (PyCFunction)VRPySelection::clear, METH_NOARGS, "clear selection - clear()" },
    {"getSelected", (PyCFunction)VRPySelection::getSelected, METH_NOARGS, "Return the selected objects - [object] getSelected()" },
    {"getPartialSelected", (PyCFunction)VRPySelection::getPartialSelected, METH_NOARGS, "Return the partially selected - [object] getPartialSelected()" },
    {"getSubselection", (PyCFunction)VRPySelection::getSubselection, METH_VARARGS, "Return the objects selected vertices - [int] getSubselection(object)" },
    {"computePCA", (PyCFunction)VRPySelection::computePCA, METH_NOARGS, "Return the selection PCA - pose computePCA()" },
    {"selectPlane", (PyCFunction)VRPySelection::selectPlane, METH_VARARGS, "Select a plane - pose selectPlane( pose, threshold )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySelection::computePCA(VRPySelection* self) {
    if (!self->valid()) return NULL;
#ifndef WITHOUT_LAPACKE_BLAS
    return VRPyPose::fromObject( self->objPtr->computePCA() );
#endif
}

PyObject* VRPySelection::selectPlane(VRPySelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* plane = 0; float t;
    if (! PyArg_ParseTuple(args, "Of:selectPlane", &plane, &t)) return NULL;
    self->objPtr->selectPlane(*plane->objPtr, t);
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::add(VRPySelection* self, PyObject* args, PyObject* kwargs) {
    if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    PyObject* verts = 0;
    const char* kwlist[] = {"object", "vertices", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:add", (char**)kwlist, &geo, &verts)) return NULL;

    self->objPtr->add(geo->objPtr);
    if (verts) {
        vector<int> vec( PyList_GET_SIZE(verts) );
        for (int i=0; i<PyList_GET_SIZE(verts); i++) vec[i] = PyInt_AsLong( PyList_GetItem(verts,i) );
        self->objPtr->add(geo->objPtr, vec);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::sub(VRPySelection* self, PyObject* args, PyObject* kwargs) {
    if (!self->valid()) return NULL;
    //VRPyGeometry* geo = 0;
    //if (!PyArg_ParseTuple(args, "O:sub", &geo)) return NULL;
    //self->objPtr->sub(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::append(VRPySelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPySelection* sel = 0;
    if (!PyArg_ParseTuple(args, "O:append", &sel)) return NULL;
    self->objPtr->append(sel->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::clear(VRPySelection* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPySelection::getSubselection(VRPySelection* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGeometry* geo = 0;
    if (!PyArg_ParseTuple(args, "|O:getSubselection", &geo)) return NULL;

    auto toPyArray = [](vector<int>& v) {
        PyObject* res = PyList_New(v.size());
        for (unsigned int i=0; i<v.size(); i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
        return res;
    };

    if (geo) {
        auto v = self->objPtr->getSubselection( geo->objPtr );
        return toPyArray( v );
    } else {
        PyObject* res = PyDict_New();
        for (auto m : self->objPtr->getSubselections()) {
            if (m.first) PyDict_SetItem(res, VRPyGeometry::fromSharedPtr(m.first), toPyArray(m.second) );
        }
        return res;
    }
}

PyObject* VRPySelection::getSelected(VRPySelection* self) {
    if (!self->valid()) return NULL;
    auto sel = self->objPtr->getSelected();
    PyObject* res = PyList_New(sel.size());
    for (unsigned int i=0; i<sel.size(); i++) {
        auto sp = sel[i].lock();
        if (sp) PyList_SetItem(res, i, VRPyGeometry::fromSharedPtr(sp));
    }
    return res;
}

PyObject* VRPySelection::getPartialSelected(VRPySelection* self) {
    if (!self->valid()) return NULL;
    auto sel = self->objPtr->getPartials();
    PyObject* res = PyList_New(sel.size());
    for (unsigned int i=0; i<sel.size(); i++) {
        auto sp = sel[i].lock();
        if (sp) PyList_SetItem(res, i, VRPyGeometry::fromSharedPtr(sp));
    }
    return res;
}
