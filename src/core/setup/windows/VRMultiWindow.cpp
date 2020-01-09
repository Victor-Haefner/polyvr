#include "VRMultiWindow.h"
#include "VRView.h"
#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGNameAttachment.h>
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRProfiler.h"
#include "core/utils/xml.h"


#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThread.h>
#include <OpenSG/OSGThreadManager.h>
#include <OpenSG/OSGBarrier.h>
#include <OpenSG/OSGClusterNetwork.h>
#include <OpenSG/OSGGroupMCastConnection.h>
#include <OpenSG/OSGGroupSockConnection.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRMultiWindow::VRMultiWindow() {
    type = 0;
}

VRMultiWindow::~VRMultiWindow() {
    win = 0;
}

VRMultiWindowPtr VRMultiWindow::ptr() { return static_pointer_cast<VRMultiWindow>( shared_from_this() ); }
VRMultiWindowPtr VRMultiWindow::create() { return shared_ptr<VRMultiWindow>(new VRMultiWindow() ); }

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
    int N = Nx*Ny;
    servers.resize(N);
}

int VRMultiWindow::getNXTiles() { return Nx; }
int VRMultiWindow::getNYTiles() { return Ny; }

bool VRMultiWindow::init_win(const std::string &msg, const std::string &server, Real32 progress) {
    cout << endl << msg << " to " << server << " : " << progress;
    if (progress == 1) { state = JUSTCONNECTED; return true; }
    if (tries == 3) { state = NO_CONNECTION; changeListStats.stopOutput(); return false; }
    tries++;
    return true;
}

void VRMultiWindow::initialize() {
#ifndef WASM
    cout << "Initializing MultiWindow\n";
    //cout << " Render MW " << getName() << " state " << getStateString() << endl;
    win = 0; _win = 0; tries = 0; state = CONNECTING;
    win = MultiDisplayWindow::create(); _win = win;
#ifdef WITH_CLUSTERING_FIX
    win->setFrameCounting(false);
#endif
    OSG::setName( win, getName() );

    win->setSize(width, height);
    win->setHServers(Nx);
    win->setVServers(Ny);

    //win->setConnectionType(connection_type); // "Multicast", "SockPipeline" // not needed apparently!
    for (auto s : servers) win->editMFServers()->push_back(s);
    for (auto wv : views) if (auto v = wv.lock()) v->setWindow(win);

    ClusterWindow::ConnectionCB cb = boost::bind(&VRMultiWindow::init_win, this, _1, _2, _3);
    win->initAsync(cb); // TODO: why not defined for WASM build?
    //cout << endl << " render once " << endl;
    //if (state == CONNECTED) win->render(ract);
    cout << " done " << getStateString() << endl;
#endif
}

void OSG_sync(WindowMTRecPtr _win, RenderActionRefPtr ract) {
    auto win = dynamic_pointer_cast<MultiDisplayWindow>(_win);
    win->activate();
    win->frameInit();
    //win->renderAllViewports(ract);
    //win->doSwap();
    //win->frameExit();
}

void OSG_render(WindowMTRecPtr _win, RenderActionRefPtr ract) {
    auto win = dynamic_pointer_cast<MultiDisplayWindow>(_win);
    //win->activate();
    //win->frameInit();
    win->renderAllViewports(ract);
    win->swap();
    win->frameExit();
}

/**

clustering issues may arise when not calling _win->render() after commiting changelist changes!
keep this in mind when trying to optimize regarding to system state like the flags 'content' and 'active'

*/

void VRMultiWindow::sync(bool fromThread) {
    if (state == JUSTCONNECTED) {
        int pID = VRProfiler::get()->regStart("Multiwindow init "+getName());
        Thread::getCurrentChangeList()->clear();
        Thread::getCurrentChangeList()->fillFromCurrentState();
        state = CONNECTED;
        VRProfiler::get()->regStop(pID);
    }

    if (state == CONNECTED) {
        try {
            int pID = VRProfiler::get()->regStart("Multiwindow sync "+getName());
            OSG_sync(_win, ract);
            VRProfiler::get()->regStop(pID);
        } catch(exception& e) { reset(); }
    }
}

void VRMultiWindow::render(bool fromThread) {
    if (state == CONNECTED) {
        try {
            int pID = VRProfiler::get()->regStart("Multiwindow render "+getName());
            state = RENDERING;
            OSG_render(_win, ract);
            state = CONNECTED;
            VRProfiler::get()->regStop(pID);
        } catch(exception& e) { reset(); }
    }
}

void VRMultiWindow::reset() { state = INITIALIZING; }
int VRMultiWindow::getState() { return state; }
void VRMultiWindow::setConnectionType(string ct) { connection_type = ct; }
string VRMultiWindow::getConnectionType() { return connection_type; }

string VRMultiWindow::getStateString() {
    if (state == CONNECTED) return "connected";
    if (state == RENDERING) return "rendering";
    if (state == INITIALIZING) return "initializing";
    if (state == CONNECTING) return "connecting";
    if (state == JUSTCONNECTED) return "just connected";
    if (state == NO_CONNECTION) return "not connencted";
    return "invalid state";
}

void VRMultiWindow::save(XMLElementPtr node) {
    VRWindow::save(node);
    node->setAttribute("Nx", toString(Nx));
    node->setAttribute("Ny", toString(Ny));
    node->setAttribute("ConnType", connection_type);

    XMLElementPtr sn;
    for (uint i=0; i<servers.size(); i++) {
        string s = servers[i];
        sn = node->addChild("Server");
        sn->setAttribute("Address", s);
    }
}

void VRMultiWindow::load(XMLElementPtr node) {
    VRWindow::load(node);
    Nx = toInt( node->getAttribute("Nx") );
    Ny = toInt( node->getAttribute("Ny") );
    if (node->hasAttribute("ConnType")) connection_type = node->getAttribute("ConnType");

    for (auto el : node->getChildren()) {
        if (!el) continue;
        string name = el->getName();
        if (name != "Server") continue;
        string addr = el->getAttribute("Address");
        addServer(addr);
    }

    setNTiles(Nx, Ny);
}

OSG_END_NAMESPACE;
