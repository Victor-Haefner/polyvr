#ifndef VRPYIMAGE_H_INCLUDED
#define VRPYIMAGE_H_INCLUDED

#include "VRPyBase.h"
#include <OpenSG/OSGImage.h>

struct VRPyImage : VRPyBaseT<OSG::Image> {
    static PyMethodDef methods[];
};

#endif // VRPYIMAGE_H_INCLUDED
