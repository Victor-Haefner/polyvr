#include "VRPyLeap.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Leap, New_ptr);
simpleVRPyType(LeapFrame, 0);

PyMethodDef VRPyLeapFrame::methods[] = {
    {NULL} /* Sentinel */
};

PyMethodDef VRPyLeap::methods[] = {
    {"registerFrameCallback", (PyCFunction) VRPyLeap::registerFrameCallback, METH_VARARGS, "Add description"},
    {"clearFrameCallbacks",   (PyCFunction) VRPyLeap::clearFrameCallbacks,   METH_VARARGS, "Add description"},
    {"setPose",               (PyCFunction) VRPyLeap::setPose,               METH_VARARGS, "Add description"},
    {"open",                  PyWrapOpt(Leap, open, "Connect to device", "localhost|6437", bool, string, int) },
    {NULL} /* Sentinel */
};

/*PyObject* VRPyLeap::open(VRPyLeap* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* host = 0;
    int port = 0;
    if (!PyArg_ParseTuple(args, "|si:open", (char*) &host, &port)) return NULL; // TODO
    if (host) {
        if (port) self->objPtr->open(host, port);
        else self->objPtr->open(host);
    } else self->objPtr->open();

    Py_RETURN_TRUE;
}*/

PyObject* VRPyLeap::setPose(VRPyLeap* self, PyObject* args) {
    if (!self->valid()) return NULL;

    PyObject *fl, *dl, *ul;
    if (! PyArg_ParseTuple(args, "OOO", &fl, &dl, &ul)) return NULL;
    self->objPtr->setPose( parseVec3dList(fl), parseVec3dList(dl), parseVec3dList(ul));
    Py_RETURN_TRUE;
}

PyObject* VRPyLeap::clearFrameCallbacks(VRPyLeap* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clearFrameCallbacks();
    Py_RETURN_TRUE;
}

PyObject* VRPyLeap::registerFrameCallback(VRPyLeap* self, PyObject* args) {
    if (!self->valid()) return NULL;

    PyObject* my_callback = NULL;
    PyObject* result = NULL;

    if (PyArg_ParseTuple(args, "O:registerFrameCallback", &my_callback)) {
        if (!PyCallable_Check(my_callback)) {
            PyErr_SetString(err, "VRPyLeap::registerFrameCallback - parameter must be callable");
            return NULL;
        }

        Py_XINCREF(my_callback);         /* Add a reference to new callback */
        // TODO: Py_XDECREF for callbacks (leaking) ?
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;

        function<void(VRLeapFramePtr)> cb = [my_callback](VRLeapFramePtr frame) {

            if (!my_callback) { return; }

            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure(); // because we come from a non python thread

            PyObject* arglist;
            PyObject* r;
            PyObject* pyFrame = VRPyBaseT<OSG::VRLeapFrame>::fromSharedPtr(frame);
            arglist = Py_BuildValue("(O)", pyFrame);
            r = PyObject_CallObject(my_callback, arglist);
            Py_DECREF(arglist);
            Py_DECREF(pyFrame);
            if (r != NULL) Py_DECREF(r);
            PyErr_SetString(err, "VRPyLeap - Error while running callback");

            PyGILState_Release(gstate);
        };

        self->objPtr->registerFrameCallback(cb);
    }
    return result;
}
