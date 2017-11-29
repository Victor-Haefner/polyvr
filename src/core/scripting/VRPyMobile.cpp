#include "VRPyMobile.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Server, 0);

PyMethodDef VRPyServer::methods[] = {
    {"answer", PyWrap(Server, answerWebSocket, "Answer web socket - answer(int id, str message)\n use the device key as id, id = dev.getKey()", void, int, string) },
    {"openWebSocket", PyWrap(Server, openWebSocket, "Opens a websocket, returns connect ID.", int, string) },
    {NULL}  /* Sentinel */
};




