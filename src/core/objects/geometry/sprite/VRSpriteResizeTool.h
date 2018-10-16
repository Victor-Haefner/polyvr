#ifndef VRSPRITERESIZETOOL_H_INCLUDED
#define VRSPRITERESIZETOOL_H_INCLUDED

#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSpriteResizeTool : public VRObject {
    private:
        VRSpritePtr target;

    public:
        VRSpriteResizeTool(VRSpritePtr target);
        ~VRSpriteResizeTool();

        static VRSpriteResizeToolPtr create(VRSpritePtr target);
};

OSG_END_NAMESPACE;

#endif // VRSPRITERESIZETOOL_H_INCLUDED
