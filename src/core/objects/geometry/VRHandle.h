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
        VRUpdatePtr updateCb;
        VRAnimPtr paramCb;

        float value = 0;
        float scale = 1;
        Vec3f axis;
        posePtr origin;
        TYPE constraint = LINEAR;

    public:
        VRHandle(string name);

        static VRHandlePtr create(string name);
        VRHandlePtr ptr();

        void updateHandle();
        void configure(VRAnimPtr cb, TYPE t, Vec3f n, float scale, bool symmetric);
        void set(posePtr p, float v);

        Vec3f getAxis();
        posePtr getOrigin();

        void drag(VRTransformPtr new_parent);
        void drop();

        void setMatrix(Matrix m);
};

OSG_END_NAMESPACE;

#endif // VRHANDLE_H_INCLUDED
