#include "VROntology.h"
#include "VROntologyT.h"
#include "addons/WorldGenerator/GIS/GISWorld.h"

#include "core/objects/VRTransform.h"

using namespace OSG;

map<string, VROntologyPtr> VROntology::library;

void VROntology::setupLibrary() {
    // ontologies
    auto mathOnto = VROntology::create("Math");
    auto featureOnto = VROntology::create("Feature");
    auto elecDevOnto = VROntology::create("ElectricalDevice");
    auto machineOnto = VROntology::create("Machine");
    auto prodMachineOnto = VROntology::create("ProductionMachine");
    auto manipOnto = VROntology::create("Manipulation");
    auto boreholeOnto = VROntology::create("Borehole");
    auto drillingOnto = VROntology::create("Drilling");
    auto processingOnto = VROntology::create("Processing");
    auto actionOnto = VROntology::create("Action");
    auto processOnto = VROntology::create("Process");
    auto productOnto = VROntology::create("Product");
    auto productionOnto = VROntology::create("Production");
    auto drillOnto = VROntology::create("Drill");
    auto robotOnto = VROntology::create("Robot");
    auto objectOnto = VROntology::create("Object");

    mathOnto->addConcept("Volume");
    mathOnto->addConcept("Vector");
    mathOnto->addConcept("Quaternion");
    mathOnto->addConcept("Position", "Vector");
    mathOnto->addConcept("Normal", "Vector");
    mathOnto->addConcept("Direction", "Vector");
    mathOnto->addConcept("Orientation", "Quaternion");
    mathOnto->addConcept("Box", "Volume");
    mathOnto->addConcept("Area");
    mathOnto->addConcept("Path");
    mathOnto->addConcept("Border", "Path");
    mathOnto->addConcept("Node");
    mathOnto->addConcept("NodeEntry");
    mathOnto->getConcept("Vector")->addProperty("x", "float");
    mathOnto->getConcept("Vector")->addProperty("y", "float");
    mathOnto->getConcept("Vector")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("x", "float");
    mathOnto->getConcept("Quaternion")->addProperty("y", "float");
    mathOnto->getConcept("Quaternion")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("w", "float");
    mathOnto->getConcept("Area")->addProperty("borders", "Border");
    mathOnto->getConcept("Area")->addProperty("subArea", "Area");
    mathOnto->getConcept("Node")->addProperty("position", "Position");
    mathOnto->getConcept("Node")->addProperty("paths", "NodeEntry");
    mathOnto->getConcept("Node")->addProperty("graphID", "int");
    mathOnto->getConcept("NodeEntry")->addProperty("path", "Path");
    mathOnto->getConcept("NodeEntry")->addProperty("node", "Node");
    mathOnto->getConcept("NodeEntry")->addProperty("sign", "float");
    mathOnto->getConcept("NodeEntry")->addProperty("direction", "Direction");
    mathOnto->getConcept("Path")->addProperty("nodes", "NodeEntry");
    mathOnto->getConcept("Box")->addProperty("min", "Vector");
    mathOnto->getConcept("Box")->addProperty("max", "Vector");
    mathOnto->addRule("inside(p,b):Box(b);Position(p);isGe(p,b.min);isGe(b.max,p)", "Box");
    mathOnto->addRule("connected(n,m):Node(n);Node(m);Path(p);has(p.nodes,n);has(p.nodes,m)", "Node");
    // TODO: quaternion rotation rule to change direction

    objectOnto->import(mathOnto);
    objectOnto->addConcept("Object");
    objectOnto->getConcept("Object")->addProperty("position", "Position");
    objectOnto->getConcept("Object")->addProperty("orientation", "Orientation");
    objectOnto->addBuiltin("rotate", &VRTransform::rotate);

    processOnto->addConcept("Process");
    processOnto->getConcept("Process")->addProperty("fragment", "Process");
    processOnto->getConcept("Process")->addProperty("state", "int");
    processOnto->addRule("is(p.state,1):Process(p);is_not(p.state,1);is_all(p.fragment.state,1)", "Process");

    featureOnto->addConcept("Feature");
    featureOnto->getConcept("Feature")->addProperty("state", "int");

    actionOnto->addConcept("Action");

    elecDevOnto->addConcept("ElectricalDevice");
    elecDevOnto->getConcept("ElectricalDevice")->addProperty("powered", "bool");

    machineOnto->import(objectOnto);
    machineOnto->import(elecDevOnto);
    machineOnto->addConcept("Machine", "Object ElectricalDevice");
    machineOnto->getConcept("Machine")->addProperty("running", "bool");

    processingOnto->import(featureOnto);
    processingOnto->import(actionOnto);
    processingOnto->addConcept("Processing", "Action");
    processingOnto->getConcept("Processing")->addProperty("result", "Feature");
    processingOnto->getConcept("Processing")->addProperty("state", "int");
    // if processing unset and feature unset and feature and processing result have same concept, result is set to feature
    //processingOnto->addRule("s(Processing).state=unset & f(Feature).state=unset & s.result.CONCEPT=f.CONCEPT ? f.state=set & s.state=set & s.result=f");
    // if processing done, then result is done and skill is unset
    processingOnto->addRule("is(s.result.state,1):Processing(s);is(s.state,1)", "Processing");

    prodMachineOnto->import(machineOnto);
    prodMachineOnto->import(processingOnto);
    prodMachineOnto->addConcept("Productionmachine","Machine");
    prodMachineOnto->getConcept("Productionmachine")->addProperty("processing", "Processing");

    boreholeOnto->import(mathOnto);
    boreholeOnto->import(featureOnto);
    boreholeOnto->addConcept("Borehole", "Feature");
    boreholeOnto->getConcept("Borehole")->addProperty("radius", "float");
    boreholeOnto->getConcept("Borehole")->addProperty("direction", "Direction");
    boreholeOnto->getConcept("Borehole")->addProperty("position", "Position");
    boreholeOnto->getConcept("Borehole")->addProperty("depth", "float");

    drillingOnto->import(boreholeOnto);
    drillingOnto->import(processingOnto);
    drillingOnto->addConcept("Drilling", "Processing");
    drillingOnto->getConcept("Drilling")->addProperty("volume", "Box");
    drillingOnto->getConcept("Drilling")->addProperty("position", "Position");
    drillingOnto->getConcept("Drilling")->addProperty("direction", "Direction");
    drillingOnto->getConcept("Drilling")->addProperty("speed", "float");
    //drillingOnto->addRule("b(Drilling).state=set & inside(b.result.position,b.volume) & b.result.direction=b.direction ? b.state=done");

    productOnto->import(featureOnto);
    productOnto->import(objectOnto);
    productOnto->addConcept("Product", "Object");
    productOnto->getConcept("Product")->addProperty("feature", "Feature");
    productOnto->getConcept("Product")->addProperty("body", "Volume");

    productionOnto->import(productOnto);
    productionOnto->import(prodMachineOnto);
    productionOnto->import(processOnto);
    productionOnto->addConcept("Production");
    productionOnto->getConcept("Production")->addProperty("machine", "Machine");
    productionOnto->getConcept("Production")->addProperty("job", "Product");
    productionOnto->getConcept("Production")->addProperty("process", "Process");

    drillOnto->import(prodMachineOnto);
    drillOnto->import(processingOnto);
    drillOnto->import(drillingOnto);
    drillOnto->addConcept("Drill", "Productionmachine");

    manipOnto->import(objectOnto);
    manipOnto->import(actionOnto);
    manipOnto->addConcept("Manipulation", "Action");
    manipOnto->addConcept("Grab", "Manipulation");
    manipOnto->addConcept("Translation", "Manipulation");
    manipOnto->addConcept("Rotation", "Manipulation");
    manipOnto->getConcept("Manipulation")->addProperty("volume", "Box");
    manipOnto->getConcept("Manipulation")->addProperty("state", "String");
    manipOnto->getConcept("Manipulation")->addProperty("object", "Object");
    //manipOnto->addRule("g(Grab).state=unset & inside(o(Object).position,g.volume) ? g.object=o & g.state=set");
    //manipOnto->addRule("g(Grab).state=set & t(Translation).state=unset & inside(o(Object).position,t.volume) ? t.object=o & t.state=set");
    //manipOnto->addRule("g(Grab).state=set & r(Rotate).state=unset & inside(o(Object).position,r.volume) ? r.object=o & r.state=set");
    //manipOnto->addRule("t(Translation).state=set & inside(p(Position), t.volume)? t.state=done");
    //manipOnto->addRule("g(Grab).state=set ? g.state=done");
    //manipOnto->addRule("r(Rotation).state=set ? r.state=done");

    robotOnto->import(manipOnto);
    robotOnto->import(machineOnto);
    robotOnto->addConcept("Robot", "Machine");
    robotOnto->getConcept("Robot")->addProperty("skill", "Manipulation");

    library["Math"] = mathOnto;
    library["Feature"] = featureOnto;
    library["Machine"] = machineOnto;
    library["ProductionMachine"] = prodMachineOnto;
    library["Manipulation"] = manipOnto;
    library["Borehole"] = boreholeOnto;
    library["Drilling"] = drillingOnto;
    library["Processing"] = processingOnto;
    library["Action"] = actionOnto;
    library["Process"] = processOnto;
    library["Product"] = productOnto;
    library["Production"] = productionOnto;
    library["Drill"] = drillOnto;
    library["Robot"] = robotOnto;
    library["Object"] = objectOnto;

    GISWorld::setupOntology();

    /* TODO:

    - add dependencies, maybe to ontologies directly?

    */
}
