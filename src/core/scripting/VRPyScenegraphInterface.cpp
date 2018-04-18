#include "VRPyScenegraphInterface.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(ScenegraphInterface, New_VRObjects_ptr);

PyMethodDef VRPyScenegraphInterface::methods[] = {
    {"clear", PyWrap(ScenegraphInterface, clear, "Clear", void) },
    {"setPort", PyWrap(ScenegraphInterface, setPort, "Set port and reset websocket", void, int) },
    {"loadStream", PyWrap(ScenegraphInterface, loadStream, "Read interface stream from file", void, string) },
    {"send", PyWrap(ScenegraphInterface, send, "Send a message over websocket", void, string) },
    {"addCallback", PyWrap(ScenegraphInterface, addCallback, "Add handler callback", void, VRMessageCbPtr) },
    {"getObject", PyWrap(ScenegraphInterface, getObject, "Returns object", VRObjectPtr, string) },
    {NULL}  /* Sentinel */
};
