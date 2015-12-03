#include "VRMillingWorkPiece.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE
using namespace std;

VRMillingWorkPiece::VRMillingWorkPiece(string name) : VRGeometry(name), octree(0.01) {
	type = "MillingWorkPiece";
	uFkt = VRFunction<int>::create("MillingWorkPiece-update", boost::bind(&VRMillingWorkPiece::update, this));
	VRSceneManager::getCurrent()->addUpdateFkt(uFkt);
}

VRMillingWorkPiecePtr VRMillingWorkPiece::ptr() { return static_pointer_cast<VRMillingWorkPiece>( shared_from_this() ); }
VRMillingWorkPiecePtr VRMillingWorkPiece::create(string name) { return shared_ptr<VRMillingWorkPiece>(new VRMillingWorkPiece(name) ); }

void VRMillingWorkPiece::setCuttingTool(VRTransformPtr geo) {
    tool = geo;
    toolPose = geo->getWorldPose();
    lastToolChange = geo->getLastChange();
}

void VRMillingWorkPiece::reset(Vec3i gSize, float bSize) {
    gridSize = gSize;
    blockSize = bSize;

    // TODO
}

void VRMillingWorkPiece::update() {
    auto geo = tool.lock();
    if (!geo) return;
    int change = geo->getLastChange();
    if (change == lastToolChange) return; // keine bewegung
    lastToolChange = change;

    // TODO
}

OSG_END_NAMESPACE
