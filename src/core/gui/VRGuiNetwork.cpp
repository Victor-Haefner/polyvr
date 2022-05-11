#include "VRGuiNetwork.h"
#include "widgets/VRWidgetsCanvas.h"

#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"

#include "core/scene/VRSceneManager.h"
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
}

VRGuiNetwork::~VRGuiNetwork() {}

void VRGuiNetwork::onTabSwitched(GtkWidget* page, unsigned int tab) {
    auto nbook = VRGuiBuilder::get()->get_widget("notebook3");
    string name = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(nbook), page);
    if (name == "Network") update();
}

void VRGuiNetwork::clear() {
    canvas->clear();
}

void VRGuiNetwork::addNode(string label, Vec2i pos) {
    auto cw = VRNetworkWidgetPtr( new VRNetworkWidget(label, canvas->getCanvas()) );
    canvas->addWidget(cw->wID, cw);
    cw->move(Vec2d(pos));
    //canvas->connect(canvas->getWidget(w->ID()), cw, "#FFEE00");
}

void VRGuiNetwork::addUDP(VRUDPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name + " -> " + remoteUri;

    addNode(label, position);
}

void VRGuiNetwork::addTCP(VRTCPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name + " -> " + remoteUri;

    addNode(label, position);
}

void VRGuiNetwork::addICE(VRICEClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name + " -> " + remoteUri;

    addNode(label, position);

    int i=1;
    for (auto c : client->getClients()) {
        cout << " VRGuiNetwork, channel clients with: " << c.first << endl;
        addNode("peer: "+c.first, position+Vec2i(25, 50*i));
        i++;

        for (auto d : c.second) {
            string sub_protocol = d.second->getProtocol();
            Vec2i pos = position+Vec2i(50, 50*i);
            if (sub_protocol == "udp") addUDP(dynamic_pointer_cast<VRUDPClient>(d.second), pos);
            if (sub_protocol == "tcp") addTCP(dynamic_pointer_cast<VRTCPClient>(d.second), pos);
            i++;
        }
    }
    position += Vec2i(0,50*i);
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
}



