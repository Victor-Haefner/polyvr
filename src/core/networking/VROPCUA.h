#ifndef VROPCUA_H_INCLUDED
#define VROPCUA_H_INCLUDED

#include "VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>
#include <OpenSG/OSGConfig.h>
#include <boost/thread/recursive_mutex.hpp>

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
        VROPCUAWeakPtr opc;

        string nodeName;
        string nodeID;
        string opcValue;
        uint8_t nodeType = 0;
        bool isValid = false;
        bool isScalar = true;
        bool isArray = false;
        bool isStruct = false;
        bool isSubbed = false;

        void delegateSet(string v);

    public:
        VROPCUANode(shared_ptr<OpcUa::Node> node = 0, VROPCUAPtr opc = 0);
        ~VROPCUANode();

        static VROPCUANodePtr create(OpcUa::Node& node, VROPCUAPtr o);

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

        void set(string value, bool blocking = true);
        void setVector(vector<string> values);
        void setOPCval(string v);

        void subscribe(VROPCUANodeCbPtr cb);
        bool isSubscribed();
        void updateValue(string val);

        string test();

        static string typeToString(uint8_t v);
};

class VROPCUA : public std::enable_shared_from_this<VROPCUA> {
    private:
        shared_ptr<OpcUa::UaClient> client = 0;
        shared_ptr<SubClient> subscriptionClient = 0;
        shared_ptr<OpcUa::Subscription> subscription = 0;

        string endpoint;
        VRUpdateCbPtr watchdogCb;

        boost::recursive_mutex commMtx;
        map<VROPCUANode*, pair<VROPCUANodePtr, string> > commQueue; // deferred variable setters
        VRThreadCbPtr commCallback;
        VRThreadCbPtr server;

        void watchdog();
        void processCommQueue();
        void startCommProcessing();

    public:
        VROPCUA();
        ~VROPCUA();

        static VROPCUAPtr create();
        VROPCUAPtr ptr();

        VROPCUANodePtr connect(string address);

        shared_ptr<SubClient> getSubscriptionClient();
        shared_ptr<OpcUa::Subscription> getSubscription();

        void queueSet(VROPCUANodePtr n, string v);

        void setupTestServer();
};

OSG_END_NAMESPACE;

#endif // VROPCUA_H_INCLUDED
