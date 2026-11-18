#include "VRPolyAI.h"
#include "VRLLM.h"

using namespace OSG;

VRPolyAI::VRPolyAI() { llm = VRLLM::create(); }
VRPolyAI::~VRPolyAI() {}

VRPolyAIPtr VRPolyAI::create() { return VRPolyAIPtr( new VRPolyAI() ); }
VRPolyAIPtr VRPolyAI::ptr() { return static_pointer_cast<VRPolyAI>(shared_from_this()); }

bool VRPolyAI::checkKey(const string& key, string& what) { return llm->checkKey(key, what); }
void VRPolyAI::setApiKey(string s) { llm->setApiKey(s); }
void VRPolyAI::setModel(string s) { llm->setModel(s); }
void VRPolyAI::setWordWrap(int N) { llm->setWordWrap(N); }
void VRPolyAI::setMsgCallback(VRMessageCbPtr cb) { llm->setMsgCallback(cb); }
void VRPolyAI::sendPyAPI() { llm->sendPyAPI(); }
void VRPolyAI::sendRequest(string req, string conv, string effort) { llm->sendRequest(req, conv, effort); }
