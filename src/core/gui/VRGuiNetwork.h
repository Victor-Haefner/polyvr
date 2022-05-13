#ifndef VRGUINETWORK_H_INCLUDED
#define VRGUINETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "widgets/VRCanvasWidget.h"
#include "core/networking/VRNetworkingFwd.h"
#include "VRGuiFwd.h"

struct _GtkWidget;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRNetworkWidget : public VRCanvasWidget {
    public:
        int wID;

    public:
        VRNetworkWidget(string label, _GtkFixed* canvas);

        int ID() override;
};

class VRGuiNetwork {
	private:
        VRWidgetsCanvasPtr canvas;

        void clear();
        int addNode(string label, Vec2i pos);
        void connectNodes(int n1, int n2, string color);

        int addUDP(VRUDPClientPtr client, Vec2i& position);
        int addTCP(VRTCPClientPtr client, Vec2i& position);
        void addICE(VRICEClientPtr client, Vec2i& position);

        void onTabSwitched(_GtkWidget* page, unsigned int tab);

	public:
		VRGuiNetwork();
		~VRGuiNetwork();

		void update();
};

OSG_END_NAMESPACE;

#endif //VRGUINETWORK_H_INCLUDED
