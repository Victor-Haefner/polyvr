#ifndef VRBRIDGE_H_INCLUDED
#define VRBRIDGE_H_INCLUDED

#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;

class VRBridge {
    private:
    public:
        VRBridge();
        ~VRBridge();

        VRBridgePtr create();
};

#endif // VRBRIDGE_H_INCLUDED
