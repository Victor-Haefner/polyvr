#include "VRLLM.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/scripting/VRScriptManager.h"

#define JSONSTR(txt) #txt

using namespace OSG;

VRLLM::VRLLM() {
    cli = VRRestClient::create("llm");
    restCb =  VRRestCb::create("llmResp", bind(&VRLLM::processResponse, this, placeholders::_1) );
    streamCb =  VRRestCb::create("llmResp", bind(&VRLLM::processStream, this, placeholders::_1) );
}

VRLLM::~VRLLM() {}

VRLLMPtr VRLLM::create() { return VRLLMPtr( new VRLLM() ); }
VRLLMPtr VRLLM::ptr() { return static_pointer_cast<VRLLM>(shared_from_this()); }

void VRLLM::setApiKey(string s) { apiKey = s; }
void VRLLM::setModel(string s) { model = s; }
void VRLLM::setWordWrap(int N) { wordWrap = N; }
void VRLLM::setInstructions(string s) { instructions = s; }
void VRLLM::setMsgCallback(VRMessageCbPtr c) { usrMsgCb = c; }
void VRLLM::setRestCallback(VRMessageCbPtr c) { usrRestCb = c; }

bool VRLLM::checkKey(const string& key, string& what) {
    what = "";

    if (startsWith(key, "sk-") ) {
        what = "key valid";
        return true;
    }

    what = "key not valid!";
    return false;
}

