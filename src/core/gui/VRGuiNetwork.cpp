#include "VRGuiNetwork.h"
#include "widgets/VRWidgetsCanvas.h"

#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/networking/VRNetworkManager.h"
#include "core/networking/VRNetworkClient.h"
#include "core/networking/VRNetworkServer.h"
#include "core/networking/tcp/VRTCPClient.h"
#include "core/networking/tcp/VRTCPServer.h"
#include "core/networking/tcp/VRICEclient.h"
#include "core/networking/udp/VRUDPClient.h"
#include "core/networking/udp/VRUDPServer.h"

using namespace OSG;

VRNetworkWidget::VRNetworkWidget() : VRCanvasWidget() {
    static int i = 0;
    i++;
    wID = i;

    origin = TOP_LEFT;

    /*auto frame = gtk_frame_new(0);
    gtk_fixed_put(canvas, frame, 0, 0);
    widget = GTK_FRAME(frame);
    gtk_widget_show_all(frame);*/
}

int VRNetworkWidget::ID() {
    return wID;
}

VRNetNodeWidget::VRNetNodeWidget(string lbl) : VRNetworkWidget() {
    /*auto box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto label = gtk_label_new(lbl.c_str());
    gtk_box_pack_start(GTK_BOX(box), label, true, true, 2);
    gtk_container_add(GTK_CONTAINER(widget), box);
    gtk_widget_show_all(GTK_WIDGET(widget));*/
}

VRDataFlowWidget::VRDataFlowWidget() : VRNetworkWidget() {
    curve = vector<double>(W,0);

    /*area = gtk_drawing_area_new();
    connect_signal<void, cairo_t*>(area, bind(&VRDataFlowWidget::onExpose, this, placeholders::_1), "draw");
    gtk_widget_set_size_request(area, W, H);

    gtk_container_add(GTK_CONTAINER(widget), area);
    gtk_widget_show_all(GTK_WIDGET(widget));*/
}

void VRDataFlowWidget::setCurve(vector<double> data) {
    curve = data;
    if (curve.size() != W) {
        W = curve.size();
        //gtk_widget_set_size_request(area, W, H);
    }
    //gtk_widget_queue_draw_area(area, 0, 0, W, H);
}

bool VRDataFlowWidget::onExpose() {
    /*guint width = gtk_widget_get_allocated_width (area);
    guint height = gtk_widget_get_allocated_height (area);

    GtkStyleContext* context = gtk_widget_get_style_context (area);
    gtk_render_background(context, cr, 0, 0, width, height);

    GdkRGBA color;
    gtk_style_context_get_color (context, gtk_style_context_get_state (context), &color);
    gdk_cairo_set_source_rgba (cr, &color);

    for (int i=0; i<min(W,int(width)); i++) {
        int h = max(1,int(curve[i]*height));
        cairo_rectangle(cr, i, height-h, 1, h);
    }
    cairo_fill (cr);*/

    return false;
}

VRGuiNetwork::VRGuiNetwork() {
    canvas = VRWidgetsCanvas::create("networkCanvas");
    //setToolButtonCallback("networkButtonUpdate", bind(&VRGuiNetwork::update, this));
    //setNoteBookCallback("notebook3", bind(&VRGuiNetwork::onTabSwitched, this, placeholders::_1, placeholders::_2));
    update();

    auto sm = VRSceneManager::get();
    updateFlowsCb = VRUpdateCb::create("network_ui_update", bind(&VRGuiNetwork::updateFlows, this));
    sm->addTimeoutFkt(updateFlowsCb, 0, 100);
}

VRGuiNetwork::~VRGuiNetwork() {}

/*void VRGuiNetwork::onTabSwitched(GtkWidget* page, unsigned int tab) {
    auto nbook = VRGuiBuilder::get()->get_widget("notebook3");
    string name = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(nbook), page);
    if (name == "Network") {
        update();
        tabIsVisible = true;
    } else tabIsVisible = false;
}*/

void VRGuiNetwork::clear() {
    canvas->clear();
}

void VRGuiNetwork::updateFlows() {
    if (!tabIsVisible) return;

    auto netMgr = VRSceneManager::get();
    if (!netMgr) return;

    //cout << "updateFlows" << endl;

    auto updateFlowWidgets = [&](VRNetworkFlow& flow) {
        if (!flows.count(&flow)) return;

        auto curve = flow.getKBperSec();
        double bMax = 1;
        for (auto c : curve) bMax = max(c*1000, bMax);
        for (auto& c : curve) c = c*1000/bMax;

        for (auto fl : flows[&flow]) {
            auto fwid = canvas->getWidget( fl );
            if (auto fw = dynamic_pointer_cast<VRDataFlowWidget>(fwid)) fw->setCurve(curve);
        }
    };

    auto clients = netMgr->getNetworkClients();
    for (auto& client : clients) {
        if (!client) continue;
        updateFlowWidgets(client->getInFlow());
        updateFlowWidgets(client->getOutFlow());
    }

    auto servers = netMgr->getNetworkServers();
    for (auto& server : servers) {
        if (!server) continue;
        updateFlowWidgets(server->getInFlow());
        updateFlowWidgets(server->getOutFlow());
    }
}

