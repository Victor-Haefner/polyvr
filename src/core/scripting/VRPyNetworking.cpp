#include "VRPyNetworking.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

#ifndef WITHOUT_HDLC
simpleVRPyType(HDLC, New_ptr);
#endif
simpleVRPyType(RestResponse, 0);
simpleVRPyType(RestClient, New_ptr);
simpleVRPyType(RestServer, New_ptr);

#ifndef WITHOUT_TCP
simpleVRPyType(NetworkClient, 0);
simpleVRPyType(UDPClient, New_ptr);
simpleVRPyType(UDPServer, New_ptr);
simpleVRPyType(TCPClient, New_ptr);
simpleVRPyType(TCPServer, New_ptr);
simpleVRPyType(ICEClient, New_ptr);

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
    {"getStatus", PyWrap(RestResponse, getStatus, "Get response status", string) },
    {"getData", PyWrap(RestResponse, getData, "Get response content", string) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRestClient::methods[] = {
    {"get", PyWrapOpt(RestClient, get, "Start GET request, (uri, timeout == 2)", "2", VRRestResponsePtr, string, int) },
    {"getAsync", PyWrapOpt(RestClient, getAsync, "Start async GET request, (uri, callback, timeout == 2)", "2", void, string, VRRestCbPtr, int) },
    {"test", PyWrap(RestClient, test, "Run test, output in console", void) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRestServer::methods[] = {
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
    {"onMessage", PyWrap(UDPServer, onMessage, "Set onMessage callback", void, function<string(string)>) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTCPClient::methods[] = {
    {"connectToPeer", PyWrap(TCPClient, connectToPeer, "Connect to another client P2P using TCP tunneling (local port, remote IP, remote port)", void, int, string, int) },
    {"onConnect", PyWrap(TCPClient, onConnect, "Set onConnect callback", void, function<void(void)>) },
    {"connected", PyWrap(TCPClient, connected, "Returns True is connected", bool) },
    {"getPublicIP", PyWrapOpt(TCPClient, getPublicIP, "Get public IP", "0", string, bool) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTCPServer::methods[] = {
    {"listen", PyWrapOpt(TCPServer, listen, "Listen on port", "", void, int, string) },
    {"close", PyWrap(TCPServer, close, "Close server", void) },
    {"onMessage", PyWrap(TCPServer, onMessage, "Set onMessage callback", void, function<string(string)>) },
    {NULL}  /* Sentinel */
};

typedef map<string, string> mapSS;
typedef map<string, map<VRICEClient::CHANNEL, VRNetworkClientPtr> > mapScli;

PyMethodDef VRPyICEClient::methods[] = {
    {"setTurnServer", PyWrap(ICEClient, setTurnServer, "Setup turn server address, something like http://my.server/PolyServ/", void, string) },
    {"onEvent", PyWrap(ICEClient, onEvent, "Set onEvent callback", void, function<void(string)>) },
    {"onMessage", PyWrap(ICEClient, onMessage, "Set onMessage callback", void, function<void(string)>) },
    {"setName", PyWrap(ICEClient, setName, "Set your name and uID to register on broker", void, string) },
    {"sendTCP", PyWrap(ICEClient, sendTCP, "Send data over the TCP connection", void, string, string, VRICEClient::CHANNEL) },
    {"send", PyWrap(ICEClient, send, "Send message to other: (uID, msg)", void, string, string) },
    {"connectTo", PyWrap(ICEClient, connectTo, "Connect to another user", void, string) },
    {"getID", PyWrap(ICEClient, getID, "Get UID", string) },
    {"getUserName", PyWrap(ICEClient, getUserName, "Get user name by UID", string, string) },
    {"getUserID", PyWrap(ICEClient, getUserID, "Get UIDs of all users with certain name", vector<string>, string) },
    {"getUsers", PyWrap(ICEClient, getUsers, "Get all users registered at turn server", mapSS) },
    {"getClient", PyWrap(ICEClient, getClient, "Get TCP client by ID", VRNetworkClientPtr, string, VRICEClient::CHANNEL) },
    {"getClients", PyWrap(ICEClient, getClients, "Get all TCP clients", mapScli) },
    {"removeUser", PyWrap(ICEClient, removeUser, "Remove user by UID", void, string) },
    {NULL}  /* Sentinel */
};

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

template<> int toValue(stringstream& ss, function<string(string)>& e) { return 0; }
template<> int toValue(stringstream& ss, function<void(string)>& e) { return 0; }
template<> int toValue(stringstream& ss, function<void(void)>& e) { return 0; }

template<> string typeName(const function<string(string)>* t) { return "string function(string)"; }
template<> string typeName(const function<void(string)>* t) { return "void function(string)"; }
template<> string typeName(const function<void(void)>* t) { return "void function()"; }
#endif




