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
    VRPyObject::compare,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    VRPyObject::hash,                         /*tp_hash */
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
    {"getChildren", (PyCFunction)VRPyObject::getChildren, METH_VARARGS, "Return the list of children objects\n\t - getChildren() : return immediate children\n\t - getChildren(bool recursive) : if true returns whole subtree\n\t - getChildren(bool recursive, str type) : filter by type" },
    {"getParent", (PyCFunction)VRPyObject::getParent, METH_NOARGS, "Return parent object" },
    {"find", (PyCFunction)VRPyObject::find, METH_VARARGS, "Find node with given name (str) in scene graph below this node" },
    {"findAll", (PyCFunction)VRPyObject::findAll, METH_VARARGS, "Find node with given base name (str) in scene graph below this node" },
    {"isPickable", (PyCFunction)VRPyObject::isPickable, METH_NOARGS, "Return if the object is pickable" },
    {"setPickable", (PyCFunction)VRPyObject::setPickable, METH_VARARGS, "Set if the object is pickable" },
    {"printOSG", (PyCFunction)VRPyObject::printOSG, METH_NOARGS, "Print the OSG structure to console" },
    {"flattenHiarchy", (PyCFunction)VRPyObject::flattenHiarchy, METH_NOARGS, "Flatten the scene graph hiarchy" },
    {"addTag", (PyCFunction)VRPyObject::addTag, METH_VARARGS, "Add a tag to the object - addTag( str tag )" },
    {"hasTag", (PyCFunction)VRPyObject::hasTag, METH_VARARGS, "Check if the object has a tag - bool hasTag( str tag )" },
    {"remTag", (PyCFunction)VRPyObject::remTag, METH_VARARGS, "Remove a tag from the object - remTag( str tag )" },
    {"hasAncestorWithTag", (PyCFunction)VRPyObject::hasAncestorWithTag, METH_VARARGS, "Check if the object or an ancestor has a tag - obj hasAncestorWithTag( str tag )" },
    {"getChildrenWithTag", (PyCFunction)VRPyObject::getChildrenWithTag, METH_VARARGS, "Get all children which have the tag - [objs] getChildrenWithTag( str tag )" },
    {"setTravMask", (PyCFunction)VRPyObject::setTravMask, METH_VARARGS, "Set the traversal mask of the object - setTravMask( int mask )" },
    {"setPersistency", (PyCFunction)VRPyObject::setPersistency, METH_VARARGS, "Set the persistency level - setPersistency( int persistency )\n   0: not persistent\n   1: persistent hiarchy\n   2: transformation\n   3: geometry\n   4: fully persistent" },
    {"getPersistency", (PyCFunction)VRPyObject::getPersistency, METH_NOARGS, "Get the persistency level - getPersistency()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyObject::setPersistency(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::setPersistency - C Object is invalid"); return NULL; }
    self->obj->setPersistency( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getPersistency(VRPyObject* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::getPersistency - C Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->getPersistency() );
}

PyObject* VRPyObject::getChildrenWithTag(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::getChildrenWithTag - C Object is invalid"); return NULL; }

    vector<OSG::VRObject*> objs = self->obj->getChildrenWithAttachment( parseString(args) );

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    }
    return li;
}

PyObject* VRPyObject::setTravMask(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::setTravMask - C Object is invalid"); return NULL; }
    self->obj->getNode()->setTravMask( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::addTag(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::addTag - C Object is invalid"); return NULL; }
    self->obj->addAttachment( parseString(args) , 0);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::hasTag(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::hasTag - C Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->hasAttachment( parseString(args) ) );
}

PyObject* VRPyObject::hasAncestorWithTag(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::hasAncestorWithTag - C Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast( self->obj->hasAncestorWithAttachment( parseString(args) ) );
}

PyObject* VRPyObject::remTag(VRPyObject* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::remTag - C Object is invalid"); return NULL; }
    self->obj->remAttachment( parseString(args) );
    Py_RETURN_TRUE;
}

int VRPyObject::compare(PyObject* p1, PyObject* p2) {
    if (Py_TYPE(p1) != Py_TYPE(p2)) return -1;
    VRPyBaseT* o1 = (VRPyBaseT*)p1;
    VRPyBaseT* o2 = (VRPyBaseT*)p2;
    return (o1->obj == o2->obj) ? 0 : -1;
}

long VRPyObject::hash(PyObject* p) {
    VRPyBaseT* o = (VRPyBaseT*)p;
    return (long)o->obj;
}

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
    if ( isNone((PyObject*)child) ) Py_RETURN_TRUE;

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
    OSG::VRObject* d = (OSG::VRObject*)self->obj->duplicate(true);
    d->setPersistency(0);
    return VRPyTypeCaster::cast(d);
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

PyObject* VRPyObject::findAll(VRPyObject* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyObject::find, C object is invalid"); return NULL; }

    string name = parseString(args);
    vector<OSG::VRObject*> objs = self->obj->findAll(name);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    }

    return li;
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

