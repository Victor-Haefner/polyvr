#include "VRRestServer.h"
#include "VRRestResponse.h"
#include "../mongoose/mongoose.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"

#include <thread>

using namespace OSG;

struct VRRestServer::Data {
    mg_mgr mgr;                // Event manager
    thread pollThread;
    bool doPoll = true;

    Data() { mg_mgr_init(&mgr); }

    ~Data() {
        doPoll = false;
        pollThread.join();
        mg_mgr_free(&mgr);
    }
};

VRRestServer::VRRestServer(string n) : name(n) {
    data = new Data();
}

VRRestServer::~VRRestServer() {
    if (data) delete data;
}

VRRestServerPtr VRRestServer::create(string name) { return VRRestServerPtr( new VRRestServer(name) ); }
VRRestServerPtr VRRestServer::ptr() { return static_pointer_cast<VRRestServer>(shared_from_this()); }

VRRestServer::Data* VRRestServer::getData() { return data; }

void VRRestServer::onMessage(void* connection, VRRestResponsePtr msg) {
    auto c = (mg_connection*)connection;
    if (!callback) { sendString(c, ""); return; }
    (*callback)(msg);
    mg_http_reply(c, msg->getStatus(), "", msg->getData().c_str()); // passing headers can lead to issues
}

void VRRestServer::sendString(void* connection, string data, int code) {
    auto c = (mg_connection*)connection;
    mg_http_reply(c, code, "", data.c_str());
}

static void VRRestServer_handler(mg_connection* connection, int ev, void* ev_data, void* s) {
    VRRestServer* server = (VRRestServer*)s;

    if (ev == MG_EV_HTTP_MSG) {
        mg_http_message* hm = (mg_http_message*)ev_data;
        string headers(hm->head.ptr, hm->head.len);
        string body(hm->body.ptr, hm->body.len);

        VRRestResponsePtr msg = VRRestResponse::create();
        msg->setHeaders(headers);
        msg->setData(body);

        auto fkt = VRUpdateCb::create("VRRestServer_handler", bind(&VRRestServer::onMessage, server, (void*)connection, msg));
        auto s = VRScene::getCurrent();
        if (s) s->queueJob(fkt);
        else mg_http_reply(connection, 200, "", "");
    }
}

void VRRestServer::listen(int port, VRRestCbPtr cb) {
    cout << "listen on port " << port << endl;

    callback = cb;
    string addr = ":"+toString(port);
    mg_http_listen(&data->mgr, addr.c_str(), VRRestServer_handler, this);
    data->doPoll = true;

    data->pollThread = thread( [&]() {
        while (data->doPoll) {
            mg_mgr_poll(&data->mgr, 2);
            if (!data->doPoll) break;
        }
    });
}
