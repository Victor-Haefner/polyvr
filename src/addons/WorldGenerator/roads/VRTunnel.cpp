#include "VRTunnel.h"
#include "VRRoad.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "core/objects/geometry/VRStroke.h"

using namespace OSG;

VRTunnel::VRTunnel(VRRoadPtr road) : VRRoadBase("tunnel"), road(road) {}
VRTunnel::~VRTunnel() {}

VRTunnelPtr VRTunnel::create(VRRoadPtr road) { return VRTunnelPtr(new VRTunnel(road)); }

VRGeometryPtr VRTunnel::createGeometry() {
    cout << " --- VRTunnel::createGeometry --- " << endl;
    auto geo = VRStroke::create("tunnel");

    vector<PathPtr> paths;
    for (auto p : road->getEntity()->getAllEntities("path")) {
        paths.push_back( toPath(p,32) );
    }
    geo->setPaths( paths );

    vector<Vec3d> profile;
    profile.push_back(Vec3d(5,0,0));
    profile.push_back(Vec3d(5,4,0));
    profile.push_back(Vec3d(4,5,0));
    profile.push_back(Vec3d(-4,5,0));
    profile.push_back(Vec3d(-5,4,0));
    profile.push_back(Vec3d(-5,0,0));
    geo->strokeProfile(profile, false, true, false);

    addChild(geo);
    return geo;
}
