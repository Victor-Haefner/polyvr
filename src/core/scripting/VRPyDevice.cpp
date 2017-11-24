#include "VRPyDevice.h"
#include "VRPyMobile.h"
#include "VRPyMouse.h"
#include "VRPyMultiTouch.h"
#include "VRPyHaptic.h"
#include "VRPyTransform.h"
#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "core/objects/VRTransform.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;
simpleVRPyType(Device, New_named_ptr)

PyMethodDef VRPyDevice::methods[] = {
    {"getName", (PyCFunction)VRPyDevice::getName, METH_NOARGS, "Return device name." },
    {"destroy", (PyCFunction)VRPyDevice::destroy, METH_NOARGS, "Destroy device." },
    {"getBeacon", (PyCFunction)VRPyDevice::getBeacon, METH_NOARGS, "Get device beacon." },
    {"setBeacon", (PyCFunction)VRPyDevice::setBeacon, METH_VARARGS, "Set device beacon." },
    {"getTarget", (PyCFunction)VRPyDevice::getTarget, METH_NOARGS, "Get device target." },
    {"setTarget", (PyCFunction)VRPyDevice::setTarget, METH_VARARGS, "Set device target." },
    {"getKey", (PyCFunction)VRPyDevice::getKey, METH_NOARGS, "Get activated device key." },
    {"getState", (PyCFunction)VRPyDevice::getState, METH_NOARGS, "Get device state." },
    {"getKeyState", (PyCFunction)VRPyDevice::getKeyState, METH_VARARGS, "Get device key state." },
    {"getSlider", (PyCFunction)VRPyDevice::getSlider, METH_VARARGS, "Get device slider state." },
    {"getMessage", (PyCFunction)VRPyDevice::getMessage, METH_NOARGS, "Get device received message." },
    {"getType", (PyCFunction)VRPyDevice::getType, METH_NOARGS, "Get device type." },
    {"setDnD", (PyCFunction)VRPyDevice::setDnD, METH_VARARGS, "Set drag && drop." },
    {"intersect", (PyCFunction)VRPyDevice::intersect, METH_VARARGS, "Intersects the scene - bool intersect( | obj scene, bool force )\n  Returns True if intersected something, else False.\n\t if force set True, ignores cached intersections" },
    {"getIntersected", (PyCFunction)VRPyDevice::getIntersected, METH_NOARGS, "Get device intersected object." },
    {"getIntersection", (PyCFunction)VRPyDevice::getIntersection, METH_NOARGS, "Get device intersection point." },
    {"getIntersectionNormal", (PyCFunction)VRPyDevice::getIntersectionNormal, METH_NOARGS, "Get normal at intersection point." },
    {"getIntersectionUV", (PyCFunction)VRPyDevice::getIntersectionUV, METH_NOARGS, "Get uv at intersection point." },
    {"getIntersectionTriangle", (PyCFunction)VRPyDevice::getIntersectionTriangle, METH_NOARGS, "Get triangle at intersection point - i,j,k dev.getIntersectionTriangle()" },
    {"addIntersection", (PyCFunction)VRPyDevice::addIntersection, METH_VARARGS, "Add device intersection node." },
    {"remIntersection", (PyCFunction)VRPyDevice::remIntersection, METH_VARARGS, "Remove device intersection node." },
    {"getDragged", (PyCFunction)VRPyDevice::getDragged, METH_NOARGS, "Get dragged object." },
    {"getDragGhost", (PyCFunction)VRPyDevice::getDragGhost, METH_NOARGS, "Get drag ghost." },
    {"drag", (PyCFunction)VRPyDevice::drag, METH_VARARGS, "Start to drag an object - drag(obj)" },
    {"drop", (PyCFunction)VRPyDevice::drop, METH_NOARGS, "Drop any object - drop()" },
    {"setSpeed", (PyCFunction)VRPyDevice::setSpeed, METH_VARARGS, "Set the navigation speed of the device - setSpeed(float sx, float sy)" },
    {"getSpeed", (PyCFunction)VRPyDevice::getSpeed, METH_NOARGS, "Get the navigation speed of the device - float getSpeed()" },
    {"addSignal", (PyCFunction)VRPyDevice::addSignal, METH_VARARGS, "Add a new signal - addSignal(int key, int state)" },
    {"trigger", (PyCFunction)VRPyDevice::trigger, METH_VARARGS, "Trigger signal - trigger(int key, int state)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyDevice::fromSharedPtr(VRDevicePtr dev) {
    string type = dev->getType();
    if (type == "mouse") return VRPyMouse::fromSharedPtr( static_pointer_cast<VRMouse>(dev) );
    else if (type == "multitouch") return VRPyMultiTouch::fromSharedPtr( static_pointer_cast<VRMultiTouch>(dev) );
    else if (type == "server") return VRPyServer::fromSharedPtr( static_pointer_cast<VRServer>(dev) );
    else if (type == "haptic") return VRPyHaptic::fromSharedPtr( static_pointer_cast<VRHaptic>(dev) );
    else if (type == "keyboard") return VRPyBaseT<VRDevice>::fromSharedPtr( dev );
    cout << "\nERROR in VRPyTypeCaster::cast device: " << type << " not handled!\n";
    return VRPyBaseT<VRDevice>::fromSharedPtr(dev);
}

PyObject* VRPyDevice::addSignal(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setSpeed, Object is invalid"); return NULL; }
    int k, s;
    if (! PyArg_ParseTuple(args, "ii", &k, &s)) return NULL;
    self->objPtr->addSignal( k,s );
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::trigger(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setSpeed, Object is invalid"); return NULL; }
    int k, s;
    if (! PyArg_ParseTuple(args, "ii", &k, &s)) return NULL;
    self->objPtr->change_button( k,s );
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::setSpeed(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setSpeed, Object is invalid"); return NULL; }
    self->objPtr->setSpeed( parseVec2f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getSpeed(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getSpeed, Object is invalid"); return NULL; }
    return toPyTuple( self->objPtr->getSpeed() );
}

PyObject* VRPyDevice::intersect(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::intersect, Object is invalid"); return NULL; }
    VRPyObject* o = 0;
    int force = 0;
    if (! PyArg_ParseTuple(args, "|Oi", &o, &force)) return NULL;
    OSG::VRIntersection ins = self->objPtr->intersect(o ? o->objPtr : 0, force);
    if (ins.hit) Py_RETURN_TRUE;
    else Py_RETURN_FALSE;
}

PyObject* VRPyDevice::drag(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::drag, Object is invalid"); return NULL; }
    OSG::VRObjectPtr obj = 0;
    if (!VRPyObject::parse(args, &obj)) return NULL;
    string name = obj->getName();
    self->objPtr->drag(obj, self->objPtr->getBeacon());
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::drop(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::drop, Object is invalid"); return NULL; }
    self->objPtr->drop();
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getName(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getName, Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getName().c_str());
}

PyObject* VRPyDevice::destroy(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::destroy, Object is invalid"); return NULL; }
    self->objPtr = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getBeacon(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getBeacon, Object is invalid"); return NULL; }
    return VRPyTransform::fromSharedPtr(self->objPtr->getBeacon());
}

PyObject* VRPyDevice::setBeacon(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setBeacon, Object is invalid"); return NULL; }
    VRPyTransform* beacon = NULL;
    if (! PyArg_ParseTuple(args, "O", &beacon)) return NULL;
    self->objPtr->setBeacon(beacon->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getTarget(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getTarget, Object is invalid"); return NULL; }
    return VRPyTransform::fromSharedPtr(self->objPtr->getTarget());
}

PyObject* VRPyDevice::setTarget(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setTarget, Object is invalid"); return NULL; }
    VRPyTransform* target = NULL;
    if (! PyArg_ParseTuple(args, "O", &target)) return NULL;
    self->objPtr->setTarget(target->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getState(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getState, Object is invalid"); return NULL; }
    return PyInt_FromLong(self->objPtr->getState());
}

PyObject* VRPyDevice::getKeyState(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getKeyState, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return PyInt_FromLong(self->objPtr->b_state(i));
}

PyObject* VRPyDevice::getKey(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getKey, Object is invalid"); return NULL; }
    return PyInt_FromLong(self->objPtr->key());
}

PyObject* VRPyDevice::getMessage(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getKey, Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getMessage().c_str());
}

PyObject* VRPyDevice::getSlider(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getSlider, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return PyFloat_FromDouble(self->objPtr->s_state(i));
}

PyObject* VRPyDevice::getType(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getType, Object is invalid"); return NULL; }
    return PyString_FromString(self->objPtr->getType().c_str());
}

PyObject* VRPyDevice::setDnD(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::setDnD, Object is invalid"); return NULL; }
    int i=0;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    self->objPtr->toggleDragnDrop(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getIntersected(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getIntersected, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->objPtr->getLastIntersection().object.lock());
}

PyObject* VRPyDevice::getIntersection(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getIntersection, Object is invalid"); return NULL; }
    OSG::Pnt3d v = self->objPtr->getLastIntersection().point;
    return toPyTuple( OSG::Vec3d(v) );
}

PyObject* VRPyDevice::getIntersectionTriangle(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getIntersectionTriangle, Object is invalid"); return NULL; }
    OSG::Vec3i v = self->objPtr->getLastIntersection().triangleVertices;
    return toPyTuple(v);
}

PyObject* VRPyDevice::getIntersectionNormal(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getIntersectionNormal, Object is invalid"); return NULL; }
    OSG::Vec3d v = self->objPtr->getLastIntersection().normal;
    return toPyTuple(v);
}

PyObject* VRPyDevice::getIntersectionUV(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getIntersectionUV, Object is invalid"); return NULL; }
    OSG::Vec2d v = self->objPtr->getLastIntersection().texel;
    return toPyTuple(v);
}

PyObject* VRPyDevice::addIntersection(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::addIntersection, Object is invalid"); return NULL; }
    VRPyObject* iobj = NULL;
    if (! PyArg_ParseTuple(args, "O", &iobj)) return NULL;
    self->objPtr->addDynTree(iobj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::remIntersection(VRPyDevice* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::remIntersection, Object is invalid"); return NULL; }
    VRPyObject* iobj = NULL;
    if (! PyArg_ParseTuple(args, "O", &iobj)) return NULL;
    self->objPtr->remDynTree(iobj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyDevice::getDragGhost(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getDragGhost, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->objPtr->getDraggedGhost());
}

PyObject* VRPyDevice::getDragged(VRPyDevice* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyDevice::getDragged, Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast(self->objPtr->getDraggedObject());
}

