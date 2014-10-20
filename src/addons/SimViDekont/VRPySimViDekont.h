#ifndef VRPYSIMVIDEKONT_H_INCLUDED
#define VRPYSIMVIDEKONT_H_INCLUDED

#include "../../core/scripting/VRPyBase.h"
#include "SimViDekont.h"

struct VRPySimViDekont : VRPyBaseT<OSG::SimViDekont> {
    static OSG::SimViDekont* svd;

    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* load(VRPySimViDekont* self, PyObject* args);
    static PyObject* update(VRPySimViDekont* self);
    static PyObject* play(VRPySimViDekont* self);
    static PyObject* stop(VRPySimViDekont* self);
    static PyObject* rewind(VRPySimViDekont* self);
};

#endif // VRPYSIMVIDEKONT_H_INCLUDED
