#include "VRSocket.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <fcntl.h>
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE
using namespace std;


//MICROHTTPD server-------------------------------------------------------------

void server_answer_job(HTTP_args* args, int i) {
    //cout << "server_answer_job: " << i << endl;
    (*args->cb)(args);
}

int server_parseURI(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    map<string, string>* uri_map = (map<string, string>*)cls;
    (*uri_map)[string(key)] = string(value);
    //printf ("GET %s: %s\n", key, value);
    return MHD_YES;
}

int server_parseFORM(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
    map<string, string>* uri_map = (map<string, string>*)cls;
    (*uri_map)[string(key)] = string(data);
    //printf ("POST %s: %s\n", key, data);
    return MHD_YES;
}

int server_answer_to_connection (void* param, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **opt) {
    HTTP_args* sad = (HTTP_args*) param;
    string method_s(method);//GET, POST, ...
    string section(url+1); //path
    sad->path = section;
    sad->params->clear();

    if (method_s == "GET")
        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, server_parseURI, sad->params);//Parse URI parameter

    if (method_s == "POST")
        MHD_get_connection_values(connection, MHD_POSTDATA_KIND, server_parseURI, sad->params);//Parse URI parameter

    /*if (method_s == "POST") {
        struct MHD_PostProcessor* pp = (MHD_PostProcessor*)(*opt);
        if (pp == NULL) {
            sad->params->clear();
            pp = MHD_create_post_processor(connection, 1024, server_parseFORM, sad->params);
            *opt = pp;
            return MHD_YES;
        }
        if (*upload_data_size) {
            MHD_post_process(pp, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            MHD_destroy_post_processor(pp);
        }
    }*/

    //cout << "HTTP: " << method_s << endl;

    //--- process request --------
    VRFunction<int>* _fkt = new VRFunction<int>("HTTP_answer_job", boost::bind(server_answer_job, sad, _1));
    VRSceneManager::get()->queueJob(_fkt);

    //--- respond to client ------
    struct MHD_Response* response = 0;

    if (sad->pages->count(sad->path)) { // return local site
        string spage = *(*sad->pages)[sad->path];
        response = MHD_create_response_from_data (spage.size(), (void*) spage.c_str(), MHD_NO, MHD_YES);
    } else { // return ressources
        struct stat sbuf;
        int fd = open(sad->path.c_str(), O_RDONLY);
        if (fstat (fd, &sbuf) != 0) cout << "Did not find ressource: " << sad->path << endl;
        else response = MHD_create_response_from_fd_at_offset (sbuf.st_size, fd, 0);
    }

    //--- send response ----------
    int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
}

void HTTP_args::print() {
    cout << "\nHTTP: " << path << endl;
    map<string, string>::iterator itr;
    for (itr = params->begin(); itr != params->end(); itr++) cout << "  " << itr->first << " : " << itr->second << endl;
}

class HTTPServer {
    public:
        //server----------------------------------------------------------------
        struct MHD_Daemon* server;
        HTTP_args* data;

        HTTPServer() {
            server = 0;
            data = new HTTP_args();
            data->params = new map<string, string>();
            data->pages = new map<string, string*>();
        }

        ~HTTPServer() {
            for (auto itr = data->pages->begin(); itr != data->pages->end(); itr++) delete itr->second;
            delete data->params;
            delete data->pages;
            delete data;
        }

        void initServer(VRHTTP_cb* fkt, int port) {
            data->cb = fkt;
            server = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL, &server_answer_to_connection, data, MHD_OPTION_END);
        }

        void addPage(string path, string page) {
            if (data->pages->count(path) == 0) (*data->pages)[path] = new string();
            *(*data->pages)[path] = page;
        }

        void remPage(string path) {
            if (data->pages->count(path)) {
                delete (*data->pages)[path];
                data->pages->erase(path);
            }
        }

        void close() {
            if (server) MHD_stop_daemon(server);
            server = 0;
        }
};


VRSocket::VRSocket(string _name) {
    tcp_fkt = 0;
    http_fkt = 0;
    socketID = 0;
    run = false;
    http_args = 0;
    http_serv = 0;

    queued_signal = new VRFunction<int>("signal_trigger", boost::bind(&VRSocket::trigger, this));
    sig = new VRSignal();
    setName(_name);
    http_serv = new HTTPServer();
}

