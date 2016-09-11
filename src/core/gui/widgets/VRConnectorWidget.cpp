#include "VRConnectorWidget.h"
#include "VRSemanticWidget.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"

#include <gtkmm/fixed.h>
#include <gtkmm/separator.h>

using namespace OSG;


VRConnectorWidget::VRConnectorWidget(Gtk::Fixed* canvas, string color) {
    sh1 = Gtk::manage( new Gtk::HSeparator() );
    sh2 = Gtk::manage( new Gtk::HSeparator() );
    sv1 = Gtk::manage( new Gtk::VSeparator() );
    sv2 = Gtk::manage( new Gtk::VSeparator() );
    this->canvas = canvas;
    canvas->put(*sh1, 0, 0);
    canvas->put(*sh2, 0, 0);
    canvas->put(*sv1, 0, 0);
    canvas->put(*sv2, 0, 0);

    sh1->modify_bg( Gtk::STATE_NORMAL, Gdk::Color(color));
    sh2->modify_bg( Gtk::STATE_NORMAL, Gdk::Color(color));
    sv1->modify_bg( Gtk::STATE_NORMAL, Gdk::Color(color));
    sv2->modify_bg( Gtk::STATE_NORMAL, Gdk::Color(color));
}

VRConnectorWidget::~VRConnectorWidget() {
    canvas->remove(*sh1);
    canvas->remove(*sh2);
    canvas->remove(*sv1);
    canvas->remove(*sv2);
}

void VRConnectorWidget::set(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2) {
    this->w1 = w1;
    this->w2 = w2;
    update();
}

void VRConnectorWidget::update() {
    auto ws1 = w1.lock();
    auto ws2 = w2.lock();
    if (ws1 && ws2) {
        Vec2f a1 = ws1->getAnchorPoint(ws2->pos);
        Vec2f a2 = ws2->getAnchorPoint(ws1->pos);
        float x1 = a1[0];
        float x2 = a2[0];
        float y1 = a1[1];
        float y2 = a2[1];

        float w = abs(x2-x1);
        float h = abs(y2-y1);

        sh1->set_size_request(0, 0);
        sh2->set_size_request(0, 0);
        sv1->set_size_request(0, 0);
        sv2->set_size_request(0, 0);

        if (w <= 2 && h <= 2) return;

        if (h <= 2) {
            sh1->show();
            sh1->set_size_request(w, 2);
            if (x2 < x1) swap(x2,x1);
            canvas->move(*sh1, x1, y1);
            return;
        }

        if (w <= 2) {
            sv1->show();
            sv1->set_size_request(2, h);
            if (y2 < y1) swap(y2,y1);
            canvas->move(*sv1, x1, y1);
            return;
        }

        if (w < h) {
            sh1->show();
            sh2->show();
            sv1->show();
            sh1->set_size_request(w*0.5, 2);
            sh2->set_size_request(w*0.5, 2);
            sv1->set_size_request(2, h);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(y1,y2);
                swap(x1,x2);
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            return;
        } else {
            sv1->show();
            sv2->show();
            sh1->show();
            sv1->set_size_request(2, h*0.5);
            sv2->set_size_request(2, h*0.5);
            sh1->set_size_request(w, 2);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(x1,x2);
                swap(y1,y2);
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            return;
        }
    }
}
