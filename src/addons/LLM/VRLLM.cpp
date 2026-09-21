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

void VRLLM::sendPyAPI() {
    string name = "PolyVR_PythonAPI";
    stores[name] = Store(name);

    auto onStoreReady = VRRestCb::create( "llmFileUpload", bind([this, name](VRRestResponsePtr r) {
        string s = r->getData();
        cout << "LLM store ready response: " << s << endl;
        auto data = parseJsonMap(s);

        stores[name].ID = data["id"];

        addStoreFile(name, name+".txt", "API test - methods: doA(), doB(), doC()");
    }, placeholders::_1) );

    setupVectorStore(name, onStoreReady);
}

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

            if (conv.queuedRequest != "") {
                sendRequest(conv.queuedRequest, conv.name);
            }
        }

        if (obj == "vector_store") {
            string name = data["name"];
            auto& store = stores[name];
            store.ID = data["id"];
            store.createdAt = toLong(data["created_at"]);
            store.ready = true;
        }
    }
}

void VRLLM::processFileUpload(string storeName, VRRestResponsePtr r) {
    string s = r->getData();
    cout << "LLM file upload response: " << s << endl;
    auto data = parseJsonMap(s);

    auto& store = stores[storeName];

    VRLLM::File f;
    f.name = data["filename"];
    f.ID = data["id"];
    f.createdAt = toLong(data["created_at"]);
    f.ready = true;
    store.files.push_back(f);

    Json::Value data2;
    data2["file_id"] = f.ID;
    string uri = "https://api.openai.com/v1/vector_stores/"+store.ID+"/files";
    send(uri, data2);
}

void VRLLM::send(const string& uri, const Json::Value& data, VRRestCbPtr cb) {
    if (apiKey == "") return;
    if (!cb) cb = restCb;

    vector<string> headers = {
        "Content-Type: application/json",
        "Authorization: Bearer " + apiKey
    };

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    string json = Json::writeString(writer, data);

    cout << "sending to " << uri << ": " << endl << json << endl;
    cli->postAsync( uri, cb, json, 30, headers );
}

void VRLLM::startConversation(const string& conv) {
    Json::Value data;
    data["metadata"]["name"] = conv;

    string uri = "https://api.openai.com/v1/conversations";
    send(uri, data);
}

void VRLLM::addStoreFile(const string& store, const string& filename, const string& content) {
    if (apiKey == "") return;

    vector<string> headers = {
        "Authorization: Bearer " + apiKey
    };

    vector<map<string,string>> form = {
        { {"name", "purpose"}, {"data", "user_data"} },
        { {"name", "file"}, {"data", content}, {"filename", filename}, {"contentType", "text/plain"} }
    };

    auto cb = VRRestCb::create( "llmFileUpload", bind(&VRLLM::processFileUpload, this, store, placeholders::_1) );

    string uri = "https://api.openai.com/v1/files";
    cli->postFormAsync( uri, cb, form, 30, headers );
}

void VRLLM::setupVectorStore(const string& store, VRRestCbPtr cb) {
    Json::Value data;
    data["name"] = store;

    string uri = "https://api.openai.com/v1/vector_stores";
    send(uri, data, cb);
}

string VRLLM::convertEffort(const string& effort, const string& model) {

    if (model == "gpt-5-nano") {
        cout << " A " << endl;
        if (effort == "fast") return "low";
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
            conversations[conv] = Conversation(conv);
            conversations[conv].queuedRequest = req;
            startConversation(conv);
            return;
        }

        con = &conversations[conv];
        if (!con->ready) return;
    }

    effort = convertEffort(effort, model);
    cout << " -- effort: " << effort << endl;
    string verbosity = "low";

    Json::Value data;
    data["model"] = model;
    data["input"] = req;
    if (con) data["conversation"] = con->ID;
    if (effort != "") data["reasoning"]["effort"] = effort;
    data["text"]["verbosity"] = verbosity;

    for (auto& s : stores) {
        auto& store = s.second;
        Json::Value tool;
        tool["type"] = "file_search";
        tool["vector_store_ids"].append(store.ID);
        data["tools"].append(tool);
    }

    string uri = "https://api.openai.com/v1/responses";
    send(uri, data);
}

/** IDEAS

- goals:
    - ask the LLM to ad a cube to the scene

- python api > LLM
    - LLM needs to know the api
    - LLM needs to be able to create a script, execute the script

*/





