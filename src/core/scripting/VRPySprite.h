#ifndef VRPYSPRITE_H_INCLUDED
#define VRPYSPRITE_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/geometry/VRSprite.h"

struct VRPySprite : VRPyBaseT<OSG::VRSprite> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* getText(VRPySprite* self);
    static PyObject* getSize(VRPySprite* self);
    static PyObject* setText(VRPySprite* self, PyObject* args);
    static PyObject* setSize(VRPySprite* self, PyObject* args);
    static PyObject* webOpen(VRPySprite* self, PyObject* args);
    static PyObject* convertToCloth(VRPySprite* self);

};

#endif // VRPYSPRITE_H_INCLUDED
