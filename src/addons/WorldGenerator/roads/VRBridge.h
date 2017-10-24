#ifndef VRBRIDGE_H_INCLUDED
#define VRBRIDGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBridge {
    private:
    public:
        VRBridge();
        ~VRBridge();

        VRBridgePtr create();
};

OSG_END_NAMESPACE;

#endif // VRBRIDGE_H_INCLUDED
