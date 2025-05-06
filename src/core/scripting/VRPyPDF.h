#ifndef VRPYPDF_H_INCLUDED
#define VRPYPDF_H_INCLUDED

#include "core/utils/pdf/VRPDF.h"
#include "core/utils/pdf/VRPDFPage.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPDFPage : VRPyBaseT<OSG::VRPDFPage> {
    static PyMethodDef methods[];
};

struct VRPyPDF : VRPyBaseT<OSG::VRPDF> {
    static PyMethodDef methods[];
};

#endif // VRPYPDF_H_INCLUDED
