#include "VRProduction.h"
#include "VRLogistics.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

using namespace std;
using namespace OSG;

// Production --------------------------------------------------

void VRProductionProcess::addFragment(VRProcessFragment* f) {
    fragments[f->ID] = f;
}

VRProductionJob::VRProductionJob(VRProduct* p) {
    product = p;
    process = new VRProductionProcess("production_of_"+p->getName());
}

VRProduction::VRProduction() {
    description = VROntology::create("Production");
    intraLogistics = FLogistics::create();
    network = intraLogistics->addNetwork();
}

VRProductionMachine::VRProductionMachine() {
    description = VROntology::create("ProductionMachine");
    geo = VRGeometry::create("ProductionMachine");
}

void VRProduction::addMachine(VRProductionMachine* pm, string machine, VRGeometryPtr m) {
    auto prod = description->getEntity("production");
    prod->add(machine, "machine");

    pm->geo = m;
    machines[pm->ID] = pm;
    auto n0ID = network->addNode(m->getPose());
    for (auto n : network->getNodes()) {
        int nID = n->getID();
        if (nID != n0ID) {
            network->connect(nID, n0ID);
            network->connect(n0ID, nID);
        }
    }
}

VRProductionJob* VRProduction::queueJob(VRProduct* p, string job) {
    auto prod = description->getEntity("production");
    prod->add(job, "job");

    auto j = new VRProductionJob(p);
    jobs.push_back(j);
    return j;
}

VRProduct::VRProduct(string name) {
    this->name = name;
    description = VROntology::create("Product");
}

void VRProduction::stop() { running = false; }
void VRProduction::start() { running = true; }

VRProcessResult::VRProcessResult(string name) { this->name = name;}

VRProcessFragment::VRProcessFragment(string name) { this->name = name; }
VRProcessResult* VRProcessFragment::addResult(string name) {
    auto r = new VRProcessResult(name);
    results.push_back(r);
    return r;
}

void VRProduction::update() {
    if (!running) return;

    int t = getTime()*1e-3;
    int dt = t-last_takt;
    if (dt < takt) return;
    last_takt = t;

    VROntologyPtr production = description;
    //for (VRProductionJob* job : jobs) {
        /*VROntologyPtr product = job->product->description;
        VROntologyPtr jobOnto = VROntology::create();
        jobOnto->merge(production);
        jobOnto->merge(product);


        // what process to produce product p
        string q = "r(process) : r.result=p & r.result.state=done";

        cout << jobOnto->answer(q) << endl;*/

        /*auto features = product->getInstances("Feature"); // get all features and derived concepts
        cout << " job with " << features.size() << " features" << endl;
        for (auto feature : features) {
            // new process fragment with result state of feature
            auto fragment = new VRProcessFragment(feature->name);
            fragment->addResult(feature->name)->state = "done";
            job->process->addFragment(fragment);
            cout << "  feature " << feature->name << endl;

            // get fragment action by asking the
        }

        cout << job->process->toString() << endl;*/
    //}
}

VRProductionProcess::VRProductionProcess(string name) { this->name = name; }

string VRProductionProcess::toString() {
    string data = "process " + name + "\n";
    for (auto f : fragments) data += " fragment " + f.second->getName() + "\n";
    return data;
}

shared_ptr<VRProduction> VRProduction::create() { return shared_ptr<VRProduction>(new VRProduction()); }

