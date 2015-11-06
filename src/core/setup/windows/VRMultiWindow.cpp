#include "VRMultiWindow.h"
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGNameAttachment.h>
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"


#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGBarrier.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRMultiWindow::VRMultiWindow() {
    type = 0;
}

VRMultiWindow::~VRMultiWindow() {
    win = 0;
}

void VRMultiWindow::addServer(string server) { servers.push_back(server); }

string VRMultiWindow::getServer(int x, int y) {
    uint i = x+y*Nx;
    if (i >= servers.size()) return "None";
    return servers[i];
}


void VRMultiWindow::setServer(int x, int y, string s) {
    uint i = x+y*Nx;
    while (i >= servers.size()) servers.push_back("");
    servers[i] = s;
    cout << "\nWM SERVER SET\n";
}

void VRMultiWindow::setNTiles(int x, int y) {
    Nx = x;
    Ny = y;
}

int VRMultiWindow::getNXTiles() { return Nx; }
int VRMultiWindow::getNYTiles() { return Ny; }

bool VRMultiWindow::init_win(const std::string &msg, const std::string &server, Real32 progress) {
    cout << endl << msg << " to " << server << " : " << progress;
    if (progress == 1) { state = CONNECTED; return true; }
    if (tries == 3) { state = NO_CONNECTION; return false; }
    tries++;
    return true;
}

void VRMultiWindow::initialize() {
    cout << "Initializing MultiWindow\n";
    //cout << " Render MW " << getName() << " state " << getStateString() << endl;
    win = 0; _win = 0; tries = 0; state = CONNECTING;
    win = MultiDisplayWindow::create(); _win = win;

    win->setSize(width, height);
    win->setHServers(Nx);
    win->setVServers(Ny);
    for (uint i=0; i< servers.size(); i++) win->editMFServers()->push_back(servers[i]);
    for (uint i=0; i< views.size(); i++) views[i]->setWindow(win);

    Thread::getCurrentChangeList()->commitChangesAndClear();
    Thread::getCurrentChangeList()->fillFromCurrentState();

    ClusterWindow::ConnectionCB cb = boost::bind(&VRMultiWindow::init_win, this, _1, _2, _3);
    win->initAsync(cb);
    if (state == CONNECTED) win->render(ract);
    cout << " done " << getStateString() << endl;
}

void VRMultiWindow::render() {
    if (state == INITIALIZING) initialize();
    if (state == CONNECTED && active && content) {
        try { _win->render(ract); }
        catch(exception& e) { reset(); }
    }
}

void VRMultiWindow::reset() { state = INITIALIZING; }

int VRMultiWindow::getState() { return state; }

string VRMultiWindow::getStateString() {
    if (state == CONNECTED) return "connected";
    if (state == INITIALIZING) return "initializing";
    if (state == CONNECTING) return "connecting";
    if (state == NO_CONNECTION) return "not connencted";
    return "invalid state";
}

void VRMultiWindow::save(xmlpp::Element* node) {
    VRWindow::save(node);
    node->set_attribute("Nx", toString(Nx));
    node->set_attribute("Ny", toString(Ny));

    xmlpp::Element* sn;
    for (uint i=0; i<servers.size(); i++) {
        string s = servers[i];
        sn = node->add_child("Server");
        sn->set_attribute("Address", s);
    }
}

void VRMultiWindow::load(xmlpp::Element* node) {
    VRWindow::load(node);
    Nx = toInt( node->get_attribute("Nx")->get_value() );
    Ny = toInt( node->get_attribute("Ny")->get_value() );
    setNTiles(Nx, Ny);

    xmlpp::Node::NodeList nl = node->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        string name = el->get_name();
        if (name != "Server") continue;

        string addr = el->get_attribute("Address")->get_value();

        addServer(addr);
    }
}

OSG_END_NAMESPACE;
