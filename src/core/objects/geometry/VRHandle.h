#ifndef VRHANDLE_H_INCLUDED
#define VRHANDLE_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "VRGeometry.h"
#include "core/math/pose.h"

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
        pose origin;
        TYPE constraint = LINEAR;

        void updateHandle();

    public:
        VRHandle(string name);

        static VRHandlePtr create(string name);
        VRHandlePtr ptr();

        void configure(VRAnimPtr cb, TYPE t, Vec3f n, float scale, bool symmetric);
        void set(pose p, float v);

        void drag(VRTransformPtr new_parent);
        void drop();

        void setMatrix(Matrix m);
};

OSG_END_NAMESPACE;

#endif // VRHANDLE_H_INCLUDED
