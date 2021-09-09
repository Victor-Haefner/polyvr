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

VRICEClient::~VRICEClient() {
    removeUser(uID);
}

VRICEClientPtr VRICEClient::create() { return VRICEClientPtr( new VRICEClient() ); }
VRICEClientPtr VRICEClient::ptr() { return static_pointer_cast<VRICEClient>(shared_from_this()); }

void VRICEClient::setTurnServer(string url, string ip) {
    turnURL = url;
    turnIP = ip;
    getUsers();
}

void VRICEClient::setName(string n) {
    name = n;
    uID = broker->get(turnURL+"/regUser.php?NAME="+n)->getData();
    //cout << "register name " << n << " -> " << uID << endl;
    getUsers();
}

void VRICEClient::removeUser(string uid) {
    broker->get(turnURL+"/remUser.php?UID="+uid);
    getUsers();
}

string VRICEClient::getID() { return uID; }

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
        users[uid] = name;
    }
    return users;
}

string VRICEClient::getUserName(string ID) {
    if (users.count(ID)) return users[ID];
    return "";
}

vector<string> VRICEClient::getUserID(string n) {
    vector<string> res;
    for (auto u : users) if (u.second == n) res.push_back(u.first);
    return res;
}

void VRICEClient::connectTo(string otherID) {
    if (uID == "" || otherID == "") {
        cout << "VRICEClient::connectTo failed, empty ID" << endl;
        return;
    }

    string uid1 = uID;
    string uid2 = otherID;
    //cout << "VRICEClient::connectTo, name/uid: " << name << "/" << uid1 << " other/uid " << users[uid2] << "/" << uid2 << endl;

    if (!users.count(uid1) || !users.count(uid2)) {
        cout << "VRICEClient::connectTo failed, unknown ID" << endl;
        return;
    }

    string data = broker->get(turnURL+"/getConnection.php?UID="+uid1+"&UID2="+uid2)->getData();
    int port = toInt( splitString(data, ':')[0] );
    //cout << " -> port " << port << endl;
    client->connect(turnIP, port);
}

void VRICEClient::send(string msg) {
    if (!client->connected()) return;
    client->send(msg);
}

VRTCPClientPtr VRICEClient::getClient() { return client; }

