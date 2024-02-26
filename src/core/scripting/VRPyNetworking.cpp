#include "VRPyNetworking.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

#ifndef WITHOUT_HDLC
simpleVRPyType(HDLC, New_ptr);
#endif
simpleVRPyType(Ping, New_ptr);
simpleVRPyType(RestResponse, 0);
simpleVRPyType(RestClient, New_optNamed_ptr);
simpleVRPyType(RestServer, New_optNamed_ptr);

simpleVRPyType(MQTTClient, New_ptr);
simpleVRPyType(MQTTServer, New_ptr);
simpleVRPyType(ProfinetClient, New_ptr);

#ifndef WITHOUT_TCP
simpleVRPyType(NetworkClient, 0);
simpleVRPyType(UDPClient, New_optNamed_ptr);
simpleVRPyType(UDPServer, New_optNamed_ptr);
simpleVRPyType(TCPClient, New_optNamed_ptr);
simpleVRPyType(TCPServer, New_optNamed_ptr);
simpleVRPyType(ICEClient, New_ptr);
simpleVRPyType(Collaboration, New_VRObjects_ptr);

template<> int toValue(string s, VRICEClient::CHANNEL& t) {
    t = VRICEClient::NONE;
    if (s == "SCENEGRAPH") t = VRICEClient::SCENEGRAPH;
    if (s == "AUDIO") t = VRICEClient::AUDIO;
    return true;
}

template<> bool toValue(PyObject* obj, VRICEClient::CHANNEL& t) {
    string v;
    toValue(obj, v);
    return toValue(v, t);
}

template<> string typeName(const VRICEClient::CHANNEL* t) { return "ICE CHANNEL ENUM"; }

template<> PyObject* VRPyTypeCaster::cast(const VRICEClient::CHANNEL& v) {
    string s;
    if (v == VRICEClient::NONE) s = "NONE";
    if (v == VRICEClient::SCENEGRAPH) s = "SCENEGRAPH";
    if (v == VRICEClient::AUDIO) s = "AUDIO";
    return cast(s);
}
#endif

