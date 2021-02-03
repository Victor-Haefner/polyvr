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
