#include "VRMQTTClient.h"
#include "../mongoose/mongoose.h"
#include "core/utils/VRMutex.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"

#include <thread>

using namespace OSG;

struct VRMQTTClient::Data {
    VRMQTTClient* client = 0;
    VRMutex mtx;
    mg_mgr mgr;                // Event manager
    string s_url = "mqtt://broker.hivemq.com:1883";
    int s_qos = 0;                             // MQTT QoS
    bool doPoll = false;
    bool mqttConnected = false;
    bool connecting = false;
    bool responsive = false;
    bool gotReadEv = false;
    thread pollThread;
    mg_connection* s_conn = 0;              // Client connection
    function<string(string)> cb;

    string name;
    string pass;

    vector<vector<string>> jobQueue;
    vector<vector<string>> messages;

    Data() : mgr() {
        mg_mgr_init(&mgr); // Initialise event manager
    }

    ~Data() {
        //cout << "~Data" << endl;
        if (pollThread.joinable()) pollThread.join();
        mg_mgr_free(&mgr);
    }

    mg_mqtt_opts basicOpts() {
        mg_mqtt_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.qos = s_qos;
        opts.client_id = mg_str("pvrTest");
        opts.user = mg_str(name.c_str());
        opts.pass = mg_str(pass.c_str());
        return opts;
    }
};

VRMQTTClient::VRMQTTClient() : VRNetworkClient("mqttClient") {
    //cout << "VRMQTTClient::VRMQTTClient" << endl;
    data = shared_ptr<Data>(new Data());
    data->client = this;

    updateCb = VRUpdateCb::create("mqtt client update", [&](){ handleMessages(); });
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRMQTTClient::~VRMQTTClient() {
    //cout << "VRMQTTClient::~VRMQTTClient" << endl;
    disconnect();
}

VRMQTTClientPtr VRMQTTClient::create() { return VRMQTTClientPtr( new VRMQTTClient() ); }
VRMQTTClientPtr VRMQTTClient::ptr() { return dynamic_pointer_cast<VRMQTTClient>(shared_from_this()); }

void VRMQTTClient::onMessage( function<string(string)> f ) { data->cb = f; };

void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_POLL) {
        //cout << "   fn POLL, data: " << fn_data << endl;
        return;
    }

    VRMQTTClient::Data* data = (VRMQTTClient::Data*)fn_data;

    //cout << "   fn " << ev << ", data: " << data << endl;

    if (ev == MG_EV_OPEN) {
        //MG_INFO(("%lu CREATED", c->id));
        // c->is_hexdumping = 1;
    }

    if (ev == MG_EV_CONNECT) {
        if (mg_url_is_ssl(data->s_url.c_str())) {
            mg_tls_opts opts;
            opts.ca = mg_unpacked("/certs/ca.pem");
            opts.name = mg_url_host(data->s_url.c_str());
            mg_tls_init(c, &opts);
        }
    }

    if (ev == MG_EV_MQTT_OPEN) { // MQTT connect is successful
        MG_INFO(("%lu CONNECTED to host: %s", c->id, data->s_url.c_str()));
        data->mqttConnected = true;
        data->connecting = false;
    }

    if (ev == MG_EV_ERROR) { // On error
        string e = string((char*)ev_data);
        if (e == "socket timeout") return; // ignore timeout message
        MG_ERROR(("%lu ERROR %s", c->id, (char *) ev_data));
    }

    if (ev == MG_EV_MQTT_MSG) { // On message
        auto lock = VRLock(data->mtx);
        struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
        //MG_INFO(("%lu RECEIVED %.*s <- %.*s", c->id, (int) mm->data.len, mm->data.ptr, (int) mm->topic.len, mm->topic.ptr));
        data->messages.push_back( { string(mm->data.ptr, mm->data.len), string(mm->topic.ptr, mm->topic.len) } );
    }

    if (ev == MG_EV_CLOSE) {
        //MG_INFO(("%lu CLOSED", c->id));
        data->s_conn = NULL;  // Mark that we're closed
        data->doPoll = false;
        data->connecting = false;
        data->mqttConnected = false;
    }

    if (ev == MG_EV_READ) {
        data->gotReadEv = true;
    }
}

