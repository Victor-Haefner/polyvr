#ifndef VRMQTTClient_H_INCLUDED
#define VRMQTTClient_H_INCLUDED

#include "../VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>
#include <OpenSG/OSGConfig.h>
#include "../VRNetworkClient.h"
#include "core/utils/VRMutex.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMQTTClient : public VRNetworkClient {
    public:
        struct Data;

    private:
        shared_ptr<Data> data;
        VRUpdateCbPtr updateCb;

        void handleMessages();

    public:
        VRMQTTClient();
        ~VRMQTTClient();

        static VRMQTTClientPtr create();
        VRMQTTClientPtr ptr();

        void disconnect();
        void connect(string host, int port) override;
        bool isConnected(string host, int port) override;
        void onMessage( function<string(string)> f ) override;

        bool connected();

        void setAuthentication(string name, string password, string clientID = "pvr");
        void subscribe(string topic, bool retain = false);
        void publish(string topic, string message);
};

OSG_END_NAMESPACE;

#endif // VRMQTTClient_H_INCLUDED