int VRGuiNetwork::addFlow(Vec2i pos, void* key) {
    /*auto dfw = VRDataFlowWidgetPtr( new VRDataFlowWidget(canvas->getCanvas()) );
    canvas->addWidget(dfw->wID, dfw);
    dfw->move(Vec2d(pos));
    if (!flows.count(key)) flows[key] = vector<int>();
    flows[key].push_back( dfw->wID );
    return dfw->wID;*/
}

int VRGuiNetwork::addNode(string label, Vec2i pos) {
    /*auto cw = VRNetNodeWidgetPtr( new VRNetNodeWidget(label, canvas->getCanvas()) );
    canvas->addWidget(cw->wID, cw);
    cw->move(Vec2d(pos));
    return cw->wID;*/
}

void VRGuiNetwork::connectNodes(int n1, int n2, string color) {
    auto w1 = canvas->getWidget(n1);
    auto w2 = canvas->getWidget(n2);
    canvas->connect(w1, w2, color);
}

int VRGuiNetwork::addUDPClient(VRUDPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name;

    int n1 = addNode(label, position);
    int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    connectNodes(n1, n2, "#FF00FF");

    addFlow(position+Vec2i(20,22), &client->getInFlow());
    addFlow(position+Vec2i(50,22), &client->getOutFlow());
    return n1;
}

int VRGuiNetwork::addUDPServer(VRUDPServerPtr server, Vec2i& position) {
    string name = server->getName();
    string protocol = server->getProtocol(); // tcp or udp

    string label = protocol + " srv " + name;

    int n1 = addNode(label, position);
    //int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    //connectNodes(n1, n2, "#FF00FF");

    addFlow(position+Vec2i(20,22), &server->getInFlow());
    addFlow(position+Vec2i(50,22), &server->getOutFlow());
    return n1;
}

int VRGuiNetwork::addTCPServer(VRTCPServerPtr server, Vec2i& position) {
    string name = server->getName();
    string protocol = server->getProtocol(); // tcp or udp

    string label = protocol + " srv " + name;

    int n1 = addNode(label, position);
    //int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    //connectNodes(n1, n2, "#FF00FF");

    addFlow(position+Vec2i(20,22), &server->getInFlow());
    addFlow(position+Vec2i(50,22), &server->getOutFlow());
    return n1;
}

int VRGuiNetwork::addTCPClient(VRTCPClientPtr client, Vec2i& position) {
    string name = client->getName();
    string protocol = client->getProtocol(); // tcp or udp
    string remoteUri = client->getConnectedUri();

    string label = protocol + " cli " + name;

    int n1 = addNode(label, position);
    int n2 = addNode(remoteUri, position+Vec2i(200, 0));
    string color = client->connected() ? "#0F0" : "#F00";
    connectNodes(n1, n2, color);

    addFlow(position+Vec2i(20,22), &client->getInFlow());
    addFlow(position+Vec2i(50,22), &client->getOutFlow());
    return n1;
}

void VRGuiNetwork::addICEClient(VRICEClientPtr client, Vec2i& position) {
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
            if (sub_protocol == "udp") nc = addUDPClient(dynamic_pointer_cast<VRUDPClient>(d.second), pos);
            if (sub_protocol == "tcp") nc = addTCPClient(dynamic_pointer_cast<VRTCPClient>(d.second), pos);
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

    Vec2i position(100, 100);
    auto clients = netMgr->getNetworkClients();
    for (auto& client : clients) {
        if (!client) continue;

        string protocol = client->getProtocol();
        position += Vec2i(0, 50);
        if (protocol == "udp") addUDPClient(dynamic_pointer_cast<VRUDPClient>(client), position);
        if (protocol == "tcp") addTCPClient(dynamic_pointer_cast<VRTCPClient>(client), position);
        if (protocol == "ice") addICEClient(dynamic_pointer_cast<VRICEClient>(client), position);
    }

    position += Vec2i (-50, 100);
    addNode("Network Servers:", position);
    position += Vec2i (50, 50);
    auto servers = netMgr->getNetworkServers();
    for (auto& server : servers) {
        if (!server) continue;

        string protocol = server->getProtocol();
        position += Vec2i(0, 50);
        if (protocol == "udp") addUDPServer(dynamic_pointer_cast<VRUDPServer>(server), position);
        if (protocol == "tcp") addTCPServer(dynamic_pointer_cast<VRTCPServer>(server), position);
    }

    //canvas->updateLayout();
}

