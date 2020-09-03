#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "addons/Semantics/VRSemanticsFwd.h"

#include "VRPyBaseFactory.h"
#include "VRPyBoundingbox.h"

#include <OpenSG/OSGNode.h>

using namespace OSG;

template<> bool toValue(PyObject* o, VRObjectPtr& v) { if (!VRPyObject::check(o)) return 0; v = ((VRPyObject*)o)->objPtr; return 1; }

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

const char* exportToFileDoc = "Export subtree to file"
"bla"
"\nblub";

// no idea why, but windows does not find the definition in VREntity.cpp
#ifdef _WIN32
template<> string typeName(const OSG::VREntity& o) { return "Entity"; }
#endif

PyMethodDef VRPyObject::methods[] = {
    {"destroy", (PyCFunction)VRPyObject::destroy, METH_NOARGS, "Destroy object and reset py object to None" },
    {"getName", (PyCFunction)VRPyObject::getName, METH_NOARGS, "Return the object name" },
    {"getBaseName", (PyCFunction)VRPyObject::getBaseName, METH_NOARGS, "Return the object base name" },
    {"setName", (PyCFunction)VRPyObject::setName, METH_VARARGS, "Set the object name" },
    {"addChild", PyWrapOpt(Object, addChild, "Add object as child", "1|-1", void, VRObjectPtr, bool, int ) },
    {"subChild", PyWrapOpt(Object, subChild, "Sub child object", "1", void, VRObjectPtr, bool ) },
    {"switchParent", PyWrapOpt(Object, switchParent, "Switch object to other parent object", "-1", void, VRObjectPtr, int) },
    {"hasDescendant", PyWrap(Object, hasDescendant, "Check if object in in subgraph", bool, VRObjectPtr) },
    {"hasAncestor", PyWrap(Object, hasAncestor, "Check if object is an ancestor", bool, VRObjectPtr) },
    {"hide", PyWrapOpt(Object, hide, "Hide object", "", void, string) },
    {"show", PyWrapOpt(Object, show, "Show object", "", void, string) },
    {"isVisible", PyWrapOpt(Object, isVisible, "Return if object is visible", "|0", bool, string, bool) },
    {"setVisible", PyWrapOpt(Object, setVisible, "Set the visibility of the object, flag changes behaviour, for example 'SHADOW'", "", void, bool, string) },
    {"toggleVisible", PyWrapOpt(Object, toggleVisible, "Toggle the visibility of the object, flag changes behaviour, for example 'SHADOW'", "", void, string) },
    {"getType", PyWrap(Object, getType, "Return the object type string (such as \"Geometry\")", string) },
    {"getID", PyWrap(Object, getID, "Return the object internal ID", int) },
    {"duplicate", PyWrapOpt(Object, duplicate, "Duplicate object, bool anchored (False), bool withSubTree (True)", "0|1", VRObjectPtr, bool, bool) },
    {"getChild", PyWrap(Object, getChild, "Return child object with index i", VRObjectPtr, int) },
    {"getChildren", PyWrapOpt(Object, getChildren, "Return the list of children objects (recursive, type-filter, includeSelf)", "0||0", vector<VRObjectPtr>, bool, string, bool) },
    {"getParent", PyWrapOpt(Object, getParent, "Return parent object, passing 'True' will take into account any DnD state", "0", VRObjectPtr, bool) },
    {"getAncestry", PyWrapOpt(Object, getAncestry, "Return all parents", "0", vector<VRObjectPtr>, VRObjectPtr) },
    {"find", PyWrap(Object, find, "Find node with given name in scene graph below this node - obj find(str)", VRObjectPtr, string) },
    {"findAll", PyWrapOpt(Object, findAll, "Find nodes with given base name (str) in scene graph below this node", " ", vector<VRObjectPtr>, string, vector<VRObjectPtr>) },
    {"isPickable", PyWrap(Object, isPickable, "Return if the object is pickable", bool) },
    {"setPickable", PyWrap(Object, setPickable, "Set if the object is pickable - setPickable(int pickable)\n   pickable can be 0 or 1 to disable or enable picking, as well as -1 to block picking even if an ancestor is pickable", void, int) },
    //{"printOSG", PyWrap(Object, printOSGTree, "Print the OSG structure to console", void) },
    {"flattenHiarchy", PyWrap(Object, flattenHiarchy, "Flatten the scene graph hiarchy", void) },
    {"addTag", PyWrap(Object, addTag, "Add a tag to the object - addTag( str tag )", void, string) },
    {"hasTag", PyWrap(Object, hasTag, "Check if the object has a tag - bool hasTag( str tag )", bool, string) },
    {"remTag", PyWrap(Object, remTag, "Remove a tag from the object - remTag( str tag )", void, string) },
    {"getTags", PyWrap(Object, getTags, "Return all tags - [str] getTags()", vector<string>) },
    {"setTagValue", PyWrap(Object, setAttachmentFromString, "Set tag value", void, string, string) },
    {"getTagValue", PyWrap(Object, getAttachmentAsString, "Return tag value", string, string) },
    {"hasAncestorWithTag", PyWrap(Object, hasAncestorWithTag, "Check if the object or an ancestor has a tag - obj hasAncestorWithTag( str tag )", VRObjectPtr, string) },
    {"getChildrenWithTag", PyWrapOpt(Object, getChildrenWithTag, "Get all children which have the tag (tag, recursive, includeSelf)", "0|0", vector<VRObjectPtr>, string, bool, bool) },
    {"setVolumeCheck", PyWrapOpt(Object, setVolumeCheck, "Enables or disabled the dynamic volume computation of that node - setVolumeCheck( bool )", "0", void, bool, bool) },
    {"setTravMask", PyWrap(Object, setTravMask, "Set the traversal mask of the object", void, int) },
    {"getTravMask", PyWrap(Object, getTravMask, "Get the traversal mask of the object", int) },
    {"setPersistency", (PyCFunction)VRPyObject::setPersistency, METH_VARARGS, "Set the persistency level - setPersistency( int persistency | bool recursive )\n   0: not persistent\n   1: persistent hiarchy\n   2: transformation\n   3: geometry\n   4: fully persistent" },
    {"getPersistency", (PyCFunction)VRPyObject::getPersistency, METH_NOARGS, "Get the persistency level - getPersistency()" },
    {"addLink", PyWrap(Object, addLink, "Link subtree", void, VRObjectPtr) },
    {"remLink", PyWrap(Object, remLink, "Unlink subtree", void, VRObjectPtr) },
    {"getLinks", PyWrap(Object, getLinks, "Return all links", vector<VRObjectPtr>) },
    {"setEntity", PyWrap(Object, setEntity, "Set entity", void, VREntityPtr) },
    {"getEntity", PyWrap(Object, getEntity, "Get entity", VREntityPtr) },
    {"clearChildren", PyWrapOpt(Object, clearChildren, "Remove all children", "1", void, bool) },
    {"getChildIndex", PyWrap(Object, getChildIndex, "Return the child index of this object", int) },
    {"getOSGTreeString", PyWrap(Object, getOSGTreeString, "Get string description of OSG subtree", string) },
    {"getBoundingbox", PyWrap(Object, getBoundingbox, "get Boundingbox", BoundingboxPtr) },
    {"getWorldBoundingbox", PyWrap(Object, getWorldBoundingbox, "get world Boundingbox", BoundingboxPtr) },
    {"setVolume", PyWrap(Object, setVolume, "Set the scenegraph volume to boundingbox", void, Boundingbox) },
    {"getPoseTo", PyWrap(Object, getPoseTo, "Get the transformation from this object to another, returns a pose", PosePtr, VRObjectPtr ) },
    {"exportToFile", PyWrap(Object, exportToFile, "Export object (and subtree) to file, supported extensions: [wrl, wrz, obj, osb, osg, ply, gltf]", void, string) },
    {"reduceModel", PyWrap(Object, reduceModel, "Reduce Model complexity", void, string) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyObject::destroy(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::destroy - C Object is invalid"); return NULL; }
    self->objPtr->destroy();
    self->objPtr = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::setPersistency(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::setPersistency - C Object is invalid"); return NULL; }
    int i = 0;
    int b = 0;
    if (!PyArg_ParseTuple(args, "i|i", &i, &b)) return NULL;
    self->objPtr->setPersistency( i );
    if (b) for (auto c : self->objPtr->getChildren(true)) c->setPersistency(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyObject::getPersistency(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyObject::getPersistency - C Object is invalid"); return NULL; }
    return PyInt_FromLong( self->objPtr->getPersistency() );
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

PyObject* VRPyObject::getName(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getName().c_str());
}

PyObject* VRPyObject::getBaseName(VRPyObject* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getBaseName().c_str());
}

PyObject* VRPyObject::setName(VRPyObject* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "C Object is invalid"); return NULL; }
    string name = parseString(args);
    self->objPtr->setName(name);
    Py_RETURN_TRUE;
}

