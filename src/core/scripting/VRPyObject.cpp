#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "core/objects/OSGObject.h"

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
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_ptr,                 /* tp_new */
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
    {"getID", (PyCFunction)VRPyObject::getID, METH_NOARGS, "Return the object internal ID" },
    {"duplicate", (PyCFunction)VRPyObject::duplicate, METH_NOARGS, "Duplicate object including subtree" },
    {"getChild", (PyCFunction)VRPyObject::getChild, METH_VARARGS, "Return child object with index i" },
    {"getChildren", (PyCFunction)VRPyObject::getChildren, METH_VARARGS, "Return the list of children objects\n\t - getChildren() : return immediate children\n\t - getChildren(bool recursive) : if true returns whole subtree\n\t - getChildren(bool recursive, str type) : filter by type" },
    {"getParent", (PyCFunction)VRPyObject::getParent, METH_NOARGS, "Return parent object" },
    {"find", (PyCFunction)VRPyObject::find, METH_VARARGS, "Find node with given name or ID in scene graph below this node - obj find(str/int)" },
    {"findAll", (PyCFunction)VRPyObject::findAll, METH_VARARGS, "Find node with given base name (str) in scene graph below this node" },
    {"isPickable", (PyCFunction)VRPyObject::isPickable, METH_NOARGS, "Return if the object is pickable" },
    {"setPickable", (PyCFunction)VRPyObject::setPickable, METH_VARARGS, "Set if the object is pickable - setPickable(int pickable)\n   pickable can be 0 or 1 to disable or enable picking, as well as -1 to block picking even if an ancestor is pickable" },
    {"printOSG", (PyCFunction)VRPyObject::printOSG, METH_NOARGS, "Print the OSG structure to console" },
    {"flattenHiarchy", (PyCFunction)VRPyObject::flattenHiarchy, METH_NOARGS, "Flatten the scene graph hiarchy" },
    {"addTag", (PyCFunction)VRPyObject::addTag, METH_VARARGS, "Add a tag to the object - addTag( str tag )" },
    {"hasTag", (PyCFunction)VRPyObject::hasTag, METH_VARARGS, "Check if the object has a tag - bool hasTag( str tag )" },
    {"remTag", (PyCFunction)VRPyObject::remTag, METH_VARARGS, "Remove a tag from the object - remTag( str tag )" },
    {"getTags", (PyCFunction)VRPyObject::getTags, METH_NOARGS, "Return all tags - [str] getTags()" },
    {"hasAncestorWithTag", (PyCFunction)VRPyObject::hasAncestorWithTag, METH_VARARGS, "Check if the object or an ancestor has a tag - obj hasAncestorWithTag( str tag )" },
    {"getChildrenWithTag", (PyCFunction)VRPyObject::getChildrenWithTag, METH_VARARGS, "Get all children which have the tag - [objs] getChildrenWithTag( str tag )" },
    {"setVolumeCheck", (PyCFunction)VRPyObject::setVolumeCheck, METH_VARARGS, "Enables or disabled the dynamic volume computation of that node - setVolumeCheck( bool )" },
    {"setTravMask", (PyCFunction)VRPyObject::setTravMask, METH_VARARGS, "Set the traversal mask of the object - setTravMask( int mask )" },
    {"setPersistency", (PyCFunction)VRPyObject::setPersistency, METH_VARARGS, "Set the persistency level - setPersistency( int persistency )\n   0: not persistent\n   1: persistent hiarchy\n   2: transformation\n   3: geometry\n   4: fully persistent" },
    {"getPersistency", (PyCFunction)VRPyObject::getPersistency, METH_NOARGS, "Get the persistency level - getPersistency()" },
    {"addLink", (PyCFunction)VRPyObject::addLink, METH_VARARGS, "Link subtree - addLink( object )" },
    {"remLink", (PyCFunction)VRPyObject::remLink, METH_VARARGS, "Unlink subtree - remLink( object )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyObject::setVolumeCheck(VRPyObject* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setVolume( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::addLink(VRPyObject* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->addLink( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::remLink(VRPyObject* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->remLink( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getID(VRPyObject* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->getID() );
}

