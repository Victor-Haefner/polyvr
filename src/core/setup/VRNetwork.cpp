#include "VRNetwork.h"
#include "core/utils/VRManager.cpp"
#include "core/networking/VRPing.h"
#ifndef WITHOUT_SSH
#include "core/networking/VRSSH.h"
#endif

#include "core/scene/VRSceneManager.h"
#include "core/utils/system/VRSystem.h"

#include "core/utils/Thread.h"

using namespace OSG;

//template<> class VRManager<VRNetworkNode>;
//template<> VRNetworkNodePtr VRManager<VRNetworkNode>::add(string name);

VRNetwork::VRNetwork() : VRManager("Network") {}
VRNetwork::~VRNetwork() {
    cout << "~VRNetwork\n";
    stopSlaves();
}

void VRNetwork::joinInitThreads() {
    cout << "----------------------- VRNetwork::joinInitThreads ------------------------" << endl;
    for (auto n : getData()) n->joinInitThread();
}

VRNetworkNodePtr VRNetwork::getNode(string name) {
    for (auto n : getData()) {
        if (n->getName() == name) return n;
    }
    return 0;
}

VRNetworkSlavePtr VRNetwork::getSlave(string name) {
    for (auto n : getData()) {
        auto s = n->get(name);
        if (s) return s;
    }
    return 0;
}

void VRNetwork::remNode(string node) { rem(node); }

void VRNetwork::remSlave(string slave) {
    for (auto n : getData()) {
        auto s = n->get(name);
        if (s) {
            n->rem(slave);
            return;
        }
    }
}

VRNetworkNode::VRNetworkNode(string name) : VRManager("NetworkNode") {
    setNameSpace("NetworkNode");
    setName(name);

    slavePath = VRSceneManager::get()->getOriginalWorkdir();

    store("address", &address);
    store("user", &user);
    store("slavePath", &slavePath);
    regStorageSetupFkt( VRStorageCb::create("network_node_update", bind(&VRNetworkNode::setup, this, placeholders::_1)) );
}

VRNetworkNode::~VRNetworkNode() { stopSlaves(); }

VRNetworkNodePtr VRNetworkNode::create(string name) { return VRNetworkNodePtr( new VRNetworkNode(name) ); }
VRNetworkNodePtr VRNetworkNode::ptr() { return static_pointer_cast<VRNetworkNode>( shared_from_this() ); }

void VRNetworkNode::setup(VRStorageContextPtr context) {
    cout << "       ----------------------- VRNetworkNode::setup ------------------------" << endl;
    initThread = new ::Thread("Network_init", [&](){ update(); initSlaves();} );
}

void VRNetworkNode::joinInitThread() {
    initThread->join();
    delete initThread;
}

string VRNetworkNode::getAddress() { return address; }
string VRNetworkNode::getUser() { return user; }
string VRNetworkNode::getSlavePath() { return slavePath; }

void VRNetworkNode::setAddress(string s) { address = s; update(); }
void VRNetworkNode::setUser(string s) { user = s; update(); }
void VRNetworkNode::setSlavePath(string s) { slavePath = s; update(); }

string VRNetworkNode::getStatNode() { return stat_node; }
string VRNetworkNode::getStatSSH() { return stat_ssh; }
string VRNetworkNode::getStatSSHkey() { return stat_ssh_key; }
string VRNetworkNode::getStatPath() { return stat_path; }

void VRNetworkNode::set(string a, string u, string p) {
    address = a;
    user = u;
    slavePath = p;
    update();
}

void VRNetworkNode::distributeKey() {
#ifndef WITHOUT_SSH
    if (!ssh) openSSHSession();
    if (!ssh) return;
    if (!ssh->hasLocalKey()) ssh->createLocalKey();
    ssh->distrib_key();
    stat_ssh = ssh->getStat();
    stat_ssh_key = ssh->getKeyStat();
    ssh.reset();
    update();
#endif
}

VRNetworkSlavePtr VRNetworkNode::add(string name) {
    auto s = VRManager<VRNetworkSlave>::add(name);
    s->setNode(ptr());
    return s;
}

void VRNetworkNode::initSlaves() {
    bool hasAutostart = false;
    int startupDelay = 0;
    for (auto s : getData()) {
        if (s->getAutostart()) hasAutostart = true;
        startupDelay = max(s->getStartupDelay(), startupDelay);
    }
    if (hasAutostart) stopSlaves();

    for (auto s : getData()) {
        s->setNode(ptr());
        if (s->getAutostart()) {
            s->start();
        }
    }
    if (hasAutostart) this_thread::sleep_for(chrono::seconds(startupDelay));
}

