#include "VROPCUA.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include <boost/bind.hpp>

#include <iostream>
#include <algorithm>
#include <time.h>

#include <thread>
#include <chrono>

#include <opc/ua/node.h>
#include <opc/ua/subscription.h>
#include <opc/ua/server/server.h>
#include <opc/ua/client/client.h>

using namespace OpcUa;

using namespace OSG;

VROPCUANode::VROPCUANode(OpcUa::Node* n) : node(n) {}
VROPCUANode::~VROPCUANode() {}

VROPCUANodePtr VROPCUANode::create(OpcUa::Node& node) { return VROPCUANodePtr( new VROPCUANode(0) ); }
//VROPCUANodePtr VROPCUANode::create(OpcUa::Node& node) { return VROPCUANodePtr( new VROPCUANode(&node) ); }



VROPCUA::VROPCUA() {}
VROPCUA::~VROPCUA() {}

VROPCUAPtr VROPCUA::create() { return VROPCUAPtr( new VROPCUA() ); }

class SubClient : public SubscriptionHandler {
    void DataChange(uint32_t handle, const OpcUa::Node & node, const Variant & val, AttributeId attr) override {
        cout << "Received DataChange event for Node " << node << endl;
    }
};

vector<OpcUa::Variant> MyMethod(NodeId context, vector<OpcUa::Variant> arguments) {
    cout << "MyMethod called! " << endl;
    vector<OpcUa::Variant> result;
    result.push_back(Variant(static_cast<uint8_t>(0)));
    return result;
}

string toString(OpcUa::Variant const& v) {
    string res;
    if (v.IsNul()) res += "None";
    if (v.IsArray()) res += "Array";
    if (v.IsScalar()) {
        res += "Scalar(";
        try { res += toString(v.As<string>()); } catch(...) {}
        try { res += toString(v.As<float>()); } catch(...) {}
        try { res += toString(v.As<double>()); } catch(...) {}
        try { res += toString(v.As<int>()); } catch(...) {}
        res += ")";
    }
    return res;
}

string toString(OpcUa::Node& n) {
    string res;
    res += n.GetBrowseName().Name + " (ID:" + toString( n.GetId().GetIntegerIdentifier() ) + ")";
    res += " type: " + toString(n.GetDataType());
    res += ", value: " + toString(n.GetDataValue().Value);
    return res;
}

void printTree(OpcUa::Node& node, string offset = "") {
    cout << offset << "opcua node: " << toString(node) << endl;
    for (OpcUa::Node child : node.GetChildren()) printTree(child, offset+" ");
}

