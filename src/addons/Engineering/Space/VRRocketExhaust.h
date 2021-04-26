#ifndef VRROCKETEXHAUST_H_INCLUDED
#define VRROCKETEXHAUST_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VREngineeringFwd.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRocketExhaust : public VRGeometry {
	private:
        VRMaterialPtr mat;
        float amount = 0;
        int cycle = 0;
        VRUpdateCbPtr updateCb;

        void update();
        void init();

	public:
		VRRocketExhaust(string name);
		~VRRocketExhaust();

		static VRRocketExhaustPtr create(string name = "rocketExhaust");
		VRRocketExhaustPtr ptr();

		void set(float amount);
};

OSG_END_NAMESPACE;

#endif //VRROCKETEXHAUST_H_INCLUDED
