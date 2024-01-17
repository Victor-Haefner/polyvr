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
#include "core/networking/mqtt/VRMQTTClient.h"
#include "core/networking/mqtt/VRMQTTServer.h"
#include "core/networking/snap7/VRProfibusClient.h"
#include "core/networking/VRCollaboration.h"
#include "VRPyBase.h"

struct VRPyProfinetClient : public VRPyBaseT<OSG::VRProfinetClient> {
    static PyMethodDef methods[];
};

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

struct VRPyMQTTClient : public VRPyBaseT<OSG::VRMQTTClient> {
    static PyMethodDef methods[];
};

struct VRPyMQTTServer : public VRPyBaseT<OSG::VRMQTTServer> {
    static PyMethodDef methods[];
};

#ifndef WITHOUT_TCP
struct VRPyNetworkClient : public VRPyBaseT<OSG::VRNetworkClient> {
    static PyMethodDef methods[];
};

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

struct VRPyCollaboration : public VRPyBaseT<OSG::VRCollaboration> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPYNETWORKING_H_INCLUDED