void VROPCUA::connect(string address) {
    //string endpoint = "opc.tcp://192.168.56.101:48030";
    //string endpoint = "opc.tcp://user:password@192.168.56.101:48030";
    //string endpoint = "opc.tcp://127.0.0.1:4840/freeopcua/server/";
    //string endpoint = "opc.tcp://localhost:53530/OPCUA/SimulationServer/";
    //string endpoint = "opc.tcp://localhost:48010";
    string endpoint = address;
    cout << "OPCUA: connect to " << endpoint << endl;

    if (client) client->Disconnect();
    client = shared_ptr<OpcUa::UaClient>( new OpcUa::UaClient(0) );
    client->Connect(endpoint);

    //get Root node on server
    //OpcUa::Node root = client->GetRootNode();
    OpcUa::Node objects = client->GetObjectsNode();
    //printTree(objects);

    //get a node from standard namespace using objectId
    /*cout << "NamespaceArray is:" << endl;
    OpcUa::Node nsnode = client.GetNode(ObjectId::Server_NamespaceArray);
    OpcUa::Variant ns = nsnode.GetValue();

    for (string d : ns.As<vector<string>>()) { cout << "    {}" << d << endl; }

    OpcUa::Node myvar;
    OpcUa::Node myobject;
    OpcUa::Node mymethod;

    //Initialize Node myvar:

    //Get namespace index we are interested in

    // From freeOpcUa Server:
    uint32_t idx = client.GetNamespaceIndex("http://examples.freeopcua.github.io");
    ////Get Node using path (BrowsePathToNodeId call)
    //vector<string> varpath({ to_string(idx) + ":NewObject", "MyVariable" });
    //myvar = objects.GetChild(varpath);
    vector<string> methodpath({ to_string(idx) + ":NewObject" });
    myobject = objects.GetChild(methodpath);
    methodpath = { to_string(idx) + ":NewObject", "MyMethod" };
    mymethod = objects.GetChild(methodpath);
    vector<OpcUa::Variant> arguments;
    arguments.push_back(static_cast<uint8_t>(0));
    myobject.CallMethod(mymethod.GetId(), arguments);

    // Example data from Prosys server:
    //vector<string> varpath({"Objects", "5:Simulation", "5:Random1"});
    //myvar = root.GetChild(varpath);

    // Example from any UA server, standard dynamic variable node:
    vector<string> varpath{ "Objects", "Server", "ServerStatus", "CurrentTime" };
    myvar = root.GetChild(varpath);

    cout << "got node: {}" << myvar << endl;

    //Subscription
    SubClient sclt;
    auto sub = client.CreateSubscription(100, sclt);
    //uint32_t handle = sub->SubscribeDataChange(myvar);
    //cout << "Got sub handle: {}, sleeping 5 seconds" << handle << endl;
    this_thread::sleep_for(chrono::seconds(5));

    cout << "Disconnecting" << endl;
    client.Disconnect();*/

    //return VROPCUANode::create( objects );
}

void startTestServerT() {
    string endpoint = "opc.tcp://localhost:4840/freeopcua/server";
    cout << "OPCUA: start server at " << endpoint << endl;
    //First setup our server
    OpcUa::UaServer server(0);
    server.SetEndpoint(endpoint);
    server.SetServerURI("urn://exampleserver.freeopcua.github.io");
    server.Start();

    //then register our server namespace and get its index in server
    uint32_t idx = server.RegisterNamespace("http://examples.freeopcua.github.io");

    //Create our address space using different methods
    OpcUa::Node objects = server.GetObjectsNode();

    //Add a custom object with specific nodeid
    NodeId nid(99, idx);
    QualifiedName qn("RootNode", idx);
    OpcUa::Node root = objects.AddObject(nid, qn);

    //Add a variable and a property with auto-generated Nodeid to our custom object
    OpcUa::Node myvar = root.AddVariable(idx, "MyVariable", Variant(8));
    OpcUa::Node myprop = root.AddVariable(idx, "MyProperty", Variant(8.8));
    OpcUa::Node mymethod = root.AddMethod(idx, "MyMethod", MyMethod);

    //browse root node on server side
    //OpcUa::Node root = server.GetRootNode();
    printTree(root);


    //Uncomment following to subscribe to datachange events inside server
    /*
    SubClient clt;
    unique_ptr<Subscription> sub = server.CreateSubscription(100, clt);
    sub->SubscribeDataChange(myvar);
    */

    //Now write values to address space and send events so clients can have some fun
    uint32_t counter = 0;
    myvar.SetValue(Variant(counter)); //will change value and trigger datachange event

    //Create event
    server.EnableEventNotification();
    Event ev(ObjectId::BaseEventType); //you should create your own type
    ev.Severity = 2;
    ev.SourceNode = ObjectId::Server;
    ev.SourceName = "Event from FreeOpcUA";
    ev.Time = DateTime::Current();

    for (;;) {
        myvar.SetValue(Variant(++counter)); //will change value and trigger datachange event
        stringstream ss;
        ss << "This is event number: " << counter;
        ev.Message = LocalizedText(ss.str());
        server.TriggerEvent(ev);
        this_thread::sleep_for(chrono::milliseconds(5000));
    }

    server.Stop();
}

void VROPCUA::setupTestServer() {
    server = VRFunction< VRThreadWeakPtr >::create( "OPCUA server", boost::bind(startTestServerT) );
    VRScene::getCurrent()->initThread(server, "OPCUA server", true, false);
}
