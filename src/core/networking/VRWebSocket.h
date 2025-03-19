#pragma once

#include "VRNetworkingFwd.h"
#include <functional>
#include <OpenSG/OSGConfig.h>
#include <core/utils/VRName.h>
#include "mongoose/mongoose.h"
#ifndef WITHOUT_JSONCPP
#include <json/json.h>
#endif

OSG_BEGIN_NAMESPACE

// under windows there is a macro that defines poll as WSAPoll ??
#ifdef poll
#undef poll
#endif

class VRWebSocket : public VRName {
    public:
        VRWebSocket(string name = "websocket");
        ~VRWebSocket();

        static VRWebSocketPtr create(string name = "websocket");

        bool open(string url);
        bool close();
        bool isConnected();

        bool send(string message);
        void registerStringCallback(std::function<void(string)> func);
#ifndef WITHOUT_JSONCPP
        void registerJsonCallback(std::function<void(Json::Value)> func);
#endif

    private:
        static void eventHandler(struct mg_connection* nc, int ev, void* ev_data, void* user_data);

        void poll(VRThreadWeakPtr t);
        void processFrame(string frameData);

        VRThreadCbPtr threadFkt;
        int threadId = -1;

        vector<std::function<void(string)>> stringCallbacks;

#ifndef WITHOUT_JSONCPP
        Json::Reader reader;
        vector<std::function<void(Json::Value)>> jsonCallbacks;
#endif

        struct mg_mgr mgr;
        struct mg_connection* connection{nullptr};

        //Json::Reader reader;

        int connectionStatus = -1; // -1 unset, 0 error, 1 ok
        bool done{true};
};

OSG_END_NAMESPACE
