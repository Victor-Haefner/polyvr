#include "VRWorldModule.h"
#include "VRWorldGenerator.h"

using namespace OSG;

VRWorldModule::VRWorldModule() {}
VRWorldModule::~VRWorldModule() {}

void VRWorldModule::setWorld(VRWorldGeneratorPtr w) {
    world = w;
    planet = world->getPlanet();
    terrain = world->getTerrain();
    ontology = world->getOntology();
}
