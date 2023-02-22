#include "VRSocket.h"
#ifndef __EMSCRIPTEN__
#include "VRPing.h"
#endif
#include "mongoose/mongoose.h"
#include "core/objects/object/VRObject.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRServer.h"
#include "core/utils/toString.h"
#include "core/utils/VRLogger.h"
#include "core/utils/system/VRSystem.h"

#include <algorithm>
#ifndef WITHOUT_CURL
#include <curl/curl.h> // TODO: windows port
#endif
#include <stdint.h>

#include <regex>

using namespace OSG;


//mongoose server-------------------------------------------------------------

HTTP_args::HTTP_args() {
    params = shared_ptr<map<string, string>>( new map<string, string>() );
    pages = shared_ptr<map<string, string>>( new map<string, string>() );
    callbacks = shared_ptr<map<string, VRServerCbWeakPtr>>( new map<string, VRServerCbWeakPtr>() );
}

HTTP_args::~HTTP_args() {}

void HTTP_args::print() {
    stringstream ss;
    ss << "HTTP args: " << path << endl;
    if (params != 0)
        for (auto p : *params) ss << "  " << p.first << " : " << p.second << endl;
    VRLog::log("net", ss.str());
}

HTTP_args* HTTP_args::copy() {
    HTTP_args* res = new HTTP_args();
    res->cb = cb;
    *res->params = *params;
    *res->pages = *pages;
    *res->callbacks = *callbacks;
    res->path = path;
    res->websocket = websocket;
    res->ws_data = ws_data;
    res->ws_id = ws_id;
    res->serv = serv;
    return res;
}

namespace OSG {
void server_answer_job(HTTP_args* args) {
    if (VRLog::tag("net")) {
        stringstream ss; ss << "server_answer_job: " << args->cb << endl;
        VRLog::log("net", ss.str());
    }
    //cout << " ---- server_answer_job: " << args->ws_data << endl;
    //args->print();
    if (args->cb) (*args->cb)(args);
    delete args;
}

static void server_answer_to_connection_m(struct mg_connection *conn, int ev, void *ev_data);
static struct mg_serve_http_opts s_http_server_opts;

class HTTPServer {
    private:
        VRThreadCbPtr serverThread;

    public:
        //server----------------------------------------------------------------
        //struct MHD_Daemon* server = 0;
        struct mg_mgr* server = 0;
        struct mg_bind_opts bind_opts;
        struct mg_connection* nc = 0;
        const char* err_str = 0;
        int port = 0;
        int threadID = 0;
        HTTP_args* data = 0;

        map<mg_connection*, int> websocket_ids;
        map<int, mg_connection*> websockets;
        map<int, string> ws_groups;

        HTTPServer() {
            data = new HTTP_args();
            data->serv = this;
        }

        ~HTTPServer() {
            delete data;
        }

        bool initServer(VRHTTP_cb* fkt, int p) {
            port = p;
            data->cb = fkt;
            server = new mg_mgr();
            mg_mgr_init(server, data);
            //server = mg_create_server(data, server_answer_to_connection_m);
            memset(&bind_opts, 0, sizeof(bind_opts));
            bind_opts.error_string = &err_str;
            for (int i=0; i<10; i++) {
                nc = mg_bind_opt(server, toString(port+i).c_str(), server_answer_to_connection_m, bind_opts);
                if (!nc) cout << "Warning in initServer: could not bind to " << server << ":" << port+i << ", increment port to " << port+i << endl;
                else {
                    port = port+i;
                    break;
                }
            }
            if (!nc) { cout << "Warning in initServer: could not bind to " << server << ":" << port << endl; return false; }
            mg_set_protocol_http_websocket(nc);
            //s_http_server_opts.document_root = ".";
            s_http_server_opts.enable_directory_listing = "yes";
            //mg_set_option(server, "listening_port", toString(port).c_str());

            serverThread = VRFunction<VRThreadWeakPtr>::create("mongoose loop", bind(&HTTPServer::loop, this, placeholders::_1));
            threadID = VRSceneManager::get()->initThread(serverThread, "mongoose", true);

            //server = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL, &server_answer_to_connection, data, MHD_OPTION_END);
            return true;
        }

        void addPage(string path, string page) {
            (*data->pages)[path] = page;
        }

        void remPage(string path) {
            if (data->pages->count(path)) {
                data->pages->erase(path);
            }
        }

        void addCallback(string path, VRServerCbPtr cb) {
            if (data->callbacks->count(path) == 0) (*data->callbacks)[path] = cb;
        }

