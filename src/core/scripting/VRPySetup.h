#ifndef VRPYSETUP_H_INCLUDED
#define VRPYSETUP_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/windows/VRWindow.h"

struct VRPySetup : VRPyBaseT<OSG::VRSetup> {
    static PyMethodDef methods[];
    static PyObject* getView(VRPySetup* self, PyObject* args);
    static PyObject* getWindow(VRPySetup* self, PyObject* args);
};

struct VRPyView : VRPyBaseT<OSG::VRView> {
    static PyMethodDef methods[];
    static PyObject* toggleStereo(VRPyView* self);
    static PyObject* setPose(VRPyView* self, PyObject* args);
    static PyObject* getPose(VRPyView* self);
    static PyObject* setSize(VRPyView* self, PyObject* args);
    static PyObject* getSize(VRPyView* self);
    static PyObject* grab(VRPyView* self);
    static PyObject* setCamera(VRPyView* self, PyObject* args);
    static PyObject* getName(VRPyView* self);
};

struct VRPyWindow : VRPyBaseT<OSG::VRWindow> {
    static PyObject* getSize(VRPyWindow* self);
    static PyMethodDef methods[];
};

#endif // VRPYSETUP_H_INCLUDED
