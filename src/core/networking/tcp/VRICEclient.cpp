#include "VRICEclient.h"
#include "VRTCPClient.h"
#include "../rest/VRRestClient.h"
#include "../rest/VRRestResponse.h"
#include "core/utils/toString.h"

using namespace OSG;

VRICEClient::VRICEClient() {
    broker = VRRestClient::create();
    client = VRTCPClient::create();
}

VRICEClient::~VRICEClient() {}

VRICEClientPtr VRICEClient::create() { return VRICEClientPtr( new VRICEClient() ); }
VRICEClientPtr VRICEClient::ptr() { return static_pointer_cast<VRICEClient>(shared_from_this()); }

void VRICEClient::setTurnServer(string url, string ip) {
    turnURL = url;
    turnIP = ip;
    getUsers();
}

void VRICEClient::setName(string n) {
    getUsers();
    name = n;
    if (!users.count(n)) broker->get(turnURL+"/regUser.php?NAME="+n);
    getUsers();
}

void VRICEClient::onEvent( function<void(string)> f ) {
    onEventCb = f;
    client->onConnect([this](){;});
}

void VRICEClient::onMessage( function<void(string)> f ) {
    onMessageCb = f;
    client->onMessage(f);
    /*auto onTCPMessage = [this](string msg) {
        cout << "VRICEClient::onMessage " << msg << endl;
        if (startsWith(msg, "TURN:")) {
            if (onEventCb) onEventCb(msg);
        } else if (onMessageCb) onMessageCb(msg);
    };
    client->onMessage(onTCPMessage);*/
}

map<string, string> VRICEClient::getUsers() {
    users.clear();
    string userDataList = broker->get(turnURL+"/listUsers.php")->getData();
    for (auto usrData : splitString(userDataList, '\n')) {
        auto data = splitString(usrData, '|');
        if (data.size() != 2) continue;
        string name = data[0];
        string uid = data[1];
        users[name] = uid;
    }
    return users;
}

string VRICEClient::getUserID(string n) {
    if (users.count(n)) return users[n];
    return "";
}

void VRICEClient::connectTo(string other) {
    string uid1 = getUserID(name);
    string uid2 = getUserID(other);
    string data = broker->get(turnURL+"/getConnection.php?UID="+uid1+"&UID2="+uid2)->getData();
    int port = toInt( splitString(data, ':')[0] );
    cout << "VRICEClient::connectTo, name/uid: " << name << "/" << uid1 << " other/uid " << other << "/" << uid2 << " -> port " << port << endl;
    client->connect(turnIP, port);
}

void VRICEClient::send(string msg) {
    if (!client->connected()) return;
    client->send(msg);
}

VRTCPClientPtr VRICEClient::getClient() { return client; }

