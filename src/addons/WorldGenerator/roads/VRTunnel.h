#ifndef VRTUNNEL_H_INCLUDED
#define VRTUNNEL_H_INCLUDED

#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;

class VRTunnel {
    private:
    public:
        VRTunnel();
        ~VRTunnel();

        VRTunnelPtr create();
};

#endif // VRTUNNEL_H_INCLUDED
