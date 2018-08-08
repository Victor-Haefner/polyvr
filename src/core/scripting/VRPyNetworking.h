#ifndef VRPYNETWORKING_H_INCLUDED
#define VRPYNETWORKING_H_INCLUDED

#include "core/networking/VRHDLC.h"
#include "VRPyBase.h"

struct VRPyHDLC : public VRPyBaseT<OSG::VRHDLC> {
    static PyMethodDef methods[];
};

#endif // VRPYNETWORKING_H_INCLUDED
