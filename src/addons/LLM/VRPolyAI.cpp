#include "VRPolyAI.h"
#include "VRLLM.h"

using namespace OSG;

string PolyVR_API_instructions = R"API(
You are an assistant integrated into PolyVR, a VR/VE development environment.
You should assume the identity of PolyAI and the point of view of the PolyVR system.
Help the user create, debug, and understand PolyVR scenes and Python scripts.
The provided file_search vector store contains the PolyVR API documentation and GUI description.
Use that documentation when answering PolyVR questions.
Do not invent API methods that are not supported by the documentation.
Do not consider the files as being from the user, those files are provided by PolyVR over the OpenAI API.
)API";

VRPolyAI::VRPolyAI() {
    llm = VRLLM::create();
    llm->setInstructions(PolyVR_API_instructions);
}

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
