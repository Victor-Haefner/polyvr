#ifndef VRCONNECTORWIDGET_H_INCLUDED
#define VRCONNECTORWIDGET_H_INCLUDED

#include "core/gui/VRGuiFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

#include <string>
#include <OpenSG/OSGVector.h>

struct _GtkWidget;
struct _GtkFixed;

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRConnectorWidget {
    _GtkWidget* sh1 = 0;
    _GtkWidget* sh2 = 0;
    _GtkWidget* sv1 = 0;
    _GtkWidget* sv2 = 0;
    _GtkFixed* canvas = 0;
    VRCanvasWidgetWeakPtr w1;
    VRCanvasWidgetWeakPtr w2;
    bool visible = true;
    Vec4d lastGeometry;

    VRConnectorWidget(_GtkFixed* canvas, string color);
    ~VRConnectorWidget();

    void set(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2);
    void setVisible(bool visible = true);
    void update();
};

OSG_END_NAMESPACE;

#endif // VRCONNECTORWIDGET_H_INCLUDED
