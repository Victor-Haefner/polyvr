#include "VRPySetup.h"
#include "VRPyBaseT.h"

#include "core/setup/windows/VRView.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyCamera.h"
#include "core/scripting/VRPyMath.h"

using namespace OSG;

simpleVRPyType(Setup, 0);
simpleVRPyType(View, 0);
simpleVRPyType(Window, 0);

PyMethodDef VRPySetup::methods[] = {
    {"getView", (PyCFunction)VRPySetup::getView, METH_VARARGS, "Get view i/name - getView(int i / str name)" },
    {"getWindow", (PyCFunction)VRPySetup::getWindow, METH_VARARGS, "Get window i - getWindow(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySetup::getView(VRPySetup* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* o;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    if (PyInt_Check(o)) return VRPyView::fromSharedPtr( self->objPtr->getView( PyInt_AsLong(o) ) );
    else return VRPyView::fromSharedPtr( self->objPtr->getView( PyString_AsString(o) ) );
}

PyObject* VRPySetup::getWindow(VRPySetup* self, PyObject* args) {
    if (!self->valid()) return NULL;
    return VRPyWindow::fromSharedPtr( self->objPtr->getWindow( parseString(args) ) );
}

PyMethodDef VRPyView::methods[] = {
    {"toggleStereo", (PyCFunction)VRPyView::toggleStereo, METH_NOARGS, "Toggle stereo mode" },
    {"toggleStats", PyWrap(View, toggleStats, "Toggle displayed stats", void) },
    {"setPose", (PyCFunction)VRPyView::setPose, METH_VARARGS, "Set the pose - setPose( pose p )" },
    {"getPose", (PyCFunction)VRPyView::getPose, METH_NOARGS, "Get the pose - pose getPose()" },
    {"setSize", (PyCFunction)VRPyView::setSize, METH_VARARGS, "Set the size in meter - setSize( [W,H] )" },
    {"getSize", (PyCFunction)VRPyView::getSize, METH_NOARGS, "Get the size in meter - [W,H] getSize()" },
    {"grab", (PyCFunction)VRPyView::grab, METH_NOARGS, "Get the current visual as texture - tex grab()" },
    {"setCamera", (PyCFunction)VRPyView::setCamera, METH_VARARGS, "Set the camera of the view - setCamera( cam )" },
    {"getName", (PyCFunction)VRPyView::getName, METH_NOARGS, "Get the name of the view - getName()" },
    {"getUser", PyWrap(View, getUser, "Get the user node", VRTransformPtr ) },
    {"testUpdate", PyWrap(View, testUpdate, "Trigger a test update for debug purpose", void ) },
    {"getRoot", PyWrap(View, getRoot, "Get the root node of the view", VRObjectPtr ) },
    {"getRenderingL", PyWrap(View, getRenderingL, "Get the left rendering studio", VRRenderStudioPtr ) },
    {"getRenderingR", PyWrap(View, getRenderingR, "Get the right rendering studio", VRRenderStudioPtr ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyView::getName(VRPyView* self) {
    if (!self->valid()) return NULL;
    return PyString_FromString( self->objPtr->getName().c_str() );
}

PyObject* VRPyView::grab(VRPyView* self) {
    if (!self->valid()) return NULL;
    return VRPyTexture::fromSharedPtr( self->objPtr->grab() );
}

PyObject* VRPyView::setSize(VRPyView* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->setProjectionSize( parseVec2f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyView::getSize(VRPyView* self) {
    if (!self->valid()) return NULL;
    return toPyObject( self->objPtr->getProjectionSize() );
}

PyObject* VRPyView::getPose(VRPyView* self) {
    if (!self->valid()) return NULL;
    auto v = self->objPtr;
    return VRPyPose::fromObject( Pose(v->getProjectionCenter(), v->getProjectionNormal(), v->getProjectionUp()) );
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

PyObject* VRPyView::setCamera(VRPyView* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyCamera* c = 0;
    if (! PyArg_ParseTuple(args, "O", &c)) return NULL;
    self->objPtr->setCamera(c->objPtr);
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


