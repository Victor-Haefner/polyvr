#include <core/utils/VRFunction.h>
#include <core/scene/VRScene.h>
#include <functional>
#include "core/utils/Thread.h"
#include "core/scene/VRSceneManager.h"
#include "VRWebSocket.h"

OSG_BEGIN_NAMESPACE
using namespace std;


VRWebSocket::VRWebSocket(string name) : VRName() {
    setName(name);
    threadFkt = VRThreadCb::create("webSocketPollThread", bind(&VRWebSocket::poll, this, placeholders::_1));
}

VRWebSocket::~VRWebSocket() {
    cout << "~VRWebSocket::VRWebSocket" << endl;
    done = true;
    close();
}

VRWebSocketPtr VRWebSocket::create(string name) { return VRWebSocketPtr( new VRWebSocket(name) ); }

bool VRWebSocket::open(string url) {
    cout << "VRWebSocket::open " << url << endl;
    close();

    mg_mgr_init(&mgr);

    connection = mg_ws_connect(&mgr, url.c_str(), eventHandler, this, 0);
    if (connection == nullptr) {
        cerr << "Cannot connect to " << url << endl;
        return false;
    }
    if (!connection->is_websocket) {
        cerr << "Error, '" << url << "' is not a websocket!" << endl;
        return false;
    }

    done = false;
    if (threadId < 0) threadId = VRSceneManager::get()->initThread(threadFkt, "webSocketPollThread", false);

    cout << "Connecting to " << url << endl;
    while (connectionStatus < 0) {
		this_thread::sleep_for(chrono::microseconds(10000));
	}

    return connectionStatus;
}

void VRWebSocket::poll(VRThreadWeakPtr t) {
    while (!done) mg_mgr_poll(&mgr, 30);
    mg_mgr_free(&mgr);
    threadId = -1;
}

/**
* Send a close request to the web socket and waits for closing confirmation.
* TODO: Cancels waiting after 1 second to avoid freezing when confirmation fails.
*/
bool VRWebSocket::close() {
    if (isConnected()) {
        cout << "Closing socket: " << this << endl;
        //mg_send_websocket_frame(connection, WEBSOCKET_OP_CLOSE, nullptr, 0);
        mg_ws_send(connection, 0, 0, WEBSOCKET_OP_CLOSE);

        // TODO: output only for testing purposes. Remove after closing of web sockets is fixed.
        int i = 0;
        cout << "Waiting for confirmation";
        while (connectionStatus != -1 && i < 100) {
			cout << ".";
			i++;
			this_thread::sleep_for(chrono::microseconds(10000));
		}
        if(i==100) cout << endl << "No closing confirmation for web socket. Canceling wait period.";
        cout << endl;
        connectionStatus = -1;
        return true;
    } else {
        done = true;
        connectionStatus = -1;
        return false;
    }
}

bool VRWebSocket::isConnected() {
    if (connectionStatus == 1) return true;
    return false;
}

bool VRWebSocket::sendMessage(string message) {
    cout << "VRWebSocket::sendMessage N " << message.size() << endl;
    if (isConnected()) {
        mg_ws_send(connection, message.c_str(), message.length(), WEBSOCKET_OP_TEXT);
        cout << " VRWebSocket::sendMessage N " << message.size() << " done" << endl;
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
#ifndef WITHOUT_JSONCPP
    for (auto& jc : jsonCallbacks) {
        Json::Value root;
        reader.parse(frameData, root);
        jc(root);
    }
#endif
}

void VRWebSocket::registerStringCallback(function<void(string)> func) {
    stringCallbacks.push_back(func);
}

#ifndef WITHOUT_JSONCPP
void VRWebSocket::registerJsonCallback(function<void(Json::Value)> func) {
    jsonCallbacks.push_back(func);
}
#endif

void VRWebSocket::eventHandler(struct mg_connection* nc, int ev, void* ev_data, void* user_data) {
    VRWebSocket* object = (VRWebSocket*)user_data;
    struct mg_http_message* hm = 0;
    struct mg_ws_message* wm = 0;
    int status = 0;

    switch (ev) {
        case MG_EV_HTTP_MSG:
            hm = (struct mg_http_message *) ev_data;
            if (mg_http_match_uri(hm, "/websocket")) mg_ws_upgrade(nc, hm, NULL);
            else if (mg_http_match_uri(hm, "/rest")) mg_http_reply(nc, 200, "", "{\"result\": \"unsupported request\"}\n"); // TODO: handle rest request??
            else { // TODO serving static files ?
              //struct mg_http_serve_opts opts = {.root_dir = s_web_root};
              //mg_http_serve_dir(nc, ev_data, &opts);
            }

        case MG_EV_CONNECT:
            status = *((int*) ev_data);
            if (status != 0) {
                printf("-- Connection error: %d\n", status);
                object->connectionStatus = 0;
                object->done = true;
            }
            break;

        case MG_EV_WS_OPEN:
            printf("-- Connected\n");
            object->connectionStatus = 1;
            break;

        case MG_EV_WS_MSG:
            wm = (struct mg_ws_message*) ev_data;
            object->processFrame(string(reinterpret_cast<char const*>(wm->data.ptr), wm->data.len));
            break;

        case MG_EV_CLOSE:
            if (object->isConnected()) {
                printf("-- Disconnected\n");
                object->connectionStatus = -1;
            }
            object->done = true;
            break;
    }
}


OSG_END_NAMESPACE
