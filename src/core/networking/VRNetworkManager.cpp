#include "VRNetworkManager.h"

#include "VRSocket.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRStorage_template.h"
#include <iostream>

// test with following line, it will send "hi" on port 5000
// sudo hping3 -p 5000 -e "hi" 141.3.150.20

OSG_BEGIN_NAMESPACE;
using namespace std;

VRNetworkManager::VRNetworkManager() {
    storeMap("Socket", &sockets);
}

VRNetworkManager::~VRNetworkManager() {
    for (auto s : sockets) delete s.second;
}

VRSocket* VRNetworkManager::getSocket(int port) {
    for (auto s : sockets) if(s.second->getPort() == port) return s.second;

    VRSocket* s = new VRSocket("Socket");
    s->setPort(port);
    sockets[s->getName()] = s;
    return s;
}

void VRNetworkManager::remSocket(string name) {
    if (sockets.count(name) == 0) return;
    delete sockets[name];
    sockets.erase(name);
}

string VRNetworkManager::changeSocketName(string name, string new_name) {
    map<string, VRSocket*>::iterator i = sockets.find(name);
    if (i == sockets.end()) return name;

    VRSocket* s = i->second;
    sockets.erase(i);
    s->setName(new_name);

    sockets[s->getName()] = s;
    return s->getName();
}

map<string, VRSocket*> VRNetworkManager::getSockets() { return sockets; }
VRSocket* VRNetworkManager::getSocket(string name) {
    if (sockets.count(name)) return sockets[name];
    else return 0;
}

void VRNetworkManager::update() {
    ;
}

/*void VRNetworkManager::saveNetworkSockets(xmlpp::Element* e) {
    map<string, VRSocket*>::iterator itr;
    for (itr = sockets.begin(); itr != sockets.end(); itr++) {
        xmlpp::Element* ei = e->add_child(itr->second->getName());
        itr->second->save(ei);
    }
}*/

/*void VRNetworkManager::loadNetworkSockets(xmlpp::Element* e) {
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string name = newSocket();
        name = changeSocketName(name, el->get_name());
        //cout << "\nLOADED SOCK " << name << endl;
        sockets[name]->load(el);
    }
}*/

OSG_END_NAMESPACE
