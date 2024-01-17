#ifndef VRNETWORKINGFWD_H_INCLUDED
#define VRNETWORKINGFWD_H_INCLUDED

#include "core/utils/VRFwdDeclTemplate.h"

ptrFwd(VRSSHSession);

namespace OSG {
    ptrFwd(VRProfinetClient);
    ptrFwd(VRMQTTClient);
    ptrFwd(VRMQTTServer);
    ptrFwd(VROPCUA);
    ptrFwd(VROPCUANode);
    ptrFwd(VRSocket);
    ptrFwd(VRWebSocket);
    ptrFwd(VRSerial);
    ptrFwd(VRHDLC);
    ptrFwd(VRNetworkClient);
    ptrFwd(VRNetworkServer);
    ptrFwd(VRUDPServer);
    ptrFwd(VRUDPClient);
    ptrFwd(VRTCPServer);
    ptrFwd(VRTCPClient);
    ptrFwd(VRTCPUtils);
    ptrFwd(VRICEClient);
    ptrFwd(VRRestServer);
    ptrFwd(VRRestClient);
    ptrFwd(VRRestResponse);
    ptrFwd(VRCollaboration);

    ptrFctFwd( VRRest, VRRestResponsePtr );
}

#endif // VRNETWORKINGFWD_H_INCLUDED
