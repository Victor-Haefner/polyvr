#include "VROPCUA.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

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

template<> string typeName(const VROPCUA& t) { return "OPCUA"; }
template<> string typeName(const VROPCUANode& t) { return "OPCUANode"; }

class SubClient : public SubscriptionHandler {
    public:
        map<uint8_t, Variant> values;

        SubClient() {}
        ~SubClient() {}

        static shared_ptr<SubClient> create() { return shared_ptr<SubClient>( new SubClient() ); }

        void DataChange(uint32_t handle, const OpcUa::Node& node, const Variant& val, AttributeId attr) override {
            //cout << "Received DataChange event handle: " << handle << ", val: " << ::toString(val) << endl;
            values[handle] = val;
        }
};

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

string toString(OpcUa::Variant const& v) {
    //if (v.IsNul()) return "None";
    string res = "None";
    uint8_t type = uint8_t(v.Type());

    if (v.IsArray()) {
        try {
            if (type == 0) return res;
            if (type == 1) res = toString(v.As< vector<bool> >());
            if (type == 2) res = toString(v.As< vector<signed char> >());
            if (type == 3) res = toString(v.As< vector<unsigned char> >());
            if (type == 4) res = toString(v.As< vector<int16_t> >());
            if (type == 5) res = toString(v.As< vector<uint16_t> >());
            if (type == 6) res = toString(v.As< vector<int32_t> >());
            if (type == 7) res = toString(v.As< vector<uint32_t> >());
            if (type == 8) res = toString(v.As< vector<int64_t> >());
            if (type == 9) res = toString(v.As< vector<uint64_t> >());
            if (type == 10) res = toString(v.As< vector<float> >());
            if (type == 11) res = toString(v.As< vector<double> >());
            if (type == 12) res = toString(v.As< vector<string> >());
        } catch(...) { cout << "OPCUA Error: toString of vector Variant failed, type: " << type << endl; }
    }

    if (v.IsScalar()) {
        try {
            if (type == 0) return res;
            if (type == 1) res = toString(v.As< bool >());
            if (type == 2) res = toString(v.As< signed char >());
            if (type == 3) res = toString(v.As< unsigned char >());
            if (type == 4) res = toString(v.As< int16_t >());
            if (type == 5) res = toString(v.As< uint16_t >());
            if (type == 6) res = toString(v.As< int32_t >());
            if (type == 7) res = toString(v.As< uint32_t >());
            if (type == 8) res = toString(v.As< int64_t >());
            if (type == 9) res = toString(v.As< uint64_t >());
            if (type == 10) res = toString(v.As< float >());
            if (type == 11) res = toString(v.As< double >());
            if (type == 12) res = toString(v.As< string >());

            if (type == 17) res = toString(v.As< NodeId >().GetIntegerIdentifier());
            if (type == 20) res = toString(v.As< QualifiedName >().Name);
            if (type == 21) res = toString(v.As< LocalizedText >().Text);
            if (type == 24) res = toString(v.As< Variant >());
        } catch(...) { cout << "OPCUA Error: toString of scalar Variant failed, type: " << type << endl; }

        /*try { res = v.As<string>(); } catch(...) {}
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
        //try { res = toString(v.As<DiagnosticInfo>()); } catch(...) {}*/
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

VROPCUANode::VROPCUANode(shared_ptr<OpcUa::Node> n, shared_ptr<SubClient> sclient, shared_ptr<OpcUa::Subscription> subs) : node(n), subscriptionClient(sclient), subscription(subs) {
    const Variant& V = node->GetValue();
    try {
        nodeType = uint8_t(V.Type());
        isScalar = V.IsScalar();
        isArray = V.IsArray();
        isValid = true;
    } catch(exception e) {
        cout << "Warning, VROPCUANode failed with exception: " << e.what() << endl;
    }
}

VROPCUANode::~VROPCUANode() {}

VROPCUANodePtr VROPCUANode::create(OpcUa::Node& node, shared_ptr<SubClient> sclient, shared_ptr<OpcUa::Subscription> subs) { return VROPCUANodePtr( new VROPCUANode( shared_ptr<OpcUa::Node>(new OpcUa::Node(node)), sclient, subs ) ); }

string VROPCUANode::ID() {
    auto nID = node->GetId();
    if (nID.IsInteger()) return toString(nID.GetIntegerIdentifier());
    if (nID.IsString()) return toString(nID.GetStringIdentifier());
    if (nID.IsBinary()) return "binary ID unsupported"; //toString(nID.GetBinaryIdentifier());
    if (nID.IsGuid()) return "guid ID unsupported"; //toString(nID.GetGuidIdentifier());
    return "unknown";
}

string VROPCUANode::name() { return node->GetBrowseName().Name; }
bool VROPCUANode::valid() { return isValid; }

string VROPCUANode::type() {
    VariantType type = node->GetValue().Type();
    return typeToString(uint8_t(type));
}

string VROPCUANode::value() {
    if (subscriptionClient && subscriptionClient->values.count(subHandle)) {
        return ::toString(subscriptionClient->values[subHandle]);
    }
    Variant v = node->GetValue();
    return ::toString(v);
}

vector<VROPCUANodePtr> VROPCUANode::getChildren() {
    vector<VROPCUANodePtr> res;
    for (OpcUa::Node child : node->GetChildren()) {
        auto n = VROPCUANode::create(child, subscriptionClient, subscription);
        if (n->valid()) res.push_back(n);
    }
    return res;
}

VROPCUANodePtr VROPCUANode::getChild(int i) { return getChildren()[i]; }

VROPCUANodePtr VROPCUANode::getChildByName(string name) {
    VROPCUANodePtr res = 0;
    try {
        auto n = node->GetChild(name);
        res = VROPCUANode::create(n, subscriptionClient, subscription);
    } catch(...) {
        cout << "WARNING, node " << VROPCUANode::name() << " " << node->ToString() << " has no child named " << name << endl;
        for (auto c : getChildren()) cout << " child: " << c->node->ToString() << endl;
        return 0;
    }
    return res->valid() ? res : 0;
}

VROPCUANodePtr VROPCUANode::getChildAtPath(string path) {
    VROPCUANodePtr n = shared_from_this();
    for (string name : splitString(path, '.') ) {
        n = n->getChildByName(name);
        //cout << "VROPCUANode::getChildAtPath " << name << " " << n->name() << endl;
        if (!n) return 0;
    }
    return n;
}

void VROPCUANode::setVector(vector<string> values) {
    if (isScalar) {
        cout << "VROPCUANode::setVector ERROR: scalar not supported, use VROPCUANode::set!\n";
    }

    if (isArray) {
        try {
            auto type = nodeType;
            if (type == 0) return;
            else if (type == 1) { vector<bool> v; bool f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 2) { vector<signed char> v; signed char f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 3) { vector<unsigned char> v; unsigned char f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 4) { vector<int16_t> v; int16_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 5) { vector<uint16_t> v; uint16_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 6) { vector<int32_t> v; int32_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 7) { vector<uint32_t> v; uint32_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 8) { vector<int64_t> v; int64_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 9) { vector<uint64_t> v; uint64_t f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 10) { vector<float> v; float f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 11) { vector<double> v; double f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type == 12) { vector<string> v; string f; for (auto s : values) { toValue(s,f); v.push_back(f); } node->SetValue( Variant(v) ); }
            else if (type > 12) cout << "VROPCUANode::setVector ERROR: type " << typeToString(type) << " not supported!\n";
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
        } catch(const exception& ex) {
            cout << "VROPCUANode::setVector ERROR: " << ex.what() << ", var type: " << typeToString(nodeType) << " self name: " << name() << ", vector length: " << values.size() << endl;
        } catch(...) {
            cout << "VROPCUANode::setVector ERROR: var type: " << typeToString(nodeType) << ", self name: " << name() << ", vector length: " << values.size() << endl;
        }
    }
}

void VROPCUANode::set(string value) {
    //cout << "VROPCUANode::set " << int(nodeType) << " " << isArray << " " << isScalar << endl;
    if (isArray) {
        cout << "VROPCUANode::set ERROR: array not supported, use VROPCUANode::setVector!\n";
        return;
    }

    if (isScalar) {
        try {
            auto type = nodeType;
            if (type == 0) return;
            else if (type == 1) { bool v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 2) { signed char v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 3) { unsigned char v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 4) { int16_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 5) { uint16_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 6) { int32_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 7) { uint32_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 8) { int64_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 9) { uint64_t v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 10) { float v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 11) { double v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type == 12) { string v; toValue(value,v); node->SetValue( Variant(v) ); }
            else if (type > 12) cout << "VROPCUANode::set ERROR: type " << typeToString(type) << " not supported!\n";
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
        } catch(const exception& ex) {
            cout << "VROPCUANode::set ERROR: " << ex.what() << ", var type: " << typeToString(nodeType) << ", self name: " << name() << ", value to set: " << value << endl;
        } catch(...) {
            cout << "VROPCUANode::set ERROR: var type: " << typeToString(nodeType) << ", self name: " << name() << ", value to set: " << value << endl;
        }

        if (subscriptionClient && subscriptionClient->values.count(subHandle)) {
            subscriptionClient->values[subHandle] = node->GetValue();
        }
    }
}

void VROPCUANode::subscribe() {
    try {
        subHandle = subscription->SubscribeDataChange(*node);
        cout << "VROPCUANode::subscribe to " << name() << endl;
    } catch(const exception& e) {
        cout << "VROPCUANode::subscribe failed with exception: " << e.what() << endl;
    } catch(...) {
        cout << "VROPCUANode::subscribe failed with unknown exception" << endl;
    }
}


VROPCUA::VROPCUA() {}
VROPCUA::~VROPCUA() {}

VROPCUAPtr VROPCUA::create() { return VROPCUAPtr( new VROPCUA() ); }

/*class VRUaClient : public OpcUa::UaClient { // override timeout
    public:
        VRUaClient() : OpcUa::UaClient(0) {
            DefaultTimeout = 1;
        }
};*/

VROPCUANodePtr VROPCUA::connect(string address) {
    string endpoint = address;
    cout << "OPCUA: connect to " << endpoint << endl;
    if (client) client->Disconnect();
    client = shared_ptr<OpcUa::UaClient>( new OpcUa::UaClient() );
    //client = shared_ptr<VRUaClient>( new VRUaClient() );

    try { cout << " try connection" << endl; client->Connect(endpoint); }
    catch(...) { cout << " connection failed!" << endl; return 0; }

    cout << " connected sucessfully!" << endl;
    subscriptionClient = SubClient::create();
    subscription = client->CreateSubscription(100, *subscriptionClient);

    OpcUa::Node objects = client->GetObjectsNode();
    return VROPCUANode::create( objects, subscriptionClient, subscription );
}




/** ------------- test server ------------- **/

vector<OpcUa::Variant> MyMethod(NodeId context, vector<OpcUa::Variant> arguments) {
    cout << "MyMethod called! " << endl;
    vector<OpcUa::Variant> result;
    result.push_back(Variant(static_cast<uint8_t>(0)));
    return result;
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

    /*SubClient clt;
    auto sub = server.CreateSubscription(100, clt);
    sub->SubscribeDataChange(myvar);*/


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
    server = VRFunction< VRThreadWeakPtr >::create( "OPCUA server", bind(startTestServerT) );
    VRScene::getCurrent()->initThread(server, "OPCUA server", true, false);
}
