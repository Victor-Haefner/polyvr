#ifndef VRPOLYAI_H_INCLUDED
#define VRPOLYAI_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRLLMFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPolyAI : public std::enable_shared_from_this<VRPolyAI> {
	private:
	    VRLLMPtr llm;
	    VRDeviceCbPtr sceneChangedCb;
	    VRServerCbPtr handleCreateScript;
	    VRServerCbPtr handleExecScript;

	    bool on_scene_changed();
	    string on_tool_executed(string name, map<string, string> params);
	    void setupKnowledgeAssets();

	public:
		VRPolyAI();
		~VRPolyAI();

		static VRPolyAIPtr create();
		VRPolyAIPtr ptr();

		bool checkKey(const string& key, string& what);
		void setApiKey(string s);
		void setModel(string s);
		void setWordWrap(int N);
		void setMsgCallback(VRMessageCbPtr cb);

		void sendPyAPI();
		void sendRequest(string req, string conv, string effort = "fast");
};

OSG_END_NAMESPACE;

#endif //VRPOLYAI_H_INCLUDED
