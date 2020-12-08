#ifndef VRSOCKET_H_INCLUDED
#define VRSOCKET_H_INCLUDED

#include <string.h>
#include <memory>
#include <OpenSG/OSGConfig.h>
#include "VRNetworkingFwd.h"
#include "core/utils/VRName.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE
using namespace std;

class VRThread;
class HTTPServer;
//struct HTTP_args;

typedef VRFunction<void*> VRHTTP_cb;

struct HTTP_args {
    HTTPServer* serv = 0;
    VRHTTP_cb* cb = 0;
    std::shared_ptr< map<string, string> > params;
    std::shared_ptr< map<string, string> > pages;
    std::shared_ptr< map<string, VRServerCbWeakPtr> > callbacks;
    string path;
    bool websocket = false;
    string ws_data;
    int ws_id = -1;

    HTTP_args();
    ~HTTP_args();
    void print();
    HTTP_args* copy();
};

class VRSocket : public VRName {
    public:
        enum CONNECTION_TYPE {UNIX, TCP, HTTP};

    private:
        VRUpdateCbPtr queued_signal;
        VRThreadCbPtr socketThread;
        string tcp_msg;
        HTTP_args* http_args;
        VRSignalPtr sig;
        int port = -1;
        string IP;
        string type;
        string callback;
        string signal;
        int threadID = -1;
        bool run = false;
        unsigned int socketID = 0;
        VRMessageCbWeakPtr tcp_fkt;
        VRHTTP_cb* http_fkt = 0;
        HTTPServer* http_serv = 0;

        void trigger();

        void handle(string s);

        void scanUnix(VRThreadWeakPtr t);
        void scanTCP(VRThreadWeakPtr t);
        void scanHTTP();

        void update();

    public:
        VRSocket(string name = "socket");
        virtual ~VRSocket();
        static VRSocketPtr create(string name = "socket");

        void initServer(CONNECTION_TYPE t, int _port);
        void sendHTTPGet(string uri);

        void setName(string s);
        void setType(string s);
        void setIP(string s);
        void setTCPCallback(VRMessageCbPtr cb);
        void setHTTPCallback(VRHTTP_cb* cb);
        void unsetCallbacks();
        void setSignal(string s);
        void setPort(int i);
        void addHTTPPage(string path, string page);
        void remHTTPPage(string path);
        void addHTTPCallback(string path, VRServerCbPtr cb);
        void remHTTPCallback(string path);

        int openWebSocket(string address, string protocols);
        void answerWebSocket(int id, string msg);
        map<string, vector<int>> getClients();

        string getType();
        string getIP();
        string getCallback();
        VRSignalPtr getSignal();
        int getPort();

        bool ping(string IP, string port);

        bool isClient();
};

OSG_END_NAMESPACE

#endif // VRSOCKET_H_INCLUDED