VRObjectPtr VRProduction::test() {
    // ontologies
    auto mathOnto = VROntology::create("Math");
    auto featureOnto = VROntology::create("Feature");
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
    mathOnto->addConcept("Quaterniond");
    mathOnto->getConcept("Vector")->addProperty("x", "float");
    mathOnto->getConcept("Vector")->addProperty("y", "float");
    mathOnto->getConcept("Vector")->addProperty("z", "float");
    mathOnto->getConcept("Quaterniond")->addProperty("x", "float");
    mathOnto->getConcept("Quaterniond")->addProperty("y", "float");
    mathOnto->getConcept("Quaterniond")->addProperty("z", "float");
    mathOnto->getConcept("Quaterniond")->addProperty("w", "float");
    mathOnto->addConcept("Position", "Vector");
    mathOnto->addConcept("Normal", "Vector");
    mathOnto->addConcept("Direction", "Vector");
    mathOnto->addConcept("Orientation", "Quaterniond");
    mathOnto->addConcept("Box", "Volume");
    mathOnto->getConcept("Box")->addProperty("min", "Vector");
    mathOnto->getConcept("Box")->addProperty("max", "Vector");
    mathOnto->addRule("inside(p,b):Box(b);Position(p);isGe(p,b/min);isGe(b/max,p)", "Box");
    // TODO: quaternion rotation rule to change direction

    objectOnto->merge(mathOnto);
    objectOnto->addConcept("Object");
    objectOnto->getConcept("Object")->addProperty("position", "Position");
    objectOnto->getConcept("Object")->addProperty("orientation", "Orientation");

    processOnto->addConcept("Process");
    processOnto->getConcept("Process")->addProperty("fragment", "Process");
    processOnto->getConcept("Process")->addProperty("state", "int");
    processOnto->addRule("is(p/state,1):Process(p);is_not(p/state,1);is_all(p/fragment/state,1)", "Process");

    featureOnto->addConcept("Feature");
    featureOnto->getConcept("Feature")->addProperty("state", "int");

    actionOnto->addConcept("Action");

    machineOnto->merge(objectOnto);
    machineOnto->addConcept("Machine", "Object");

    processingOnto->merge(featureOnto);
    processingOnto->merge(actionOnto);
    processingOnto->addConcept("Processing", "Action");
    processingOnto->getConcept("Processing")->addProperty("result", "Feature");
    processingOnto->getConcept("Processing")->addProperty("state", "int");
    // if processing unset and feature unset and feature and processing result have same concept, result is set to feature
    //processingOnto->addRule("s(Processing).state=unset & f(Feature).state=unset & s.result.CONCEPT=f.CONCEPT ? f.state=set & s.state=set & s.result=f");
    // if processing done, then result is done and skill is unset
    processingOnto->addRule("is(s/result/state,1):Processing(s);is(s/state,1)", "Processing");

    prodMachineOnto->merge(machineOnto);
    prodMachineOnto->merge(processingOnto);
    prodMachineOnto->addConcept("Productionmachine","Machine");
    prodMachineOnto->getConcept("Productionmachine")->addProperty("processing", "Processing");

    boreholeOnto->merge(mathOnto);
    boreholeOnto->merge(featureOnto);
    boreholeOnto->addConcept("Borehole", "Feature");
    boreholeOnto->getConcept("Borehole")->addProperty("radius", "float");
    boreholeOnto->getConcept("Borehole")->addProperty("direction", "Direction");
    boreholeOnto->getConcept("Borehole")->addProperty("position", "Position");
    boreholeOnto->getConcept("Borehole")->addProperty("depth", "float");

    drillingOnto->merge(boreholeOnto);
    drillingOnto->merge(processingOnto);
    drillingOnto->addConcept("Drilling", "Processing");
    drillingOnto->getConcept("Drilling")->addProperty("volume", "Box");
    drillingOnto->getConcept("Drilling")->addProperty("position", "Position");
    drillingOnto->getConcept("Drilling")->addProperty("direction", "Direction");
    drillingOnto->getConcept("Drilling")->addProperty("speed", "float");
    //drillingOnto->addRule("b(Drilling).state=set & inside(b.result.position,b.volume) & b.result.direction=b.direction ? b.state=done");

    productOnto->merge(featureOnto);
    productOnto->merge(objectOnto);
    productOnto->addConcept("Product", "Object");
    productOnto->getConcept("Product")->addProperty("feature", "Feature");
    productOnto->getConcept("Product")->addProperty("body", "Volume");

    productionOnto->merge(productOnto);
    productionOnto->merge(prodMachineOnto);
    productionOnto->merge(processOnto);
    productionOnto->addConcept("Production");
    productionOnto->getConcept("Production")->addProperty("machine", "Machine");
    productionOnto->getConcept("Production")->addProperty("job", "Product");
    productionOnto->getConcept("Production")->addProperty("process", "Process");

    drillOnto->merge(prodMachineOnto);
    drillOnto->merge(processingOnto);
    drillOnto->merge(drillingOnto);
    drillOnto->addConcept("Drill", "Productionmachine");

    manipOnto->merge(objectOnto);
    manipOnto->merge(actionOnto);
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

    robotOnto->merge(manipOnto);
    robotOnto->merge(machineOnto);
    robotOnto->addConcept("Robot", "Machine");
    robotOnto->getConcept("Robot")->addProperty("skill", "Manipulation");

    auto machine = VRGeometry::create("machine");
    machine->setPrimitive("Box 1 2 1 1 1 1");

    // drill ----------
    auto drill = new VRProductionMachine();
    drill->geo->translate(Vec3d(-1.5,0,0));
    drill->description->merge(drillOnto);
    drill->description->addVectorEntity("wsMin", "Vector", "0", "-1", "0");
    drill->description->addVectorEntity("wsMax", "Vector", "0", "1", "0");
    auto workSpace = drill->description->addEntity("workSpace", "Box");
    workSpace->set("wsMin", "min");
    workSpace->set("wsMax", "max");
    drill->description->addVectorEntity("position", "Vector", "-1.5", "0", "0");
    auto drilling = drill->description->addEntity("drilling", "Drilling");
    drill->description->addVectorEntity("drillDir", "Vector", "0", "-1", "0");
    drilling->set("workSpace", "volume");
    drilling->set("drillDir", "direction");
    drilling->set("[0,0.5]", "depth"); // hole depth
    drilling->set("[0.03,0.05]", "radius"); // hole radius

    // robot ----------
    auto robot = new VRProductionMachine();
    robot->geo->translate(Vec3d(1.5,0,0));
    robot->description->merge(robotOnto);
    auto robotI = robot->description->addEntity("robot", "Robot");
    robotI->set("position", "position");
    robot->description->addVectorEntity("position", "Vector", "1.5", "0", "0");
    robot->description->addVectorEntity("wsMin", "Vector", "-1", "-1", "-1");
    robot->description->addVectorEntity("wsMax", "Vector", "1", "1", "1");
    workSpace = robot->description->addEntity("workSpace", "Box");
    workSpace->set("wsMin", "min");
    workSpace->set("wsMax", "max");
    auto grab = robot->description->addEntity("grab", "Grab");
    grab->set("workSpace", "volume");
    auto rotation = robot->description->addEntity("rotation", "Rotation");
    rotation->set("workSpace", "volume");
    auto translation = robot->description->addEntity("translation", "Translation");
    translation->set("workSpace", "volume");

    // product ---------------
    auto product = new VRProduct("testProduct");
    product->description->merge(productOnto);
    product->description->merge(boreholeOnto);
    auto Product = product->description->addEntity("testProduct", "Product");
    auto Btop = product->description->addEntity("Btop", "Borehole");
    auto Bbottom = product->description->addEntity("Bbottom", "Borehole");
    Product->add("Btop", "feature");
    Product->add("Bbottom", "feature");
    Btop->set("0.1", "radius");
    Btop->set("0.3", "depth");
    Btop->set("entryTop", "entrypoint");
    Btop->set("dirTop", "direction");
    product->description->addVectorEntity("entryTop", "Position", "0", "0.5", "0");
    product->description->addVectorEntity("dirTop", "Direction", "0", "-1", "0");
    Bbottom->set("0.1", "Radius");
    Bbottom->set("0.3", "Depth");
    Bbottom->set("entryBottom", "Entrypoint");
    Bbottom->set("dirBottom", "Direction");
    product->description->addVectorEntity("entryBottom", "Position", "0", "-0.5", "0");
    product->description->addVectorEntity("dirBottom", "Direction", "0", "1", "0");
    auto box = product->description->addEntity("box", "Box");
    box->set("bmin", "min");
    box->set("bmax", "max");
    product->description->addVectorEntity("bmin", "Vector", "-0.5", "-0.5", "-0.5");
    product->description->addVectorEntity("bmax", "Vector", "0.5", "0.5", "0.5");

    // production -----------------------------------------------
    auto production = new VRProduction();
    production->description->merge(productionOnto);
    production->description->addEntity("production", "Production");
    production->addMachine(robot, "robot", static_pointer_cast<VRGeometry>(machine->duplicate()));
    production->addMachine(drill, "drill", static_pointer_cast<VRGeometry>(machine->duplicate()));
    production->queueJob(product, "testProduct");
    production->start();

    string q = "q(x):Process(x);is(x/state,1);Production(y);has(y,x);is(y/job,testProduct)";
    auto reasoner = VRReasoner::create();
    reasoner->process(q, production->description);

    VRObjectPtr anchor = VRObject::create("production");
    anchor->addChild(drill->geo);
    anchor->addChild(robot->geo);
    return anchor;
}
