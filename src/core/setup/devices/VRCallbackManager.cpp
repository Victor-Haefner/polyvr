#include "VRCallbackManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

int VRCallbackManager::id_increment(string name) {
    labels[id] = name;
    id++;
    return id-1;
}

int VRCallbackManager::findByName(string name) {
    map<int, string>::const_iterator itr;
    for(itr = labels.begin(); itr != labels.end(); ++itr) {
        if ((*itr).second == name) return (*itr).first;
    }
    return -1;
}

string VRCallbackManager::fromInt(int i) {char buf[100]; sprintf(buf,"%d",i); return string(buf); }

string VRCallbackManager::getLinkLabel(unsigned int i) { if (i >= labels.size()) return ""; return labels[i]; }

VRCallbackManager::VRCallbackManager(): id(0) {}

//int add(VRDevCb* fkt, string name = "") {
int VRCallbackManager::addCallback(VRFunction_base* fkt, string name) {
    callbacks[id] = fkt;
    return id_increment(name);
}

int VRCallbackManager::addSignal(VRSignal* sig, string name) {
    signals[id] = sig;
    return id_increment(name);
}

VRFunction_base* VRCallbackManager::getCallback(string name) {
    if (findByName(name) == -1) return 0;
    return callbacks[findByName(name)];
}

void VRCallbackManager::splitLink(int sig, int cb) { // TODO
    /*signals[sig]->sub(cb);
    list<int*>::iterator itr;
    for (itr=bonds.begin(); itr!=bonds.end(); itr++) {
        if ((*itr)[0] == sig and (*itr)[1] == cb) {
            delete[] (*itr);
            itr = bonds.erase(itr);
        }
    }*/
}

void VRCallbackManager::splitLink(VRSignal* sig, VRFunction_base* fkt) { // TODO
    //sig->sub(fkt);
}

//main bond function
bool VRCallbackManager::bindLink(int id_sig, int id_cb) {
    if (signals.count(id_sig) == 0) return false;

    int* tmp = new int[2];
    tmp[0] = id_sig; tmp[1] = id_cb;
    bonds.push_back(tmp);

    VRFunction_base* fkt = callbacks[id_cb];

    signals[id_sig]->add( (VRDevCb*)fkt, id_cb);
    return true;
}

/*bool bond(string sig, string cb) {//remove this one perhapst, or make names unique?
    int id_sig = findByName(sig);
    int id_cb = findByName(cb);
    return bond(id_sig, id_cb);
}*/

bool VRCallbackManager::bindLink(VRFunction_base* fkt, VRSignal* sig, string fkt_name, string sig_name) { return bindLink(sig, fkt, sig_name, fkt_name); }
bool VRCallbackManager::bindLink(VRSignal* sig, VRFunction_base* fkt, string sig_name, string fkt_name) {
    int id_sig = addSignal(sig, sig_name);
    int id_cb = addCallback(fkt, fkt_name);
    return bindLink(id_sig, id_cb);
}

int VRCallbackManager::getNumCallbacks() {return callbacks.size(); }
int VRCallbackManager::getNumSignals() {return signals.size(); }
int VRCallbackManager::getNumLinks() {return bonds.size(); }

int VRCallbackManager::getLinkSignal(unsigned int i) {
    if (i >= bonds.size()) return -1;
    list<int*>::iterator itr = bonds.begin();
    for (unsigned int j=0;j<i;j++) { itr++; }
    return (*itr)[0];
}
int VRCallbackManager::getLinkCallback(unsigned int i) {
    if (i >= bonds.size()) return -1;
    list<int*>::iterator itr = bonds.begin();
    for (unsigned int j=0;j<i;j++) { itr++; }
    return (*itr)[1];
}

string VRCallbackManager::getSignalLabel(unsigned int i) {
    if (i >= signals.size()) return "";
    map<int, VRSignal*>::iterator itr = signals.begin();
    for (unsigned int j=0;j<i;j++) { itr++; }
    return string(fromInt((*itr).first) + " " + labels[(*itr).first]);
}

string VRCallbackManager::getCallbackLabel(unsigned int i) {
    if (i >= callbacks.size()) return "";
    //map<int, VRDevCb*>::iterator itr = callbacks.begin();
    map<int, VRFunction_base*>::iterator itr = callbacks.begin();
    for (unsigned int j=0;j<i;j++) { itr++; }
    return string(fromInt((*itr).first) + " " + labels[(*itr).first]);
}

OSG_END_NAMESPACE;