VRSocket::~VRSocket() {
    run = false;
    shutdown(socketID, SHUT_RDWR);
    VRSceneManager::get()->stopThread(threadID);
    delete sig;
    delete queued_signal;
    if (http_args) delete http_args;
    if (http_serv) delete http_serv;
}

void VRSocket::trigger() {
    if (tcp_fkt) (*tcp_fkt)(tcp_msg);
    if (http_fkt) (*http_fkt)(http_args);
}

void VRSocket::handle(string s) {
    VRScene* scene = VRSceneManager::get()->getActiveScene();
    tcp_msg = s;
    scene->queueJob(queued_signal);
}

void VRSocket::sendMessage(string msg) {
    cout << "\nSOCKET SEND " << msg << endl;

    unsigned int sockfd, n;
    struct sockaddr_in serv_addr;
    struct sockaddr* serv;
    struct hostent *server;

    //char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("ERROR opening socket"); return; }

    server = gethostbyname(IP.c_str());
    if (server == NULL) { fprintf(stderr,"ERROR, no such host\n"); return; }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    //fcntl(sockfd, F_SETFL, O_NONBLOCK);
    serv = (struct sockaddr*)&serv_addr;
    if (connect(sockfd, serv, sizeof(serv_addr)) < 0) { perror("ERROR connecting"); return; }

    n = write(sockfd, msg.c_str(), msg.size());
    if (n < 0) { perror("ERROR writing to socket"); return; }

    close(sockfd);

    /* Now read server response */
    //n = read(sockfd, buffer, 255);
    //if (n < 0) { perror("ERROR reading from socket"); return; }
    //printf("%s\n",buffer);
}

void VRSocket::scanUnix(VRThread* thread) {
    //scan what new stuff is in the socket
    unsigned int s, len, contype;
    struct sockaddr_un local_u, remote_u;
    struct sockaddr* local;
    struct sockaddr* remote;
    unsigned int t;

    contype = AF_UNIX;

    int _s;
    if ((_s = socket(contype, SOCK_STREAM, 0)) < 0) { cout << "\nERROR: socket\n"; return; }
    s = _s;

    ifconf ic;
    ioctl(s, SIOCGIFCONF, ic);
    IP = string(ic.ifc_buf);

    local_u.sun_family = AF_UNIX; // unix
    strcpy(local_u.sun_path, UNIX_SOCK_PATH); // unix
    unlink(local_u.sun_path); // unix
    len = strlen(local_u.sun_path) + sizeof(local_u.sun_family); // unix
    local = (struct sockaddr *)&local_u;
    remote = (struct sockaddr *)&remote_u;
    t = sizeof(remote_u);

    if (bind(s, local, len) == -1) { cout << "\nERROR: socket bind\n"; return; }
    if (listen(s, 5) == -1) { cout << "\nERROR: socket listen\n"; return; }

    string cmd;

    while(true) {
        //printf("\n\nWaiting for a connection...\n");
        if ((_s = accept(s, remote, &t)) < 0) { cout << "\nERROR: socket accept\n"; return; }
        socketID = _s;

        //const int soc_len = 8192;//auch im client socket muss die laenge passen!
        const int soc_len = 1024;
        char str[soc_len];
        int cmd_len;

        //printf("Connected.\n");

        cmd_len = recv(socketID, str, soc_len, 0);

        if (cmd_len < 0) cout << "\n recv error\n";
        if (cmd_len <= 0) return;

        cmd = string(str);
        handle(cmd);

        strcpy(str, cmd.c_str());
        //cout << "\nsend: " << str << flush;
        if ( send(socketID, str, strlen(str), 0) < 0) { perror("send"); return; }

        close(socketID);
    }
}

