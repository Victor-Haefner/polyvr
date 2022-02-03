#ifndef VRPYNETWORKING_H_INCLUDED
#define VRPYNETWORKING_H_INCLUDED

#include "core/networking/VRHDLC.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/networking/rest/VRRestServer.h"
#include "core/networking/udp/VRUDPClient.h"
#include "core/networking/udp/VRUDPServer.h"
#include "core/networking/tcp/VRTCPClient.h"
#include "core/networking/tcp/VRTCPServer.h"
#include "core/networking/tcp/VRICEclient.h"
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

#ifndef WITHOUT_TCP
struct VRPyTCPClient : public VRPyBaseT<OSG::VRTCPClient> {
    static PyMethodDef methods[];
};

struct VRPyTCPServer : public VRPyBaseT<OSG::VRTCPServer> {
    static PyMethodDef methods[];
};

struct VRPyICEClient : public VRPyBaseT<OSG::VRICEClient> {
    static PyMethodDef methods[];
};

struct VRPyUDPClient : public VRPyBaseT<OSG::VRUDPClient> {
    static PyMethodDef methods[];
};

struct VRPyUDPServer : public VRPyBaseT<OSG::VRUDPServer> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPYNETWORKING_H_INCLUDED
