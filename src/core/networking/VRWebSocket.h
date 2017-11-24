#pragma once

#include <OpenSG/OSGConfig.h>
#include <core/utils/VRName.h>
#include <boost/thread/thread.hpp>
#include "mongoose/mongoose_67.h"
#include <jsoncpp/json/json.h>

OSG_BEGIN_NAMESPACE

class VRWebSocket : public VRName {
    public:
        VRWebSocket(string name = "websocket");
        ~VRWebSocket();

        bool open(string url);
        bool close();

        bool sendMessage(string message);
        void registerStringCallback(std::function<void(string)> func);
        void registerJsonCallback(std::function<void(Json::Value)> func);

    private:
        static void eventHandler(struct mg_connection* nc, int ev, void* ev_data);

        void poll(VRThreadWeakPtr t);
        void processFrame(string frameData);

        VRThreadCbPtr threadFkt;
        int threadId{-1};

        vector<std::function<void(string)>> stringCallbacks;
        vector<std::function<void(Json::Value)>> jsonCallbacks;

        struct mg_mgr mgr;
        struct mg_connection* connection{nullptr};

        Json::Reader reader;

        bool isConnected{false};
        bool done{false};
};

OSG_END_NAMESPACE
