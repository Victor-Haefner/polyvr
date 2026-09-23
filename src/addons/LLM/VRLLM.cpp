#include "VRLLM.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"
#include "core/scripting/VRScriptManager.h"

#define JSONSTR(txt) #txt

using namespace OSG;

VRLLM::VRLLM() {
    cli = VRRestClient::create("llm");
    restCb =  VRRestCb::create("llmResp", bind(&VRLLM::processResponse, this, placeholders::_1) );
    setupKnowledgeAssets();
}

VRLLM::~VRLLM() {}

VRLLMPtr VRLLM::create() { return VRLLMPtr( new VRLLM() ); }
VRLLMPtr VRLLM::ptr() { return static_pointer_cast<VRLLM>(shared_from_this()); }

void VRLLM::setApiKey(string s) { apiKey = s; }
void VRLLM::setModel(string s) { model = s; }
void VRLLM::setCallback(VRMessageCbPtr c) { cb = c; }

void VRLLM::setupKnowledgeAssets() {
    auto scene = VRScene::getCurrent();
    if (!scene) return;

    string PolyVR_API = R"API(
PolyVR is a IDE for developing virtual environments.
It has a focus on engineering applications and includes many interfaces and simulation modules.
The application logic is created with Python scripts, shader can be written with GLSL, and UI elements with websites.

The API is provided as separate files:
    - PolyVR_GUI.txt contains a description of the GUI as presented to the user
    - PolyVR_API_modules.txt contains a list of all modules available to the user when scripting the application logic
    )API";

    string PolyVR_GUI = R"API(
# PolyVR GUI layout
- Toolbar at the top
    - Buttons: New, Open, Save, Save.., Close, Exit, Profiler, Export, About
    - Checkbox "Fotomode"
    - Radiobuttons for GUI Theme: "Light", "Dark"
    - Buttons for GUI Fontsize: +, -, 1
- 3D view on the right, below the toolbar
- Toolbar related to 3d view below the 3d view
    - Combobox to change the active camera
    - Combobox with checkboxes to control active navigation systems
    - Combobox with checkboxes to control debug layers
        - Cameras
        - Lights
        - Pause window
        - Physics
        - Objects
        - Setup
        - Statistics
        - Stencil
    - Buttons: Fullscreen, See All
- Output consoles, below the 3D view and its toolbar
    - Tabs for the different output consoles
        - tabs: "Console", "Errors", "Syntax", "Search results", "Reasoning", "Tracking", "Collaboration"
    - A clear button that wipes all output consoles
    - A pause checkbox to toggle the auto scroll of the output consoles
- A panel below the main toolbar and left of the 3d view
    - Tabs: Apps, Setup, Scene
        - Apps Panel:
            - entry to filter displayed projects
            - Two tabs: Projects, Examples
                - Same for each tab, a list of PolyVR projects to run
                    - For each project entry: a button "run" and a button "run without scripts" hidden in a drop down "advanced"
        - Setup Panel:
            - configuration of hardware environments
            - TODO ;)
        - Scene Panel:
            - Tabs: Rendering, Scenegraph, Scripting, Network
            - TODO ;)
    )API";

    string PolyVR_API_modules = R"API(
Here follows a list of all modules and objects available to the user in the PolyVR scripting environment.

    )API";

    for (auto mod : scene->getPyVRModules()) {
        PolyVR_API_modules += "# Module " + mod + "\n";
        for (auto tpe : scene->getPyVRTypes(mod)) {
            string dcr = scene->getPyVRDescription(mod, tpe);
            PolyVR_API_modules += " ## Class " + tpe + "\n";
            PolyVR_API_modules += "     " + dcr + "\n";
            for (auto mth : scene->getPyVRMethods(mod, tpe)) {
                string mdoc = scene->getPyVRMethodDoc(mod, tpe, mth);
                PolyVR_API_modules += " ### Method " + mth + "\n";
                PolyVR_API_modules += "      " + mdoc + "\n";
            }
        }
    }

    knowledgeAssets["PolyVR_API"] = PolyVR_API;
    knowledgeAssets["PolyVR_GUI"] = PolyVR_API;
    knowledgeAssets["PolyVR_API_modules"] = PolyVR_API_modules;
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
    if (store.ID.empty()) {
        cout << "Error in processFileUpload: store " << storeName << ", ID is empty!" << endl;
        return;
    }

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

void VRLLM::deleteStore(const string& id) {
    if (apiKey.empty() || id.empty()) return;
    vector<string> headers = { "Authorization: Bearer " + apiKey };
    string uri = "https://api.openai.com/v1/vector_stores/" + id;
    cli->deleteAsync(uri, restCb, 30, headers);
}

void VRLLM::deleteFile(const string& id) {
    if (apiKey.empty() || id.empty()) return;
    vector<string> headers = { "Authorization: Bearer " + apiKey };
    string uri = "https://api.openai.com/v1/files/" + id;
    cli->deleteAsync(uri, restCb, 30, headers);
}

void VRLLM::deleteFileEntry(const string& store, const string& id) {
    if (apiKey.empty() || store.empty() || id.empty()) return;
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

    auto cb = VRRestCb::create( "processFileUpload", bind(&VRLLM::processFileUpload, this, store, placeholders::_1) );
    cli->postFormAsync( "https://api.openai.com/v1/files", cb, form, 30, headers );
}