        void remCallback(string path) {
            if (data->callbacks->count(path)) {
                data->callbacks->erase(path);
            }
        }

        void loop(VRThreadWeakPtr wt) {
            if (server) mg_mgr_poll(server, 100);
            //if (auto t = wt.lock()) if (t->control_flag == false) return;
        }

        void close() {
            //cout << "  HTTPServer::close" << endl;
            if (server) {
                //cout << "   HTTPServer::close stop thread " << threadID << endl;
                auto sm = VRSceneManager::get();
                if (sm) sm->stopThread(threadID);
                //cout << "   HTTPServer::close mg_mgr_free server" << endl;
                mg_mgr_free(server);
                //cout << "   HTTPServer::close delete server" << endl;
                delete server;
            }
            server = 0;
            //cout << "   HTTPServer::close done" << endl;
        }

        void websocket_send(int id, string message) {
            if (websockets.count(id) && websockets[id]) mg_send_websocket_frame(websockets[id], WEBSOCKET_OP_TEXT, message.c_str(), message.size());
        }

        int websocket_open(string address, string protocols) {
            static size_t clientID = 1e5; clientID++;
            int newID = clientID;
            auto c = mg_connect_ws(server, server_answer_to_connection_m, address.c_str(), protocols.c_str(), NULL);
            if (c) websockets[newID] = c;
            else newID = -1;
            return newID;
        }

        map<string, vector<int>> getClients() {
            map<string, vector<int>> res;
            for (auto ws : websockets) {
                int ID = ws.first;
                string group = "ungrouped";
                if (ws_groups.count(ID)) group = ws_groups[ID];
                if (!res.count(group)) res[group] = vector<int>();
                res[group].push_back(ID);
            }
            return res;
        }
};

string processPHP(HTTP_args* sad) {
    // copy php file and prepend something to simulate GET/POST parameters
    systemCall("cp "+sad->path+" "+sad->path+"_tmp.php" );
    string toPrepend = "if (isset($argv[1])) { parse_str($argv[1], $_GET); parse_str($argv[1], $_POST); }";
    systemCall("awk -i inplace 'NR==1{print; print \""+toPrepend+"\"} NR!=1' " + sad->path+"_tmp.php");

    // execute php
    string folder = getFolderName(sad->path);
    string file = getFileName(sad->path, true) + "_tmp.php";
    string cmd = "cd "+folder+" ; php "+file+" "+sad->paramsString;
    string res = systemCall(cmd);
    //cout << "processPHP: " << cmd << endl << res << endl;
    systemCall("rm "+sad->path+"_tmp.php" );
    return subString( res, 0, res.size()-1 );
}

static void server_answer_to_connection_m(struct mg_connection *conn, int ev, void *ev_data) {
    //VRLog::setTag("net",1);
    bool v = VRLog::tag("net");
    if (v) {
        if (ev == MG_EV_CONNECT) { VRLog::log("net", "MG_EV_CONNECT\n"); return; }
        if (ev == MG_EV_RECV) { VRLog::log("net", "MG_EV_RECV\n"); return; }
        if (ev == MG_EV_POLL) { return; }
        if (ev == MG_EV_ACCEPT) { VRLog::log("net", "MG_EV_ACCEPT\n"); return; }
        if (ev == MG_EV_SEND) { VRLog::log("net", "MG_EV_SEND\n"); return; }
        if (ev == MG_EV_TIMER) { VRLog::log("net", "MG_EV_TIMER\n"); return; }
        if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE) { VRLog::log("net", "MG_EV_WEBSOCKET_HANDSHAKE_DONE\n"); return; }
        if (ev == MG_EV_WEBSOCKET_CONTROL_FRAME) { VRLog::log("net", "MG_EV_WEBSOCKET_CONTROL_FRAME\n"); return; }
    }

    HTTP_args* sad = (HTTP_args*) conn->mgr->user_data;

    if (ev == MG_EV_CLOSE) {
        VRLog::log("net", "MG_EV_CLOSE\n");
        //cout << "MG_EV_CLOSE\n" << endl;
        //HTTP_args* sad = (HTTP_args*) conn->mgr_data;
        if (sad->serv->websocket_ids.count(conn)) {
            int wsid = sad->serv->websocket_ids[conn];
            sad->serv->websockets.erase(wsid);
            sad->serv->websocket_ids.erase(conn);
            if (sad->serv->ws_groups.count(wsid)) sad->serv->ws_groups.erase(wsid);
        }
        return;
    }

