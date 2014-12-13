#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

#include <OpenSG/OSGNode.h>

template<> PyTypeObject VRPyBaseT<OSG::VRObject>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Object",             /*tp_name*/
    sizeof(VRPyObject),             /*tp_basicsize*/
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
    "VRObject binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyObject::methods,             /* tp_methods */
    VRPyObject::members,             /* tp_members */
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

PyMemberDef VRPyObject::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyObject::methods[] = {
    {"getName", (PyCFunction)VRPyObject::getName, METH_NOARGS, "Return the object name" },
    {"setName", (PyCFunction)VRPyObject::setName, METH_VARARGS, "Set the object name" },
    {"addChild", (PyCFunction)VRPyObject::addChild, METH_VARARGS, "Add object as child" },
    {"switchParent", (PyCFunction)VRPyObject::switchParent, METH_VARARGS, "Switch object to other parent object" },
    {"destroy", (PyCFunction)VRPyObject::destroy, METH_NOARGS, "Destroy object" },
    {"hide", (PyCFunction)VRPyObject::hide, METH_NOARGS, "Hide object" },
    {"show", (PyCFunction)VRPyObject::show, METH_NOARGS, "Show object" },
    {"isVisible", (PyCFunction)VRPyObject::isVisible, METH_NOARGS, "Return if object is visible" },
    {"setVisible", (PyCFunction)VRPyObject::setVisible, METH_VARARGS, "Set the visibility of the object" },
    {"getType", (PyCFunction)VRPyObject::getType, METH_NOARGS, "Return the object type string (such as \"Geometry\")" },
    {"duplicate", (PyCFunction)VRPyObject::duplicate, METH_NOARGS, "Duplicate object including subtree" },
    {"getChild", (PyCFunction)VRPyObject::getChild, METH_VARARGS, "Return child object with index i" },
    {"getChildren", (PyCFunction)VRPyObject::getChildren, METH_VARARGS, "Return the list of children objects" },
    {"getParent", (PyCFunction)VRPyObject::getParent, METH_NOARGS, "Return parent object" },
    {"find", (PyCFunction)VRPyObject::find, METH_VARARGS, "Find node with given name (str) in scene graph below this node" },
    {"isPickable", (PyCFunction)VRPyObject::isPickable, METH_NOARGS, "Return if the object is pickable" },
    {"setPickable", (PyCFunction)VRPyObject::setPickable, METH_VARARGS, "Set if the object is pickable" },
    {"printOSG", (PyCFunction)VRPyObject::printOSG, METH_NOARGS, "Print the OSG structure to console" },
    {"flattenHiarchy", (PyCFunction)VRPyObject::flattenHiarchy, METH_NOARGS, "Flatten the scene graph hiarchy" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyObject::flattenHiarchy(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::flattenHiarchy - C Object is invalid"); return NULL; }
    self->obj->flattenHiarchy();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::printOSG(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::printOSG - C Object is invalid"); return NULL; }
    OSG::NodeRecPtr n = self->obj->getNode();
    OSG::VRObject::printOSGTree(n);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getName(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getName().c_str());
}

PyObject* VRPyObject::setName(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    string name = parseString(args);
    self->obj->setName(name);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::hide(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->obj->hide();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::show(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->obj->show();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::isVisible(VRPyObject* self) {
	if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    if (self->obj->isVisible()) Py_RETURN_TRUE;
	else Py_RETURN_FALSE;
}

PyObject* VRPyObject::setVisible(VRPyObject* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->obj->setVisible( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getType(VRPyObject* self) {
	if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getType().c_str());
}

/*PyObject* VRPyObject::setVisible(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getName().c_str());
}

PyObject* VRPyObject::isVisible(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getName().c_str());
}*/

PyObject* VRPyObject::destroy(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->obj->destroy();
    self->obj = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::addChild(VRPyObject* self, PyObject* args, PyObject *kwds) {
    VRPyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }

    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::addChild, Parent is invalid"); return NULL; }
    if (child->obj == 0) { PyErr_SetString(err, "VRPyObject::addChild, Child is invalid"); return NULL; }

    self->obj->addChild(child->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::switchParent(VRPyObject* self, PyObject* args, PyObject *kwds) {
    VRPyObject* parent = NULL;
    if (! PyArg_ParseTuple(args, "O", &parent)) return NULL;
    if (parent == NULL) {
        PyErr_SetString(err, "Missing parent parameter");
        return NULL;
    }

    if (self->obj == 0) { PyErr_SetString(err, "C Child is invalid"); return NULL; }
    if (parent->obj == 0) { PyErr_SetString(err, "C Parent is invalid"); return NULL; }

    self->obj->switchParent(parent->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::duplicate(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "C Child is invalid"); return NULL; }
    OSG::VRObject* d = (OSG::VRObject*)self->obj->duplicate();
    d->addAttachment("dynamicaly_generated", 0);
    return VRPyObject::fromPtr( d );
}

PyObject* VRPyObject::getChild(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::getChild, Child is invalid"); return NULL; }

    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    OSG::VRObject* c = self->obj->getChild(i);

    return VRPyTypeCaster::cast(c);
}

PyObject* VRPyObject::getChildren(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::getChild, Child is invalid"); return NULL; }

    PyObject* ptype = 0; int doRecursive = 0;
    if (PyTuple_Size(args) == 1) if (! PyArg_ParseTuple(args, "i", &doRecursive)) return NULL;
    if (PyTuple_Size(args) == 2) if (! PyArg_ParseTuple(args, "iO", &doRecursive, &ptype)) return NULL;

    string type;
    if (ptype) type = PyString_AsString(ptype);

    vector<OSG::VRObject*> objs = self->obj->getChildren(doRecursive, type);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    }

    return li;
}

PyObject* VRPyObject::getParent(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::getParent, C object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->obj->getParent());
}

PyObject* VRPyObject::find(VRPyObject* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::find, C object is invalid"); return NULL; }

    string name = parseString(args);
    OSG::VRObject* c = self->obj->find(name);
    if (c) { return VRPyTypeCaster::cast(c); }
    else { Py_RETURN_NONE; }
}

PyObject* VRPyObject::isPickable(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::isPickable, C Object is invalid"); return NULL; }
    return PyBool_FromLong(self->obj->isPickable());
}

PyObject* VRPyObject::setPickable(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::setPickable, C Object is invalid"); return NULL; }
    bool pickable = parseBool(args);
    self->obj->setPickable(pickable);
    Py_RETURN_TRUE;
}

