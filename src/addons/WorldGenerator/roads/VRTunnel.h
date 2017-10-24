#ifndef VRTUNNEL_H_INCLUDED
#define VRTUNNEL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTunnel {
    private:
    public:
        VRTunnel();
        ~VRTunnel();

        VRTunnelPtr create();
};

OSG_END_NAMESPACE;

#endif // VRTUNNEL_H_INCLUDED
