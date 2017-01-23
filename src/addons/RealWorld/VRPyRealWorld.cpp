#include "VRPyRealWorld.h"
#include "../../core/scripting/VRPyTransform.h"
#include "../../core/scripting/VRPyBaseT.h"

using namespace OSG;

simplePyType(RealWorld, New_toZero);

PyMethodDef VRPyRealWorld::methods[] = {
    {"init", (PyCFunction)VRPyRealWorld::initWorld, METH_VARARGS, "Init real obj - init(root)" },
    {"update", (PyCFunction)VRPyRealWorld::update, METH_VARARGS, "Update real obj - update([x,y,z])" },
    {"enableModule", (PyCFunction)VRPyRealWorld::enableModule, METH_VARARGS, "Enable a module - enableModule(str, bool threaded, bool physicalized)" },
    {"disableModule", (PyCFunction)VRPyRealWorld::disableModule, METH_VARARGS, "Disable a module - disableModule(str)" },
    {"configure", (PyCFunction)VRPyRealWorld::configure, METH_VARARGS, "Configure a variable - configure( str var, str value )"
            "\n\tpossible variables: [ 'CHUNKS_PATH' ]"},
    {NULL}  /* Sentinel */
};

PyObject* VRPyRealWorld::initWorld(VRPyRealWorld* self, PyObject* args) {
    VRPyObject* child = 0;
    PyObject* origin = 0;
    if (! PyArg_ParseTuple(args, "OO", &child, &origin)) return NULL;
    if (!self->objPtr) self->objPtr = RealWorld::create( child->objPtr, parseVec2fList(origin) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::configure(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    const char* var = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss", &var, &val)) return NULL;
    self->objPtr->configure( var, val );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::update(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->update( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::enableModule(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    const char* name;
    int t, p;
    if (! PyArg_ParseTuple(args, "sii", (char*)&name, &t, &p)) return NULL;
    self->objPtr->enableModule( name, true, t, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::disableModule(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->enableModule( parseString(args), false, false, false );
    Py_RETURN_TRUE;
}
