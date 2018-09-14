#ifndef VRHANDLE_H_INCLUDED
#define VRHANDLE_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "VRGeometry.h"
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRHandle : public VRGeometry {
    public:
        enum TYPE {
            LINEAR,
            ROTATOR
        };

    private:
        VRUpdateCbPtr updateCb;
        VRAnimCbPtr paramCb;

        float value = 0;
        float scale = 1;
        Vec3d axis;
        PosePtr origin;
        TYPE constraint = LINEAR;

        vector<VRHandleWeakPtr> siblings;

    public:
        VRHandle(string name);

        static VRHandlePtr create(string name = "handle");
        VRHandlePtr ptr();

        void updateHandle(bool su = false);
        void configure(VRAnimCbPtr cb, TYPE t, Vec3d n, float scale);
        void addSibling(VRHandlePtr cb);
        void set(PosePtr p, float v);

        Vec3d getAxis();
        PosePtr getOrigin();

        void drag(VRTransformPtr new_parent);
        void drop();

        void setMatrix(Matrix4d m);
};

OSG_END_NAMESPACE;

#endif // VRHANDLE_H_INCLUDED
