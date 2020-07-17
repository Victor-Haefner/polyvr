#include "VRSyncConnection.h"

using namespace OSG;


// ------------------- VRSyncConnection


VRSyncConnection::VRSyncConnection(string uri) : uri(uri) {
    socket = VRWebSocket::create("sync node ws");
    if (uri != "") connect();
}

VRSyncConnection::~VRSyncConnection() { cout << "~VRSyncConnection::VRSyncConnection" << endl; }
VRSyncConnectionPtr VRSyncConnection::create(string name) { return VRSyncConnectionPtr( new VRSyncConnection(name) ); }

void VRSyncConnection::connect() {
    bool result = socket->open(uri);
    if (!result) cout << "VRSyncConnection, Failed to open websocket to " << uri << endl;
}

bool VRSyncConnection::send(string message){
    if (!socket->sendMessage(message)) return 0;
    return 1;
}
