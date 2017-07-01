#include "VRWorldGenerator.h"
#include "roads/VRRoadNetwork.h"
#include "core/objects/VRTransform.h"

using namespace OSG;

VRWorldGenerator::VRWorldGenerator() : VRObject("WorldGenerator") {}
VRWorldGenerator::~VRWorldGenerator() {}

VRWorldGeneratorPtr VRWorldGenerator::create() {
    auto wg = VRWorldGeneratorPtr( new VRWorldGenerator() );
    wg->init();
    return wg;
}

VRWorldGeneratorPtr VRWorldGenerator::ptr() { return dynamic_pointer_cast<VRWorldGenerator>( shared_from_this() ); }

VRRoadNetworkPtr VRWorldGenerator::getRoadNetwork() { return roads; }

void VRWorldGenerator::addAsset( string name, VRTransformPtr geo ) {
    assets[name] = geo;
}

VRTransformPtr VRWorldGenerator::getAsset(string name) {
    if (!assets.count(name)) return 0;
    return dynamic_pointer_cast<VRTransform>( assets[name]->duplicate() );
}

void VRWorldGenerator::init() {
    roads = VRRoadNetwork::create();
    roads->setWorld( ptr() );
    addChild(roads);
}

void VRWorldGenerator::setOntology(VROntologyPtr o) { ontology = o; }
VROntologyPtr VRWorldGenerator::getOntology() { return ontology; }