void VRLLM::setupVectorStore(const string& store, function<void(void)>& onStoreReady) {
    auto onStoreStatus = VRRestCb::create( "onStoreStatus", bind([this, store, onStoreReady](VRRestResponsePtr r) {
        string s = r->getData();
        cout << "LLM store status response: " << s << endl;

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

        cout << "keep store: " << keepID << endl;
        for (const auto& id : removeIDs) deleteStore(id);

        if (!keepID.empty()) {
            stores[store].ID = keepID;
            onStoreReady();
        } else {
            auto onNewStore = VRRestCb::create( "onNewStore", bind([this, store, onStoreReady](VRRestResponsePtr r) {
                string s = r->getData();
                cout << "LLM store ready response: " << s << endl;
                auto data = parseJsonMap(s);
                stores[store].ID = data["id"];
                onStoreReady();
            }, placeholders::_1) );

            Json::Value data;
            data["name"] = store;
            send("https://api.openai.com/v1/vector_stores", data, onNewStore);
        }
    }, placeholders::_1) );

    stores[store] = Store(store);
    get("https://api.openai.com/v1/vector_stores", onStoreStatus);
}

time_t getBuildTimestamp() {
    std::tm tm = {};
    std::istringstream ss(string(__DATE__) + " " + __TIME__);
    ss >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
    tm.tm_isdst = -1; // let mktime determine DST automatically
    return std::mktime(&tm);
}

void VRLLM::sendPyAPI() {
    string storeName = "PolyVR_PythonAPI";

    auto onFilesStatus = VRRestCb::create( "onFilesStatus", bind([this, storeName](VRRestResponsePtr r) {
        string s = r->getData();
        cout << "LLM files status response: " << s << endl;

        auto& store = stores[storeName];
        if (store.ID.empty()) {
            cout << "Error in processFileUpload: store " << storeName << ", ID is empty!" << endl;
            return;
        }

        Json::Value data;
        Json::Reader reader;
        if (!reader.parse(s, data)) return;

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
            if (!knowledgeAssets.count(name)) continue;

            FileStatus fstat;
            fstat.ID = file["id"].asString();
            fstat.filename = filename;
            fstat.bytes = file["bytes"].asInt64();
            fstat.createdAt = file["created_at"].asInt64();
            filesStatus[name].push_back( fstat );
        }

        time_t bTime = getBuildTimestamp();
        vector<string> removeIDs;
        for (auto a : knowledgeAssets) {
            bool needsReupload = false;
            FileStatus keepFS;
            string keepID;

            if (!filesStatus.count(a.first)) needsReupload = true;
            else {
                for (auto f : filesStatus[a.first]) {
                    cout << " ++++ " << a.first << ": " << bTime << ", " << f.createdAt << ", " << keepFS.createdAt << endl;
                    if (f.createdAt < bTime) { cout << "too old" << endl; removeIDs.push_back(f.ID); continue; }
                    if (f.bytes != a.second.size()) { cout << "wrong bytes" << endl; removeIDs.push_back(f.ID); continue; }
                    if (keepFS.createdAt > 0) {
                        if (f.createdAt < keepFS.createdAt) { cout << "older than keeper" << endl; removeIDs.push_back(f.ID); continue; }
                        else { cout << "keeper older" << endl; removeIDs.push_back(keepID); }
                    }
                    keepID = f.ID;
                    keepFS = f;
                }
            }

            if (keepID.empty()) needsReupload = true;
            else {
                cout << " .. keep file " << keepID << endl;

                VRLLM::File f;
                f.name = keepFS.filename;
                f.ID = keepID;
                f.createdAt = keepFS.createdAt;
                f.ready = true;
                store.files.push_back(f);

                bool hasEntry = find( store.hostEntries.begin(), store.hostEntries.end(), f.ID ) != store.hostEntries.end();
                if (!hasEntry) {
                    Json::Value data2;
                    data2["file_id"] = f.ID;
                    string uri = "https://api.openai.com/v1/vector_stores/"+store.ID+"/files";
                    send(uri, data2);
                }
            }

            if (needsReupload) { setupFile(storeName, a.first+".txt", a.second); continue; }
        }

        for (const auto& id : removeIDs) deleteFile(id);

        for (auto& fe : store.hostEntries) {
            bool doRemove = true;
            for (auto f : store.files) if (f.ID == fe) doRemove = false;
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
        for (const auto& fe : data["data"]) {
            string ID = fe["id"].asString();
            store.hostEntries.push_back(ID);
        }

        get("https://api.openai.com/v1/files", onFilesStatus);
    }, placeholders::_1) );

    function<void(void)> onStoreReady = [this, storeName, onFileEntriesStatus]() {
        string storeID = stores[storeName].ID;
        get("https://api.openai.com/v1/vector_stores/"+storeID+"/files", onFileEntriesStatus);
        //get("https://api.openai.com/v1/files", onFilesStatus);
    };

    setupVectorStore(storeName, onStoreReady);
}

/** IDEAS

- goals:
    - ask the LLM to ad a cube to the scene

- python api > LLM
    - LLM needs to know the api
    - LLM needs to be able to create a script, execute the script

*/





