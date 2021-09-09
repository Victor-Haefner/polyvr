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
simpleVRPyType(TCPClient, New_ptr);
simpleVRPyType(TCPServer, New_ptr);
simpleVRPyType(ICEClient, New_ptr);
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
PyMethodDef VRPyTCPClient::methods[] = {
    {"connect", PyWrap(TCPClient, connect, "Connect to server", void, string, int) },
    {"connectToPeer", PyWrap(TCPClient, connectToPeer, "Connect to another client P2P using TCP tunneling (local port, remote IP, remote port)", void, int, string, int) },
    {"send", PyWrapOpt(TCPClient, send, "Send message to server", "", void, const string&, string) },
    {"onMessage", PyWrap(TCPClient, onMessage, "Set onMessage callback", void, function<void(string)>) },
    {"onConnect", PyWrap(TCPClient, onConnect, "Set onConnect callback", void, function<void(void)>) },
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

PyMethodDef VRPyICEClient::methods[] = {
    {"setTurnServer", PyWrap(ICEClient, setTurnServer, "Setup turn server address and ip", void, string, string) },
    {"onEvent", PyWrap(ICEClient, onEvent, "Set onEvent callback", void, function<void(string)>) },
    {"onMessage", PyWrap(ICEClient, onMessage, "Set onMessage callback", void, function<void(string)>) },
    {"setName", PyWrap(ICEClient, setName, "Set your name and uID to register on broker", void, string) },
    {"sendTCP", PyWrap(ICEClient, sendTCP, "Send data over the TCP connection", void, string) },
    {"send", PyWrap(ICEClient, send, "Send message to other: (uID, msg)", void, string, string) },
    {"connectTo", PyWrap(ICEClient, connectTo, "Connect to another user", void, string) },
    {"getID", PyWrap(ICEClient, getID, "Get UID", string) },
    {"getUserName", PyWrap(ICEClient, getUserName, "Get user name by UID", string, string) },
    {"getUserID", PyWrap(ICEClient, getUserID, "Get UIDs of all users with certain name", vector<string>, string) },
    {"getUsers", PyWrap(ICEClient, getUsers, "Get all users registered at turn server", mapSS) },
    {"getClient", PyWrap(ICEClient, getClient, "Get internal TCP client", VRTCPClientPtr) },
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