void VRSocket::scanTCP(VRThread* thread) {
    //scan what new stuff is in the socket
    unsigned int socketAcc, len, contype;
    struct sockaddr_in local_i, remote_i;
    struct sockaddr* local;
    struct sockaddr* remote;
    unsigned int t;

    contype = AF_INET;

    int s;
    if ((s = socket(contype, SOCK_STREAM, 0)) < 0) { cout << "\nERROR: socket\n"; return; }
    socketID = s;

    ifconf ic;
    ioctl(socketID, SIOCGIFCONF, ic);
    IP = string(ic.ifc_buf);

    local_i.sin_family = AF_INET;
    local_i.sin_port = htons( port );
    local_i.sin_addr.s_addr = INADDR_ANY;
    len = sizeof(local_i);
    local = (struct sockaddr *)&local_i;
    remote = (struct sockaddr *)&remote_i;
    t = sizeof(remote_i);

    if (bind(socketID, local, len) < 0) { cout << "\nERROR: socket bind\n"; return; }
    if (listen(socketID, 5) < 0) { cout << "\nERROR: socket listen\n"; return; }

    string cmd;

    while(run) {
        printf("\n\nWaiting for a connection...\n");
        if ((s = accept(socketID, remote, &t)) == -1) { cout << "\nERROR: socket accept\n"; break; }
        socketAcc = s;

        //const int soc_len = 8192;//auch im client socket muss die laenge passen!
        const int soc_len = 1024;
        char str[soc_len];
        int cmd_len;

        cmd_len = recv(socketAcc, str, soc_len, 0);
        cout << "\nSOCKET CONNECTED " << str << endl;

        if (cmd_len < 0) cout << "\n recv error\n";
        if (cmd_len <= 0) break;

        cmd = string(str);
        handle(cmd);

        strcpy(str, cmd.c_str());
        //cout << "\nsend: " << str << flush;
        if ( send(socketAcc, str, strlen(str), 0) < 0) { perror("send"); break; }

        close(socketAcc);
    }
    //close(sListen);
    //close(sBind);
    close(socketID);
}

void VRSocket::initServer(CONNECTION_TYPE t, int _port) {
    VRFunction<VRThread*>* socket = 0;
    port = _port;
    switch(t) {
        case UNIX:
            socket = new VRFunction<VRThread*>("UNIXSocket", boost::bind(&VRSocket::scanUnix, this, _1));
            break;
        case TCP:
            socket = new VRFunction<VRThread*>("TCPSocket", boost::bind(&VRSocket::scanTCP, this, _1));
            break;
        default:
            break;
    }

    run = true;
    threadID = VRSceneManager::get()->initThread(socket, "socket", true);
}


void VRSocket::save(xmlpp::Element* e) {
    e->set_attribute("type", type);
    stringstream ss; ss << port;
    e->set_attribute("port", ss.str());
    e->set_attribute("ip", IP);
    //e->set_attribute("callback", callback);
    e->set_attribute("signal", signal);
}

void VRSocket::load(xmlpp::Element* e) {
    setType( e->get_attribute("type")->get_value() );
    setPort( toInt(e->get_attribute("port")->get_value().c_str()) );
    setIP( e->get_attribute("ip")->get_value() );
    //setCallback( e->get_attribute("callback")->get_value() );
    setSignal( e->get_attribute("signal")->get_value() );
}

void VRSocket::update() {
    run = false;
    shutdown(socketID, SHUT_RDWR);
    VRSceneManager::get()->stopThread(threadID);
    if (http_serv) http_serv->close();

    sig->setName("on_" + name + "_" + type);

    if (type == "tcpip receive") if (tcp_fkt) initServer(TCP, port);
    if (type == "http receive") if (http_serv and http_fkt) http_serv->initServer(http_fkt, port);
}

bool VRSocket::isClient() {
    if (type == "tcpip send") return true;
    if (type == "http post") return true;
    if (type == "http get") return true;
    return false;
}

void VRSocket::setName(string n) { name = n; update(); }
void VRSocket::setType(string t) { type = t; update(); }
void VRSocket::setCallback(VRTCP_cb* cb) { tcp_fkt = cb; update(); }
void VRSocket::setCallback(VRHTTP_cb* cb) { http_fkt = cb; update(); }
void VRSocket::setIP(string s) { IP = s; }
void VRSocket::setSignal(string s) { signal = s; update(); }
void VRSocket::setPort(int i) { port = i; update(); }
void VRSocket::unsetCallbacks() { tcp_fkt = 0; http_fkt = 0; update(); }
void VRSocket::addHTTPPage(string path, string page) { if (http_serv) http_serv->addPage(path, page); }
void VRSocket::remHTTPPage(string path) { if (http_serv) http_serv->remPage(path); }

string VRSocket::getType() { return type; }
string VRSocket::getIP() { return IP; }
string VRSocket::getCallback() { return callback; }
VRSignal* VRSocket::getSignal() { return sig; }
int VRSocket::getPort() { return port; }


OSG_END_NAMESPACE
