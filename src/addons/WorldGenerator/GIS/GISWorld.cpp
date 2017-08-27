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
    world->merge( VROntology::library["Object"] );

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
    auto RoadIntersection = world->addConcept("RoadIntersection", "Way");
    auto Lane = world->addConcept("Lane", "Way");
    auto GreenBelt = world->addConcept("GreenBelt", "Area");
    auto Building = world->addConcept("Building", "Area");
    auto RoadMarking = world->addConcept("RoadMarking", "Border");
    auto StopLine = world->addConcept("StopLine", "RoadMarking");
    auto RoadTrack = world->addConcept("RoadTrack", "Path");
    auto Kerb = world->addConcept("Kerb", "Border", "A stone edging to a pavement or raised way");
    auto Arrow = world->addConcept("Arrow");
    auto ParkingLot = world->addConcept("ParkingLot", "Road");
    auto ParallelParkingLot = world->addConcept("ParallelParkingLot", "ParkingLot");
    auto PerpendicularParkingLot = world->addConcept("PerpendicularParkingLot", "ParkingLot");
    auto AngledParkingLot = world->addConcept("AngledParkingLot", "ParkingLot");
    auto Sign = world->addConcept("Sign", "Object");

    World->addProperty("ways", WayNetwork);
    WayNetwork->addProperty("ways", Way);
    WayNetwork->addProperty("nodes", Crossing);
    Way->addProperty("crossings", Crossing);
    Way->addProperty("ID", "int");
    Way->addProperty("name", "string");
    Way->addProperty("areas", "Area");
    Way->addProperty("path", "Path");
    Way->addProperty("markings", RoadMarking);
    Way->addProperty("tracks", RoadTrack);
    Way->addProperty("lanes", "Area");
    Arrow->addProperty("position", "float");
    Arrow->addProperty("direction", "float");
    Arrow->addProperty("lane", Lane);
    Lane->addProperty("width", "float");
    Lane->addProperty("direction", "int");
    Lane->addProperty("path", "Path");
    Lane->addProperty("arrows", "Arrow");
    Lane->addProperty("pedestrian", "bool");
    GreenBelt->addProperty("width", "float");
    GreenBelt->addProperty("path", "Path");
    Road->addProperty("sidewalks", Sidewalk);
    Road->addProperty("intersections", RoadIntersection);
    Road->addProperty("buildings", Building);
    Road->addProperty("type", "string");
    Road->addProperty("signs", Sign);
    Sign->addProperty("type", "string");
    Sign->addProperty("direction", "Direction");
    Sign->addProperty("road", Road);
    RoadIntersection->addProperty("node", "Node");
    RoadIntersection->addProperty("roads", Road);
    Sidewalk->addProperty("kerbs", Kerb);
    RoadMarking->addProperty("width", "float");
    RoadMarking->addProperty("style", "string");
    RoadMarking->addProperty("dashNumber", "int");
    RoadTrack->addProperty("width", "float");

    auto Plant = world->addConcept("Plant");
    auto Tree = world->addConcept("Tree", "Plant");
    auto Bush = world->addConcept("Bush", "Plant");
}
