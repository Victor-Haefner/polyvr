#include "VRPyRealWorld.h"
#include "../../core/scripting/VRPyTransform.h"
#include "../../core/scripting/VRPyBaseT.h"

using namespace OSG;

simplePyType(RealWorld, New_named_ptr);

PyMethodDef VRPyRealWorld::methods[] = {
    {"init", (PyCFunction)VRPyRealWorld::initWorld, METH_VARARGS, "Init world - init( size [X,Y])" },
    {"update", (PyCFunction)VRPyRealWorld::update, METH_VARARGS, "Update world chunks around position - update([x,y,z])" },
    {"enableModule", (PyCFunction)VRPyRealWorld::enableModule, METH_VARARGS, "Enable a module - enableModule(str, bool threaded, bool physicalized)" },
    {"disableModule", (PyCFunction)VRPyRealWorld::disableModule, METH_VARARGS, "Disable a module - disableModule(str)" },
    {"configure", (PyCFunction)VRPyRealWorld::configure, METH_VARARGS, "Configure a variable - configure( str var, str value )"
            "\n\tpossible variables: [ 'CHUNKS_PATH' ]"},
    {NULL}  /* Sentinel */
};

PyObject* VRPyRealWorld::initWorld(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    PyObject* size = 0;
    if (! PyArg_ParseTuple(args, "O", &size)) return NULL;
    self->objPtr->init( parseVec2iList(size) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::configure(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    const char* var = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss", &var, &val)) return NULL;
    self->objPtr->configure( var?var:"", val?val:"" );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::update(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->update( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::enableModule(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    const char* name = 0;
    int t, p;
    if (! PyArg_ParseTuple(args, "sii", (char*)&name, &t, &p)) return NULL;
    self->objPtr->enableModule( name?name:"", true, t, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::disableModule(VRPyRealWorld* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->enableModule( parseString(args), false, false, false );
    Py_RETURN_TRUE;
}
