#include "VRWorldModule.h"
#include "VRWorldGenerator.h"

using namespace OSG;

VRWorldModule::VRWorldModule() {}
VRWorldModule::~VRWorldModule() {}

void VRWorldModule::setWorld(VRWorldGeneratorPtr w) {
    world = w;
    planet = w->getPlanet();
    terrains = w->getTerrains();
    terrain = w->getTerrain();
    ontology = w->getOntology();
    roads = w->getRoadNetwork();
    lodTree = w->getLodTree();
}
