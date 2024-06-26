#ifndef VRGUINETWORK_H_INCLUDED
#define VRGUINETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "widgets/VRCanvasWidget.h"
#include "core/networking/VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "VRGuiFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkWidget : public VRCanvasWidget {
    public:
        int wID;

    public:
        VRNetworkWidget();

        int ID() override;
};

class VRNetNodeWidget : public VRNetworkWidget {
    public:
        VRNetNodeWidget(string label);
};

class VRDataFlowWidget : public VRNetworkWidget {
    private:
        //_GtkWidget* area = 0;
        vector<double> curve;
        int W = 24;
        int H = 16;

    public:
        VRDataFlowWidget();

        void setCurve(vector<double> data);
};

class VRGuiNetwork {
	private:
        bool tabIsVisible = false;
        VRWidgetsCanvasPtr canvas;
        VRUpdateCbPtr updateFlowsCb;
        map<void*, vector<int>> flows;

        void clear();
        int addFlow(Vec2i pos, void* key);
        int addNode(string label, Vec2i pos);
        void connectNodes(int n1, int n2, string color);

        int addUDPClient(VRUDPClientPtr client, Vec2i& position);
        int addUDPServer(VRUDPServerPtr server, Vec2i& position);
        int addTCPClient(VRTCPClientPtr client, Vec2i& position);
        int addTCPServer(VRTCPServerPtr server, Vec2i& position);
        void addICEClient(VRICEClientPtr client, Vec2i& position);

        void onTabSwitched(string tab);
        void updateFlows();

	public:
		VRGuiNetwork();
		~VRGuiNetwork();

		void update();
};

OSG_END_NAMESPACE;

#endif //VRGUINETWORK_H_INCLUDED
