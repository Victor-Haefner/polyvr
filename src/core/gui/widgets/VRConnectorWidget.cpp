#include "VRConnectorWidget.h"
#include "VRSemanticWidget.h"

#include "../VRGuiSemantics.h"

using namespace OSG;


VRConnectorWidget::VRConnectorWidget(_GtkFixed* canvas, string color) {
    this->canvas = canvas;

    /*GdkColor col;
    gdk_color_parse(color.c_str(), &col);

    auto addSeparator = [&](GtkOrientation o) {
        auto s = gtk_separator_new(o);
        gtk_fixed_put(canvas, s, 0, 0);
        gtk_widget_modify_bg(s, GTK_STATE_NORMAL, &col);
        return s;
    };

    sh1 = addSeparator(GTK_ORIENTATION_HORIZONTAL);
    sh2 = addSeparator(GTK_ORIENTATION_HORIZONTAL);
    sv1 = addSeparator(GTK_ORIENTATION_VERTICAL);
    sv2 = addSeparator(GTK_ORIENTATION_VERTICAL);*/
}

VRConnectorWidget::~VRConnectorWidget() {
    //for (auto s : {sh1, sh2, sv1, sv2}) gtk_container_remove(GTK_CONTAINER(canvas), s);
}

void VRConnectorWidget::set(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2) {
    this->w1 = w1;
    this->w2 = w2;
    update();
}

void VRConnectorWidget::setVisible(bool v) {
    visible = v;
    /*for (auto s : {sh1, sh2, sv1, sv2}) {
        if (v) gtk_widget_map(s);
        else gtk_widget_unmap(s);
    }*/
}

void VRConnectorWidget::update() {
    auto ws1 = w1.lock();
    auto ws2 = w2.lock();
    if (ws1 && ws2) {
        Vec2d a1 = ws1->getAnchorPoint(ws2->pos);
        Vec2d a2 = ws2->getAnchorPoint(ws1->pos);

        float x1 = a1[0];
        float x2 = a2[0];
        float y1 = a1[1];
        float y2 = a2[1];

        Vec4d geom(x1,x2,y1,y2);
        if (lastGeometry.dist(geom) < 0.1) return;
        lastGeometry = geom;

        float w = abs(x2-x1);
        float h = abs(y2-y1);

        //for (auto s : {sh1, sh2, sv1, sv2}) gtk_widget_set_size_request(s, 0, 0);

        if (w <= 2 && h <= 2) return;

        if (h <= 2) {
            /*gtk_widget_show(sh1);
            gtk_widget_set_size_request(sh1, w, 2);
            if (x2 < x1) swap(x2,x1);
            gtk_fixed_move(canvas, sh1, x1, y1);*/
            return;
        }

        if (w <= 2) {
            /*gtk_widget_show(sv1);
            gtk_widget_set_size_request(sv1, 2, h);
            if (y2 < y1) swap(y2,y1);
            gtk_fixed_move(canvas, sv1, x1, y1);*/
            return;
        }

        if (w < h) {
            /*gtk_widget_show(sh1);
            gtk_widget_show(sh2);
            gtk_widget_show(sv1);
            gtk_widget_set_size_request(sh1, w*0.5, 2);
            gtk_widget_set_size_request(sh2, w*0.5, 2);
            gtk_widget_set_size_request(sv1, 2, h);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                gtk_fixed_move(canvas, sh1, x1, y2);
                gtk_fixed_move(canvas, sh2, x1+w*0.5, y1);
                gtk_fixed_move(canvas, sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                gtk_fixed_move(canvas, sh1, x1, y2);
                gtk_fixed_move(canvas, sh2, x1+w*0.5, y1);
                gtk_fixed_move(canvas, sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                gtk_fixed_move(canvas, sh1, x1, y1);
                gtk_fixed_move(canvas, sh2, x1+w*0.5, y2);
                gtk_fixed_move(canvas, sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(y1,y2);
                swap(x1,x2);
                gtk_fixed_move(canvas, sh1, x1, y1);
                gtk_fixed_move(canvas, sh2, x1+w*0.5, y2);
                gtk_fixed_move(canvas, sv1, x1+w*0.5, y1);
                return;
            }*/
            return;
        } else {
            /*gtk_widget_show(sv1);
            gtk_widget_show(sv2);
            gtk_widget_show(sh1);
            gtk_widget_set_size_request(sv1, 2, h*0.5);
            gtk_widget_set_size_request(sv2, 2, h*0.5);
            gtk_widget_set_size_request(sh1, w, 2);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                gtk_fixed_move(canvas, sv1, x2, y1);
                gtk_fixed_move(canvas, sv2, x1, y1+h*0.5);
                gtk_fixed_move(canvas, sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(x1,x2);
                swap(y1,y2);
                gtk_fixed_move(canvas, sv1, x1, y1);
                gtk_fixed_move(canvas, sv2, x2, y1+h*0.5);
                gtk_fixed_move(canvas, sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                gtk_fixed_move(canvas, sv1, x2, y1);
                gtk_fixed_move(canvas, sv2, x1, y1+h*0.5);
                gtk_fixed_move(canvas, sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                gtk_fixed_move(canvas, sv1, x1, y1);
                gtk_fixed_move(canvas, sv2, x2, y1+h*0.5);
                gtk_fixed_move(canvas, sh1, x1, y1+h*0.5);
                return;
            }*/
            return;
        }
    }
}
