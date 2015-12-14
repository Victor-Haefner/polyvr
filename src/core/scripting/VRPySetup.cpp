#include "VRPySetup.h"
#include "VRPyBaseT.h"

#include "core/setup/windows/VRView.h"
#include "core/scripting/VRPyPose.h"

using namespace OSG;

simpleVRPyType(Setup, 0);
simpleVRPyType(View, 0);

PyMethodDef VRPySetup::methods[] = {
    {"getView", (PyCFunction)VRPySetup::getView, METH_VARARGS, "Get view i - getView(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySetup::getView(VRPySetup* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return VRPyView::fromSharedPtr( self->objPtr->getView(i) );
}

PyMethodDef VRPyView::methods[] = {
    {"toggleStereo", (PyCFunction)VRPyView::toggleStereo, METH_NOARGS, "Toggle stereo - toggleStereo()" },
    {"setPose", (PyCFunction)VRPyView::setPose, METH_VARARGS, "Set the pose - setPose( pose p)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyView::setPose(VRPyView* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyPose* p = 0;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;
    self->objPtr->setProjectionCenter(p->objPtr->pos());
    self->objPtr->setProjectionNormal(p->objPtr->dir());
    self->objPtr->setProjectionUp(p->objPtr->up());
    Py_RETURN_TRUE;
}

PyObject* VRPyView::toggleStereo(VRPyView* self) {
    if (!self->valid()) return NULL;
    self->objPtr->setStereo(!self->objPtr->isStereo());
    Py_RETURN_TRUE;
}
