#ifndef VRSOCKET_H_INCLUDED
#define VRSOCKET_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#include <algorithm>
#include <curl/curl.h>
#include <stdint.h>
#include <microhttpd.h>
#include <jsoncpp/json/json.h>

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRDevice.h"
#include "core/utils/VRName.h"

#define UNIX_SOCK_PATH "/tmp/vrf_soc"
#define HTTP_SOCK_ADD "141.3.150.20"

OSG_BEGIN_NAMESPACE
using namespace std;

class VRThread;
class HTTPServer;
struct HTTP_args;

typedef VRFunction<HTTP_args*> VRHTTP_cb;
typedef VRFunction<string> VRTCP_cb;

struct HTTP_args {
    VRHTTP_cb* cb;
    map<string, string>* params;
    map<string, string*>* pages;
    string path;

    void print();
};

class VRSocket : public VRName {
    public:
        enum CONNECTION_TYPE {UNIX, TCP, HTTP};

    private:
        VRFunction<int>* queued_signal;
        string tcp_msg;
        HTTP_args* http_args;
        VRSignal* sig;
        int port;
        string IP;
        string type;
        string callback;
        string signal;
        int threadID;
        bool run;
        unsigned int socketID;
        VRTCP_cb* tcp_fkt;
        VRHTTP_cb* http_fkt;
        HTTPServer* http_serv;

        void trigger();

        void handle(string s);

        void scanUnix(VRThread* t);
        void scanTCP(VRThread* t);
        void scanHTTP();

        void update();

    public:
        VRSocket(string name);
        ~VRSocket();

        void initServer(CONNECTION_TYPE t, int _port);
        void sendMessage(string msg);

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);

        void setName(string s);
        void setType(string s);
        void setIP(string s);
        void setCallback(VRTCP_cb* cb);
        void setCallback(VRHTTP_cb* cb);
        void unsetCallbacks();
        void setSignal(string s);
        void setPort(int i);
        void addHTTPPage(string path, string page);
        void remHTTPPage(string path);

        string getType();
        string getIP();
        string getCallback();
        VRSignal* getSignal();
        int getPort();

        bool isClient();
};

OSG_END_NAMESPACE

// refactoring

// common:
//      VRSocket::setVerbose(int i);

// client scenario:
//      res = VRSocket::connect(VRSocket::CONNECTION_TYPE, string IP, int port);

// server scenario:
//      VRSocket::initServer(VRSocket::CONNECTION_TYPE, int port, VRFunction* handler);

#endif // VRSOCKET_H_INCLUDED