PyObject* VRPyObject::setPersistency(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::setPersistency - C Object is invalid"); return NULL; }
    self->objPtr->setPersistency( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getPersistency(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getPersistency - C Object is invalid"); return NULL; }
    return PyInt_FromLong( self->objPtr->getPersistency() );
}

PyObject* VRPyObject::getChildrenWithTag(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getChildrenWithTag - C Object is invalid"); return NULL; }

    vector<OSG::VRObjectPtr> objs = self->objPtr->getChildrenWithAttachment( parseString(args) );

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    }
    return li;
}

PyObject* VRPyObject::setTravMask(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::setTravMask - C Object is invalid"); return NULL; }
    self->objPtr->getNode()->node->setTravMask( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::addTag(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::addTag - C Object is invalid"); return NULL; }
    self->objPtr->addAttachment( parseString(args) , 0);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::hasTag(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::hasTag - C Object is invalid"); return NULL; }
    return PyBool_FromLong( self->objPtr->hasAttachment( parseString(args) ) );
}

PyObject* VRPyObject::hasAncestorWithTag(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::hasAncestorWithTag - C Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast( self->objPtr->hasAncestorWithAttachment( parseString(args) ) );
}

PyObject* VRPyObject::getTags(VRPyObject* self) {
    if (!self->valid()) return NULL;
    auto tags = self->objPtr->getAttachmentNames();
    PyObject* li = PyList_New(tags.size());
    for (uint i=0; i<tags.size(); i++) PyList_SetItem(li, i, PyString_FromString(tags[i].c_str()));
    return li;
}

PyObject* VRPyObject::remTag(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::remTag - C Object is invalid"); return NULL; }
    self->objPtr->remAttachment( parseString(args) );
    Py_RETURN_TRUE;
}

int VRPyObject::compare(PyObject* p1, PyObject* p2) {
    if (Py_TYPE(p1) != Py_TYPE(p2)) return -1;
    VRPyBaseT* o1 = (VRPyBaseT*)p1;
    VRPyBaseT* o2 = (VRPyBaseT*)p2;
    return (o1->objPtr == o2->objPtr) ? 0 : -1;
}

long VRPyObject::hash(PyObject* p) {
    VRPyBaseT* o = (VRPyBaseT*)p;
    return (long)o->objPtr.get();
}

PyObject* VRPyObject::flattenHiarchy(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::flattenHiarchy - C Object is invalid"); return NULL; }
    self->objPtr->flattenHiarchy();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::printOSG(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::printOSG - C Object is invalid"); return NULL; }
    auto n = self->objPtr->getNode();
    OSG::VRObject::printOSGTree(n);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getName(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getName().c_str());
}

PyObject* VRPyObject::setName(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    string name = parseString(args);
    self->objPtr->setName(name);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::hide(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->objPtr->hide();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::show(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->objPtr->show();
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::isVisible(VRPyObject* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    if (self->objPtr->isVisible()) Py_RETURN_TRUE;
	else Py_RETURN_FALSE;
}

PyObject* VRPyObject::setVisible(VRPyObject* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->objPtr->setVisible( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getType(VRPyObject* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getType().c_str());
}

PyObject* VRPyObject::destroy(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    self->objPtr->destroy();
    self->objPtr = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::addChild(VRPyObject* self, PyObject* args, PyObject *kwds) {
    VRPyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if ( isNone((PyObject*)child) ) Py_RETURN_TRUE;

    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::addChild, Parent is invalid"); return NULL; }
    if (child->objPtr == 0) { PyErr_SetString(err, "VRPyObject::addChild, Child is invalid"); return NULL; }

    self->objPtr->addChild(child->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::switchParent(VRPyObject* self, PyObject* args, PyObject *kwds) {
    VRPyObject* parent = NULL;
    if (! PyArg_ParseTuple(args, "O", &parent)) return NULL;
    if (parent == NULL) {
        PyErr_SetString(err, "Missing parent parameter");
        return NULL;
    }

    if (self->objPtr == 0) { PyErr_SetString(err, "C Child is invalid"); return NULL; }
    if (parent->objPtr == 0) { PyErr_SetString(err, "C Parent is invalid"); return NULL; }

    self->objPtr->switchParent(parent->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::duplicate(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Child is invalid"); return NULL; }
    OSG::VRObjectPtr d = (OSG::VRObjectPtr)self->objPtr->duplicate(true);
    d->setPersistency(0);
    return VRPyTypeCaster::cast(d);
}

PyObject* VRPyObject::getChild(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getChild, Child is invalid"); return NULL; }

    int i = 0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    OSG::VRObjectPtr c = self->objPtr->getChild(i);

    return VRPyTypeCaster::cast(c);
}

PyObject* VRPyObject::getChildren(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getChild, Child is invalid"); return NULL; }

    const char* ptype = 0; int doRecursive = 0;
    if (! PyArg_ParseTuple(args, "|is", &doRecursive, (char*)&ptype)) return NULL;
    string stype; if(ptype) stype = string(ptype);
    vector<OSG::VRObjectPtr> objs = self->objPtr->getChildren(doRecursive, stype);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    return li;
}

PyObject* VRPyObject::getParent(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getParent, C object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->objPtr->getParent());
}

PyObject* VRPyObject::find(VRPyObject* self, PyObject* args) {
	if (!self->valid()) return NULL;
    string name; int ID = -1;
    PyObject* o = parseObject(args);
    if (PyString_Check(o)) name = PyString_AsString(o);
    if (PyInt_Check(o)) ID = PyInt_AsLong(o);
    if (name != "") return VRPyTypeCaster::cast( self->objPtr->find(name) );
    if (ID >= 0) return VRPyTypeCaster::cast( self->objPtr->find(ID) );
    Py_RETURN_NONE;
}

PyObject* VRPyObject::findAll(VRPyObject* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::find, C object is invalid"); return NULL; }

    string name = parseString(args);
    vector<OSG::VRObjectPtr> objs = self->objPtr->findAll(name);

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, VRPyTypeCaster::cast(objs[i]));
    }

    return li;
}

PyObject* VRPyObject::isPickable(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::isPickable, C Object is invalid"); return NULL; }
    return PyBool_FromLong(self->objPtr->isPickable());
}

PyObject* VRPyObject::setPickable(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::setPickable, C Object is invalid"); return NULL; }
    self->objPtr->setPickable( parseInt(args) );
    Py_RETURN_TRUE;
}

