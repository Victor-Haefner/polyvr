#include "VRCanvasWidget.h"
#include "VRConnectorWidget.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

using namespace OSG;

VRCanvasWidget::VRCanvasWidget() {}
VRCanvasWidget::~VRCanvasWidget() {}

VRCanvasWidgetPtr VRCanvasWidget::ptr() { return static_pointer_cast<VRCanvasWidget>(shared_from_this()); }

void VRCanvasWidget::move(Vec2d p) {
    pos = p;
    float w = size[0];
    float h = size[1];
    uiSignal( "canvas_widget_move", {{"ID", toString(ID())}, {"x",toString(p[0]-w*0.5)}, {"y",toString(p[1]-h*0.5)}} );
}

void VRCanvasWidget::setFolding(bool folded) {
    subTreeFolded = folded;
    for (auto ww : children) {
        if (auto w = ww.second.lock()) w->setVisible(!folded);
    }
}

void VRCanvasWidget::setVisible(bool v) {
    visible = v;
    uiSignal("canvas_widget_set_visible", {{"value",toString(v)}});
    for (auto wc : connectors) if (auto c = wc.second.lock()) c->setVisible(v);

    if (subTreeFolded && visible) return;
    for (auto ww : children) {
        if (auto w = ww.second.lock()) w->setVisible(v);
    }
}

Vec3d VRCanvasWidget::getPosition() { return Vec3d(pos[0], pos[1], 0); }

Vec3d VRCanvasWidget::getSize() {
    int W = size[0];
    int H = size[1];
    return Vec3d(W*0.5, H*0.5, 0);
}

Vec2d VRCanvasWidget::getAnchorPoint(Vec2d p) {
    int W = size[0];
    int H = size[1];
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