void VRNetwork::stopSlaves() {
    cout << "VRNetwork::stopSlaves" << endl;
    vector<::Thread*> threads;
    for (auto n : getData()) {
	auto t = new ::Thread("stop slaves", [&](){n->stopSlaves();} );
        threads.push_back(t);
    }
    cout << " wait for all threads to finish.. (" << threads.size() << ")" << endl;
    for (auto t : threads) { t->join(); delete t; }
    cout << "  done!" << endl;
}

void VRNetworkNode::stopSlaves() {
    string script = getSlavePath() + "/src/cluster/stop";
    if (os == "win") script = getSlavePath() + "/src/cluster/stop-win.bat";
    execCmd(script);
    update();
}

bool VRNetworkNode::hasFile(string path) {
    string cmd = "";
    if (os == "nix" || os == "") cmd = "ls "+path;
    if (os == "win") cmd = "if exist \""+path+"\" echo|set /p=\""+path+"\"";
    string res = execCmd(cmd, true);
    bool b = res.substr(0, path.size()) == path;
    //cout << "os: " << os << endl;
    //cout << "hasFile: " << endl << res << endl << " -> " << b << endl;
    return b;
}

bool VRNetworkNode::isLocal() {
    return (address == "localhost" || address == "127.0.0.1");
}

void VRNetworkNode::update() {
#ifndef WITHOUT_SSH
    stat_node = "ok";
    stat_ssh = "";
    stat_ssh_key = "no ssh used";
    stat_path = "";

    VRPing p;
    if ( !p.start(address, 1) ) { stat_node = "not reachable"; return; }

    if (!isLocal()) {
        if (!p.startOnPort(address, "22", 1)) { stat_node = "no ssh, install with sudo apt install ssh"; return; }
        ssh.reset();
        openSSHSession();
        if (ssh) stat_ssh_key = ssh->getKeyStat();
        else { stat_path = "no ssh access"; return; }
        getRemoteOS();
    }

    string sExe = "VRServer";
    if (os == "win") sExe += ".exe";

    if (!hasFile(slavePath + "/src/cluster/start")) stat_path = "PolyVR root directory not found";
    else if (!hasFile(slavePath + "/src/cluster/"+sExe)) stat_path = "Slave not compiled, on Ubuntu execute " + slavePath + "/src/cluster/setup";
    else stat_path = "ok";
#endif
}

string VRNetworkNode::getRemoteOS() {
    if (isLocal()) return os;
    if (!ssh) openSSHSession();
    if (!ssh) return "";
    string n = getName();
#ifndef WITHOUT_SSH
    os = ssh->getRemoteOS();
#endif
    return os;
}

void VRNetworkNode::openSSHSession() {
    if (ssh) return;
#ifndef WITHOUT_SSH
    ssh = VRSSHSession::open(address, user);
    stat_ssh = ssh->getStat();
    if (stat_ssh != "ok") ssh = 0;
#endif
}

string VRNetworkNode::execCmd(string cmd, bool read) {
    if (stat_node != "ok" ) return "";
    if (isLocal()) return systemCall(cmd);
    if (!ssh) openSSHSession();
    if (!ssh) return "";
#ifndef WITHOUT_SSH
    return ssh->exec_cmd(cmd, read);
#else
    return "";
#endif
}

VRNetworkSlave::VRNetworkSlave(string name) {
    setNameSpace("NetworkNode");
    setName(name);

    store("connection_type", &connection_type);
    store("fullscreen", &fullscreen);
    store("active_stereo", &active_stereo);
    store("display", &display);
    store("autostart", &autostart);
    store("port", &port);
    store("startupDelay", &startupDelay);
    store("geometry", &geometry);
}

VRNetworkSlave::~VRNetworkSlave() {}

VRNetworkSlavePtr VRNetworkSlave::create(string name) { return VRNetworkSlavePtr( new VRNetworkSlave(name) ); }

