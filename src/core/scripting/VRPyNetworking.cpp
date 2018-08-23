#include "VRPyNetworking.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(HDLC, New_ptr);

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
