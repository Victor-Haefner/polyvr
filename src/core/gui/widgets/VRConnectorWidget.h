#ifndef VRCONNECTORWIDGET_H_INCLUDED
#define VRCONNECTORWIDGET_H_INCLUDED

#include "core/gui/VRGuiFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

#include <string>
#include <OpenSG/OSGConfig.h>

struct _GtkSeparator;
struct _GtkFixed;

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRConnectorWidget {
    _GtkSeparator* sh1 = 0;
    _GtkSeparator* sh2 = 0;
    _GtkSeparator* sv1 = 0;
    _GtkSeparator* sv2 = 0;
    _GtkFixed* canvas = 0;
    VRSemanticWidgetWeakPtr w1;
    VRSemanticWidgetWeakPtr w2;
    bool visible = true;

    VRConnectorWidget(_GtkFixed* canvas, string color);
    ~VRConnectorWidget();

    void set(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2);
    void setVisible(bool visible = true);
    void update();
};

OSG_END_NAMESPACE;

#endif // VRCONNECTORWIDGET_H_INCLUDED
