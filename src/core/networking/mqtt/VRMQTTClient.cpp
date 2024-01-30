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
    thread pollThread;
    mg_connection* s_conn = 0;              // Client connection
    function<string(string)> cb;

    string name;
    string pass;

    vector<vector<string>> jobQueue;
    vector<vector<string>> messages;

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
    cout << "VRMQTTClient::VRMQTTClient" << endl;
    data = new Data();
    data->client = this;
    mg_mgr_init(&data->mgr); // Initialise event manager

    updateCb = VRUpdateCb::create("mqtt client update", [&](){ handleMessages(); });
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRMQTTClient::~VRMQTTClient() {
    cout << "VRMQTTClient::~VRMQTTClient" << endl;
    data->doPoll = false;
    data->pollThread.join();
    mg_mgr_free(&data->mgr); // Finished, cleanup
    if (data) delete data;
}

VRMQTTClientPtr VRMQTTClient::create() { return VRMQTTClientPtr( new VRMQTTClient() ); }
VRMQTTClientPtr VRMQTTClient::ptr() { return dynamic_pointer_cast<VRMQTTClient>(shared_from_this()); }

void VRMQTTClient::onMessage( function<string(string)> f ) { data->cb = f; };

void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    VRMQTTClient::Data* data = (VRMQTTClient::Data*)fn_data;

    if (ev == MG_EV_OPEN) {
        MG_INFO(("%lu CREATED", c->id));
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
    }

    if (ev == MG_EV_ERROR) { // On error
        MG_ERROR(("%lu ERROR %s", c->id, (char *) ev_data));
    }

    if (ev == MG_EV_MQTT_MSG) { // On message
        struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
        //MG_INFO(("%lu RECEIVED %.*s <- %.*s", c->id, (int) mm->data.len, mm->data.ptr, (int) mm->topic.len, mm->topic.ptr));
        data->messages.push_back( { string(mm->data.ptr, mm->data.len), string(mm->topic.ptr, mm->topic.len) } );
    }

    if (ev == MG_EV_CLOSE) {
        MG_INFO(("%lu CLOSED", c->id));
        data->s_conn = NULL;  // Mark that we're closed
        data->doPoll = false;
        //data->pollThread.join(); TODO: needs to into main thread?
    }
}

void VRMQTTClient::connect(string host, int port) { // connect("broker.hivemq.com", 1883)
    auto lock = VRLock(data->mtx);

    string address = "mqtt://"+host+":"+toString(port);
    data->s_url = address;

    mg_mqtt_opts opts = data->basicOpts();
    opts.clean = true;
    opts.version = 4;
    data->s_conn = mg_mqtt_connect(&data->mgr, data->s_url.c_str(), &opts, fn, data);

    data->doPoll = true;
    data->pollThread = thread( [&]() {
        while (data->doPoll) { mg_mgr_poll(&data->mgr, 1000);
        processJobs(); }
    } );
}

void VRMQTTClient::handleMessages() {
    auto lock = VRLock(data->mtx);
    if (!data->cb) return;

    for (auto m : data->messages) {
        data->cb(m[1] + ": " + m[0]);
    }
    data->messages.clear();
}

void VRMQTTClient::processJobs() {
    auto lock = VRLock(data->mtx);
    if (!data->mqttConnected) return;

    vector<vector<string>> postponed;

    for (auto job : data->jobQueue) {
        if (job[0] == "subscribe") {
            cout << "try subscribing" << endl;
            mg_mqtt_opts opts = data->basicOpts();
            opts.topic = mg_str(job[1].c_str());
            opts.retain = bool(job[2] == "1");
            mg_mqtt_sub(data->s_conn, &opts);
            // TODO: check if connected and logged in, else put job in postponed
            // postponed.push_back(job);
        }

        if (job[0] == "publish") {
            mg_mqtt_opts opts = data->basicOpts();
            opts.topic = mg_str(job[1].c_str());
            opts.message = mg_str(job[2].c_str());
            opts.retain = false;
            mg_mqtt_pub(data->s_conn, &opts);
            // TODO: check if connected and logged in, else put job in postponed
            // postponed.push_back(job);
        }
    }

    data->jobQueue = postponed;
}

void VRMQTTClient::setAuthentication(string name, string pass) { auto lock = VRLock(data->mtx); data->name = name; data->pass = pass; }
void VRMQTTClient::publish(string topic, string message) { auto lock = VRLock(data->mtx); data->jobQueue.push_back({"publish", topic, message}); }

void VRMQTTClient::subscribe(string topic, bool retain) {
    auto lock = VRLock(data->mtx);
    data->jobQueue.push_back( { "subscribe", topic, string( retain ? "1" : "0" ) } );
}