void VRLLM::addKnowledgeAsset(string name, string store, string data) {
    if (!stores.count(store)) stores[store] = Store(store);
    auto& s = stores[store];
    File f;
    f.data = data;
    f.name = name;
    s.files[name] = f;
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

void VRLLM::processEvent(string s) {
    //cout << "LLM response: " << s << endl;
    if (usrRestCb) (*usrRestCb)(s);

    Json::Value data;
    Json::Reader reader;
    if (!reader.parse(s, data)) return;

    auto processMessage = [&](string s) {
        //cout << " " << s;
        if (wordWrap > 0) wrapper.wrap(s, wordWrap);
        if (usrMsgCb) (*usrMsgCb)(s);
    };

    if (data.isMember("type")) {
        string type = data["type"].asString();

        if (type == "response.output_text.delta") {
            processMessage(data["delta"].asString());
            return;
        }
    }

    if (data.isMember("output")) {
        for (const auto& o : data["output"]) {
            if (!o.isMember("content")) continue;
            if (o["type"].asString() != "message") continue;

            for (const auto& c : o["content"]) {
                if (c["type"].asString() != "output_text") continue;
                if (c.isMember("text")) processMessage(c["text"].asString());
            }
        }
    }

    if (data.isMember("object")) {
        auto obj = data["object"].asString();

        if (obj == "conversation") {
            string name = data["metadata"]["name"].asString();
            auto& conv = conversations[name];
            conv.ID = data["id"].asString();
            conv.createdAt = data["created_at"].asInt64();
            conv.ready = true;

            if (!conv.queuedRequest.empty()) {
                sendRequest(conv.queuedRequest, conv.name);
                conv.queuedRequest = "";
            }
        }

        if (obj == "vector_store") {
            string name = data["name"].asString();
            auto& store = stores[name];
            store.ID = data["id"].asString();
            store.createdAt = data["created_at"].asInt64();
            store.ready = true;
        }
    }
}

void VRLLM::processStream(VRRestResponsePtr r) {
    string s = r->getData();
    streamBuffer += s;

    size_t pos;
    while ((pos = streamBuffer.find("\n\n")) != string::npos) {
        string event = streamBuffer.substr(0, pos);
        streamBuffer.erase(0, pos + 2);

        // get JSON payload
        size_t dataPos = event.find("data:");
        if (dataPos == string::npos) continue;
        string data = event.substr(dataPos + 5);

        size_t first = data.find_first_not_of(" \t\r\n");
        if (first != string::npos) data.erase(0, first);
        processEvent(data);
    }
}

void VRLLM::processResponse(VRRestResponsePtr r) {
    string s = r->getData();
    processEvent(s);
}

void VRLLM::processFileUpload(string storeName, VRRestResponsePtr r) {
    string s = r->getData();
    //cout << "LLM file upload response: " << s << endl;
    auto data = parseJsonMap(s);

    auto& store = stores[storeName];
    if (store.ID.empty()) {
        cout << "Error in processFileUpload: store " << storeName << ", ID is empty!" << endl;
        return;
    }

    VRLLM::File f;
    f.name = data["filename"];
    f.ID = data["id"];
    f.createdAt = toLong(data["created_at"]);
    f.ready = true;
    store.files[f.name] = f;

    cout << " add file " << f.name << " entry to store " << storeName << endl;
    Json::Value data2;
    data2["file_id"] = f.ID;
    string uri = "https://api.openai.com/v1/vector_stores/"+store.ID+"/files";
    send(uri, data2, restCb);
}

void VRLLM::get(const string& uri, VRRestCbPtr cb) {
    if (apiKey == "") return;
    if (!cb) cb = restCb;

    vector<string> headers = {
        "Authorization: Bearer " + apiKey
    };

    cli->getAsync( uri, cb, 30, headers );
}

void VRLLM::send(const string& uri, const Json::Value& data, VRRestCbPtr cb) {
    if (apiKey == "") return;

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
    send(uri, data, restCb);
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
    data["stream"] = true;
    if (con) data["conversation"] = con->ID;
    if (effort != "") data["reasoning"]["effort"] = effort;
    data["text"]["verbosity"] = verbosity;
    if (instructions.size() > 0) data["instructions"] = instructions;

    for (auto& s : stores) {
        auto& store = s.second;
        Json::Value tool;
        tool["type"] = "file_search";
        tool["vector_store_ids"].append(store.ID);
        data["tools"].append(tool);
    }

    string uri = "https://api.openai.com/v1/responses";
    send(uri, data, streamCb);
}

void VRLLM::deleteStore(const string& id) {
    if (apiKey.empty() || id.empty()) return;
    cout << " delete store " << id << endl;
    vector<string> headers = { "Authorization: Bearer " + apiKey };
    string uri = "https://api.openai.com/v1/vector_stores/" + id;
    cli->deleteAsync(uri, restCb, 30, headers);
}

void VRLLM::deleteFile(const string& id) {
    if (apiKey.empty() || id.empty()) return;
    cout << " delete file " << id << endl;
    vector<string> headers = { "Authorization: Bearer " + apiKey };
    string uri = "https://api.openai.com/v1/files/" + id;
    cli->deleteAsync(uri, restCb, 30, headers);
}

void VRLLM::deleteFileEntry(const string& store, const string& id) {
    if (apiKey.empty() || store.empty() || id.empty()) return;
    cout << " delete file entry " << id << " in store " << store << endl;
    vector<string> headers = { "Authorization: Bearer " + apiKey };
    string uri = "https://api.openai.com/v1/vector_stores/" + store + "/files/" + id;
    cli->deleteAsync(uri, restCb, 30, headers);
}

void VRLLM::setupFile(const string& store, const string& filename, const string& content) {
    if (apiKey == "") return;

    vector<string> headers = { "Authorization: Bearer " + apiKey };

    vector<map<string,string>> form = {
        { {"name", "purpose"}, {"data", "user_data"} },
        { {"name", "file"}, {"data", content}, {"filename", filename}, {"contentType", "text/plain"} }
    };

    cout << " upload file " << filename << endl;
    auto cb = VRRestCb::create( "processFileUpload", bind(&VRLLM::processFileUpload, this, store, placeholders::_1) );
    cli->postFormAsync( "https://api.openai.com/v1/files", cb, form, 30, headers );
}

void VRLLM::setupVectorStore(const string& store, function<void(string)>& onStoreReady) {
    auto onStoreStatus = VRRestCb::create( "onStoreStatus", bind([this, store, onStoreReady](VRRestResponsePtr r) {
        string s = r->getData();
        //cout << "LLM store status response: " << s << endl;

        Json::Value data;
        Json::Reader reader;
        if (!reader.parse(s, data)) return;

        string keepID;
        Json::Int64 newest = 0;
        vector<string> removeIDs;

        for (const auto& store : data["data"]) {
            if (store["name"].asString() != "PolyVR_PythonAPI") continue;

            string id = store["id"].asString();
            Json::Int64 created = store["created_at"].asInt64();

            if (keepID.empty() || created > newest) {
                if (!keepID.empty()) removeIDs.push_back(keepID);
                keepID = id;
                newest = created;
            } else {
                removeIDs.push_back(id);
            }
        }

        cout << "  keep store: " << keepID << endl;
        for (const auto& id : removeIDs) deleteStore(id);

        if (!keepID.empty()) {
            stores[store].ID = keepID;
            onStoreReady(store);
        } else {
            auto onNewStore = VRRestCb::create( "onNewStore", bind([this, store, onStoreReady](VRRestResponsePtr r) {
                string s = r->getData();
                cout << "LLM store ready response: " << s << endl;
                auto data = parseJsonMap(s);
                stores[store].ID = data["id"];
                onStoreReady(store);
            }, placeholders::_1) );

            Json::Value data;
            data["name"] = store;
            send("https://api.openai.com/v1/vector_stores", data, onNewStore);
        }
    }, placeholders::_1) );

    cout << " .. setupVectorStore " << store << endl;
    get("https://api.openai.com/v1/vector_stores", onStoreStatus);
}

time_t getBuildTimestamp() {
    std::tm tm = {};
    std::istringstream ss(string(__DATE__) + " " + __TIME__);
    ss >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
    tm.tm_isdst = -1; // let mktime determine DST automatically
    return std::mktime(&tm);
}

Json::Value VRLLM::parseJson(const string& s) {
    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(s, value)) cout << " Error, json parse failed!" << endl;
    return value;
}

