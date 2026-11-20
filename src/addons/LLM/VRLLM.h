#ifndef VRLLM_H_INCLUDED
#define VRLLM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/networking/VRNetworkingFwd.h"
#include "VRLLMFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/WordWrap.h"

#include <string>
#include <functional>

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
            Json::Value queuedRequest;

            Conversation() {}
            Conversation(string n) : name(n) {}
        };

        struct File {
            string name;
            string ID;
            string data;
            size_t createdAt = 0;
            bool ready = false;
        };

        struct Store {
            string name;
            string ID;
            size_t createdAt = 0;
            bool ready = false;
            string queuedRequest;

            map<string,File> files;
            vector<string> hostEntries;

            Store() {}
            Store(string n) : name(n) {}
        };

        struct Parameter {
            string name;
            string type;
            string description;
        };

        struct Tool {
            string name;
            string description;
            map<string, Parameter> parameters;
            function<void(string,map<string,string>,string&)> cb;

            string pendingCallID;
            string pendingCallResult;

            Tool() {}
            Tool(string n) : name(n) {}
        };

	private:
	    VRRestClientPtr cli;
	    string apiKey;
	    string model = "gpt-5.6-luna";
	    string instructions;
	    VRMessageCbPtr usrMsgCb;
	    VRMessageCbPtr usrRestCb;
	    VRRestCbPtr restCb;
	    VRRestCbPtr streamCb;

        string streamBuffer;
        WordWrap wrapper;
        int wordWrap = 0;

	    map<string, Store> stores;
	    map<string, Tool> tools;
	    map<string, Conversation> conversations;

        string lastConversation;
        string lastEffort;

	    void get(const string& uri, VRRestCbPtr cb = 0);
	    void send(const string& uri, const Json::Value& data, VRRestCbPtr cb);
	    map<string, string> parseJsonMap(const string& data);
	    string convertEffort(const string& effort, const string& model);
	    Json::Value parseJson(const string& s);

	    void processEvent(string data);
	    void processStream(VRRestResponsePtr r);
	    void processResponse(VRRestResponsePtr r);
	    void processFileUpload(string store, VRRestResponsePtr r);

	    void deleteFile(const string& id);
	    void deleteStore(const string& id);
	    void deleteFileEntry(const string& store, const string& id);
        void setupFile(const string& store, const string& file, const string& content);
        void setupVectorStore(const string& store, function<void(string)>& cb);
        void startConversation(const string& conv);

		void sendJsonRequest(Json::Value req, string conv, string effort = "fast");
        void sendStream(const string& uri, const Json::Value& data);
        void processStreamChunk(const string& chunk);
        void sendToolOutput(const string& callID, const string& output);

	public:
		VRLLM();
		~VRLLM();

		static VRLLMPtr create();
		VRLLMPtr ptr();

		void setApiKey(string s);
		void setModel(string s);
		void setWordWrap(int N);
		void setMsgCallback(VRMessageCbPtr cb);
		void setRestCallback(VRMessageCbPtr cb);

		void setInstructions(string i);
		void addKnowledgeAsset(string name, string store, string data);
		void addTool(string name, function<void(string,map<string,string>,string&)> cb, string description);
        void addToolParameter(string tool, string param, string type, string description);

		bool checkKey(const string& key, string& what);
		void sendKnowledgeAssets();
		void sendRequest(string req, string conv, string effort = "fast");
};

OSG_END_NAMESPACE;

#endif //VRLLM_H_INCLUDED
