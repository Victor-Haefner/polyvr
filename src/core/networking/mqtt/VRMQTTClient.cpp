#include "VRMQTTClient.h"
#include "../mongoose/mongoose.h"

#include <thread>

using namespace OSG;

struct VRMQTTClient::Data {
    mg_mgr mgr;                // Event manager
    const char *s_url = "mqtt://broker.hivemq.com:1883";
    const char *s_sub_topic = "mg/+/test";     // Publish topic
    const char *s_pub_topic = "mg/clnt/test";  // Subscribe topic
    int s_qos = 1;                             // MQTT QoS
    bool doPoll = false;
    thread pollThread;
    mg_connection *s_conn;              // Client connection
};

VRMQTTClient::VRMQTTClient() {
    data = new Data();
    mg_mgr_init(&data->mgr); // Initialise event manager
}

VRMQTTClient::~VRMQTTClient() {
    mg_mgr_free(&data->mgr); // Finished, cleanup
    if (data) delete data;
}

VRMQTTClientPtr VRMQTTClient::create() { return VRMQTTClientPtr( new VRMQTTClient() ); }
VRMQTTClientPtr VRMQTTClient::ptr() { return shared_from_this(); }


void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    VRMQTTClient::Data* data = (VRMQTTClient::Data*)fn_data;

    if (ev == MG_EV_OPEN) {
        MG_INFO(("%lu CREATED", c->id));
        // c->is_hexdumping = 1;
    }

    if (ev == MG_EV_CONNECT) {
        if (mg_url_is_ssl(data->s_url)) {
            mg_tls_opts opts;
            opts.ca = mg_unpacked("/certs/ca.pem");
            opts.name = mg_url_host(data->s_url);
            mg_tls_init(c, &opts);
        }
    }

    if (ev == MG_EV_ERROR) { // On error, log error message
        MG_ERROR(("%lu ERROR %s", c->id, (char *) ev_data));
    }

    if (ev == MG_EV_MQTT_OPEN) { // MQTT connect is successful
        mg_str subt = mg_str(data->s_sub_topic);
        mg_str pubt = mg_str(data->s_pub_topic);
        mg_str msg = mg_str("hello");
        MG_INFO(("%lu CONNECTED to %s", c->id, data->s_url));
        struct mg_mqtt_opts sub_opts;
        memset(&sub_opts, 0, sizeof(sub_opts));
        sub_opts.topic = subt;
        sub_opts.qos = data->s_qos;
        mg_mqtt_sub(c, &sub_opts);
        MG_INFO(("%lu SUBSCRIBED to %.*s", c->id, (int) subt.len, subt.ptr));
        struct mg_mqtt_opts pub_opts;
        memset(&pub_opts, 0, sizeof(pub_opts));
        pub_opts.topic = pubt;
        pub_opts.message = msg;
        pub_opts.qos = data->s_qos, pub_opts.retain = false;
        mg_mqtt_pub(c, &pub_opts);
        MG_INFO(("%lu PUBLISHED %.*s -> %.*s", c->id, (int) msg.len, msg.ptr, (int) pubt.len, pubt.ptr));
    }

    if (ev == MG_EV_MQTT_MSG) {
        // When we get echo response, print it
        struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
        MG_INFO(("%lu RECEIVED %.*s <- %.*s", c->id, (int) mm->data.len, mm->data.ptr, (int) mm->topic.len, mm->topic.ptr));
    }

    if (ev == MG_EV_CLOSE) {
        MG_INFO(("%lu CLOSED", c->id));
        data->s_conn = NULL;  // Mark that we're closed
        data->doPoll = false;
        data->pollThread.join();
    }
}

// connect("mqtt://broker.hivemq.com:1883", "mg/clnt/test")
void VRMQTTClient::connect(string address, string sub_topic, string pub_topic) {
    data->s_url = address.c_str();
    data->s_sub_topic = sub_topic.c_str();  // Publish topic
    data->s_pub_topic = pub_topic.c_str();  // Subscribe topic

    mg_mqtt_opts opts;
    opts.clean = true;
    opts.qos = data->s_qos;
    opts.topic = mg_str(data->s_pub_topic);
    opts.version = 4;
    opts.message = mg_str("bye");
    data->s_conn = mg_mqtt_connect(&data->mgr, data->s_url, &opts, fn, data);

    data->doPoll = true;
    data->pollThread = thread( [&](){ while (data->doPoll) mg_mgr_poll(&data->mgr, 1000); } );
}