PyMethodDef VRPyPing::methods[] = {
    {"startOnPort", PyWrap(Ping, startOnPort, "Ping server on port (address, port, timeout)", bool, string, string, int) },
    {"start", PyWrap(Ping, start, "Ping server (address, timeout)", bool, string, int) },
    {"getMAC", PyWrap(Ping, getMAC, "Get MAC by IP, only works in local network (IP, interface)", string, string, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyProfinetClient::methods[] = {
    {"connect", PyWrap(ProfinetClient, connect, "Connect to server, address, rack, slot", void, string, int, int) },
    {"isConnected", PyWrap(ProfinetClient, isConnected, "Return if connected", bool) },
    {"read", PyWrap(ProfinetClient, read, "read binary data, db, offset, length, dbtype = database|merker", string, int, int, int, string) },
    {"write", PyWrap(ProfinetClient, write, "write binary data, db, offset, data, dbtype = database|merker", void, int, int, string, string) },
    {"readBool", PyWrap(ProfinetClient, readBool, "Read a bool, db, pos, bit, dbtype = database|merker", bool, int, int, int, string) },
    {"readShort", PyWrap(ProfinetClient, readShort, "Read a short, db, pos, dbtype = database|merker", short, int, int, string) },
    {"readInt", PyWrap(ProfinetClient, readInt, "Read an int, db, pos, dbtype = database|merker", int, int, int, string) },
    {"readFloat", PyWrap(ProfinetClient, readFloat, "Read a float, db, pos, dbtype = database|merker", float, int, int, string) },
    {"writeBool", PyWrap(ProfinetClient, writeBool, "Write a bool, db, pos, bit, bool, dbtype = database|merker", void, int, int, int, bool, string) },
    {"writeShort", PyWrap(ProfinetClient, writeShort, "Write a short, db, pos, int, dbtype = database|merker", void, int, int, short, string) },
    {"writeInt", PyWrap(ProfinetClient, writeInt, "Write an int, db, pos, int, dbtype = database|merker", void, int, int, int, string) },
    {"writeFloat", PyWrap(ProfinetClient, writeFloat, "Write a float, db, pos, float, dbtype = database|merker", void, int, int, float, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMQTTClient::methods[] = {
    {"disconnect", PyWrap(MQTTClient, disconnect, "Disconnect from server", void) },
    {"connected", PyWrap(MQTTClient, connected, "Returns if connected to a server", bool) },
    {"setAuthentication", PyWrap(MQTTClient, setAuthentication, "Set authentication parameters, (name, password)", void, string, string) },
    {"subscribe", PyWrapOpt(MQTTClient, subscribe, "Subscribe to topic, set retain to get current topic value, (topic, retain)", "0", void, string, bool) },
    {"publish", PyWrap(MQTTClient, publish, "Publish a topic (topic, message)", void, string, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMQTTServer::methods[] = {
    {"listen", PyWrap(MQTTServer, listen, "Listen on port", void, int) },
    {"close", PyWrap(MQTTServer, close, "Close server", void) },
    {"onMessage", PyWrap(MQTTServer, onMessage, "Set onMessage callback", void, function<string(string)>) },
    {NULL}  /* Sentinel */
};

#ifndef WITHOUT_HDLC
PyMethodDef VRPyHDLC::methods[] = {
    {"setCallback", PyWrap(HDLC, setCallback, "Set callback", void, VRHDLCCbPtr) },
    {"isIdle", PyWrap(HDLC, isIdle, "Stop animation", bool) },
    {"getInterface", PyWrap(HDLC, getInterface, "Stop animation", string) },
    {"connect", PyWrap(HDLC, connect, "Stop animation", void) },
    {"connected", PyWrap(HDLC, connected, "Stop animation", bool) },
    {"handleData", PyWrap(HDLC, handleData, "Stop animation", void) },
    {"handle", PyWrap(HDLC, handle, "Stop animation", bool, unsigned char) },
    {"process", PyWrap(HDLC, process, "Stop animation", bool, vector<unsigned char>) },
    {"readData", PyWrap(HDLC, readData, "Stop animation", bool) },
    {"readAllData", PyWrap(HDLC, readAllData, "Stop animation", void) },
    {"waitForMessage", PyWrap(HDLC, waitForMessage, "Stop animation", void) },
    {"sendData", PyWrapOpt(HDLC, sendData, "Stop animation", "0", void, vector<unsigned char>, bool) },
    {"pauseSend", PyWrap(HDLC, pauseSend, "Stop animation", void, int) },
    {"pauseReceive", PyWrap(HDLC, pauseReceive, "Stop animation", void, int) },
    {"getLastInput", PyWrap(HDLC, getLastInput, "Stop animation", size_t) },
    {NULL}  /* Sentinel */
};
#endif

PyMethodDef VRPyRestResponse::methods[] = {
    {"getStatus", PyWrap(RestResponse, getStatus, "Get response status", int) },
    {"getHeaders", PyWrap(RestResponse, getHeaders, "Get response headers", string) },
    {"getData", PyWrap(RestResponse, getData, "Get response data", string) },
    {"setStatus", PyWrap(RestResponse, setStatus, "Set response status", void, int) },
    {"setHeaders", PyWrap(RestResponse, setHeaders, "Set response headers", void, string) },
    {"setData", PyWrap(RestResponse, setData, "Set response status", void, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRestClient::methods[] = {
    {"get", PyWrapOpt(RestClient, get, "Start GET request, (uri, timeout == 2)", "2", VRRestResponsePtr, string, int) },
    {"getAsync", PyWrapOpt(RestClient, getAsync, "Start async GET request, (uri, callback, timeout == 2)", "2", void, string, VRRestCbPtr, int) },
    {"post", PyWrapOpt(RestClient, post, "Start POST request, (uri, data, callback, timeout == 2)", "2", void, string, const string&, int) },
    {"test", PyWrap(RestClient, test, "Run test, output in console", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRestServer::methods[] = {
    {"listen", PyWrap(RestServer, listen, "Listen on port, (port, callback)", void, int, VRRestCbPtr) },
    {NULL}  /* Sentinel */
};

#ifndef WITHOUT_TCP
PyMethodDef VRPyNetworkClient::methods[] = {
    {"getProtocol", PyWrap(NetworkClient, getProtocol, "Return protocol, either UDP or TCP", string) },
    {"connect", PyWrap(NetworkClient, connect, "Connect to server", void, string, int) },
    {"send", PyWrapOpt(NetworkClient, send, "Send message to server", "|0", void, const string&, string, bool) },
    {"onMessage", PyWrap(NetworkClient, onMessage, "Set onMessage callback", void, function<string(string)>) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyUDPClient::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyUDPServer::methods[] = {
    {"listen", PyWrapOpt(UDPServer, listen, "Listen on port", "", void, int) },
    {"close", PyWrap(UDPServer, close, "Close server", void) },
    {"onMessage", PyWrapOpt(UDPServer, onMessage, "Set onMessage callback", "0", void, function<string(string)>, bool) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTCPClient::methods[] = {
    {"connectToPeer", PyWrap(TCPClient, connectToPeer, "Connect to another client P2P using TCP tunneling (local port, remote IP, remote port)", void, int, string, int) },
    {"onConnect", PyWrap(TCPClient, onConnect, "Set onConnect callback", void, function<void(void)>) },
    {"connected", PyWrap(TCPClient, connected, "Returns True is connected", bool) },
    {"getPublicIP", PyWrapOpt(TCPClient, getPublicIP, "Get public IP", "0", string, bool) },
    {NULL}  /* Sentinel */
};

typedef function<string(string, size_t)> serverCb;

PyMethodDef VRPyTCPServer::methods[] = {
    {"listen", PyWrapOpt(TCPServer, listen, "Listen on port", "", void, int, string) },
    {"close", PyWrap(TCPServer, close, "Close server", void) },
    {"onMessage", PyWrap(TCPServer, onMessage, "Set onMessage callback, callback signature:  cb(msg, uID)", void, serverCb) },
    {NULL}  /* Sentinel */
};

typedef map<string, string> mapSS;
typedef map<string, map<VRICEClient::CHANNEL, VRNetworkClientPtr> > mapScli;

PyMethodDef VRPyICEClient::methods[] = {
    {"setTurnServer", PyWrap(ICEClient, setTurnServer, "Setup turn server address, something like http://my.server/PolyServ/", void, string) },
    {"onEvent", PyWrap(ICEClient, onEvent, "Set onEvent callback", void, function<void(string)>) },
    {"onMessage", PyWrap(ICEClient, onMessage, "Set onMessage callback", void, function<void(string)>) },
    {"setName", PyWrapOpt(ICEClient, setName, "Set your name and uID to register on broker, optional async", "0", void, string, bool) },
    {"sendTCP", PyWrap(ICEClient, sendTCP, "Send data over the TCP connection", void, string, string, VRICEClient::CHANNEL) },
    {"send", PyWrap(ICEClient, send, "Send message to other: (uID, msg)", void, string, string) },
    {"connectTo", PyWrapOpt(ICEClient, connectTo, "Connect to another user, optional async", "0", void, string, bool) },
    {"getID", PyWrap(ICEClient, getID, "Get UID", string) },
    {"getUserName", PyWrap(ICEClient, getUserName, "Get user name by UID", string, string) },
    {"getUserID", PyWrap(ICEClient, getUserID, "Get UIDs of all users with certain name", vector<string>, string) },
    {"getUsers", PyWrap(ICEClient, getUsers, "Get all users registered at turn server", mapSS) },
    {"getClient", PyWrap(ICEClient, getClient, "Get TCP client by ID", VRNetworkClientPtr, string, VRICEClient::CHANNEL) },
    {"getClients", PyWrap(ICEClient, getClients, "Get all TCP clients", mapScli) },
    {"removeUser", PyWrap(ICEClient, removeUser, "Remove user by UID", void, string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCollaboration::methods[] = {
    {"setServer", PyWrap(Collaboration, setServer, "Set server, something like http://my.server/PolyServ/", void, string) },
    {"setupLocalServer", PyWrap(Collaboration, setupLocalServer, "Setup local server, this will clone the PolyServ repo if necessary", void) },
    {"setAvatarDevices", PyWrap(Collaboration, setAvatarDevices, "Set device beacons used for remote avatar representation", void, VRTransformPtr, VRTransformPtr, VRTransformPtr) },
    {"setAvatarGeometry", PyWrap(Collaboration, setAvatarGeometry, "Set avatar template geometries used to represent remote users, (torso, lHand, rHand, scale)", void, VRTransformPtr, VRTransformPtr, VRTransformPtr, float) },
    {"debugAvatars", PyWrap(Collaboration, debugAvatars, "Print an analysis of the avatar IDs, local, remote, etc..", void) },
    {NULL}  /* Sentinel */
};

template<> bool toValue(PyObject* o, function<string(string, size_t)>& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
	PyObject* args = PyTuple_New(2);
    e = bind(VRPyBase::execPyCall2<string, size_t, string>, o, args, placeholders::_1, placeholders::_2);
    return 1;
}

template<> bool toValue(PyObject* o, function<string(string)>& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
	PyObject* args = PyTuple_New(1);
    e = bind(VRPyBase::execPyCall<string, string>, o, args, placeholders::_1);
    return 1;
}

template<> bool toValue(PyObject* o, function<void(string)>& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
	PyObject* args = PyTuple_New(1);
    e = bind(VRPyBase::execPyCallVoid<string>, o, args, placeholders::_1);
    return 1;
}

template<> bool toValue(PyObject* o, function<void(void)>& e) {
    //if (!VRPyEntity::check(o)) return 0; // TODO: add checks!
    Py_IncRef(o);
	PyObject* args = PyTuple_New(0);
    e = bind(VRPyBase::execPyCallVoidVoid, o, args);
    return 1;
}

template<> int toValue(stringstream& ss, function<string(string, size_t)>& e) { return 0; }
template<> int toValue(stringstream& ss, function<string(string)>& e) { return 0; }
template<> int toValue(stringstream& ss, function<void(string)>& e) { return 0; }
template<> int toValue(stringstream& ss, function<void(void)>& e) { return 0; }

template<> string typeName(const function<string(string, size_t)>* t) { return "string function(string, int)"; }
template<> string typeName(const function<string(string)>* t) { return "string function(string)"; }
template<> string typeName(const function<void(string)>* t) { return "void function(string)"; }
template<> string typeName(const function<void(void)>* t) { return "void function()"; }
#endif




