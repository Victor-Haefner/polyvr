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

	    map<string, Store> stores;
	    map<string, Conversation> conversations;

	    void send(const string& uri, const Json::Value& data, VRRestCbPtr cb = 0);
	    map<string, string> parseJsonMap(const string& data);
	    string convertEffort(const string& effort, const string& model);
	    void processResponse(VRRestResponsePtr r);
	    void processFileUpload(string store, VRRestResponsePtr r);

        void setupVectorStore(const string& store, VRRestCbPtr cb);
        void addStoreFile(const string& store, const string& filename, const string& data);
        void startConversation(const string& conv);

	public:
		VRLLM();
		~VRLLM();

		static VRLLMPtr create();
		VRLLMPtr ptr();

		void setApiKey(string s);
		void setModel(string s);
		void sendPyAPI();
		void sendRequest(string req, string conv, string effort = "fast");
		void setCallback(VRMessageCbPtr cb);
};

OSG_END_NAMESPACE;

#endif //VRLLM_H_INCLUDED
