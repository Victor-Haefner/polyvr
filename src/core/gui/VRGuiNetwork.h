#ifndef VRGUINETWORK_H_INCLUDED
#define VRGUINETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "widgets/VRCanvasWidget.h"
#include "core/networking/VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "VRGuiFwd.h"

struct _GtkWidget;
struct _cairo;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkWidget : public VRCanvasWidget {
    public:
        int wID;

    public:
        VRNetworkWidget(_GtkFixed* canvas);

        int ID() override;
};

class VRNetNodeWidget : public VRNetworkWidget {
    public:
        VRNetNodeWidget(string label, _GtkFixed* canvas);
};

class VRDataFlowWidget : public VRNetworkWidget {
    private:
        _GtkWidget* area = 0;
        vector<double> curve;
        int W = 24;
        int H = 16;

        bool onExpose(_cairo* cr);

    public:
        VRDataFlowWidget(_GtkFixed* canvas);

        void setCurve(vector<double> data);
};

class VRGuiNetwork {
	private:
        VRWidgetsCanvasPtr canvas;
        bool tabIsVisible = false;
        VRUpdateCbPtr updateFlowsCb;

        void clear();
        int addFlow(Vec2i pos);
        int addNode(string label, Vec2i pos);
        void connectNodes(int n1, int n2, string color);

        int addUDP(VRUDPClientPtr client, Vec2i& position);
        int addTCP(VRTCPClientPtr client, Vec2i& position);
        void addICE(VRICEClientPtr client, Vec2i& position);

        void onTabSwitched(_GtkWidget* page, unsigned int tab);

        void updateFlows();

	public:
		VRGuiNetwork();
		~VRGuiNetwork();

		void update();
};

OSG_END_NAMESPACE;

#endif //VRGUINETWORK_H_INCLUDED
