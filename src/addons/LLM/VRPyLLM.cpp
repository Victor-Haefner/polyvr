#include "VRPyLLM.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/objects/VRObjectFwd.h"

using namespace OSG;

simpleVRPyType(LLM, New_ptr);

PyMethodDef VRPyLLM::methods[] = {
    {"setApiKey", PyWrap(LLM, setApiKey, "Set API Key", void, string) },
    {"setModel", PyWrap(LLM, setModel, "Set model, for example gpt-5-nano, default is gpt-5.6-luna", void, string) },
    {"sendPyAPI", PyWrap(LLM, sendPyAPI, "Send the python API of PolyVR", void) },
    {"setMsgCallback", PyWrap(LLM, setMsgCallback, "Set msg callback, signature: def onMsg(msg)", void, VRMessageCbPtr) },
    {"setRestCallback", PyWrap(LLM, setRestCallback, "Set rest callback, signature: def onRestMsg(msg)", void, VRMessageCbPtr) },
    {"sendRequest", PyWrapOpt(LLM, sendRequest, "Send request, (request, conversation, effort = 'fast'), effort: [fast, normal, deep]", "fast", void, string, string, string) },
    {NULL}  /* Sentinel */
};
