#ifndef PYVRLOGISTICS_H_INCLUDED
#define PYVRLOGISTICS_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRLogistics.h"

struct VRPyFNode : VRPyBaseT<OSG::FNode> {
    static PyMethodDef methods[];
};

struct VRPyFNetwork : VRPyBaseT<OSG::FNetwork> {
    static PyMethodDef methods[];
};

struct VRPyFPath : VRPyBaseT<OSG::FPath> {
    static PyMethodDef methods[];
};

struct VRPyFTransporter : VRPyBaseT<OSG::FTransporter> {
    static PyMethodDef methods[];
};

struct VRPyFContainer : VRPyBaseT<OSG::FContainer> {
    static PyMethodDef methods[];
};

struct VRPyFObject : VRPyBaseT<OSG::FObject> {
    static PyMethodDef methods[];
};

struct VRPyFProduct : VRPyBaseT<OSG::FProduct> {
    static PyMethodDef methods[];
};

struct VRPyFLogistics : VRPyBaseT<OSG::FLogistics> {
    static PyMethodDef methods[];
};

#endif // PYVRLOGISTICS_H_INCLUDED
