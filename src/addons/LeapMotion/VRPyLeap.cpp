#include "VRPyLeap.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Leap, New_ptr);
simpleVRPyType(LeapFrame, 0);

PyMethodDef VRPyLeapFrame::methods[] = {
    {NULL} /* Sentinel */
};

PyMethodDef VRPyLeap::methods[] = {
    {"registerFrameCallback", (PyCFunction)VRPyLeap::registerFrameCallback, METH_VARARGS, "Add description" },
    {"clearFrameCallbacks",   PyWrap(Leap, clearFrameCallbacks, "Add description", void ) },
    {"getPose",               PyWrap(Leap, getPose,             "Get the leap's current transformation", PosePtr) },
    {"setPose",               PyWrap(Leap, setPose,             "Set the transformation of the leap", void, PosePtr ) },
    {"calibrate",             PyWrap(Leap, calibrateManual,     "Set calibration parameters for the leap manually using \n"
                                                                "Vec3d pos: center position of the screen relative to the leap \n"
                                                                "Vec3d dir: direction from pos to leap \n"
                                                                "Vec3d up: relative up direction \n", void, Vec3d, Vec3d, Vec3d) },
    {"open",                  PyWrap(Leap, setAddress,          "Add description", void, string ) },
    {"getDragThreshold",      PyWrap(Leap, getDragThreshold,    "Get drag threshold value of grab interaction", float ) },
    {"getDropThreshold",      PyWrap(Leap, getDropThreshold,    "Get drop threshold value of grab interaction", float ) },
    {"setDragThreshold",      PyWrap(Leap, setDragThreshold,    "Set drag threshold of grab interaction, float [0,1]", void, float ) },
    {"setDropThreshold",      PyWrap(Leap, setDropThreshold,    "Set drop threshold of grab interaction, float [0,1]", void, float ) },
    {"getPinchStrength",      PyWrap(Leap, getPinchStrength,    "Get PinchStrength value of grab interaction", float, int ) },
    {"getGrabStrength",       PyWrap(Leap, getGrabStrength,     "Get GrabStrength value of grab interaction", float, int ) },
    {"getConfidence",         PyWrap(Leap, getConfidence,       "Get Confidence value of grab interaction", float, int ) },
    {"getIsPinching",         PyWrap(Leap, getIsPinching,       "Get isPinching boolean value of grab interaction", bool, int ) },
    {"addHandsGeometry",      PyWrap(Leap, addHandsGeometry,    "Add Leap Hands Geometry", vector<VRObjectPtr> ) },
    {"enableDnD",             PyWrap(Leap, enableDnD,           "Enable leap DnD", void, VRObjectPtr ) },
    {"getHandPose",           PyWrap(Leap, getHandPose,         "Get pose of the leap hands", vector<PosePtr> ) },
    {NULL} /* Sentinel */
};

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
