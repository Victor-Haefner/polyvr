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


string VROPCUANode::typeToString(uint8_t v) {
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
    return typeToString(uint8_t(type));
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

void VROPCUANode::set(string value) {
    uint8_t type = uint8_t(node->GetValue().Type());
    if (type == 0) return;
    if (type == 1) { bool v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 2) { signed char v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 3) { unsigned char v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 4) { int16_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 5) { uint16_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 6) { int32_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 7) { uint32_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 8) { int64_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 9) { uint64_t v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 10) { float v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 11) { double v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type == 12) { string v; toValue(value,v); node->SetValue( Variant(v) ); }
    if (type > 12) cout << "VROPCUANode::set ERROR: type " << typeToString(type) << " not supported!\n";
    //if (type == 13) { date_time v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 14) { guid v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 15) { byte_string v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 16) { xml_element v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 17) { node_id v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 18) { expanded_node_id v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 19) { status_code v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 20) { qualified_name v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 21) { localized_text v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 22) { extension_object v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 23) { data_value v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 24) { variant v; toValue(value,v); node->SetValue( Variant(v) ); }
    //if (type == 25) { diagnostic_info v; toValue(value,v); node->SetValue( Variant(v) ); }
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
    string endpoint = address;
    cout << "OPCUA: connect to " << endpoint << endl;
    if (client) client->Disconnect();
    client = shared_ptr<OpcUa::UaClient>( new OpcUa::UaClient(0) );
    try { client->Connect(endpoint); }
    catch(...) { return 0;}
    OpcUa::Node objects = client->GetObjectsNode();
    return VROPCUANode::create( objects );
}

void startTestServerT() {
    string endpoint = "opc.tcp://localhost:4840/freeopcua/server";
    cout << "OPCUA: start server at " << endpoint << endl;
    OpcUa::UaServer server(0);
    server.SetEndpoint(endpoint);
    server.SetServerURI("urn://exampleserver.freeopcua.github.io");
    server.Start();

    uint32_t idx = server.RegisterNamespace("http://examples.freeopcua.github.io");

    // add data
    NodeId nid(99, idx);
    QualifiedName qn("RootNode", idx);
    OpcUa::Node objects = server.GetObjectsNode();
    OpcUa::Node root = objects.AddObject(nid, qn);
    OpcUa::Node myvar = root.AddVariable(idx, "MyVariable", Variant(7));
    OpcUa::Node myprop = root.AddVariable(idx, "MyProperty", Variant(8.8));
    OpcUa::Node mymethod = root.AddMethod(idx, "MyMethod", MyMethod);
    printTree(root);


    //Uncomment following to subscribe to datachange events inside server

    SubClient clt;
    auto sub = server.CreateSubscription(100, clt);
    sub->SubscribeDataChange(myvar);


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
