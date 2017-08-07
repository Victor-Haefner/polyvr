#include "VRTrafficSimulation.h"
#include "../roads/VRRoad.h"
#include "../VRWorldGenerator.h"
#include "../terrain/VRTerrain.h"
#include "core/utils/toString.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRObjectManager.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"

using namespace OSG;

VRTrafficSimulation::VRTrafficSimulation() : VRObject("TrafficSimulation") {}
VRTrafficSimulation::~VRTrafficSimulation() {}

VRTrafficSimulationPtr VRTrafficSimulation::create() { return VRTrafficSimulationPtr( new VRTrafficSimulation() ); }

void VRTrafficSimulation::setRoadNetwork(VRRoadNetworkPtr rds) { roads = rds; }

void VRTrafficSimulation::updateModel() {
    ;
}

void VRTrafficSimulation::doTimeStep() {
    ;
}
