#include "VRProduction.h"
#include "VRLogistics.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

#include <GL/glut.h>

using namespace std;
using namespace OSG;

// Production --------------------------------------------------

void VRProcess::addFragment(VRProcessFragment* f) {
    fragments[f->ID] = f;
}

VRProductionJob::VRProductionJob(VRProduct* p) {
    product = p;
    process = new VRProcess("production_of_"+p->name);
}

VRProduction::VRProduction() {
    description = new VROntology();
    intraLogistics = new FLogistics();
    network = intraLogistics->addNetwork();

    //auto fkt = new VRFunction<int>("production_update", boost::bind(&VRProduction::update, this));
    //VRSceneManager::getCurrent()->addUpdateFkt(fkt);
}

VRProductionMachine::VRProductionMachine() {
    description = new VROntology();
    geo = new VRGeometry("ProductionMachine");
}

void VRProduction::addMachine(VRProductionMachine* pm, string machine, VRGeometry* m) {
    auto prod = description->getInstance("production");
    prod->add(machine, "machine");

    pm->geo = m;
    machines[pm->ID] = pm;
    FNode* n0 = network->addNode();
    for (auto n : network->getNodes()) if (n != n0) { n0->connect(n); n->connect(n0); }
}

VRProductionJob* VRProduction::queueJob(VRProduct* p, string job) {
    auto prod = description->getInstance("production");
    prod->add(job, "job");

    auto j = new VRProductionJob(p);
    jobs.push_back(j);
    return j;
}

VRProduct::VRProduct(string name) {
    this->name = name;
    description = new VROntology();
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

    int t = glutGet(GLUT_ELAPSED_TIME);
    int dt = t-last_takt;
    if (dt < takt) return;
    last_takt = t;

    VROntology* production = description;
    for (VRProductionJob* job : jobs) {
        VROntology* product = job->product->description;

        VROntology* jobOnto = new VROntology();
        jobOnto->merge(production);
        jobOnto->merge(product);


        // what process to produce product p
        string q = "r(process) : r.result=p & r.result.state=done";

        cout << jobOnto->answer(q) << endl;

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
    }
}

VRProcess::VRProcess(string name) { this->name = name; }

string VRProcess::toString() {
    string data = "process " + name + "\n";
    for (auto f : fragments) data += " fragment " + f.second->name + "\n";
    return data;
}

