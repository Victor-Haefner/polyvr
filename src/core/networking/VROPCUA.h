#ifndef VROPCUA_H_INCLUDED
#define VROPCUA_H_INCLUDED

#include "VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <vector>
#include <OpenSG/OSGConfig.h>

namespace OpcUa {
    class Node;
    class UaClient;
    class Subscription;
}

class SubClient;

ptrFctFwd( VROPCUANode, OSG::VROPCUANodeWeakPtr );

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROPCUANode : public std::enable_shared_from_this<VROPCUANode> {
    private:
        shared_ptr<OpcUa::Node> node = 0;
        shared_ptr<SubClient> subscriptionClient = 0;
        shared_ptr<OpcUa::Subscription> subscription = 0;
        VROPCUANodeCbPtr callback;

        string opcValue;
        uint8_t nodeType = 0;
        bool isValid = false;
        bool isScalar = true;
        bool isArray = false;
        bool isStruct = false;
        bool isSubscribed = false;

    public:
        VROPCUANode(shared_ptr<OpcUa::Node> node = 0, shared_ptr<SubClient> sclient = 0, shared_ptr<OpcUa::Subscription> subs = 0);
        ~VROPCUANode();

        static VROPCUANodePtr create(OpcUa::Node& node, shared_ptr<SubClient> sclient, shared_ptr<OpcUa::Subscription> subs);

        VROPCUANodePtr ptr();

        string ID();
        string name();
        string value();
        string type();
        bool valid();
        vector<VROPCUANodePtr> getChildren();
        shared_ptr<OpcUa::Node> getOpcNode();

        VROPCUANodePtr getChild(int i);
        VROPCUANodePtr getChildByName(string name);
        VROPCUANodePtr getChildAtPath(string path); // Names separated by '.'

        void set(string value);
        void setVector(vector<string> values);

        void subscribe(VROPCUANodeCbPtr cb);
        void updateValue(string val);

        static string typeToString(uint8_t v);
};

class VROPCUA {
    private:
        shared_ptr<OpcUa::UaClient> client = 0;
        shared_ptr<SubClient> subscriptionClient = 0;
        shared_ptr<OpcUa::Subscription> subscription = 0;
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
