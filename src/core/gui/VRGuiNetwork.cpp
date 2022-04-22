#include "VRGuiNetwork.h"
#include "widgets/VRWidgetsCanvas.h"

#include "VRGuiUtils.h"

#include "core/scene/VRScene.h"

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
    update();
}

VRGuiNetwork::~VRGuiNetwork() {}

void VRGuiNetwork::clear() {
    canvas->clear();
}

void VRGuiNetwork::addNode(string label, Vec2i pos) {
    auto cw = VRNetworkWidgetPtr( new VRNetworkWidget(label, canvas->getCanvas()) );
    canvas->addWidget(cw->wID, cw);
    cw->move(Vec2d(pos));
    //canvas->connect(canvas->getWidget(w->ID()), cw, "#FFEE00");
}

void VRGuiNetwork::update() {
    clear();
    addNode("node1", Vec2i(50,  50));
    addNode("node2", Vec2i(50, 100));
    addNode("node3", Vec2i(50, 150));
    addNode("node4", Vec2i(50, 200));

    auto scene = VRScene::getCurrent();
    if (!scene) return;

    ;
}



