#ifndef VRGIZMO_H_INCLUDED
#define VRGIZMO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRToolsFwd.h"
#include "core/objects/VRTransform.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGizmo : public VRTransform {
	private:
	    VRTransformPtr target;
	    Vec3d tOffset;
	    VRUpdateCbPtr updateCb;
	    PosePtr tBase;
	    PosePtr mBase;
	    Vec3d sBase;
	    Vec3d rBase;

	    VRGeometryPtr cRot;
	    VRGeometryPtr cRotX;
	    VRGeometryPtr cRotY;
	    VRGeometryPtr cRotZ;
	    VRGeometryPtr aTransX;
	    VRGeometryPtr aTransY;
	    VRGeometryPtr aTransZ;
	    VRGeometryPtr aScaleX;
	    VRGeometryPtr aScaleY;
	    VRGeometryPtr aScaleZ;

	    void setup();
	    void update();

	public:
		VRGizmo(string name);
		~VRGizmo();

		static VRGizmoPtr create(string name = "gizmo");
		VRGizmoPtr ptr();

        void setTarget(VRTransformPtr t);
};

OSG_END_NAMESPACE;

#endif //VRGIZMO_H_INCLUDED
