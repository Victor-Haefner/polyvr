#include "VRICEclient.h"
#include "VRTCPClient.h"
#include "../rest/VRRestClient.h"
#include "../rest/VRRestResponse.h"
#include "core/utils/toString.h"
#include "core/scene/VRScene.h"

using namespace OSG;

VRICEClient::VRICEClient() {
    broker = VRRestClient::create();
    client = VRTCPClient::create();
    updateCb = VRUpdateCb::create("ice update", bind(&VRICEClient::update, this));
    VRScene::getCurrent()->addTimeoutFkt(updateCb, 0, 1000);
}

VRICEClient::~VRICEClient() {
    removeUser(uID);
}

VRICEClientPtr VRICEClient::create() { return VRICEClientPtr( new VRICEClient() ); }
VRICEClientPtr VRICEClient::ptr() { return static_pointer_cast<VRICEClient>(shared_from_this()); }

void VRICEClient::setTurnServer(string url, string ip) {
    turnURL = url;
    turnIP = ip;
    pollUsers();
}

void VRICEClient::setName(string n) {
    name = n;
    uID = broker->get(turnURL+"/regUser.php?NAME="+n)->getData();
    //cout << "register name " << n << " -> " << uID << endl;
    pollUsers();
}

void VRICEClient::removeUser(string uid) {
    broker->get(turnURL+"/remUser.php?UID="+uid);
    pollUsers();
}

string VRICEClient::getID() { return uID; }

void VRICEClient::onEvent( function<void(string)> f ) {
    onEventCb = f;
    client->onConnect([this](){;});
}

void VRICEClient::onMessage( function<void(string)> f ) {
    onMessageCb = f;
    client->onMessage(f);
}

void VRICEClient::update() {
    pollUsers();
    pollMessages();
}

void VRICEClient::updateUsers() {
    users.clear();
    for (auto usrData : splitString(usersList, '\n')) {
        auto data = splitString(usrData, '|');
        if (data.size() != 2) continue;
        string name = data[0];
        string uid = data[1];
        users[uid] = name;
    }
}

map<string, string> VRICEClient::getUsers() {
    pollUsers();
    return users;
}

void VRICEClient::send(string otherID, string msg) { // TODO: check if msg needs to be processed to confom URL syntax
    broker->get(turnURL+"/addMessage.php?ORG="+uID+"&UID="+otherID+"&MSG="+msg)->getData();
}

void VRICEClient::pollMessages() {
    if (uID == "") return;
    string data = broker->get(turnURL+"/getMessages.php?UID="+uID)->getData();
    if (data != "") {
        if (onEventCb) onEventCb("message|"+data);
    }
}

void VRICEClient::pollUsers() {
    string data = broker->get(turnURL+"/listUsers.php")->getData();
    if (data == usersList) return;

    usersList = data;
    updateUsers();
    if (onEventCb) onEventCb("users changed");
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

void VRICEClient::sendTCP(string msg) {
    if (!client->connected()) return;
    client->send(msg);
}

VRTCPClientPtr VRICEClient::getClient() { return client; }

