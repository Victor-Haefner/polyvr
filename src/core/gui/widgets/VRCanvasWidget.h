#ifndef VRCANVASWIDGET_H_INCLUDED
#define VRCANVASWIDGET_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "../VRGuiFwd.h"

#include <map>

struct _GtkFixed;
struct _GtkFrame;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCanvasWidget : public std::enable_shared_from_this<VRCanvasWidget> {
	public:
        Vec2d pos;
        bool visible = true;
        bool subTreeFolded = false;

        _GtkFixed* canvas = 0;
        _GtkFrame* widget = 0;

        map<VRCanvasWidget*, VRCanvasWidgetWeakPtr> children;
        map<VRConnectorWidget*, VRConnectorWidgetWeakPtr> connectors;

	public:
		VRCanvasWidget(_GtkFixed* canvas);
		~VRCanvasWidget();

		VRCanvasWidgetPtr ptr();

        virtual int ID() = 0;

        void move(Vec2d p);

        Vec3d getPosition();
        Vec3d getSize();
        Vec2d getAnchorPoint(Vec2d p);

        void setVisible(bool visible = true);
        void setFolding(bool folded = true);
};

OSG_END_NAMESPACE;

#endif //VRCANVASWIDGET_H_INCLUDED
