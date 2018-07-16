#ifndef VROPCUA_H_INCLUDED
#define VROPCUA_H_INCLUDED

#include "VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <string.h>
#include <OpenSG/OSGConfig.h>

namespace OpcUa {
    class Node;
    class UaClient;
}

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROPCUANode {
    private:
        OpcUa::Node* node = 0;

    public:
        VROPCUANode(OpcUa::Node* node = 0);
        ~VROPCUANode();

        static VROPCUANodePtr create(OpcUa::Node& node);
};

class VROPCUA {
    private:
        shared_ptr<OpcUa::UaClient> client = 0;
        VRThreadCbPtr server;

    public:
        VROPCUA();
        ~VROPCUA();

        static VROPCUAPtr create();

        void connect(string address);

        void setupTestServer();
};

OSG_END_NAMESPACE;

#endif // VROPCUA_H_INCLUDED
