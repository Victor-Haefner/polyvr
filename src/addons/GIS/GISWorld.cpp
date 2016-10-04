#include "GISWorld.h"

#include "addons/Semantics/Reasoning/VROntology.h"

using namespace OSG;
using namespace std;

GISWorld::GISWorld() {
    ;
}

void GISWorld::setupOntology() {
    world = VROntology::create("world");

    auto World = world->addConcept("World");
    auto Area = world->addConcept("Area");
    auto Border = world->addConcept("Border");
    auto Country = world->addConcept("Country", "Area");
    auto Region = world->addConcept("Region", "Area");
    auto City = world->addConcept("City", "Area");

    World->addProperty("countries", Country);
    Area->addProperty("borders", Border);

    auto RoadNetwork = world->addConcept("RoadNetwork");
    auto Road = world->addConcept("Road");
    auto Crossing = world->addConcept("Crossing");
    auto Intersection = world->addConcept("Intersection", "Crossing");
    auto Lane = world->addConcept("Lane");
    auto Building = world->addConcept("Building");
    auto Walkway = world->addConcept("Walkway", "Border");

    Country->addProperty("roadnetwork", RoadNetwork);
    RoadNetwork->addProperty("roads", Road);
    RoadNetwork->addProperty("nodes", Crossing);
    Road->addProperty("lanes", Lane);
    Road->addProperty("intersections", Intersection);
    Road->addProperty("crossings", Crossing);
    Road->addProperty("borders", Border);
    Road->addProperty("buildings", Building);



    auto Plant = world->addConcept("Plant");
    auto Tree = world->addConcept("Tree", "Plant");
    auto Bush = world->addConcept("Bush", "Plant");
}
