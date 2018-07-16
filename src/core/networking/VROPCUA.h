#ifndef VROPCUA_H_INCLUDED
#define VROPCUA_H_INCLUDED

#include "VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <vector>
#include <OpenSG/OSGConfig.h>

namespace OpcUa {
    class Node;
    class UaClient;
}

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROPCUANode {
    private:
        shared_ptr<OpcUa::Node> node = 0;

    public:
        VROPCUANode(shared_ptr<OpcUa::Node> node = 0);
        ~VROPCUANode();

        static VROPCUANodePtr create(OpcUa::Node& node);

        int ID();
        string name();
        string value();
        string type();
        vector<VROPCUANodePtr> getChildren();

        void set(string value);

        static string typeToString(uint8_t v);
};

class VROPCUA {
    private:
        shared_ptr<OpcUa::UaClient> client = 0;
        VRThreadCbPtr server;

    public:
        VROPCUA();
        ~VROPCUA();

        static VROPCUAPtr create();

        VROPCUANodePtr connect(string address);

        void setupTestServer();
};

OSG_END_NAMESPACE;

#endif // VROPCUA_H_INCLUDED
