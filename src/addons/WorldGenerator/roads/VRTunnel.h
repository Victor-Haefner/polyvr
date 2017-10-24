#ifndef VRTUNNEL_H_INCLUDED
#define VRTUNNEL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRRoadBase.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTunnel : public VRRoadBase {
    private:
        VRRoadPtr road;

    public:
        VRTunnel(VRRoadPtr road);
        ~VRTunnel();

        static VRTunnelPtr create(VRRoadPtr road);

        VRGeometryPtr createGeometry();
};

OSG_END_NAMESPACE;

#endif // VRTUNNEL_H_INCLUDED
