#ifndef VRAXLESEGMENTATION_H_INCLUDED
#define VRAXLESEGMENTATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "VRMechanismFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRAxleSegmentation {
    private:
        VRObjectPtr obj;
        double radius = 0;

    public:
        VRAxleSegmentation();
        ~VRAxleSegmentation();
        static VRAxleSegmentationPtr create();

        void analyse(VRObjectPtr obj);
};

OSG_END_NAMESPACE;

#endif // VRAXLESEGMENTATION_H_INCLUDED