void VRNetworkSlave::start() {
    cout << "VRNetworkSlave::start " << getName() << endl;
    if (!node) return;
    //if (!exists(path + "VRServer")) { stat = "no slave exec. VRServer in src/cluster/"; return; } // TODO: check on remote!

    string args;
    if (!fullscreen) args += " -w";
    if (active_stereo) args += " -A";
    if (connection_type == "Multicast") args += " -m " + getName();
    if (connection_type == "SockPipeline") args += " -p " + node->getAddress() + ":" + toString(port);
    if (connection_type == "StreamSock") args += " " + node->getAddress() + ":" + toString(port);

    string os = node->getRemoteOS();

    string cmd = "";
    if (os == "nix") {
        string disp = "export DISPLAY=\"" + display + "\" && ";
        string pipes = " > /dev/null 2> /dev/null < /dev/null &";
        string script = node->getSlavePath() + "/src/cluster/start";
        //string pipes = " > /dev/null 2> /dev/null < /dev/null"; // TODO: without & it returns the correct code, but it also makes the app stuck!
        cmd = disp + script + args + pipes;
    }

    if (os == "win") {
        //string disp = " -display \\\\.\\DISPLAY"+display+" "; // needed?
        string pipes = ""; // TODO: maybe needed?
        string script = node->getSlavePath() + "/src/cluster/start-win-ssh.bat";
        if (node->isLocal()) script = node->getSlavePath() + "/src/cluster/start-win.bat";
        string geo = " -geometry "+geometry+" "; // TODO: add window offset and size as parameters in UI
        cmd = script + args + geo + pipes;
    }

    cout << " start command:\n" << cmd << endl;
    stat = node->execCmd(cmd, false);
    cout << "  stat: " << stat << endl;
    update();
}

void VRNetworkSlave::stop() {
    if (!node) return;
    node->stopSlaves();
    update();
}

void VRNetworkSlave::setNode(VRNetworkNodePtr n) { node = n; }

void VRNetworkSlave::set(string ct, bool fs, bool as, bool au, string a, int p, int d, string g) {
    connection_type = ct;
    fullscreen = fs;
    active_stereo = as;
    autostart = au;
    display = a;
    port = p;
    startupDelay = d;
    geometry = g;
    update();
}

vector<string> VRNetworkSlave::getAvailableDisplays() {
    string xservers = node->execCmd("ls /tmp/.X11-unix");
    cout << " xservers: " << xservers << endl;

    vector<string> res;
    for (auto x : splitString(xservers)) {
        if (x.size() < 2) continue;
        int xID = toInt( subString(x, 1) );
        cout << "  xID " << xID << endl;
        string screens = node->execCmd("xdpyinfo -display :0 | grep screen");
        for ( auto screen : splitString(screens,'\n') ) {
            if ( startsWith(screen, "screen #") ) {
                int screenID = toInt( splitString( splitString(screen,'#')[1], ':' )[0] );
                cout << "   screenID " << screenID << endl;
                res.push_back(":" + toString(xID) + "." + toString(screenID));
            }
        }
    }
    return res;
}

void VRNetworkSlave::update() {}

string VRNetworkSlave::getStatMulticast() { return stat_multicast; }
string VRNetworkSlave::getStat() { return stat; }

string VRNetworkSlave::getDisplay() { return display; }
string VRNetworkSlave::getConnectionType() { return connection_type; }
bool VRNetworkSlave::getFullscreen() { return fullscreen; }
bool VRNetworkSlave::getActiveStereo() { return active_stereo; }
bool VRNetworkSlave::getAutostart() { return autostart; }
int VRNetworkSlave::getPort() { return port; }
int VRNetworkSlave::getStartupDelay() { return startupDelay; }
string VRNetworkSlave::getGeometry() { return geometry; }

void VRNetworkSlave::setPort(int p) { port = p; update(); }
void VRNetworkSlave::setDelay(int d) { startupDelay = d; update(); }
void VRNetworkSlave::setDisplay(string a) { display = a; update(); }
void VRNetworkSlave::setConnectionType(string b) { connection_type = b; update(); }
void VRNetworkSlave::setFullscreen(bool b) { fullscreen = b; update(); }
void VRNetworkSlave::setActiveStereo(bool b) { active_stereo = b; update(); }
void VRNetworkSlave::setAutostart(bool b) { autostart = b; update(); }
void VRNetworkSlave::setGeometry(string g) { geometry = g; update(); }

string VRNetworkSlave::getConnectionIdentifier() {
    if (connection_type == "Multicast") return getName();
    else return node->getAddress() + ":" + toString(port);
}



