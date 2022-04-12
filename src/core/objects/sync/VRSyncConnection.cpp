#include "VRSyncConnection.h"
#include "VRSyncNode.h"
#include "core/networking/tcp/VRTCPClient.h"
#include "core/objects/OSGTransform.h"
#include "core/objects/OSGObject.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiConsole.h"
#endif

using namespace OSG;


typedef unsigned char BYTE;

static const string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(BYTE c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

string VRSyncConnection::base64_encode(BYTE const* buf, UInt32 bufLen) {
  string ret;
  UInt32 i = 0;
  UInt32 j = 0;
  BYTE char_array_3[3];
  BYTE char_array_4[4];

  while (bufLen--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}

vector<BYTE> VRSyncConnection::base64_decode(string const& encoded_string) {
  UInt32 in_len = encoded_string.size();
  UInt32 i = 0;
  UInt32 j = 0;
  UInt32 in_ = 0;
  BYTE char_array_4[4], char_array_3[3];
  vector<BYTE> ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}

VRSyncConnection::VRSyncConnection(string host, int port, string localUri) : localUri(localUri) {
    timer = VRTimer::create();
    client = VRTCPClient::create();
    client->setGuard("TCPPVR\n");
    uri = host+":"+toString(port);
    if (host != "") connect();
}

VRSyncConnection::VRSyncConnection(VRTCPClientPtr client, string localUri) : localUri(localUri), client(client) {
    timer = VRTimer::create();
    uri = client->getConnectedUri();
}

VRSyncConnection::~VRSyncConnection() { cout << "~VRSyncConnection::VRSyncConnection" << endl; }
VRSyncConnectionPtr VRSyncConnection::create(string host, int port, string localUri) { return VRSyncConnectionPtr( new VRSyncConnection(host, port, localUri) ); }
VRSyncConnectionPtr VRSyncConnection::create(VRTCPClientPtr client, string localUri) { return VRSyncConnectionPtr( new VRSyncConnection(client, localUri) ); }

string VRSyncConnection::getID() { return uuid; }
string VRSyncConnection::getUri() { return uri; }
string VRSyncConnection::getLocalUri() { return localUri; }

void VRSyncConnection::setID(string u) { uuid = u; }

void VRSyncConnection::connect() {
    client->connect(uri);
    //if (!result) cout << "VRSyncConnection, Failed to open websocket to " << uri << endl;
}

bool VRSyncConnection::send(string message, int frameDelay) {
    if (frameDelay > 0) {
        VRScene::getCurrent()->queueJob( VRUpdateCb::create("sync send", bind(&VRSyncConnection::send, this, message, frameDelay-1) ) );
        return 1;
    }

    timer->reset();
    if (!client) {
        cout << "Error in VRSyncConnection::send, failed! no client.. tried to send " << message << endl;
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( "Error in VRSyncConnection::send, failed! no client.. tried to send " + message + "\n", "red");
#endif
        return 0;
    }
    if (!client->connected()) { // this only means it is connected to the turn server, NOT the other peer!
        cout << "Error in VRSyncConnection::send, failed! client not connected.. tried to send " << message << endl;
#ifndef WITHOUT_GTK
        VRConsoleWidget::get("Collaboration")->write( "Error in VRSyncConnection::send, failed! client not connected.. tried to send " + message + "\n", "red");
#endif
        return 0;
    }
    client->send(message, "TCPPVR\n");
    return 1;
}

void VRSyncConnection::keepAlive() {
    //cout << "keepAlive? " << timer->stop() << endl;
    if (timer->stop() > 3*60*1000) { // 3 min
        send("keepAlive");
    }
}

string VRSyncConnection::getStatus() {
    string s;
    s = " connection with "+uri+", "+toString(client->connected());
    return s;
}

VRSyncConnection::Avatar& VRSyncConnection::getAvatar() { return avatar; }

void VRSyncConnection::setupDevices(UInt32 headTransform, UInt32 devTransform, UInt32 devAnchor) {
    avatar.localHeadID = headTransform;
    avatar.localDevID = devTransform;
    avatar.localAnchorID = devAnchor;
}

UInt32 VRSyncConnection::getNodeID(VRObjectPtr t) {
    return t->getNode()->node->getId();
}

UInt32 VRSyncConnection::getTransformID(VRTransformPtr t) {
    return t->getOSGTransformPtr()->trans->getId();
}

void VRSyncConnection::setupAvatar(VRTransformPtr headTransform, VRTransformPtr devTransform, VRTransformPtr devAnchor) { // some geometries
    avatar.head = headTransform;
    avatar.dev = devTransform;
    avatar.anchor = devAnchor;

    headTransform->enableOptimization(false);
    devTransform->enableOptimization(false);
    devAnchor->enableOptimization(false);

    UInt32 headTransID = getTransformID(headTransform);
    UInt32 deviceTransID = getTransformID(devTransform);
    UInt32 deviceAnchorID = getNodeID(devAnchor);
    string msg = "addAvatar|"+toString(headTransID)+":"+toString(deviceTransID)+":"+toString(deviceAnchorID);

#ifndef WITHOUT_GTK
    VRConsoleWidget::get("Collaboration")->write( "Setup avatar, send device IDs to remote, msg: "+msg+"\n");
#endif

    send(msg);
}

void VRSyncConnection::updateAvatar(string data) {
    PosePtr p = toValue<PosePtr>(splitString(data, '|')[1]);
    if (avatar.head) avatar.head->setPose(p);
}

void VRSyncConnection::handleAvatar(string data) {
    if (!avatar.localHeadID) return;
    if (!avatar.localDevID) return;
    if (!avatar.localAnchorID) return;

    auto IDs = splitString( splitString(data, '|')[1], ':');
    UInt32 headTransID = toInt(IDs[0]); // remote geometry
    UInt32 deviceTransID = toInt(IDs[1]); // remote geometry
    UInt32 deviceAnchorID = toInt(IDs[2]); // remote geometry

    string mapping = "mapping";
    mapping += "|"+toString(headTransID)+":"+toString(avatar.localHeadID);
    mapping += "|"+toString(deviceTransID)+":"+toString(avatar.localDevID);
    mapping += "|"+toString(deviceAnchorID)+":"+toString(avatar.localAnchorID);
    send(mapping);
}

void VRSyncConnection::addRemoteMapping(UInt32 lID, UInt32 rID) {
    remoteToLocalID[rID] = lID;
    localToRemoteID[lID] = rID;
}

void VRSyncConnection::handleMapping(string mappingData) {
    auto pairs = splitString(mappingData, '|');
    for (auto p : pairs) {
        auto IDs = splitString(p, ':');
        if (IDs.size() != 2) continue;
        UInt32 lID = toInt(IDs[0]);
        UInt32 rID = toInt(IDs[1]);
        addRemoteMapping(lID, rID);
    }
}

void VRSyncConnection::handleRemoteMapping(string mappingData, VRSyncNodePtr syncNode) {
    if (!syncNode) return;
    auto pairs = splitString(mappingData, '|');
    string originRemoteID = pairs[1];
    auto origin = syncNode->getRemote(originRemoteID);
    cout << " VRSyncConnection::handleRemoteMapping origin " << originRemoteID << " -> " << origin << endl;
    if (!origin) return;

    mappingData = "";
    for (auto p : pairs) {
        auto IDs = splitString(p, ':');
        if (IDs.size() != 2) continue;
        UInt32 oID = toInt(IDs[0]);
        UInt32 rID = toInt(IDs[1]);
        UInt32 lID = origin->getLocalID(oID);
        cout << "  oID, rID, lID " << Vec3i(oID, rID, lID) << endl;
        if (lID != 0) {
            addRemoteMapping(lID, rID);
            mappingData += "|" + toString(rID) + ":" + toString(lID);
        }
    }

    if (mappingData != "") send("mapping"+mappingData);
}

UInt32 VRSyncConnection::getLocalID(UInt32 id) {
    if (!remoteToLocalID.count(id)) return 0;
    return remoteToLocalID[id];
}

UInt32 VRSyncConnection::getRemoteID(UInt32 id) {
    if (!localToRemoteID.count(id)) return 0;
    return localToRemoteID[id];
}

// checks if a container was changed by remote
bool VRSyncConnection::isRemoteChange(const UInt32& id) {
    return bool(::find(syncedContainer.begin(), syncedContainer.end(), id) != syncedContainer.end());
}

void VRSyncConnection::logSyncedContainer(UInt32 id) {
    if (isRemoteChange(id)) syncedContainer.push_back(id);
}

void VRSyncConnection::clearSyncedContainer() {
    syncedContainer.clear();
}

void VRSyncConnection::handleTypeMapping(string mappingData) {
    auto pairs = splitString(mappingData, '|');
    for (auto p : pairs) {
        auto IDs = splitString(p, ':');
        if (IDs.size() != 2) continue;
        UInt32 rID = toInt(IDs[0]);
        string name = IDs[1];
        FieldContainerType* fcT = factory->findType(name.c_str());
        if (!fcT) {
            cout << "Warning in VRSyncNode::handleTypeMapping, unknown remote type " << name.c_str() << endl;
            continue;
        }
        typeMapping[rID] = fcT->getId();
        //cout << " typeMapping: " << rID << " " << name << " " << fcT->getId() << endl;
    }
    //printRegistredContainers();
}

UInt32 VRSyncConnection::getLocalType(UInt32 id) {
    return typeMapping[id];
}

