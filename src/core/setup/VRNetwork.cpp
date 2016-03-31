#include "VRNetwork.h"
#include "core/utils/VRManager.cpp"
#include "core/networking/VRPing.h"
#include "core/networking/VRSSH.h"
#include "core/gui/VRGuiUtils.h"

#include <libssh/libssh.h>
#include <boost/filesystem.hpp>

using namespace OSG;

//template<> class VRManager<VRNetworkNode>;
//template<> VRNetworkNodePtr VRManager<VRNetworkNode>::add(string name);

VRNetwork::VRNetwork() : VRManager("Network") {}
VRNetwork::~VRNetwork() { cout << "~VRNetwork"; }


VRNetworkNode::VRNetworkNode(string name) : VRManager("NetworkNode") {
    setNameSpace("NetworkNode");
    setName(name);

    store("address", &address);
    store("user", &user);
    regStorageUpdateFkt( VRFunction<int>::create("network_node_update", boost::bind(&VRNetworkNode::update, this)) );
    regStorageUpdateFkt( VRFunction<int>::create("network_node_update2", boost::bind(&VRNetworkNode::initSlaves, this)) );
}

VRNetworkNode::~VRNetworkNode() {}

VRNetworkNodePtr VRNetworkNode::create(string name) { return VRNetworkNodePtr( new VRNetworkNode(name) ); }
VRNetworkNodePtr VRNetworkNode::ptr() { return static_pointer_cast<VRNetworkNode>( shared_from_this() ); }

string VRNetworkNode::getAddress() { return address; }
string VRNetworkNode::getUser() { return user; }

void VRNetworkNode::setAddress(string s) { address = s; update(); }
void VRNetworkNode::setUser(string s) { user = s; update(); }

string VRNetworkNode::getStatNode() { return stat_node; }
string VRNetworkNode::getStatSSH() { return stat_ssh; }
string VRNetworkNode::getStatSSHkey() { return stat_ssh_key; }

void VRNetworkNode::set(string a, string u) {
    address = a;
    user = u;
    update();
}

void VRNetworkNode::distributeKey() {
    if (stat_node != "ok") return;
    auto ssh = VRSSHSession::open(address, user);
    ssh->distrib_key();
    stat_ssh = ssh->getStat();
    stat_ssh_key = ssh->getKeyStat();
    ssh.reset();
    update();
}

VRNetworkSlavePtr VRNetworkNode::add(string name) {
    auto s = VRManager<VRNetworkSlave>::add(name);
    s->setNode(ptr());
    return s;
}

void VRNetworkNode::initSlaves() {
    for (auto s : getData()) {
        s->setNode(ptr());
        if (s->getAutostart()) s->start();
    }
}

void VRNetworkNode::update() {
    stat_node = "ok";
    stat_ssh = "";
    stat_ssh_key = "";

    VRPing p;
    if ( !p.start(address, "22", 1) ) { stat_node = "ping failed"; return; }

    auto ssh = VRSSHSession::open(address, user);
    stat_ssh = ssh->getStat();
    stat_ssh_key = ssh->getKeyStat();
}

void VRNetworkNode::execCmd(string cmd, bool read) {
    auto ssh = VRSSHSession::open(address, user);
    ssh->exec_cmd(cmd, read);
}

VRNetworkSlave::VRNetworkSlave(string name) {
    setNameSpace("NetworkNode");
    setName(name);

    store("multicast", &multicast);
    store("fullscreen", &fullscreen);
    store("active_stereo", &active_stereo);
    store("display", &display);
    store("autostart", &autostart);

    regStorageUpdateFkt( VRFunction<int>::create("network_slave_update", boost::bind(&VRNetworkSlave::update, this)) );
}

VRNetworkSlave::~VRNetworkSlave() {}

VRNetworkSlavePtr VRNetworkSlave::create(string name) { return VRNetworkSlavePtr( new VRNetworkSlave(name) ); }

void VRNetworkSlave::update() {} // TODO: compute stats

void VRNetworkSlave::start() {
    if (!node) return;
    string path = "~/Projects/polyvr/src/cluster/start";
    string disp = "export DISPLAY=\"" + display + "\" && ";
    string pipes = " > /dev/null 2> /dev/null < /dev/null &";
    string args = multicast ? " -m " + getName() + " " : " -p -a " + node->getAddress();
    args += fullscreen ? "" : " -w ";
    args += active_stereo ? " -A " : "";
    node->execCmd(disp + path + args + pipes, false);
    update();
}

void VRNetworkSlave::stop() {
    if (!node) return;
    node->execCmd("killall VRServer");
    update();
}

void VRNetworkSlave::setNode(VRNetworkNodePtr n) { node = n; }

void VRNetworkSlave::set(bool mc, bool fs, bool as, bool au, string a) {
    multicast = mc;
    fullscreen = fs;
    active_stereo = as;
    autostart = au;
    display = a;
    update();
}

string VRNetworkSlave::getStatMulticast() { return stat_multicast; }
string VRNetworkSlave::getStat() { return stat; }

string VRNetworkSlave::getDisplay() { return display; }
bool VRNetworkSlave::getMulticast() { return multicast; }
bool VRNetworkSlave::getFullscreen() { return fullscreen; }
bool VRNetworkSlave::getActiveStereo() { return active_stereo; }
bool VRNetworkSlave::getAutostart() { return autostart; }

void VRNetworkSlave::setDisplay(string a) { display = a; update(); }
void VRNetworkSlave::setMulticast(bool b) { multicast = b; update(); }
void VRNetworkSlave::setFullscreen(bool b) { fullscreen = b; update(); }
void VRNetworkSlave::setAutostart(bool b) { autostart = b; update(); }




