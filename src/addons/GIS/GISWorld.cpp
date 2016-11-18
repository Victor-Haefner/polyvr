#include "GISWorld.h"

#include "addons/Semantics/Reasoning/VROntology.h"

#include <iostream>

using namespace OSG;
using namespace std;

GISWorld::GISWorld() {
    ;
}

void GISWorld::setupOntology() {
    VROntologyPtr world = VROntology::create("World");
    VROntology::library["World"] = world;
    world->merge( VROntology::library["Math"] );

    auto World = world->addConcept("World", "Area");
    auto Country = world->addConcept("Country", "Area");
    auto Region = world->addConcept("Region", "Area");
    auto City = world->addConcept("City", "Area");

    World->addProperty("countries", Country);
    World->addProperty("cities", City);
    Country->addProperty("regions", Region);

    auto WayNetwork = world->addConcept("WayNetwork", "", "The network of all ways in a country");
    auto Way = world->addConcept("Way", "Area");
    auto Road = world->addConcept("Road", "Way");
    auto Sidewalk = world->addConcept("Sidewalk", "Way");
    auto Crossing = world->addConcept("Crossing", "Node");
    auto RoadIntersection = world->addConcept("RoadIntersection", "Crossing");
    auto Lane = world->addConcept("Lane", "Way");
    auto Building = world->addConcept("Building", "Area");
    auto RoadMarking = world->addConcept("RoadMarking", "Border");
    auto Kerb = world->addConcept("Kerb", "Border", "A stone edging to a pavement or raised way");

    World->addProperty("ways", WayNetwork);
    WayNetwork->addProperty("ways", Way);
    WayNetwork->addProperty("nodes", Crossing);
    Way->addProperty("crossings", Crossing);
    Way->addProperty("areas", "Area");
    Lane->addProperty("width", "float");
    Road->addProperty("lanes", Lane);
    Road->addProperty("sidewalks", Sidewalk);
    Road->addProperty("intersections", RoadIntersection);
    Road->addProperty("buildings", Building);
    Road->addProperty("path", "Path");
    Road->addProperty("markings", RoadMarking);
    Sidewalk->addProperty("kerbs", Kerb);
    RoadMarking->addProperty("width", "float");

    auto Plant = world->addConcept("Plant");
    auto Tree = world->addConcept("Tree", "Plant");
    auto Bush = world->addConcept("Bush", "Plant");
}
