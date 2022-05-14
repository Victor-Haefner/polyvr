#include "VRGuiNetwork.h"
#include "widgets/VRWidgetsCanvas.h"

#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"

#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include "core/networking/VRNetworkManager.h"
#include "core/networking/VRNetworkClient.h"
#include "core/networking/tcp/VRTCPClient.h"
#include "core/networking/tcp/VRICEclient.h"
#include "core/networking/udp/VRUDPClient.h"

#include <gtk/gtk.h>

using namespace OSG;

VRNetworkWidget::VRNetworkWidget(string lbl, _GtkFixed* canvas) : VRCanvasWidget(canvas) {
    static int i = 0;
    i++;
    wID = i;

    origin = TOP_LEFT;

    auto frame = gtk_frame_new("");
    auto box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto label = gtk_label_new(lbl.c_str());
    gtk_fixed_put(canvas, frame, 0, 0);
    gtk_box_pack_start(GTK_BOX(box), label, true, true, 2);
    gtk_container_add(GTK_CONTAINER(frame), box);

    widget = GTK_FRAME(frame);
    gtk_widget_show_all(frame);
}

int VRNetworkWidget::ID() {
    return wID;
}

VRGuiNetwork::VRGuiNetwork() {
    canvas = VRWidgetsCanvas::create("networkCanvas");
    setToolButtonCallback("networkButtonUpdate", bind(&VRGuiNetwork::update, this));
    setNoteBookCallback("notebook3", bind(&VRGuiNetwork::onTabSwitched, this, placeholders::_1, placeholders::_2));
    update();

    auto sm = VRSceneManager::get();
    updateFlowsCb = VRUpdateCb::create("network_ui_update", bind(&VRGuiNetwork::updateFlows, this));
    sm->addUpdateFkt(updateFlowsCb);
}

VRGuiNetwork::~VRGuiNetwork() {}

void VRGuiNetwork::onTabSwitched(GtkWidget* page, unsigned int tab) {
    auto nbook = VRGuiBuilder::get()->get_widget("notebook3");
    string name = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(nbook), page);
    if (name == "Network") {
        update();
        tabIsVisible = true;
    } else tabIsVisible = false;
}

void VRGuiNetwork::clear() {
    canvas->clear();
}

int VRGuiNetwork::addNode(string label, Vec2i pos) {
    auto cw = VRNetworkWidgetPtr( new VRNetworkWidget(label, canvas->getCanvas()) );
    canvas->addWidget(cw->wID, cw);
    cw->move(Vec2d(pos));
    return cw->wID;
    //canvas->connect(canvas->getWidget(w->ID()), cw, "#FFEE00");
}

void VRGuiNetwork::connectNodes(int n1, int n2, string color) {
    auto w1 = canvas->getWidget(n1);
    auto w2 = canvas->getWidget(n2);
    canvas->connect(w1, w2, color);
}

int VRGuiNetwork::addUDP(VRUDPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name;

    int n1 = addNode(label, position);
    int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    connectNodes(n1, n2, "#FF00FF");
    return n1;
}

int VRGuiNetwork::addTCP(VRTCPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name;

    int n1 = addNode(label, position);
    int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    string color = client->connected() ? "#0F0" : "#F00";
    connectNodes(n1, n2, color);
    return n1;
}

void VRGuiNetwork::addICE(VRICEClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getTurnServer();

    string label = protocol + " cli " + name;

    int n1 = addNode(label, position);
    int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    connectNodes(n1, n2, "#FF00FF");

    int i=1;
    for (auto c : client->getClients()) {
        cout << " VRGuiNetwork, channel clients with: " << c.first << endl;
        int np = addNode("peer: "+c.first, position+Vec2i(25, 50*i));
        connectNodes(n1, np, "#FF00FF");
        i++;

        for (auto d : c.second) {
            string sub_protocol = d.second->getProtocol();
            Vec2i pos = position+Vec2i(50, 50*i);
            int nc = 0;
            if (sub_protocol == "udp") nc = addUDP(dynamic_pointer_cast<VRUDPClient>(d.second), pos);
            if (sub_protocol == "tcp") nc = addTCP(dynamic_pointer_cast<VRTCPClient>(d.second), pos);
            connectNodes(np, nc, "#FF00FF");
            i++;
        }
    }
    position += Vec2i(0,50*(i-1));
}

void VRGuiNetwork::update() {
    clear();
    addNode("Network Clients:", Vec2i(50, 50));

    auto netMgr = VRSceneManager::get();
    if (!netMgr) return;

    //map<string, VRNetworkRemote> remotes;

    auto clients = netMgr->getNetworkClients();
    Vec2i position(100, 100);
    for (auto& client : clients) {
        if (!client) continue;

        string protocol = client->getProtocol();
        position += Vec2i(0, 50);
        if (protocol == "udp") addUDP(dynamic_pointer_cast<VRUDPClient>(client), position);
        if (protocol == "tcp") addTCP(dynamic_pointer_cast<VRTCPClient>(client), position);
        if (protocol == "ice") addICE(dynamic_pointer_cast<VRICEClient>(client), position);
    }

    //canvas->updateLayout();
}

void VRGuiNetwork::updateFlows() {
    if (!tabIsVisible) return;

    auto netMgr = VRSceneManager::get();
    if (!netMgr) return;
    auto clients = netMgr->getNetworkClients();

    for (auto& client : clients) {
        if (!client) continue;
        double kbs = client->getOutFlow().getKBperSec();
        if (kbs > 0) cout << " updateFlows, kbs: " << kbs << endl;
    }
}

