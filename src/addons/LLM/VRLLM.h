#ifndef VRLLM_H_INCLUDED
#define VRLLM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/networking/VRNetworkingFwd.h"
#include "VRLLMFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <string>

#ifndef WITHOUT_JSONCPP
#include <json/json.h>
#endif

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRLLM : public enable_shared_from_this<VRLLM> {
    public:
        struct Conversation {
            string name;
            string ID;
            size_t createdAt = 0;
            bool ready = false;
            string queuedRequest;

            Conversation() {}
            Conversation(string n) : name(n) {}
        };

        struct File {
            string name;
            string ID;
            size_t createdAt = 0;
            bool ready = false;
        };

        struct Store {
            string name;
            string ID;
            size_t createdAt = 0;
            bool ready = false;
            string queuedRequest;

            vector<File> files;

            Store() {}
            Store(string n) : name(n) {}
        };

	private:
	    VRRestClientPtr cli;
	    string apiKey;
	    string model = "gpt-5.6-luna";
	    VRMessageCbPtr cb;
	    VRRestCbPtr restCb;

	    map<string,string> knowledgeAssets;
	    map<string, Store> stores;
	    map<string, Conversation> conversations;

	    void get(const string& uri, VRRestCbPtr cb = 0);
	    void send(const string& uri, const Json::Value& data, VRRestCbPtr cb = 0);
	    map<string, string> parseJsonMap(const string& data);
	    string convertEffort(const string& effort, const string& model);
	    void processResponse(VRRestResponsePtr r);
	    void processFileUpload(string store, VRRestResponsePtr r);

	    void setupKnowledgeAssets();
	    void deleteFile(const string& id);
	    void deleteStore(const string& id);
        void setupFile(const string& store, const string& file, const string& content);
        void setupVectorStore(const string& store, function<void(void)>& cb);
        void startConversation(const string& conv);

	public:
		VRLLM();
		~VRLLM();

		static VRLLMPtr create();
		VRLLMPtr ptr();

		void setApiKey(string s);
		void setModel(string s);
		void setCallback(VRMessageCbPtr cb);

		void sendPyAPI();
		void sendRequest(string req, string conv, string effort = "fast");
};

OSG_END_NAMESPACE;

#endif //VRLLM_H_INCLUDED