    if (ev == MG_EV_WEBSOCKET_FRAME) {
        sad->websocket = true;
        sad->path = "";
        sad->params->clear();

        VRLog::log("net", "MG_EV_WEBSOCKET_FRAME\n");
        if (v) sad->print();

        struct websocket_message* wm = (struct websocket_message*) ev_data;
        sad->ws_data = string(reinterpret_cast<char const*>(wm->data), wm->size);

        if (!sad->serv->websocket_ids.count(conn)) {
            static size_t ID = 0; ID++;
            sad->serv->websocket_ids[conn] = ID;
            sad->serv->websockets[ID] = conn;
        }
        sad->ws_id = sad->serv->websocket_ids[conn];
        if ( startsWith(sad->ws_data, "register ") ) sad->serv->ws_groups[sad->ws_id] = splitString(sad->ws_data, ' ')[1];
        else if ( startsWith(sad->ws_data, "register|") ) sad->serv->ws_groups[sad->ws_id] = splitString(sad->ws_data, '|')[1];

        auto fkt = VRUpdateCb::create("HTTP_answer_job", bind(server_answer_job, sad->copy()));
        VRSceneManager::get()->queueJob(fkt);
        return;
    }

    if (ev == MG_EV_HTTP_REQUEST) {
        VRLog::log("net", "MG_EV_HTTP_REQUEST\n");
        //HTTP_args* sad = (HTTP_args*) conn->mgr_data;
        sad->websocket = false;

        struct http_message *hm = (struct http_message *)ev_data;
        string method_s(hm->method.p, hm->method.len);//GET, POST, ...
        string section(hm->uri.p+1, hm->uri.len-1); //path
        sad->path = section;
        sad->params->clear();

        string params;
        if (hm->query_string.p) params = string(hm->query_string.p, hm->query_string.len);
        sad->paramsString = params;
        for (auto pp : splitString(params, '&')) {
            vector<string> d = splitString(pp, '=');
            if (d.size() != 2) continue;
            (*sad->params)[d[0]] = d[1];
        }

        if (v) VRLog::log("net", "HTTP Request\n");
        if (v) sad->print();

        auto sendString = [&](string data, int code = 200) {
            mg_send_head(conn, code, data.size(), "Transfer-Encoding: chunked");
            mg_send_http_chunk(conn, data.c_str(), data.size());
            mg_send_http_chunk(conn, "", 0);
        };

        //--- respond to client ------
        if (sad->path == "") {
            if (v) VRLog::log("net", "Send empty string\n");
            sendString("");
        }

        if (sad->path != "") {
            if (sad->pages->count(sad->path)) { // return local site
                string spage = (*sad->pages)[sad->path];
                int port_server1 = 5500;
                if (auto setup = VRSetup::getCurrent())
                    if (auto server = dynamic_pointer_cast<VRServer>(setup->getDevice("server1")))
                        port_server1 = server->getPort();
                spage = std::regex_replace(spage, std::regex("\\$PORT_server1\\$"), toString(port_server1));
                sendString(spage);
                if (v) VRLog::log("net", "Send local site\n");
            } else if(sad->callbacks->count(sad->path)) { // return callback
                VRServerCbPtr cb = (*sad->callbacks)[sad->path].lock();
                if (cb) {
                    string res = (*cb)(*sad->params);
                    sendString(res);
                    if (v) VRLog::log("net", "Send callback response\n");
                }
            } else { // return ressources
                bool doRootSrv = false;

                if (!exists( sad->path )) { // test for path to PolyServ
                    if (startsWith(sad->path, "ressources/PolyServ")) {
                        string D = VRSceneManager::get()->getOriginalWorkdir();
                        sad->path = D+"/"+sad->path;
                        doRootSrv = true;
                    }
                }

                if (!exists( sad->path )) {
                    if (v) VRLog::wrn("net", "Did not find ressource: " + sad->path + "\n");
                    if (v) VRLog::log("net", "Send empty string\n");
                    sendString("<head><style>body{background:#f0f;color:white;font-size:20vh;font-weight:bold;display:flex;justify-content:center;align-items:center;width:100vw;height:100vh;margin:0;}</style></head><body>Not Found!</body>", 404);
                }
                else {
                    if (endsWith(sad->path, ".php", false)) {
                        if (v) VRLog::log("net", "Serve PHP\n");
                        sendString( processPHP(sad) );
                    } else {
                        if (v) VRLog::log("net", "Serve ressource "+sad->path+"\n");
                        if (!doRootSrv) mg_serve_http(conn, hm, s_http_server_opts);
                        else sendString(readFileContent(sad->path));
                        // mg_http_serve_file
                        return;
                    }
                }
            }
        }

        //--- process request --------
        auto fkt = VRUpdateCb::create("HTTP_answer_job", bind(server_answer_job, sad->copy()));
        VRSceneManager::get()->queueJob(fkt);
        return;
    }

