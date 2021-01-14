#ifndef VRPYNETWORKING_H_INCLUDED
#define VRPYNETWORKING_H_INCLUDED

#include "core/networking/VRHDLC.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/networking/rest/VRRestServer.h"
#include "VRPyBase.h"

struct VRPyHDLC : public VRPyBaseT<OSG::VRHDLC> {
    static PyMethodDef methods[];
};

struct VRPyRestResponse : public VRPyBaseT<OSG::VRRestResponse> {
    static PyMethodDef methods[];
};

struct VRPyRestClient : public VRPyBaseT<OSG::VRRestClient> {
    static PyMethodDef methods[];
};

struct VRPyRestServer : public VRPyBaseT<OSG::VRRestServer> {
    static PyMethodDef methods[];
};

#endif // VRPYNETWORKING_H_INCLUDED
