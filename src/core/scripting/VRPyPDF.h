#ifndef VRPYPDF_H_INCLUDED
#define VRPYPDF_H_INCLUDED

#include "core/utils/VRPDF.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPDF : VRPyBaseT<OSG::VRPDF> {
    static PyMethodDef methods[];
};

#endif // VRPYPDF_H_INCLUDED
