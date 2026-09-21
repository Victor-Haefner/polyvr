#include "VRLLM.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"

#define JSONSTR(txt) #txt

using namespace OSG;

VRLLM::VRLLM() {
    cli = VRRestClient::create("llm");
    restCb =  VRRestCb::create("llmResp", bind(&VRLLM::processResponse, this, placeholders::_1) );
}

VRLLM::~VRLLM() {}

VRLLMPtr VRLLM::create() { return VRLLMPtr( new VRLLM() ); }
VRLLMPtr VRLLM::ptr() { return static_pointer_cast<VRLLM>(shared_from_this()); }

void VRLLM::setApiKey(string s) { apiKey = s; }
void VRLLM::setModel(string s) { model = s; }
void VRLLM::setCallback(VRMessageCbPtr c) { cb = c; }

map<string, string> VRLLM::parseJsonMap(const string& data) {
    map<string, string> m;
#ifndef WITHOUT_JSONCPP
    Json::Value root;
    Json::Reader reader;
    Json::StreamWriterBuilder writer;

    if (!reader.parse(data, root) || !root.isObject()) return m;

    for (const auto& key : root.getMemberNames()) {
        const Json::Value& val = root[key];
        if (!val.isObject() && !val.isArray()) m[key] = val.asString();
        else m[key] = Json::writeString(writer, val);
    }
#endif // WITHOUT_JSONCPP
    return m;
}

void VRLLM::processResponse(VRRestResponsePtr r) {
    string s = r->getData();
    cout << "LLM response: " << s << endl;
    if (cb) (*cb)(s);

    auto data = parseJsonMap(s);
    if (data.count("object")) {
        auto obj = data["object"];
        cout << " ..is object " << obj << endl;
        if (obj == "conversation") {
            auto metadata = parseJsonMap(data["metadata"]);
            string name = metadata["name"];
            auto& conv = conversations[name];
            conv.ID = data["id"];
            conv.createdAt = toLong(data["created_at"]);
            conv.ready = true;
            cout << "  ..conversation ready " << conv.name << endl;
        }
    }
}

void VRLLM::send(const string& uri, const Json::Value& data) {
    if (apiKey == "") return;

    vector<string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + apiKey
    };

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    string json = Json::writeString(writer, data);

    cout << json << endl;
    cli->postAsync( uri, restCb, json, 30, headers );
}

void VRLLM::startConversation(const string& conv) {
    Json::Value data;
    data["metadata"]["name"] = conv;

    string uri = "https://api.openai.com/v1/conversations";
    send(uri, data);
}

string VRLLM::convertEffort(const string& effort, const string& model) {
    cout << "convertEffort " << effort << ", " << model << endl;
    if (model == "gpt-5-nano") {
        cout << " A " << endl;
        if (effort == "fast") return "minimal";
        if (effort == "normal") return "medium";
        if (effort == "deep") return "high";
    }

    if (model == "gpt-5.6-luna") {
        cout << " B " << endl;
        if (effort == "fast") return "none";
        if (effort == "normal") return "medium";
        if (effort == "deep") return "high";
    }

    return "";
}

void VRLLM::sendRequest(string req, string conv, string effort) {
    Conversation* con = 0;

    if (conv != "") {
        if (!conversations.count(conv)) {
            cout << "unknown conversation, start new " << conv << endl;
            conversations[conv] = Conversation(conv);
            startConversation(conv);
            return; // TODO: add request to queue
        }

        con = &conversations[conv];
        cout << "conversation ready? " << con->ready << endl;
        if (!con->ready) return; // TODO: add request to queue
    }

    effort = convertEffort(effort, model);
    string verbosity = "low";

    Json::Value data;
    data["model"] = model;
    data["input"] = req;
    if (con) data["conversation"] = con->ID;
    if (effort != "") data["reasoning"]["effort"] = effort;
    data["text"]["verbosity"] = verbosity;

    string uri = "https://api.openai.com/v1/responses";
    send(uri, data);
}