    VRLog::log("net", "unhandled event: " + toString(ev));
}
}


VRSocket::VRSocket(string name) {
    http_fkt = 0;
    socketID = 0;
    run = false;
    http_args = 0;
    http_serv = 0;

    setOverrideCallbacks(true);
    queued_signal = VRUpdateCb::create("signal_trigger", bind(&VRSocket::trigger, this));
    sig = VRSignal::create();
    setNameSpace("Sockets");
    setName(name);
    http_serv = new HTTPServer();

    store("type", &type);
    store("port", &port);
    store("ip", &IP);
    store("signal", &signal);
}

VRSocket::~VRSocket() {
    cout << " VRSocket::~VRSocket " << name << ", " << run << endl;
    run = false;
    cout << "  http_serv->close" << endl;
    if (http_serv) http_serv->close();
    //shutdown(socketID, SHUT_RDWR);
    cout << "  sm->stopThread " << threadID << endl;
    if (auto sm = VRSceneManager::get()) sm->stopThread(threadID);
    cout << "  delete http_args" << endl;
    if (http_args) delete http_args;
    cout << "  delete http_serv" << endl;
    if (http_serv) delete http_serv;
    cout << "  ~VRSocket done" << endl;
}

std::shared_ptr<VRSocket> VRSocket::create(string name) { return std::shared_ptr<VRSocket>(new VRSocket(name)); }

map<string, vector<int>> VRSocket::getClients() { return http_serv ? http_serv->getClients() : map<string, vector<int>>(); }

int VRSocket::openWebSocket(string address, string protocols) {
    if (http_serv) return http_serv->websocket_open(address, protocols);
    return -1;
}

void VRSocket::answerWebSocket(int id, string msg) {
    if (http_serv) http_serv->websocket_send(id, msg);
}

void VRSocket::trigger() {
    if (auto f = tcp_fkt.lock()) (*f)(tcp_msg);
    if (http_fkt) (*http_fkt)(http_args);
}

void VRSocket::handle(string s) {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    tcp_msg = s;
    scene->queueJob(queued_signal);
}

//CURL HTTP client--------------------------------------------------------------
namespace OSG {
size_t httpwritefkt( char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* s = (string*)userdata;
    s->append(ptr, size*nmemb);
    return size*nmemb;
}
}

void VRSocket::update() {
    run = false;
    //shutdown(socketID, SHUT_RDWR);
    if (auto sm = VRSceneManager::get()) sm->stopThread(threadID);
    if (http_serv) http_serv->close();

    sig->setName("on_" + name + "_" + type);

    if (type == "http receive" && http_serv && http_fkt) {
        http_serv->initServer(http_fkt, port);
        port = http_serv->port;
    }
}

bool VRSocket::isClient() {
    if (type == "tcpip send") return true;
    if (type == "http post") return true;
    if (type == "http get") return true;
    return false;
}

void VRSocket::setName(string n) { name = n; update(); }
void VRSocket::setType(string t) { type = t; update(); }
void VRSocket::setTCPCallback(VRMessageCbPtr cb) { tcp_fkt = cb; update(); }
void VRSocket::setHTTPCallback(VRHTTP_cb* cb) { http_fkt = cb; update(); }
void VRSocket::setIP(string s) { IP = s; }
void VRSocket::setSignal(string s) { signal = s; update(); }
void VRSocket::setPort(int i) { port = i; update(); }
void VRSocket::unsetCallbacks() { tcp_fkt.reset(); http_fkt = 0; update(); }
void VRSocket::addHTTPPage(string path, string page) { if (http_serv) http_serv->addPage(path, page); }
void VRSocket::remHTTPPage(string path) { if (http_serv) http_serv->remPage(path); }
void VRSocket::addHTTPCallback(string path, VRServerCbPtr cb) { if (http_serv) http_serv->addCallback(path, cb); }
void VRSocket::remHTTPCallback(string path) { if (http_serv) http_serv->remCallback(path); }

string VRSocket::getType() { return type; }
string VRSocket::getIP() { return IP; }
string VRSocket::getCallback() { return callback; }
VRSignalPtr VRSocket::getSignal() { return sig; }
int VRSocket::getPort() { return port; }

bool VRSocket::ping(string IP, string port) {
#ifndef __EMSCRIPTEN__
    VRPing ping;
    return ping.start(IP, port, 0);
#else
    return false;
#endif
}


