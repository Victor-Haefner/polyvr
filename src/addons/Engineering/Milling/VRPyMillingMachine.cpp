#include "VRPyMillingMachine.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTransform.h"

using namespace OSG;

simpleVRPyType(MillingMachine, New_ptr);

PyMethodDef VRPyMillingMachine::methods[] = {
    {"connect", (PyCFunction)VRPyMillingMachine::connect, METH_VARARGS, "Connect to machine - connect( 'ip:port' )" },
    {"disconnect", (PyCFunction)VRPyMillingMachine::disconnect, METH_NOARGS, "Disconnect from machine" },
    {"connected", (PyCFunction)VRPyMillingMachine::connected, METH_NOARGS, "Return if machine connected - bool connected()" },
    {"setSpeed", (PyCFunction)VRPyMillingMachine::setSpeed, METH_VARARGS, "Set milling speed and direction - setSpeed( [x,y,z] )" },
    {"setGeometry", (PyCFunction)VRPyMillingMachine::setGeometry, METH_VARARGS, "Set the geometries - setGeometry( [objX, objY, objZ, endEffector] )" },
    {"update", (PyCFunction)VRPyMillingMachine::update, METH_NOARGS, "Update the machine visualisation - update()" },
    {"setPosition", (PyCFunction)VRPyMillingMachine::setPosition, METH_VARARGS, "Set the position of the machine - setPosition( [x,y,z] )" },
    {"getPosition", (PyCFunction)VRPyMillingMachine::setPosition, METH_NOARGS, "Get the position of the machine - [x,y,z] getPosition()" },
    {"state", (PyCFunction)VRPyMillingMachine::state, METH_NOARGS, "Get the state of the machine" },
    {"mode", (PyCFunction)VRPyMillingMachine::mode, METH_NOARGS, "Get the mode of the machine" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMillingMachine::connected(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    return PyBool_FromLong( self->objPtr->connected() );
}

PyObject* VRPyMillingMachine::getPosition(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->getPosition() );
}

PyObject* VRPyMillingMachine::state(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->getState() );
}

PyObject* VRPyMillingMachine::mode(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    return PyInt_FromLong( self->objPtr->getMode() );
}

PyObject* VRPyMillingMachine::update(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    self->objPtr->update();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setPosition(VRPyMillingMachine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setPosition( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setGeometry(VRPyMillingMachine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    vector<OSG::VRTransformPtr> geos;
    vector<PyObject*> objs = parseList(args);
    for (auto o : objs) {
        OSG::VRTransformPtr g = ((VRPyTransform*)o)->objPtr;
        geos.push_back(g);
    }
    self->objPtr->setGeometry(geos);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::connect(VRPyMillingMachine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->connect( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::disconnect(VRPyMillingMachine* self) {
    if (!self->valid()) return NULL;
    self->objPtr->disconnect();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingMachine::setSpeed(VRPyMillingMachine* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setSpeed( parseVec3f(args) );
    Py_RETURN_TRUE;
}
