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
	    VRTransformPtr refT;
	    PosePtr tOffset;
	    VRUpdateCbPtr updateCb;
	    PosePtr tBase;
	    PosePtr mBase;
	    PosePtr rBase;
	    Vec3d sBase;
	    double angle0 = 0;

	    static string rotVP;
	    static string rotFP;

	    vector<bool> config;

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
	    void updateHandleVisibility();
	    void update();

	public:
		VRGizmo(string name);
		~VRGizmo();

		static VRGizmoPtr create(string name = "gizmo");
		VRGizmoPtr ptr();

        void setTarget(VRTransformPtr t);
        VRTransformPtr getTarget();

        void enableTranslation(bool x, bool y, bool z, bool onlyTranslation = false);
        void enableRotation(bool x, bool y, bool z, bool w, bool onlyRotation = false);
        void enableScaling(bool x, bool y, bool z, bool onlyScaling = false);
};

OSG_END_NAMESPACE;

#endif //VRGIZMO_H_INCLUDED