VRObject* VRProduction::test() {
    // ontologies
    auto mathOnto = new VROntology();
    auto featureOnto = new VROntology();
    auto machineOnto = new VROntology();
    auto prodMachineOnto = new VROntology();
    auto manipOnto = new VROntology();
    auto boreholeOnto = new VROntology();
    auto drillingOnto = new VROntology();
    auto processingOnto = new VROntology();
    auto actionOnto = new VROntology();
    auto processOnto = new VROntology();
    auto productOnto = new VROntology();
    auto productionOnto = new VROntology();
    auto drillOnto = new VROntology();
    auto robotOnto = new VROntology();
    auto objectOnto = new VROntology();

    mathOnto->addConcept("Volume");
    mathOnto->addConcept("Vector");
    mathOnto->addConcept("Quaternion");
    mathOnto->getConcept("Vector")->addProperty("x", "float");
    mathOnto->getConcept("Vector")->addProperty("y", "float");
    mathOnto->getConcept("Vector")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("x", "float");
    mathOnto->getConcept("Quaternion")->addProperty("y", "float");
    mathOnto->getConcept("Quaternion")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("w", "float");
    mathOnto->addConcept("Position", "Vector");
    mathOnto->addConcept("Normal", "Vector");
    mathOnto->addConcept("Direction", "Vector");
    mathOnto->addConcept("Orientation", "Quaternion");
    mathOnto->addConcept("Box", "Volume");
    mathOnto->getConcept("Box")->addProperty("min", "Vector");
    mathOnto->getConcept("Box")->addProperty("max", "Vector");
    mathOnto->addRule("inside(p,b):Box(b);Position(p);is_ge(p,b/min);is_ge(b/max,p)");
    // TODO: quaternion rotation rule to change direction

    objectOnto->merge(mathOnto);
    objectOnto->addConcept("Object");
    objectOnto->getConcept("Object")->addProperty("position", "Position");
    objectOnto->getConcept("Object")->addProperty("orientation", "Orientation");

    processOnto->addConcept("Process");
    processOnto->getConcept("Process")->addProperty("fragment", "Process");
    processOnto->getConcept("Process")->addProperty("state", "int");
    processOnto->addRule("is(p/state,1):Process(p);is_not(p/state,1);is_all(p/fragment/state,1)");

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
    processingOnto->addRule("is(s/result/state,1):Processing(s);is(s/state,1)");

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
    productionOnto->addConcept("Production");
    productionOnto->getConcept("Production")->addProperty("machine", "Machine");
    productionOnto->getConcept("Production")->addProperty("job", "Product");

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

    auto machine = new VRGeometry("machine");
    machine->setPrimitive("Box", "1 2 1 1 1 1");

    // drill ----------
    auto drill = new VRProductionMachine();
    drill->geo->translate(Vec3f(-1.5,0,0));
    drill->description->merge(drillOnto);
    drill->description->addVectorInstance("wsMin", "Vector", "0", "-1", "0");
    drill->description->addVectorInstance("wsMax", "Vector", "0", "1", "0");
    auto workSpace = drill->description->addInstance("workSpace", "Box");
    workSpace->set("wsMin", "min");
    workSpace->set("wsMax", "max");
    drill->description->addVectorInstance("position", "Vector", "-1.5", "0", "0");
    auto drilling = drill->description->addInstance("drilling", "Drilling");
    drill->description->addVectorInstance("drillDir", "Vector", "0", "-1", "0");
    drilling->set("workSpace", "volume");
    drilling->set("drillDir", "direction");
    drilling->set("[0,0.5]", "depth"); // hole depth
    drilling->set("[0.03,0.05]", "radius"); // hole radius

    // robot ----------
    auto robot = new VRProductionMachine();
    robot->geo->translate(Vec3f(1.5,0,0));
    robot->description->merge(robotOnto);
    auto robotI = robot->description->addInstance("robot", "Robot");
    robotI->set("position", "position");
    robot->description->addVectorInstance("position", "Vector", "1.5", "0", "0");
    robot->description->addVectorInstance("wsMin", "Vector", "-1", "-1", "-1");
    robot->description->addVectorInstance("wsMax", "Vector", "1", "1", "1");
    workSpace = robot->description->addInstance("workSpace", "Box");
    workSpace->set("wsMin", "min");
    workSpace->set("wsMax", "max");
    auto grab = robot->description->addInstance("grab", "Grab");
    grab->set("workSpace", "volume");
    auto rotation = robot->description->addInstance("rotation", "Rotation");
    rotation->set("workSpace", "volume");
    auto translation = robot->description->addInstance("translation", "Translation");
    translation->set("workSpace", "volume");

    // product ---------------
    auto product = new VRProduct("testProduct");
    product->description->merge(productOnto);
    product->description->merge(boreholeOnto);
    auto Product = product->description->addInstance("testProduct", "Product");
    auto Btop = product->description->addInstance("Btop", "Borehole");
    auto Bbottom = product->description->addInstance("Bbottom", "Borehole");
    Product->add("Btop", "feature");
    Product->add("Bbottom", "feature");
    Btop->set("0.1", "radius");
    Btop->set("0.3", "depth");
    Btop->set("entryTop", "entrypoint");
    Btop->set("dirTop", "direction");
    product->description->addVectorInstance("entryTop", "Position", "0", "0.5", "0");
    product->description->addVectorInstance("dirTop", "Direction", "0", "-1", "0");
    Bbottom->set("0.1", "Radius");
    Bbottom->set("0.3", "Depth");
    Bbottom->set("entryBottom", "Entrypoint");
    Bbottom->set("dirBottom", "Direction");
    product->description->addVectorInstance("entryBottom", "Position", "0", "-0.5", "0");
    product->description->addVectorInstance("dirBottom", "Direction", "0", "1", "0");
    auto box = product->description->addInstance("box", "Box");
    box->set("bmin", "min");
    box->set("bmax", "max");
    product->description->addVectorInstance("bmin", "Vector", "-0.5", "-0.5", "-0.5");
    product->description->addVectorInstance("bmax", "Vector", "0.5", "0.5", "0.5");

    // production -----------------------------------------------
    auto production = new VRProduction();
    production->description->merge(productionOnto);
    production->description->merge(processOnto);
    production->description->addInstance("production", "Production");
    production->addMachine(robot, "robot", (VRGeometry*)machine->duplicate());
    production->addMachine(drill, "drill", (VRGeometry*)machine->duplicate());
    production->queueJob(product, "testProduct");
    production->start();

    string q = "q(x):Process(x);is(x/state,1);Production(y);has(y,x);has(factory,y);is(y/job,testProduct)";
    production->description->answer(q);

    VRObject* anchor = new VRObject("production");
    anchor->addChild(drill->geo);
    anchor->addChild(robot->geo);
    return anchor;
}
