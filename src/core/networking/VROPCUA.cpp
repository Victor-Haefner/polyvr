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
#include <opc/ua/protocol/variant.h>
#include <opc/ua/subscription.h>
#include <opc/ua/server/server.h>
#include <opc/ua/client/client.h>

using namespace OpcUa;
using namespace OSG;


string VROPCUANode::toString(uint8_t v) {
    if (v == 0) return "null";
    if (v == 1) return "boolean";
    if (v == 2) return "sbyte";
    if (v == 3) return "byte";
    if (v == 4) return "int16";
    if (v == 5) return "uint16";
    if (v == 6) return "int32";
    if (v == 7) return "uint32";
    if (v == 8) return "int64";
    if (v == 9) return "uint64";
    if (v == 10) return "float";
    if (v == 11) return "double";
    if (v == 12) return "string";
    if (v == 13) return "date_time";
    if (v == 14) return "guid";
    if (v == 15) return "byte_string";
    if (v == 16) return "xml_element";
    if (v == 17) return "node_id";
    if (v == 18) return "expanded_node_id";
    if (v == 19) return "status_code";
    if (v == 20) return "qualified_name";
    if (v == 21) return "localized_text";
    if (v == 22) return "extension_object";
    if (v == 23) return "data_value";
    if (v == 24) return "variant";
    if (v == 25) return "diagnostic_info";
    return "none";
}


vector<OpcUa::Variant> MyMethod(NodeId context, vector<OpcUa::Variant> arguments) {
    cout << "MyMethod called! " << endl;
    vector<OpcUa::Variant> result;
    result.push_back(Variant(static_cast<uint8_t>(0)));
    return result;
}

string toString(OpcUa::Variant const& v) {
    //if (v.IsNul()) return "None";
    if (v.IsArray()) return "Array";

    string res = "None";
    if (v.IsScalar()) {
        try { res = v.As<string>(); } catch(...) {}
        try { res = toString(v.As<float>()); } catch(...) {}
        try { res = toString(v.As<double>()); } catch(...) {}
        try { res = toString(v.As<int>()); } catch(...) {}
        try { res = toString(v.As<bool>()); } catch(...) {}
        try { res = toString(v.As<int8_t>()); } catch(...) {}
        try { res = toString(v.As<uint8_t>()); } catch(...) {}
        try { res = toString(v.As<int16_t>()); } catch(...) {}
        try { res = toString(v.As<uint16_t>()); } catch(...) {}
        try { res = toString(v.As<int32_t>()); } catch(...) {}
        try { res = toString(v.As<uint32_t>()); } catch(...) {}
        try { res = toString(v.As<int64_t>()); } catch(...) {}
        try { res = toString(v.As<uint64_t>()); } catch(...) {}
        //try { res = toString(v.As<DateTime>()); } catch(...) {}
        //try { res = toString(v.As<Guid>()); } catch(...) {}
        //try { res = toString(v.As<ByteString>()); } catch(...) {}
        try { res = toString(v.As<NodeId>().GetIntegerIdentifier()); } catch(...) {}
        //try { res = toString(v.As<StatusCode>()); } catch(...) {}
        try { res = v.As<LocalizedText>().Text; } catch(...) {}
        try { res = v.As<QualifiedName>().Name; } catch(...) {}
        try { res = toString(v.As<Variant>()); } catch(...) {}
        //try { res = toString(v.As<DiagnosticInfo>()); } catch(...) {}
    }

    return res;
}

string toString(OpcUa::Node& n) {
    string res;
    res += n.GetBrowseName().Name + " (ID:" + toString( n.GetId().GetIntegerIdentifier() ) + ")";
    res += " type: " + toString(n.GetDataType());
    res += ", value: " + toString(n.GetValue());
    return res;
}

void printTree(OpcUa::Node& node, string offset = "") {
    cout << offset << "opcua node: " << toString(node) << endl;
    for (OpcUa::Node child : node.GetChildren()) printTree(child, offset+" ");
}




VROPCUANode::VROPCUANode(shared_ptr<OpcUa::Node> n) : node(n) {}
VROPCUANode::~VROPCUANode() {}

VROPCUANodePtr VROPCUANode::create(OpcUa::Node& node) { return VROPCUANodePtr( new VROPCUANode( shared_ptr<OpcUa::Node>(new OpcUa::Node(node)) ) ); }

int VROPCUANode::ID() { return node->GetId().GetIntegerIdentifier(); }
string VROPCUANode::name() { return node->GetBrowseName().Name; }

string VROPCUANode::type() {
    VariantType type = node->GetValue().Type();
    return VROPCUANode::toString(uint8_t(type));
    //Variant v = node->GetDataType();
    //return toString(v);
}

string VROPCUANode::value() {
    Variant v = node->GetValue();
    return ::toString(v);
}

vector<VROPCUANodePtr> VROPCUANode::getChildren() {
    vector<VROPCUANodePtr> res;
    for (OpcUa::Node child : node->GetChildren()) res.push_back( VROPCUANode::create(child) );
    return res;
}




VROPCUA::VROPCUA() {}
VROPCUA::~VROPCUA() {}

VROPCUAPtr VROPCUA::create() { return VROPCUAPtr( new VROPCUA() ); }

class SubClient : public SubscriptionHandler {
    void DataChange(uint32_t handle, const OpcUa::Node & node, const Variant & val, AttributeId attr) override {
        cout << "Received DataChange event for Node " << node << endl;
    }
};

VROPCUANodePtr VROPCUA::connect(string address) {
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
    VROPCUANodePtr res = VROPCUANode::create( objects );
    return res;
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
