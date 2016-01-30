#include "VRPySetup.h"
#include "VRPyBaseT.h"

#include "core/setup/windows/VRView.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyImage.h"

using namespace OSG;

simpleVRPyType(Setup, 0);
simpleVRPyType(View, 0);
simpleVRPyType(Window, 0);

PyMethodDef VRPySetup::methods[] = {
    {"getView", (PyCFunction)VRPySetup::getView, METH_VARARGS, "Get view i - getView(int i)" },
    {"getWindow", (PyCFunction)VRPySetup::getWindow, METH_VARARGS, "Get window i - getWindow(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySetup::getView(VRPySetup* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i;
    if (! PyArg_ParseTuple(args, "i", &i)) return NULL;
    return VRPyView::fromSharedPtr( self->objPtr->getView(i) );
}

PyObject* VRPySetup::getWindow(VRPySetup* self, PyObject* args) {
    if (!self->valid()) return NULL;
    return VRPyWindow::fromSharedPtr( self->objPtr->getWindow( parseString(args) ) );
}

PyMethodDef VRPyView::methods[] = {
    {"toggleStereo", (PyCFunction)VRPyView::toggleStereo, METH_NOARGS, "Toggle stereo - toggleStereo()" },
    {"setPose", (PyCFunction)VRPyView::setPose, METH_VARARGS, "Set the pose - setPose( pose p )" },
    {"getPose", (PyCFunction)VRPyView::getPose, METH_NOARGS, "Get the pose - pose getPose()" },
    {"setSize", (PyCFunction)VRPyView::setSize, METH_VARARGS, "Set the size in meter - setSize( [W,H] )" },
    {"getSize", (PyCFunction)VRPyView::getSize, METH_NOARGS, "Get the size in meter - [W,H] getSize()" },
    {"grab", (PyCFunction)VRPyView::grab, METH_NOARGS, "Get the current visual as texture - tex grab()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyView::grab(VRPyView* self) {
    if (!self->valid()) return NULL;
    return VRPyImage::fromSharedPtr( self->objPtr->grab() );
}

PyObject* VRPyView::setSize(VRPyView* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setProjectionSize( parseVec2f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyView::getSize(VRPyView* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->getProjectionSize() );
}

PyObject* VRPyView::getPose(VRPyView* self) {
    if (!self->valid()) return NULL;
    auto v = self->objPtr;
    return VRPyPose::fromObject( pose(v->getProjectionCenter(), v->getProjectionNormal(), v->getProjectionUp()) );
}

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

PyMethodDef VRPyWindow::methods[] = {
    {"getSize", (PyCFunction)VRPyWindow::getSize, METH_NOARGS, "Get the size in pixel - [W,H] getSize()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyWindow::getSize(VRPyWindow* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->getSize() );
}


