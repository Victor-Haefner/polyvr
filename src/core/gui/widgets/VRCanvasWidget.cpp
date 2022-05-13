#include "VRCanvasWidget.h"
#include "VRConnectorWidget.h"

#include <gtk/gtk.h>

using namespace OSG;

VRCanvasWidget::VRCanvasWidget(GtkFixed* canvas) {
    this->canvas = canvas;
}

VRCanvasWidget::~VRCanvasWidget() {
    auto parent = gtk_widget_get_parent(GTK_WIDGET(widget));
    if (parent == GTK_WIDGET(canvas)) {
        gtk_container_remove(GTK_CONTAINER(canvas), GTK_WIDGET(widget));
        gtk_widget_show_all(GTK_WIDGET(canvas));
    }
}

VRCanvasWidgetPtr VRCanvasWidget::ptr() { return static_pointer_cast<VRCanvasWidget>(shared_from_this()); }

void VRCanvasWidget::move(Vec2d p) {
    pos = p;
    float w = gtk_widget_get_allocated_width(GTK_WIDGET(widget));
    float h = gtk_widget_get_allocated_height(GTK_WIDGET(widget));
    //cout << "move fixed " << GTK_WIDGET(widget) << ", " << int(p[0]-w*0.5) << " " << int(p[1]-h*0.5) << ", size: " << Vec2i(w,h) << endl;
    gtk_fixed_move(canvas, GTK_WIDGET(widget), p[0]-w*0.5, p[1]-h*0.5);
}

void VRCanvasWidget::setFolding(bool folded) {
    subTreeFolded = folded;
    for (auto ww : children) {
        if (auto w = ww.second.lock()) w->setVisible(!folded);
    }
}

void VRCanvasWidget::setVisible(bool v) {
    visible = v;
    if (v) gtk_widget_show_all(GTK_WIDGET(widget));
    else gtk_widget_hide(GTK_WIDGET(widget));
    for (auto wc : connectors) if (auto c = wc.second.lock()) c->setVisible(v);

    if (subTreeFolded && visible) return;
    for (auto ww : children) {
        if (auto w = ww.second.lock()) w->setVisible(v);
    }
}

Vec3d VRCanvasWidget::getPosition() { return Vec3d(pos[0], pos[1], 0); }

Vec3d VRCanvasWidget::getSize() {
    int W = gtk_widget_get_allocated_width(GTK_WIDGET(widget));
    int H = gtk_widget_get_allocated_height(GTK_WIDGET(widget));
    return Vec3d(W*0.5, H*0.5, 0);
}

Vec2d VRCanvasWidget::getAnchorPoint(Vec2d p) {
    int W = gtk_widget_get_allocated_width(GTK_WIDGET(widget));
    int H = gtk_widget_get_allocated_height(GTK_WIDGET(widget));
    float w = abs(p[0]-pos[0]);
    float h = abs(p[1]-pos[1]);

    if (origin == TOP_LEFT) {
        if (w >= h && p[0] < pos[0]) return pos + Vec2d(0, H*0.5);
        if (w >= h && p[0] > pos[0]) return pos + Vec2d(W, H*0.5);
        if (w < h  && p[1] < pos[1]) return pos + Vec2d(W*0.5, 0);
        if (w < h  && p[1] > pos[1]) return pos + Vec2d(W*0.5, H);
    }

    if (origin == CENTER) {
        if (w >= h && p[0] < pos[0]) return pos - Vec2d(W*0.5, 0);
        if (w >= h && p[0] > pos[0]) return pos + Vec2d(W*0.5, 0);
        if (w < h  && p[1] < pos[1]) return pos - Vec2d(0, H*0.5);
        if (w < h  && p[1] > pos[1]) return pos + Vec2d(0, H*0.5);
    }
    return pos;
}

