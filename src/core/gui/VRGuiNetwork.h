#ifndef VRGUINETWORK_H_INCLUDED
#define VRGUINETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRGuiFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGuiNetwork {
	private:
        VRWidgetsCanvasPtr canvas;

	public:
		VRGuiNetwork();
		~VRGuiNetwork();
};

OSG_END_NAMESPACE;

#endif //VRGUINETWORK_H_INCLUDED
