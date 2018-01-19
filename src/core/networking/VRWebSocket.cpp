#include <core/utils/VRFunction.h>
#include <core/scene/VRScene.h>
#include "core/scene/VRSceneManager.h"
#include "VRWebSocket.h"

OSG_BEGIN_NAMESPACE
using namespace std;


VRWebSocket::VRWebSocket(string name) : VRName() {
    setName(name);
    mg_mgr_init(&mgr, (void*) this);
}

VRWebSocket::~VRWebSocket() {
    done = true;
    close();
}

bool VRWebSocket::open(string url) {
    connection = mg_connect_ws(&mgr, eventHandler, url.c_str(), NULL, NULL);
    if (connection == nullptr) {
        cerr << "Cannot connect to " << url << endl;
        return false;
    }

    if (threadId < 0) {
        threadFkt = VRThreadCb::create("webSocketPollThread", boost::bind(&VRWebSocket::poll, this, _1));
        threadId = VRSceneManager::get()->initThread(threadFkt, "webSocketPollThread", false);
    }

    return true;
}

void VRWebSocket::poll(VRThreadWeakPtr t) {
    while (!done) {
        mg_mgr_poll(&mgr, 30);
    }
    mg_mgr_free(&mgr);
}

bool VRWebSocket::close() {
    cout << "Closing socket" << endl;
    if (connection) {
        mg_send_websocket_frame(connection, WEBSOCKET_OP_CLOSE, nullptr, 0);
        return true;
    }

    return false;
}

bool VRWebSocket::sendMessage(string message) {

    if (connection) {
        mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, message.c_str(), message.length());
        return true;
    } else {
        cerr << "Not connected to WebSocket Server." << endl;
        return false;
    }
}

void VRWebSocket::processFrame(string frameData) {
    for (auto& sc : stringCallbacks) {
        sc(frameData);
    }
    for (auto& jc : jsonCallbacks) {
        Json::Value root;
        reader.parse(frameData, root);
        jc(root);
    }
}

void VRWebSocket::registerStringCallback(function<void(string)> func) {
    stringCallbacks.push_back(func);
}

void VRWebSocket::registerJsonCallback(function<void(Json::Value)> func) {
    jsonCallbacks.push_back(func);
}

void VRWebSocket::eventHandler(struct mg_connection* nc, int ev, void* ev_data) {

    VRWebSocket* object = (VRWebSocket*) nc->mgr->user_data;

    switch (ev) {
        case MG_EV_CONNECT: {
            int status = *((int*) ev_data);
            if (status != 0) {
                printf("-- Connection error: %d\n", status);
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            printf("-- Connected\n");
            object->isConnected = true;
            break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message* wm = (struct websocket_message*) ev_data;
            object->processFrame(string(reinterpret_cast<char const*>(wm->data), wm->size));
            break;
        }
        case MG_EV_CLOSE: {
            if (object->isConnected) printf("-- Disconnected\n");
            object->done = true;
            break;
        }
    }
}


OSG_END_NAMESPACE
