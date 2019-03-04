#ifndef VRPyColorChooser_H_INCLUDED
#define VRPyColorChooser_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRColorChooser.h"

struct VRPyColorChooser : VRPyBaseT<OSG::VRColorChooser> {
    static PyMethodDef methods[];
};

#endif // VRPyColorChooser_H_INCLUDED
