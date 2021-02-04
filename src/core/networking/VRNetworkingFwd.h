#ifndef VRNETWORKINGFWD_H_INCLUDED
#define VRNETWORKINGFWD_H_INCLUDED

#include "core/utils/VRFwdDeclTemplate.h"

namespace OSG {
    ptrFwd(VROPCUA);
    ptrFwd(VROPCUANode);
    ptrFwd(VRSocket);
    ptrFwd(VRWebSocket);
    ptrFwd(VRSerial);
    ptrFwd(VRHDLC);
    ptrFwd(VRTCPServer);
    ptrFwd(VRTCPClient);
    ptrFwd(VRRestServer);
    ptrFwd(VRRestClient);
    ptrFwd(VRRestResponse);

    ptrFctFwd( VRRest, VRRestResponsePtr );
}

#endif // VRNETWORKINGFWD_H_INCLUDED
