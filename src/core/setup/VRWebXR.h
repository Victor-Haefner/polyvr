#ifndef VRWEBXR_H_INCLUDED
#define VRWEBXR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRSetupFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/setup/devices/VRDevice.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWebXR : public VRDevice {
	private:
        PosePtr room;
        PosePtr head;
        PosePtr tempCam;
        map <string,VRTransformPtr> trackers;

        PosePtr toPose(float x, float y, float z, float qx, float qy, float qz, float qw);
        void init();

	public:
		VRWebXR();
		~VRWebXR();

		static VRWebXRPtr create();
		VRWebXRPtr ptr();

        void preRender();
        void postRender();

		void setPose(string ID, float x, float y, float z, float qx, float qy, float qz, float qw);
};

OSG_END_NAMESPACE;

#endif //VRWEBXR_H_INCLUDED
