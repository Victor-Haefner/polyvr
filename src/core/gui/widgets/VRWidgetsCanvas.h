#ifndef VRWIDGETSCANVAS_H_INCLUDED
#define VRWIDGETSCANVAS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/VRMathFwd.h"
#include "addons/Algorithms/VRAlgorithmsFwd.h"
#include "../VRGuiFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <map>

struct _GtkFixed;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWidgetsCanvas : public std::enable_shared_from_this<VRWidgetsCanvas> {
	private:
	    _GtkFixed* canvas = 0;

        map<int, VRCanvasWidgetPtr> widgets;
        map<int, map<int, VRConnectorWidgetPtr> > connectors;
        map<int, int> widgetIDs;

        VRGraphLayoutPtr layout;
        GraphPtr layout_graph;
        VRUpdateCbPtr updateLayoutCb;

	public:
		VRWidgetsCanvas(string canvasName);
		~VRWidgetsCanvas();

		static VRWidgetsCanvasPtr create(string canvasName);
		VRWidgetsCanvasPtr ptr();

		_GtkFixed* getCanvas();
		VRGraphLayoutPtr getLayout();

        void clear();
        void addNode(int sID);
        void remNode(int sID);
        int getNode(int sID);

        void addWidget(int sID, VRCanvasWidgetPtr w);
        void remWidget(int sID);
        VRCanvasWidgetPtr getWidget(int sID);

        void connect(VRCanvasWidgetPtr p1, VRCanvasWidgetPtr p2, string color);
        void disconnect(VRCanvasWidgetPtr p1, VRCanvasWidgetPtr p2);
        void disconnectAny(VRCanvasWidgetPtr p1);

        void foldAll(bool b);
        void updateLayout();
};

OSG_END_NAMESPACE;

#endif //VRWIDGETSCANVAS_H_INCLUDED