void VRLLM::sendKnowledgeAssets() {
    function<void(string)> onStoreReady = [this](string storeName) {
        string storeID = stores[storeName].ID;
        cout << " request file entries from store.." << endl;

        auto onFilesStatus = VRRestCb::create( "onFilesStatus", bind([this, storeName](VRRestResponsePtr r) {
            string s = r->getData();
            cout << "LLM files status response: " << s << endl;

            auto& store = stores[storeName];
            if (store.ID.empty()) {
                cout << "Error in processFileUpload: store " << storeName << ", ID is empty!" << endl;
                return;
            }

            Json::Value data = parseJson(s);
            if (data.empty()) { cout << " no data" << endl; return; }

            struct FileStatus {
                string ID;
                string filename;
                size_t bytes = 0;
                time_t createdAt = 0;
            };

            map<string, vector<FileStatus>> filesStatus;
            for (const auto& file : data["data"]) {
                string filename = file["filename"].asString();
                if (filename.size() < 6) continue;
                string name = subString(filename, 0, -5);
                if (!store.files.count(name)) continue;

                cout << "  found hosted asset " << filename << endl;
                FileStatus fstat;
                fstat.ID = file["id"].asString();
                fstat.filename = filename;
                fstat.bytes = file["bytes"].asInt64();
                fstat.createdAt = file["created_at"].asInt64();
                filesStatus[name].push_back( fstat );
            }

            time_t bTime = getBuildTimestamp();
            vector<string> removeIDs;
            for (auto& f : store.files) {
                bool needsReupload = false;
                FileStatus keepFS;
                string keepID;
                cout << "  check asset " << f.first << endl;

                if (!filesStatus.count(f.first)) needsReupload = true;
                else {
                    for (auto fs : filesStatus[f.first]) {
                        //cout << " ++++ " << a.first << ": " << bTime << ", " << f.createdAt << ", " << keepFS.createdAt << endl;
                        if (fs.createdAt < bTime) { cout << "   too old" << endl; removeIDs.push_back(fs.ID); continue; }
                        if (fs.bytes != f.second.data.size()) { cout << "   wrong bytes" << endl; removeIDs.push_back(fs.ID); continue; }
                        if (keepFS.createdAt > 0) {
                            if (fs.createdAt < keepFS.createdAt) { cout << "   older than keeper" << endl; removeIDs.push_back(fs.ID); continue; }
                            else { cout << "   keeper older" << endl; removeIDs.push_back(keepID); }
                        }
                        keepID = fs.ID;
                        keepFS = fs;
                    }
                }

                if (keepID.empty()) needsReupload = true;
                else {
                    cout << " .. keep file " << keepID << endl;
                    f.second.name = keepFS.filename;
                    f.second.ID = keepID;
                    f.second.createdAt = keepFS.createdAt;
                    f.second.ready = true;

                    bool hasEntry = find( store.hostEntries.begin(), store.hostEntries.end(), f.second.ID ) != store.hostEntries.end();
                    if (!hasEntry) {
                        Json::Value data2;
                        data2["file_id"] = f.second.ID;
                        string uri = "https://api.openai.com/v1/vector_stores/"+store.ID+"/files";
                        send(uri, data2, restCb);
                    }
                }

                if (needsReupload) {
                    cout << " .. reupload file " << keepID << endl;
                    setupFile(storeName, f.first+".txt", f.second.data);
                    continue;
                }
            }

            for (const auto& id : removeIDs) deleteFile(id);

            for (auto& fe : store.hostEntries) {
                bool doRemove = true;
                for (auto f : store.files) if (f.second.ID == fe) doRemove = false;
                if (doRemove) deleteFileEntry(store.ID, fe);
            }
        }, placeholders::_1) );

        auto onFileEntriesStatus = VRRestCb::create( "onFileEntriesStatus", bind([this, storeName, onFilesStatus](VRRestResponsePtr r) {
            string s = r->getData();
            cout << "LLM file store entries: " << s << endl;

            Json::Value data;
            Json::Reader reader;
            if (!reader.parse(s, data)) return;

            auto& store = stores[storeName];
            if (data["data"].size() == 0) {
                cout << "  .. no file entries" << endl;
            } else {
                for (const auto& fe : data["data"]) {
                    string ID = fe["id"].asString();
                    store.hostEntries.push_back(ID);
                    cout << "  store file entry: " << ID << endl;
                }
            }

            cout << " request files list.." << endl;
            get("https://api.openai.com/v1/files", onFilesStatus);
        }, placeholders::_1) );

        get("https://api.openai.com/v1/vector_stores/"+storeID+"/files", onFileEntriesStatus);
        //get("https://api.openai.com/v1/files", onFilesStatus);
    };

    for (auto& s : stores) {
        setupVectorStore(s.second.name, onStoreReady);
    }
}

/** IDEAS

- check out MCP, Model Context Protocol, might be helpful
- Add interface to support skills

*/