void VRMQTTClient::disconnect() {
    if (!data) return;
    if (!data->doPoll) return;

    data->doPoll = false;
    data->connecting = false;
    data->mqttConnected = false;

    auto f = VRUpdateCb::create("mqttThreadCleanup", bind([](shared_ptr<Data> h) { h.reset(); }, data));
    data.reset();

    auto s = VRScene::getCurrent();
    if (s) s->queueJob(f, 0, 2000);
}

void VRMQTTClient::connect(string host, int port) { // connect("broker.hivemq.com", 1883)
    if (data->connecting) return;
    if (data->mqttConnected) return;
    data->connecting = true;

    string address = "mqtt://"+host+":"+toString(port);
    data->s_url = address;

    mg_mqtt_opts opts = data->basicOpts();
    opts.clean = true;
    opts.version = 4;
    data->s_conn = mg_mqtt_connect(&data->mgr, data->s_url.c_str(), &opts, fn, data.get());

    data->doPoll = true;
    data->pollThread = thread( bind( [&](shared_ptr<Data> data) {
        bool doPing = false;
        int pingSent = 0;
        while (data->doPoll) {
            if (doPing) {
                mg_mqtt_ping(data->s_conn);
                pingSent++;
                doPing = false;
                data->gotReadEv = false;
            }

            VRTimer t;
            mg_mgr_poll(&data->mgr, 1000); // TODO: maybe decrease this to 2 ms?
            if (!data->doPoll) break;
            double T = t.stop();
            if (T > 500) doPing = true;

            if (pingSent > 1 && !data->gotReadEv) data->responsive = false;
            if (data->gotReadEv) { data->responsive = true; pingSent = 0; data->gotReadEv = false; }

            //cout << "mqtt ping " << T << ", pingSent " << pingSent << ", responsive " << data->responsive << endl;
            processJobs();
        }
    }, data ) );
}

bool VRMQTTClient::connected() {
    if (!data) return false;
    return data->responsive;
}

void VRMQTTClient::handleMessages() {
    vector<vector<string>> mcopy;

    {
        if (!data) return;
        auto lock = VRLock(data->mtx);
        if (!data) return;
        if (!data->cb) return;
        if (!data->messages.size()) return;
        mcopy = data->messages; // copy to avoid holding lock when calling cb
        data->messages.clear();
    }

    for (auto& m : mcopy) {
        data->cb(m[1] + "|" + m[0]);
    }
}

void VRMQTTClient::processJobs() {
    auto lock = VRLock(data->mtx);
    if (!data->mqttConnected) return;

    for (auto job : data->jobQueue) {
        if (job[0] == "subscribe") {
            mg_mqtt_opts opts = data->basicOpts();
            opts.topic = mg_str(job[1].c_str());
            opts.retain = bool(job[2] == "1");
            mg_mqtt_sub(data->s_conn, &opts);
        }

        if (job[0] == "publish") {
            mg_mqtt_opts opts = data->basicOpts();
            opts.topic = mg_str(job[1].c_str());
            opts.message = mg_str(job[2].c_str());
            opts.retain = false;
            mg_mqtt_pub(data->s_conn, &opts);
        }
    }

    data->jobQueue.clear();
}

void VRMQTTClient::setAuthentication(string name, string pass) { auto lock = VRLock(data->mtx); data->name = name; data->pass = pass; }
void VRMQTTClient::publish(string topic, string message) { auto lock = VRLock(data->mtx); data->jobQueue.push_back({"publish", topic, message}); }

void VRMQTTClient::subscribe(string topic, bool retain) {
    auto lock = VRLock(data->mtx);
    data->jobQueue.push_back( { "subscribe", topic, string( retain ? "1" : "0" ) } );
}







