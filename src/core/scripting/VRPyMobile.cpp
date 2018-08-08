#include "VRPyMobile.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Server, 0);

#define myMap map<string,vector<int>>

PyMethodDef VRPyServer::methods[] = {
    {"answer", PyWrap(Server, answerWebSocket, "Answer web socket - answer(int id, str message)\n use the device key as id, id = dev.getKey()", void, int, string) },
    {"openWebSocket", PyWrapOpt(Server, openWebSocket, "Opens a websocket, returns connect ID.", "", int, string, string) },
    {"getClients", PyWrap(Server, getClients, "Get list of current client IDs", myMap ) },
    {NULL}  /* Sentinel */
};




