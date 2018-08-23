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

class VROPCUANode : public std::enable_shared_from_this<VROPCUANode> {
    private:
        shared_ptr<OpcUa::Node> node = 0;

        uint8_t nodeType = 0;
        bool isScalar = true;
        bool isArray = false;

    public:
        VROPCUANode(shared_ptr<OpcUa::Node> node = 0);
        ~VROPCUANode();

        static VROPCUANodePtr create(OpcUa::Node& node);

        string ID();
        string name();
        string value();
        string type();
        vector<VROPCUANodePtr> getChildren();

        VROPCUANodePtr getChild(int i);
        VROPCUANodePtr getChildByName(string name);
        VROPCUANodePtr getChildAtPath(string path); // Names separated by '.'

        void set(string value);
        void setVector(vector<string> values);

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
